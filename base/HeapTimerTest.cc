#include "HeapTimer.h"
#include <cmath>
#include <iostream>
#include <set>

int main() {
	HeapTimer* heapTimer = new HeapTimer();
	std::set<size_t> st;
	for (int i = 0; i < 20; ++i) {
		size_t time = rand() / 100;
		//std::cout << time << " ";
		//if (i == 30) std::cout << "\n";
		st.insert(time);
		heapTimer->addTimer(new TimerNode(time));
	}

	auto timer = new TimerNode(10241024);
	heapTimer->addTimer(timer);
	
	auto timer2 = new TimerNode(102410299);
	heapTimer->addTimer(timer2);
	
	heapTimer->addTimer(new TimerNode(900241024));
	heapTimer->addTimer(new TimerNode(900241024));

	heapTimer->adjustTimer(timer2, 78900);
	heapTimer->adjustTimer(timer, 100);

	//for (auto i : st) std::cout << i << " ";
	//std::cout << "\n";
	heapTimer->tick();
}
