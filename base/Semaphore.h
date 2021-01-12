#ifndef BASE_SEMAPHORE_H
#define BASE_SEMAPHORE_H

#include <semaphore.h>


class Semaphore {
public:
	Semaphore(int num = 0) {
		sem_init(&sem_, 0, num);
	}
	~Semaphore() {
		sem_destroy(&sem_);
	}

	void wait() {
		sem_wait(&sem_);
	}

	void post() {
		sem_post(&sem_);
	}

private:
	sem_t sem_;
};


#endif //BASE_SEMAPHORE_H
