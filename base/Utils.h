#ifndef BASE_UTILS
#define BASE_UTILS

#include "HeapTimer.h"
#include "../web/HttpContext.h"

class Utils {
public:
	Utils() {}
	~Utils() {}

	void init() {
	}

	void init(int timeslot) {
	    m_TIMESLOT = timeslot;
	}


	void showError(int connfd, const char *info) {
	    send(connfd, info, strlen(info), 0);
		close(connfd);
	}

	int setnonblocking(int fd) {
		int old_option = fcntl(fd, F_GETFL);
		int new_option = old_option | O_NONBLOCK;
		fcntl(fd, F_SETFL, new_option);
		return old_option;
	}

	void addfd(int epollfd, int fd, bool oneShot, bool isLevelTriger) { 
		epoll_event event;
		event.data.fd = fd;
		if (isLevelTriger)
			event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
		else
			event.events = EPOLLIN | EPOLLRDHUP;
		if (oneShot)
			event.events |= EPOLLONESHOT;
		epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
		setnonblocking(fd);
	}

	void addsig(int sig, void(handler)(int), bool restart) {
		struct sigaction sa;
		memset(&sa, '\0', sizeof(sa));
		sa.sa_handler = handler;
		if (restart) sa.sa_flags |= SA_RESTART;
		sigfillset(&sa.sa_mask);
		int ret = sigaction(sig, &sa, nullptr);
		assert(ret != -1);
	}


	void sigHandler(int sig) {
		int saveErrno = errno;
		int msg = sig;
		send(u_pipefd[1], (char *)&msg, 1, 0);
		errno = saveErrno;
	}

	void timerHandler() {
	    m_timer_lst.tick();
		alarm(m_TIMESLOT);
	}
};


#endif
