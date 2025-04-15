CXX = g++
CFLAGS = -std=c++14 -O0 -Wall -g 

TARGET = test
OBJS = ../code/log/*.cpp ../code/pool/*.cpp ../code/coroutine/*.cpp \
       ../code/http/*.cpp \
       ../code/buffer/*.cpp ../code/*.cpp ./test.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ./$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ./$(TARGET)




