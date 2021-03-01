#ifndef BASE_LOG_H
#define BASE_LOG_H

#include <iostream>
#include <string>
#include <pthread.h>
#include "BoundedBlockQueue.h"
#include <stdarg.h>

class Log {
public:
	static Log* getInstance() {
		static Log instance;
		return &instance;
	}

	bool init(std::string filename,
			  bool closeLog,
			  int logBufSize = 8192,
			  int splitLines = 500000,
			  int maxSize = 0);

	void writeLog(int level, const char *format, ...);
	
	void flush();
	bool isOpen() {return closeLog_;}

	static void* flushLogThread(void *args);
private:
	Log() {}
	virtual ~Log();

	void asyncWriteLog();
private:
	BoundedBlockQueue<std::string> *logQueue_;
	std::string dirName_;
	std::string logName_;
	
	bool isAsync_ = false;
	int lineCnt_ = 0;
	int splitLines_;
	bool closeLog_;
	int today_;
	
	char *logBuf_;
	int logBufSize_;
	
	FILE *fp_ = nullptr;
	Mutex mutex_;
};


#define LOG_DEBUG(format, ...) { if (Log::getInstance()->isOpen()) {Log::getInstance()->writeLog(0, format, ##__VA_ARGS__); Log::getInstance()->flush();}}
#define LOG_INFO(format, ...)  { if (Log::getInstance()->isOpen()) {Log::getInstance()->writeLog(1, format, ##__VA_ARGS__); Log::getInstance()->flush();}}
#define LOG_WARN(format, ...)  { if (Log::getInstance()->isOpen()) {Log::getInstance()->writeLog(2, format, ##__VA_ARGS__); Log::getInstance()->flush();}}
#define LOG_ERROR(format, ...) { if (Log::getInstance()->isOpen()) {Log::getInstance()->writeLog(3, format, ##__VA_ARGS__); Log::getInstance()->flush();}}


#endif
