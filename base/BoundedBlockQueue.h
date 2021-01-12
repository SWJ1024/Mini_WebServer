#ifndef BASE_BOUNDEDBLOCKQUEUE_H
#define BASE_BOUNDEDBLOCKQUEUE_H

#include <assert.h>
#include <pthread.h>
#include "Condition.h"
#include <deque>

template<typename T>
class BoundedBlockQueue {
public:
	BoundedBlockQueue(size_t num = 100) 
		: mxaSize_(num),
		  mutex_(),
		  notEmpty_(mutex_),
		  notFull_(mutex_)
	{
	}

	~BoundedBlockQueue() {}

	void clear() {
		MutexGuard lock(mutex_);
		deq_.clear();
		//while (!deq_.empty()) deq_.pop_front();
	}


	bool isEmpty() const {
		MutexGuard lock(mutex_);
		return deq_.empty();
	}

	bool isFull() const {
		MutexGuard lock(mutex_);
		return deq_.size() == mxaSize_;
	}

	T getFront() {
		MutexGuard lock(mutex_);
		while (deq_.empty()) {
			notEmpty_.wait();
		}
		assert(!deq_.empty());
		auto t(std::move(deq_.front()));
		deq_.pop_front();
		notFull_.signal();
		return t;
	}

	T getBack() {
		MutexGuard lock(mutex_);
		while (deq_.empty()) {
			notEmpty_.wait();
		}
		assert(!deq_.empty());
		auto t(std::move(deq_.back()));
		deq_.pop_back();
		notFull_.signal();
		return t;
	}

	void put(const T& x) {
		MutexGuard lock(mutex_);
		while (deq_.size() == mxaSize_) {
			notFull_.wait();
		}
		assert(deq_.size() < mxaSize_);
		deq_.push_back(x);
		notEmpty_.signal();
	}

	void put(T&& x) {
		MutexGuard lock(mutex_);
		while (deq_.size() == mxaSize_) {
			notFull_.wait();
		}
		assert(deq_.size() < mxaSize_);
		deq_.push_back(std::move(x));
		notEmpty_.signal();
	}

	size_t size() const {
		MutexGuard lock(mutex_);
		return deq_.size();
	}

	size_t capacity() const {
		return mxaSize_;
	}

private:
	mutable Mutex mutex_;
	Condition notEmpty_, notFull_;
	size_t mxaSize_;
	std::deque<T> deq_;
};



#endif
