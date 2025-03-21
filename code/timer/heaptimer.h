#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

//节点类
struct TimerNode {
    int id;
    TimeStamp expires;  // 超时时间点
    TimeoutCallBack cb; // 回调function<void()>
    bool operator<(const TimerNode& t) {    // 重载比较运算符
        return expires < t.expires;
    }
    bool operator>(const TimerNode& t) {    // 重载比较运算符
        return expires > t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }  // 保留（扩充）容量
    ~HeapTimer() { clear(); }
    
    // 调整超时时间
    void adjust(int id, int newExpires);
    void add(int id, int timeOut, const TimeoutCallBack& cb);
    void doWork(int id);
    void clear();
    void tick();
    void pop();
    int GetNextTick();
    int size() { return heap_.size(); }

private:
    void del_(size_t i);
    //加入一个节点后，要将小的节点向上调整。
    //参数是vector的下标
    void siftup_(size_t i);
    //向下调整。
    bool siftdown_(size_t i, size_t n);
    //传入的参数为 vector 的下标，通过这两个下标来交换。
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    // key:id, value:vector的下标
    // id对应的在heap_中的下标，方便用heap_的时候查找
    std::unordered_map<int, size_t> ref_;
};

#endif //HEAP_TIMER_H
