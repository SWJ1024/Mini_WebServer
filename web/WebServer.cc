#include "WebServer.h"


WebServer::WebServer() {
	users_ = new HttpContext[MAX_FD];
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
			Log::getInstance()->init("../ServerLog", closeLog_, 2000, 800000, 800);
		}
		else {
			Log::getInstance()->init("../ServerLog", closeLog_, 2000, 800000, 0);
		}
	}
}

void WebServer::initSqlConnPool() {
	sqlConnPool_ = SqlConnPool::GetInstance();
	sqlConnPool_->init("localhost", user_, passwd_, databaseName_, 3306, sqlNum_, closeLog_);

	//userskasfjls
}


void WebServer::initThreadPool() {
	threadPool_ = new ThreadPool<HttpContext> (actionModel_, sqlConnPool_, threadNum_);
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
	assert(listenfd >= 0);

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



}


bool WebServer::dealClientdata() {
	struct sockaddr_in clntAddr;
	socket_t clntLen;
	if (listenTrigerMode_) {
		while (1) {
			int confd = accept(listenfd_, (struct socketaddr*)&clntAddr, &clntLen);
			if (confd < 0) {
				LOG_ERROR("%s:error is %d", "accept error", errno);
				break;
			}
			if (HttpContext::userCnt_ >= MAX_FD) {
				utils_.showError(confd, "Internal server busy");
				LOG_ERROR("%s", "Internal server busy");
				break;
			}
			//timer
		}
		return false;
	}
	else {
		int confd = accept(listenfd_, (struct socketaddr*)&clntAddr, &clntLen);
		if (confd < 0) {
			LOG_ERROR("%s:error is %d", "accept error", errno);
			return false;
		}
		if (HttpContext::userCnt_ >= MAX_FD) {
			utils_.showError(confd, "Internal server busy");
			LOG_ERROR("%s", "Internal server busy");
			return false;
		}
		//timer()
	}
	return true;
}


bool WebServer::dealTimer(TimerNode* timer) {
	
}


bool WebServer::dealSignal() {


}


void WebServer::adjustTimer(TimerNode* timer) {
	
}


bool WebServer::dealRead(int sockfd) {
	TimerNode* timer = usersTimer_[sockfd].timer_;
	if (actionModel_ == 1) { //reaactor
		if (timer) adjustTimer(timer);

	}
	else {     				 //proactor

	}

}


bool WebServer::dealWrite(int fd) {


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
				TimerNode* timer = usersTimer_[sockfd].timer_;
				dealTimer(timer);
			}
			else if ((sockfd == pipefd[0]) && (events_[i].events & EPOLLIN)) {
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
//////////////////
		}	
	}
}
