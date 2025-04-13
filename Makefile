CXX = g++
CFLAGS = -std=c++11 -O2 -Wall -g 

TARGET = server
OBJS = ./code/log/*.cpp ./code/pool/*.cpp ./code/coroutine/*.cpp \
       ./code/http/*.cpp \
       ./code/buffer/*.cpp ./code/*.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ./$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ./$(TARGET)
