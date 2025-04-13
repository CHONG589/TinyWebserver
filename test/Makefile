CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = test
OBJS = ../code/log/*.cpp ../code/pool/*.cpp \
       ../code/http/*.cpp \
       ../code/buffer/*.cpp ./test.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ./$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ./$(TARGET)




