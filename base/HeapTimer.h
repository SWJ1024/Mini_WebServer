#ifndef BASE_HEAPTIMER_H
#define BASE_HEAPTIMER_H

#include <vector>
#include <time.h>
#include <iostream>
#include <netinet/in.h>
class TimerNode;

struct ClientData {
	struct sockaddr_in addr_;
	int sockfd;
	TimerNode* timer_;
};


class TimerNode {
public:
	TimerNode(int t) : expire_(t) {};
	time_t expire_;
	ClientData* data_;

	//void func() {
	//	std::cout << expire_ << "\n";
	//}

	void (*func_) (ClientData*);

	bool operator < (const TimerNode &rhs) const {
		return expire_ < rhs.expire_;
	}

};


class HeapTimer {
public:
	void addTimer(TimerNode*);
	void adjustTimer(TimerNode*, size_t);
	void delTimer(TimerNode*);
	void tick();

private:
	size_t findTimer(TimerNode*);
	void heapDown(size_t);
	void heapUp(size_t);

private:
	std::vector<TimerNode*> heap_{nullptr};
};









#endif
