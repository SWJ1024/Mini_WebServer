#include "Log.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

Log::~Log() {
	if (fp_) fclose(fp_);
}


void Log::asyncWriteLog() {
	while (1) {	
		std::string log = logQueue_->getFront();
		MutexGuard lock(mutex_);
		printf("%s\n", log.c_str());
		fputs(log.c_str(), fp_);
	}
}

void* Log::flushLogThread(void *) {
	Log::getInstance()->asyncWriteLog();
	return nullptr;
}

bool Log::init(std::string filename,
			  bool closeLog,
			  int logBufSize,
			  int splitLines,
			  int maxSize) {
	splitLines_ = splitLines;
	logBufSize_ = logBufSize;
	closeLog_ = closeLog;

	time_t tSec = time(nullptr);
	struct tm t = *localtime(&tSec);
	today_ = t.tm_mday;
	
	char logFullName[256]; 
	auto p = filename.find('/');
	if (p != std::string::npos) {
		logName_ = filename.substr(p+1);
		dirName_ = filename.substr(0, p);
	}
	else logName_ = filename;

	snprintf(logFullName, 255, "%s%d_%02d_%02d_%s", dirName_.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, logName_.c_str());
	if ((fp_ = fopen(logFullName, "a")) == nullptr) {
		return false;
	}

	if (maxSize >= 1) {
		isAsync_ = true;
		logQueue_ = new BoundedBlockQueue<std::string> (maxSize);
		pthread_t tid;
		pthread_create(&tid, nullptr, flushLogThread, nullptr);
	}

	logBuf_ = new char[logBufSize_];
	memset(logBuf_, '\0', logBufSize);

	return true;
}


std::string levelCache[5] = {"[debug]:", "[info]:",  "[warn]:", "[erro]:", "[info]:"}; 

void Log::writeLog(int level, const char *format, ...) {
	struct timeval now;
	gettimeofday(&now, nullptr);
	time_t tSec = now.tv_sec;
	struct tm t = *localtime(&tSec);
	
	{
		MutexGuard lock(mutex_);
		++lineCnt_;
		if (today_ != t.tm_mday || lineCnt_ % splitLines_ == 0) {
			fflush(fp_);
			fclose(fp_);
			char logFullName[256];
			if (today_ != t.tm_mday) {
				today_ = t.tm_mday;
				lineCnt_ = 0;
				snprintf(logFullName, 255, "%s%d_%02d_%02d%s", dirName_.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, logName_.c_str());
			}
			else {
				snprintf(logFullName, 255, "%s%d_%02d_%02d%s.%d", dirName_.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, logName_.c_str(), lineCnt_ / splitLines_);
			}
			fp_ = fopen(logFullName, "a");
		}
	}

	std::string logLevel = "[info]:";
	if (level < 5) logLevel = levelCache[level];

	va_list valist;
	va_start(valist, format);
	std::string logStr;
	{
		MutexGuard lock(mutex_);
		int n = snprintf(logBuf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
						t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
						t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec, logLevel.c_str());
		    
		int m = vsnprintf(logBuf_ + n, logBufSize_ - 1, format, valist);
		logBuf_[n + m] = '\n';
		logBuf_[n + m + 1] = '\0';
		logStr = logBuf_;
	}

	if (isAsync_ && !logQueue_->isFull()) {
		logQueue_->put(std::move(logStr));
	}
	else {
		MutexGuard lock(mutex_);
		fputs(logStr.c_str(), fp_);
	}
	
	va_end(valist);
}


void Log::flush() {
	MutexGuard lock(mutex_);
	fflush(fp_);
}
