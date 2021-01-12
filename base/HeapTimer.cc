#include "HeapTimer.h"


size_t HeapTimer::findTimer(TimerNode *timer) {
	if (!timer) return 0;
	size_t pos = 1;
	while (pos < heap_.size() && heap_[pos] != timer) ++pos;
	return pos == heap_.size() ? 0 : pos;
}

void HeapTimer::heapDown(size_t i) {
	while (1) {
		size_t left_child = 2*i, right_child = 2*i + 1;
		if (left_child >= heap_.size()) break;
		size_t j = left_child;
		if (right_child < heap_.size() && heap_[left_child]->expire_ > heap_[right_child]->expire_) {
			j = right_child;
		}
		if (heap_[j]->expire_ >= heap_[i]->expire_) break;
		std::swap(heap_[i], heap_[j]);
		i = j;
	}	
}


void HeapTimer::heapUp(size_t i) {
	while (i > 1) {
		size_t j = i/2;
		if (heap_[i]->expire_ >= heap_[j]->expire_) break;
		std::swap(heap_[i], heap_[j]);
		i = j;
	}	
}


void HeapTimer::addTimer(TimerNode* timer) {
	if (!timer) return;
	heap_.push_back(timer);
	heapUp(heap_.size() - 1);	
}


void HeapTimer::adjustTimer(TimerNode* timer, size_t newExpireTime) {
	size_t pos = findTimer(timer);
	if (pos == 0) return;
	heap_[pos]->expire_ = newExpireTime;
	if (pos != 1 && heap_[pos]->expire_ < heap_[pos/2]->expire_) heapUp(pos);
	else heapDown(pos);
}


void HeapTimer::delTimer(TimerNode* timer) {
	size_t pos = findTimer(timer);
	if (pos == 0) return;
	
	std::swap(heap_[pos], heap_[heap_.size()-1]);
	//调用析构？
	heap_.pop_back();
	if (pos != 1 && heap_[pos]->expire_ < heap_[pos/2]->expire_) heapUp(pos);
	else heapDown(pos);
}


void HeapTimer::tick() {
	time_t now = time(nullptr);
	while (heap_.size() > 1) {
		//if (heap_[1]->expire_ < now) break;

		heap_[1]->func();		
		//如何调用回调函数？
		std::swap(heap_[1], heap_[heap_.size()-1]);
		heap_.pop_back();
		heapDown(1);	
	}
}
