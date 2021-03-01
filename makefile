CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cc ./base/HeapTimer.cc ./web/HttpConnect.cc ./base/Log.cc ./base/SqlConnPool.cc Webserver.cc config.cc ./base/Utils.cc
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient

clean:
	rm  -r server
