#ifndef WEB_HTTPCONTEXT_H
#define WEB_HTTPCONTEXT_H

#include <sys/wait.h>
#include <sys/io.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <map>

class HttpContext{
public:
	static const size_t READ_BUFFER_SIZE = 2048;
	static const size_t WRITE_BUFFER_SIZE = 2048;
	
	enum HttpRequestParseState{
		kExpectRequestLine,
		kExpectHeaders,
		kExpectBody,
	};

	enum HttpMethod {
		GET = 0,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
		PATH	
	};

	enum HttpCode {
		NO_REQUEST,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURCE,
		FORBIDDEN_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION
	};

	enum HttpLineState {
		LINE_OK = 0,
		LINE_BAD,
		LINE_OPEN
	};

	enum HttpCheckState {
		CHECK_STATE_REQUESTLINE = 0,
		CHECK_STATE_HEADER,
		CHECK_STATE_CONTENT
	};

	HttpContext() : state_(kExpectRequestLine) {};
	~HttpContext();

	static int epollfd_;
	static int userCnt_;


	void init();
	void init(int sockfd, const sockaddr_in &addr, char *root, bool isLevelTriger, int closeLog, std::string user, std:string passwd, std::string sqlName);
private:
	HttpCode parseRequest();
	HttpCode processRequestLine(char*);
	HttpCode processRequestHeader(char*);
	HttpCode processRequestBody(char*);

	HttpLineState parseLine();
	bool processWrite(HttpCode);

	bool processAddRespond(const char* format, ...);
	bool preocessAddContent(const char *content);
	bool processAddStatusLine(int status, const char *title);
	bool processAddHeaders(int content_length);
	bool processAddContentType();
	bool processAddContentLength(int content_length);
	bool processAddLinger();
	bool processAddBlankLine();
								 


	bool readOnce();
	bool write();
private:
	int sockfd_;
	bool isLevelTriger_;

	char readBuf_[READ_BUFFER_SIZE];
	char writeBuf_[WRITE_BUFFER_SIZE];

	size_t checkIdx_;
	size_t readIdx_;
	size_t writeIdx_;

	size_t bytesToSend_;
	size_t bytesHaveSend_;

	struct iovec iov_[2];
	size_t iovcnt_;

	HttpMethod method_;
	HttpRequestParseState state_;

	int contentLength_;
	bool linger_;
	char* host_;
};







#endif
