#include "HttpContext.h"
#include "../base/Log.h"
#include <fstream>
#include <mysql/mysql.h>


const char *OK_200_TITLE = "OK";
const char *ERROR_400_TITLE = "Bad Request";
const char *ERROR_400_FORM = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *ERROR_403_TITLE = "Forbidden";
const char *ERROR_403_FORM = "You do not have permission to get file form this server.\n";
const char *ERROR_404_TITLE = "Not Found";
const char *ERROR_404_FORM = "The requested file was not found on this server.\n";
const char *ERROR_500_TITLE = "Internal Error";
const char *ERROR_500_FORM = "There was an unusual problem serving the request file.\n";

HttpContext::epollfd_ = -1;
HttpContext::userCnt_ = 0;


int setNonblocking(int fd) {
	int oldOpt = fcntl(fd, F_GETFL);
	int newOpt = oldOpt | O_NONBLOCK;
	fcntl(fd, F_SETFL, newOpt);
	return oldOpt;
}


void addfd(int epollfd, int fd, bool isOneShot, bool isLevelTriger) {
	epoll_event ev;
	ev.data.fd = fd;
	if (isLevelTriger) {
		ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	}
	else {
		ev.events = EPOLLIN | EPOLLRDHUP;
	}

	if (isOneShot) ev.events |= EPOLLONESHOT;
	
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
	setNonblocking(fd);
}


void removefd(int epollfd, int fd) {
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}


//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int option, bool isLevelTriger) {
	epoll_event ev;
	ev.data.fd = fd;
	if (isLevelTriger) {
		ev.events = option | EPOLLONESHOT | EPOLLRDHUP;
	}
	else {
		ev.events = option | EPOLLONESHOT | EPOLLRDHUP;
	}
	
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}


void HttpContext::init() {
	memset(readBuf_, '\0', READ_BUFFER_SIZE);
    memset(writeBuf_, '\0', WRITE_BUFFER_SIZE);
	memset(m_real_file, '\0', FILENAME_LEN);
}


void HttpContext::init(int sockfd, const sockaddr_in &addr, char *root, bool isLevelTriger, int closeLog, std::string user, std:string passwd, std::string sqlName) {
	
	init();
}


bool HttpContext::readOnce() {
	if (readIdx_ >= READ_BUFFER_SIZE) return false;
	int bytesCnt = 0;
	if (isLevelTriger_) { //LT
		bytesCnt = recv(sockfd_, readBuf_ + readIdx_, READ_BUFFER_SIZE - readIdx_, 0);
		if (bytesCnt <= 0) return false;
		readIdx_ += bytesCnt;
		return true;
	}
	else { //ET
		while (1) {
			bytesCnt = recv(sockfd_, readBuf_ + readIdx_, READ_BUFFER_SIZE - readIdx_, 0);
			if (bytesCnt == 0) return false;
			else if (bytesCnt == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) break;
				return false;
			}
			readIdx_ += bytesCnt;
		}
		return true;
	}
}

bool HttpContext::write() {
	if (bytesToSend_ == 0) {
///
	}
	int bytesCnt = 0;

	while (1) {
		bytesCnt = writev(sockfd_, iov_, iovcnt_);
		if (bytesCnt < 0) {
			if (errno == EAGAIN) {
//
				return true;
			}
			umap();
			return false;
		}
		bytesHaveSend_ += bytesCnt;
		bytesToSend_ -= bytesCnt;

		if (bytesHaveSend_ >= iov_[0].iov_len) {
			iov_[0].iov_len = 0;
			iov_[1].iov_base = ;/////
			iov_[1].iov_len = bytesToSend_;
		}
		else {
			iov_[0].iov_base = writeBuf_ + bytesHaveSend_;
			iov_[0].iov_len = iov_[0].iov_len - bytesHaveSend_; 
		}

		if (bytesToSend_ <= 0) {

		}

	}
}


HttpContext::HttpLineState HttpContext::parseLine() {
	char temp;
	for (; checkIdx_ < readIdx_; ++checkIdx_) { 
		temp = readBuf_[checkIdx_];
		if (temp == '\r') {
			if (checkIdx_ + 1 == readIdx_) return LINE_OPEN;
			else if (readBuf_[checkIdx_+1] == '\n') {
				readBuf_[checkIdx_++] = '\0';
				readBuf_[checkIdx_++] = '\0';
				return LINE_OK;
			}
			else return LINE_BAD;
		}
		else if (temp == '\n') {
			if (checkIdx_ > 1 && readBuf_[checkIdx_-1] == '\r') {
				readBuf_[checkIdx_-1] = '\0';
				readBuf_[checkIdx_++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}


HttpContext::HttpCode HttpContext::parseRequest() {
	char *text = nullptr;
	HttpCode ret = NO_REQUEST;
	HttpLineState lineState = LINE_OK;
	while ((state_ == kExpectBody??? && lineState == LINE_OK) || (lineState = parseLine()) == LINE_OK) {
		text = readBuf_ + checkIdx_;
		LOG_INFO("%s", text);
		switch(state_) {
			case kExpectRequestLine: {
				ret = processRequestLine(text);
				if (ret == BAD_REQUEST) return BAD_REQUEST;
				break;
			}
			case kExpectHeaders: {
				ret = processRequestHeader(text);
				if (ret == BAD_REQUEST) return BAD_REQUEST;
				else if (ret == GET_REQUEST) return processGetRequest();
				break;
			}
			case kExpectBody: {
				ret = processRequestBody(text);
				if (ret == BAD_REQUEST) return BAD_REQUEST;
				else if (ret == GET_REQUEST) return processGetRequest();
				//line????
				break;
			}
			default:
				return INTERNAL_ERROR;
		}
	}
	return NO_REQUEST;
}

//POST    /../../xx.html(URL)   HTTP/1.1
HttpContext::HttpCode HttpContext::processRequestLine(char *text) {
	auto urlPos = strpbrk(text, " \t");
	if (urlPos == nullptr) return BAD_REQUEST;

	*urlPos++ = '\0';
	urlPos += strspn(urlPos, " \t");
	
	char *methodPos = text;
	if (strcasecmp(methodPos, "GET") == 0) method_ = GET; 
	else if (strcasecmp(methodPos, "POST") == 0) {
		method_ = POST;
		cgi = 1????;
	}
	else {
		LOG_INFO("%s", "other method");
		return BAD_REQUEST;
	}

	auto versionPos = strpbrk(urlPos, " \t");
	if (versionPos == nullptr) return BAD_REQUEST;

	*versionPos++ = '\0';
	versionPos += strspn(versionPos, " \t");
	if (strcasecmp(versionPos, "HTTP/1.1") != 0) {
		LOG_INFO("%s", "NOT HTTP1.1");
		return BAD_REQUEST;
	}

	if (strncasecmp(urlPos, "http://", 7) == 0) {
		urlPos += 7;
		urlPos = strchr(urlPos, '/');
	}
	else if (strncasecmp(urlPos, "https://", 8) == 0) {
		urlPos += 8;
		urlPos = strchr(urlPos, '/');
	}

	if (urlPos == nullptr || urlPos[0] != '/') return BAD_REQUEST;
	if (strlen(urlPos) == 1) strcat(url, "judge.html");

	state_ = kExpectHeaders;
	return NO_REQUEST;
}

HttpContext::HttpCode HttpContext::processRequestHeader(char *text) {
	if (text[0] == '\0') {
		if (contentLength_ != 0) {
			state_ = kExpectBody;
			return NO_REQUEST;
		}
		else return GET_REQUEST;
	}
	else if (strncasecmp(text, "Connection:", 11) == 0) {
		text += 11;
		text += strspn(text, " \t");
		if (strcasecmp(text, "keep-alive") == 0) linger_ = true;
	}
	else if (strncasecmp(text, "Content-length", 15) == 0) {
		text += 15;
		text += strspn(text, " \t");
		contentLength_ = atol(text);
	}
	else if (strncasecmp(text, "Host:", 5) == 0) {
		text += 5;
		text += strspn(text, " \t");
		host_ = text;
	}
	else {
		LOG_INFO("unknown header: %s", text);
	}
	return NO_REQUEST;
}

HttpContext::HttpCode HttpContext::processRequestBody(char *text) {
	if (1) {
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

bool HttpContext::processWrite(HttpCode ret) {
	switch (ret) {
		case INTERNAL_ERROR: {
			processAddStatusLine(500, ERROR_500_TITLE);
			processAddHeaders(strlen(ERROR_500_FORM));
			if (!preocessAddContent(ERROR_500_FORM)) return false;
			break;
		}
		case BAD_REQUEST: {
			processAddStatusLine(404, ERROR_404_TITLE);
			processAddHeaders(strlen(ERROR_404_FORM));
			if (!preocessAddContent(ERROR_404_FORM)) return false;
			break;
		}
		case FORBIDDEN_REQUEST: {
			processAddStatusLine(403, ERROR_404_TITLE);
			processAddHeaders(strlen(ERROR_403_FORM));
			if (!preocessAddContent(ERROR_403_FORM)) return false;
			break;
		}
		case FILE_REQUEST: {
			processAddStatusLine(200, OK_200_TITLE);
			if (/*1*/) {

			}
			else {

			}
			break;
		}
		default: {
			return false;
			break;
		}
	}
}

bool HttpContext::processAddRespond(const char* format, ...) {
	if (writeIdx_ >= WRITE_BUFFER_SIZE) return false;
	va_list argList;
	va_start(argList, format);

	int len = vsnprintf(writeBuf_ + writeIdx_, WRITE_BUFFER_SIZE - 1 - writeIdx_, format, argList);
	if (len >= (WRITE_BUFFER_SIZE - 1 - writeIdx_)) {
		va_end(argList);
		return false;
	}
	writeIdx_ += len;

	va_end(argList);
	LOG_INFO("request %s", writeBuf_);
	return true;
}


bool HttpContext::processAddStatusLine(int status, const char *title) {
	return processAddRespond("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpContext::processAddHeaders(int contentLength) {
	return processAddContentLength(contentLength) && processAddLinger() && processAddBlankLine();
}

bool HttpContext::processAddContentType() {
	return processAddRespond("Content-Type:%s\r\n", "text/html");
}

bool HttpContext::processAddContentLength(int contentLength) {
	return processAddRespond("Content-Length:%d\r\n", contentLength);
}

bool HttpContext::processAddLinger() {
	return processAddRespond("Connection:%s\r\n", linger_ ? "keep-alive" : "close");
}

bool HttpContext::processAddBlankLine() {
	return processAddRespond("%s", "\r\n");
}

bool HttpContext::preocessAddContent(const char *content) {
	return processAddRespond("%s", content);
}
