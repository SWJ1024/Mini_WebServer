#ifndef BASE_MUTEX_H
#define BASE_MUTEX_H

#include <pthread.h>

class Mutex {
public:
	Mutex() {
		pthread_mutex_init(&mutex_, nullptr);
	}
	~Mutex() {
		pthread_mutex_destroy(&mutex_);
	}

	void lock() {
		pthread_mutex_lock(&mutex_);
	}
	void unlock() {
		pthread_mutex_unlock(&mutex_);
	}

	pthread_mutex_t *getPthreadMutex() {
		return &mutex_;
	}


private:
	pthread_mutex_t mutex_;
};


class MutexGuard {
public:
	MutexGuard(Mutex &m) 
		: mutex_(m) 
	{
		mutex_.lock();
	}

	~MutexGuard() {
		mutex_.unlock();
	}

private:
	Mutex& mutex_;
};


#endif //BASE_MUTEX_H
