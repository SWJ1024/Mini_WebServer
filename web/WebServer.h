#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include "../base/ThreadPool.h"
#include "HttpContext.h"
#include "../base/HeapTimer.h"
#include "../base/Utils.h"

class WebServer {
public:
	WebServer();
	~WebServer();

	void init(int port, std::string user, std::string passwd, std::string databaseName,
			bool logWrite, bool isLinger, int trigerMode, int sqlNum,
			int threadNum, bool closeLog, int actionModel);


	void initLog();
	void initSqlConnPool();
	void initThreadPool();
	void initTrigerMode();

	void eventListen();
	void eventLoop();

private:
	bool dealClientdata();
	bool dealTimer(TimerNode*);
	bool dealSignal();
	bool dealRead(int);
	bool dealWrite(int);

private:
	const static int MAX_EVENT_NUM = 10000;
	const static int MAX_FD = 65536;
	const static int TIMESLOT = 5;


	int listenfd_;
	int epollfd_;
	epoll_event events_[MAX_EVENT_NUM];

	Utils utils_;

	int port_;
	std::string user_;
	std::string passwd_;
	std::string databaseName_;
	
	bool logWrite_;
	bool closeLog_;

	bool isLinger_;

	int trigerMode_;
	bool listenTrigerMode_;
	bool connectTrigerMode_;

	ThreadPool<HttpContext> *threadPool_;
	int sqlNum_;
	int actionModel_;
	int threadNum_;

	SqlConnPool* sqlConnPool_;


	char* rootPath_;

	HttpContext* users_;
	ClientData* usersTimer_;
};







#endif
