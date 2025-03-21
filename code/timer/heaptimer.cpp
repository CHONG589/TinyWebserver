#include "heaptimer.h"

void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i <heap_.size());
    assert(j >= 0 && j <heap_.size());
    swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;    // 结点内部id所在索引位置也要变化
    ref_[heap_[j].id] = j;    
}

void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t parent = (i-1) / 2;
    while(parent >= 0) {
        if(heap_[parent] > heap_[i]) {
            SwapNode_(i, parent);
            i = parent;
            parent = (i-1)/2;
        } else {
            break;
        }
    }
}

// false：不需要下滑  true：下滑成功
bool HeapTimer::siftdown_(size_t i, size_t n) {
    assert(i >= 0 && i < heap_.size());
    assert(n >= 0 && n <= heap_.size());    // n:共几个结点
    auto index = i;
    //child 为 i 的左孩子
    auto child = 2*index+1;
    while(child < n) {
        if(child+1 < n && heap_[child+1] < heap_[child]) {
            //如果 i 的右孩子小于 n，且右孩子小于左孩子，那么
            //将 child 指向右孩子，因为右孩子比较小。 
            child++;
        }
        //到达这里后，child 指向的孩子是比较小的那一个
        if(heap_[child] < heap_[index]) {
            //如果 child 指向的孩子小于 i 指向的父节点，那么
            //交换两个节点。
            SwapNode_(index, child);
            //将当前的 child 节点作为父节点，继续向下调整。
            index = child;
            child = 2*child+1;
        }
        //好像这里不用break，不然，这里就执行了一次就跳出？
        //不是等 child >= n 才跳出吗？
        //break;
    }
    return index > i;
}

// 删除指定位置的结点
void HeapTimer::del_(size_t index) {
    assert(index >= 0 && index < heap_.size());
    // 将要删除的结点换到队尾，然后调整堆
    size_t tmp = index;
    size_t n = heap_.size() - 1;
    assert(tmp <= n);
    // 如果就在队尾，就不用移动了
    if(index < heap_.size()-1) {
        //将要删除的节点换至最后一个节点
        SwapNode_(tmp, heap_.size()-1);
        if(!siftdown_(tmp, n)) {
            //siftdown不行，说明不应该向下调整，而是向上调整
            siftup_(tmp);
        }
    }
    //删除最后一个节点。
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

// 调整指定id的结点，即修改新的 expires
void HeapTimer::adjust(int id, int newExpires) {
    //ref_.count(id)表示在 map 中存不存在 key 为 id 的。
    assert(!heap_.empty() && ref_.count(id));
    heap_[ref_[id]].expires = Clock::now() + MS(newExpires);
    siftdown_(ref_[id], heap_.size());
}

// 增加一个新的结点，并触发堆调整
void HeapTimer::add(int id, int timeOut, const TimeoutCallBack& cb) {
    assert(id >= 0);
    // 如果有，则调整
    if(ref_.count(id)) {
        //如果此 id 本来就已经存在，那么就是修改 expires 和 cb 函数
        int tmp = ref_[id];
        heap_[tmp].expires = Clock::now() + MS(timeOut);
        heap_[tmp].cb = cb;
        if(!siftdown_(tmp, heap_.size())) {
            siftup_(tmp);
        }
    } 
    else {
        //如果没有，那么增加在堆尾，然后向上调整。
        size_t n = heap_.size();
        ref_[id] = n;
        //heap_.push_back({id, Clock::now() + MS(timeOut), cb});  // 右值
        TimerNode newnode;
        newnode.id = id;
        newnode.expires = Clock::now() + MS(timeOut);
        newnode.cb = cb;
        heap_.push_back(newnode);
        siftup_(n);
    }
}

// 作用是触发回调函数，并删除指定id的结点。
void HeapTimer::doWork(int id) {
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    auto node = heap_[i];
    node.cb();  // 触发回调函数
    del_(i);
}

// 触发所有超时结点的回调函数，并删除超时结点。
// 因为存储方式是小根堆的形式，所以每次只弹出堆头部的结点，
// 而堆头的节点callback函数一定是最先超时的，所以只要遇到
// 不超时，就跳出循环。
void HeapTimer::tick() {
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

// 删除堆头部的结点。
void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

/**
 * 经过 tick() 后，保证了堆中的连接都是没有超时的，这时就可以获取
 * 堆中下一个超时时间，并返回。
 */
int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}
