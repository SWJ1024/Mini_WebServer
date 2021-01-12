#ifndef BASE_THRREADPOOL_H
#define BASE_THRREADPOOL_H

#include "Condition.h"
#include "SqlConnPool.h"
#include <vector>
#include <deque>

template<typename T>
class ThreadPool {
	public:
		ThreadPool(int actorModel, SqlConnPool* sqlConnPool, int threadNum = 8, int maxRequest = 10000);
		~ThreadPool();

		T* take();
		bool put(T* request, int state = -1);
	private:
		static void* startThread(void *arg);
		void runTask();


		Mutex mutex_;
		Condition condition_;
		std::vector<pthread_t> threads_;
		int actorModel_;
		int threadNum_;
		int maxRequest_;
		SqlConnPool* sqlConnPool_;
		std::deque<T*> tasks_;
};


template<typename T>
bool ThreadPool<T>::put(T* request, int state) {
	MutexGuard lock(mutex_);
	if (tasks_.size() == maxRequest_) return false;
	if (state != -1) request->state_ = state;
	//tasks_.emplace_back(std::move(t));
	tasks_.push_back(request);
	condition_.signal();
	return true;
}

template<typename T>
T* ThreadPool<T>::take() {
	MutexGuard lock(mutex_);
	while (tasks_.empty()) {
		condition_.wait();
	}
	T* t = tasks_.front();
	tasks_.pop_front();
	return t;
}

template<typename T>
void ThreadPool<T>::runTask() {
	while (1) {
		auto request = take();
		if (!request) continue;
		
		if (actorModel_ == 1) {

		}
		else {

		}

	}
}


template<typename T>
ThreadPool<T>::ThreadPool(int actorModel, SqlConnPool* sqlConnPool, int threadNum, int maxRequest)
    : mutex_(),
	  condition_(mutex_),
	  actorModel_(actorModel),
	  threadNum_(threadNum),
	  maxRequest_(maxRequest),
	  sqlConnPool_(sqlConnPool) {

		threads_.resize(threadNum);
		for (int i = 0; i < threadNum; ++i) {
			pthread_create(&threads_[i], nullptr, startThread, this);
			pthread_detach(threads_[i]);
		}

	}



template<typename T>
ThreadPool<T>::~ThreadPool() {

}

template<typename T>
void* ThreadPool<T>::startThread(void *argc) {
	ThreadPool *pool = static_cast<ThreadPool*> (argc);
	pool->runTask();
	return nullptr;
}





#endif
