#include <string>
#include <vector>

#include "coroutine/fiber.h"
#include "base/thread.h"
#include "zchlog.h"

void run_in_fiber2() {
    LOG_INFO() << "run_in_fiber2 begin";
    LOG_INFO() << "run_in_fiber2 end";
}

void run_in_fiber() {
    LOG_INFO() << "run_in_fiber begin";

    LOG_INFO() << "before run_in_fiber yield";
    Fiber::GetThis()->yield();
    LOG_INFO() << "after run_in_fiber yield";

    LOG_INFO() << "run_in_fiber end";
    // fiber结束之后会自动返回主协程运行
}

void test_fiber() {
    LOG_INFO() << "test_fiber begin";

    // 初始化线程主协程
    Fiber::GetThis();

    Fiber::ptr fiber(new Fiber(run_in_fiber, 0, false));
    LOG_INFO() << "use_count:" << fiber.use_count(); // 1

    LOG_INFO() << "before test_fiber resume";
    fiber->resume();
    LOG_INFO() << "after test_fiber resume";

    /** 
     * 关于fiber智能指针的引用计数为3的说明：
     * 一份在当前函数的fiber指针，一份在MainFunc的cur指针
     * 还有一份在在run_in_fiber的GetThis()结果的临时变量里
     */
    LOG_INFO() << "use_count:" << fiber.use_count(); // 3

    LOG_INFO() << "fiber status: " << fiber->getState(); // READY

    LOG_INFO() << "before test_fiber resume again";
    fiber->resume();
    LOG_INFO() << "after test_fiber resume again";

    LOG_INFO() << "use_count:" << fiber.use_count(); // 1
    LOG_INFO() << "fiber status: " << fiber->getState(); // TERM

    fiber->reset(run_in_fiber2); // 上一个协程结束之后，复用其栈空间再创建一个新协程
    fiber->resume();

    LOG_INFO() << "use_count:" << fiber.use_count(); // 1
    LOG_INFO() << "test_fiber end";
}

int main(int argc, char *argv[]) {

    // 初始化日志系统 (加载配置文件)
    zch::InitLogFromJson("/home/zch/Project/TinyWebserver/config/log_config.json");

    LOG_INFO() << "main begin";

    std::vector<Thread::ptr> thrs;
    for (int i = 0; i < 2; i++) {
        thrs.push_back(Thread::ptr(
            new Thread(&test_fiber, "thread_" + std::to_string(i))));
    }

    for (auto i : thrs) {
        i->join();
    }

    LOG_INFO() << "main end";
    return 0;
}
