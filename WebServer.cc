#include "WebServer.h"


WebServer::WebServer() {
	users_ = new HttpConnect[MAX_FD];
	usersTimer_ = new ClientData[MAX_FD];

	char serverPath[200];
	getcwd(serverPath, 200);
	char root[6] = "/root";
	rootPath_ = (char *)malloc(strlen(serverPath) + strlen(root) + 1);
	strcpy(rootPath_, serverPath);
	strcat(rootPath_, root);
}

WebServer::~WebServer() {
	close(epollfd_);
	close(listenfd_);
	close(pipefd_[1]);
	close(pipefd_[0]);
	
	delete[] users_;
	delete[] usersTimer_;
	delete sqlConnPool_;
}

void WebServer::init(int port, std::string user, std::string passwd, std::string databaseName,
		bool logWrite, bool isLinger, int trigerMode, int sqlNum,
		int threadNum, bool closeLog, int actionModel) {
	port_ = port;
	user_ = user;
	passwd_ = passwd;
	databaseName_ = databaseName;
	logWrite_ = logWrite;
	isLinger_ = isLinger;
	trigerMode_ = trigerMode;
	sqlNum_ = sqlNum;
	threadNum_ = threadNum;
	closeLog_ = closeLog;
	actionModel_ = actionModel;
}


void WebServer::initLog() {
	if (!closeLog_) {
		if (logWrite_) {
			Log::getInstance()->init("ServerLog", closeLog_, 2000, 800000, 800);
		}
		else {
			Log::getInstance()->init("ServerLog", closeLog_, 2000, 800000, 0);
		}
	}
	LOG_INFO("%s", "????");
	printf("%s", "?\n");
}

void WebServer::initSqlConnPool() {
	sqlConnPool_ = SqlConnPool::GetInstance();
	sqlConnPool_->init("localhost", user_, passwd_, databaseName_, 3306, sqlNum_, closeLog_);

	users_->initmysql_result(sqlConnPool_);
}


void WebServer::initThreadPool() {
	threadPool_ = new ThreadPool<HttpConnect> (actionModel_, sqlConnPool_, threadNum_);
}


void WebServer::initTrigerMode() {
	if (trigerMode_ == 0) {      // LT + LT
		listenTrigerMode_ = false;
		connectTrigerMode_ = false;
	}
	else if (trigerMode_ == 1) { // LT + ET
		listenTrigerMode_ = false;
		connectTrigerMode_ = true;
	}
	else if (trigerMode_ == 2) { // ET + LT
		listenTrigerMode_ = true;
		connectTrigerMode_ = false;
	}
	else if (trigerMode_ == 3) { // ET + ET
		listenTrigerMode_ = true;
		connectTrigerMode_ = true;
	}
}



void WebServer::eventListen() {
	listenfd_ = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd_ >= 0);

	if (isLinger_) {
		struct linger option = {1, 1};
		setsockopt(listenfd_, SOL_SOCKET, SO_LINGER , &option, sizeof(option));
	}
	else {
		struct linger option = {0, 1};
		setsockopt(listenfd_, SOL_SOCKET, SO_LINGER , &option, sizeof(option));
	}

	int ret = 0;
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port_);

	int option = 1;
	setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	ret = bind(listenfd_, (struct sockaddr*)&addr, sizeof(addr));
	assert(ret >= 0);
	ret = listen(listenfd_, 5);
	assert(ret >= 0);

	utils_.init(TIMESLOT);

	epollfd_ = epoll_create(5);
	assert(epollfd_ != -1);

	utils_.addfd(epollfd_, listenfd_, false, listenTrigerMode_);

	HttpConnect::epollfd_ = epollfd_;

	ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd_);
    assert(ret != -1);
	
	utils_.setnonblocking(pipefd_[1]);
	
	utils_.addfd(epollfd_, pipefd_[0], false, 0);
	utils_.addsig(SIGPIPE, SIG_IGN);
	utils_.addsig(SIGALRM, utils_.sigHandler, false);
	utils_.addsig(SIGTERM, utils_.sigHandler, false);
	
	alarm(TIMESLOT);
	Utils::pipefd_ = pipefd_;
	Utils::epollfd_ = epollfd_;

}


void WebServer::addClntToTimer(int confd, struct sockaddr_in clntAddr) {
	users_[confd].init(confd, clntAddr, rootPath_, connectTrigerMode_, closeLog_, user_, passwd_, databaseName_);
	usersTimer_[confd].addr_ = clntAddr;
	usersTimer_[confd].sockfd = confd;
	
	time_t cur = time(NULL);
	auto timer = new TimerNode(cur + 3 * TIMESLOT);
	timer->data_ = &usersTimer_[confd];
	
	timer->func_ = cbFunc;
	usersTimer_[confd].timer_ = timer;
	utils_.timer_.addTimer(timer);
}

bool WebServer::dealClientdata() {
	struct sockaddr_in clntAddr;
	socklen_t clntLen;
	if (listenTrigerMode_) {
		while (1) {
			int confd = accept(listenfd_, (struct sockaddr*)&clntAddr, &clntLen);
			if (confd < 0) {
				LOG_ERROR("%s:error is %d", "accept error", errno);
				break;
			}
			if (HttpConnect::userCnt_ >= MAX_FD) {
				utils_.showError(confd, "Internal server busy");
				LOG_ERROR("%s", "Internal server busy");
				break;
			}
			addClntToTimer(confd, clntAddr);
		}
		return false;
	}
	else {
		int confd = accept(listenfd_, (struct sockaddr*)&clntAddr, &clntLen);
		if (confd < 0) {
			LOG_ERROR("%s:error is %d", "accept error", errno);
			return false;
		}
		if (HttpConnect::userCnt_ >= MAX_FD) {
			utils_.showError(confd, "Internal server busy");
			LOG_ERROR("%s", "Internal server busy");
			return false;
		}
		addClntToTimer(confd, clntAddr);
	}
	return true;
}



bool WebServer::dealTimer(TimerNode* timer, int sockfd) {
	timer->func_(&usersTimer_[sockfd]);
	if (timer) {		
		utils_.timer_.delTimer(timer);
	}
	LOG_INFO("close fd %d", usersTimer_[sockfd].sockfd);
	return true;
}


bool WebServer::dealSignal(bool &timeout, bool &stopServer) {
	char signals[1024];
	int ret = recv(pipefd_[0], signals, sizeof(signals), 0);
		
	if (ret == -1) return false;	
	else if (ret == 0) return false;				
	else {
		for (int i = 0; i < ret; ++i) {						
			switch (signals[i]) {
				case SIGALRM: {
					timeout = true;
					break;
				}
				case SIGTERM: {
					stopServer = true;
					break;
				}
			}
		}
	}
	return true;
}


void WebServer::adjustTimer(TimerNode* timer) {
	time_t cur = time(NULL);
    //timer->expire_ = cur + 3 * TIMESLOT;
	utils_.timer_.adjustTimer(timer, cur+3*TIMESLOT);
	LOG_INFO("%s", "adjust timer once");
}


void WebServer::dealRead(int sockfd) {
	TimerNode* timer = usersTimer_[sockfd].timer_;
	if (actionModel_ == 1) { //reaactor
		if (timer) adjustTimer(timer);
		threadPool_->put(users_ + sockfd, 0);
        while (true) {
			if (1 == users_[sockfd].improv__) {
				if (1 == users_[sockfd].timerFlag__) {
					dealTimer(timer, sockfd);
					users_[sockfd].timerFlag__ = 0;
				}
				users_[sockfd].improv__ = 0;
				break;
			}
		}
	}
	else {     				 //proactor
		if (users_[sockfd].readOnce()) {
			LOG_INFO("deal with the client(%s)", inet_ntoa(users_[sockfd].getAddr()->sin_addr));
			//若监测到读事件，将该事件放入请求队列
			threadPool_->put(users_ + sockfd);
			if (timer) {
				adjustTimer(timer);
			}
		}
		else {
			dealTimer(timer, sockfd);		
		}
	}
}


void WebServer::dealWrite(int sockfd) {
	auto timer = usersTimer_[sockfd].timer_;
	if (actionModel_ == 1) { //reactor
		if (timer) adjustTimer(timer);
		 threadPool_->put(users_ + sockfd, 1);
		 while (true) {
			 if (1 == users_[sockfd].improv__) {
				 if (1 == users_[sockfd].timerFlag__) {
					 dealTimer(timer, sockfd);
					 users_[sockfd].timerFlag__ = 0;
				 }
				 users_[sockfd].improv__ = 0;
				 break;
			 }
		 }
	}
	else {                  //proactor
	   if (users_[sockfd].write()) {
		   LOG_INFO("send data to the client(%s)", inet_ntoa(users_[sockfd].getAddr()->sin_addr));
		   if (timer) {
			   adjustTimer(timer);
		   }
	   }
	   else {
		   dealTimer(timer, sockfd);
	   }
	}
}


void WebServer::eventLoop() {
	bool timeout = false;
	bool stopServer = false;

	while (!stopServer) {
		int cnt = epoll_wait(epollfd_, events_, MAX_EVENT_NUM, -1);
		if (cnt < 0 && errno != EINTR) {
			LOG_ERROR("%s", "epoll fail");
			break;
		}

		for (int i = 0; i < cnt; ++i) {
			int sockfd = events_[i].data.fd;
			
			if (sockfd == listenfd_) {
				if (!dealClientdata()) continue;
			}
			else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
				//服务端关闭连接
				TimerNode* timer = usersTimer_[sockfd].timer_;
				dealTimer(timer, sockfd);
			}
			else if ((sockfd == pipefd_[0]) && (events_[i].events & EPOLLIN)) {
				if (!dealSignal(timeout, stopServer)) {
					LOG_ERROR("%s", "deal client data fail");
				}
			}
			else if (events_[i].events & EPOLLIN) {
				dealRead(sockfd);
			}
			else if (events_[i].events & EPOLLOUT) {
				dealWrite(sockfd);
			}
		}
		if (timeout) {
			LOG_INFO("%s", "timer tick");
			utils_.timerHandler();
			timeout = false;
		}	
	}
}
