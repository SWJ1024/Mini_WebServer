#ifndef BASE_CONDITION_H
#define BASE_CONDITION_H

#include "Mutex.h"

class Condition {
public:
	Condition(Mutex &m) : mutex_(m) {
		pthread_cond_init(&cond_, nullptr);
	}
	~Condition() {
		pthread_cond_destroy(&cond_);
	}

	void wait() {
		pthread_cond_wait(&cond_, mutex_.getPthreadMutex());
	}

	void signal() {
		pthread_cond_signal(&cond_);
	}
	void broadcast() {
		pthread_cond_broadcast(&cond_);
	}


private:
	Mutex &mutex_;
	pthread_cond_t cond_;
};


#endif //BASE_CONDITION_H
