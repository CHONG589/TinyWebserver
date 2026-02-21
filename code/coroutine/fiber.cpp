#include <atomic>
#include <assert.h>

#include "fiber.h"
#include "scheduler.h"

// 全局静态变量，用于生成协程id
static std::atomic<uint64_t> s_fiber_id{0};
// 全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t> s_fiber_count{0};

// 线程局部变量，当前线程正在运行的协程
static thread_local Fiber *t_fiber = nullptr;
// 线程局部变量，当前线程的主协程，切换到这个协程，就相当于切换到了主线程
// 中运行，智能指针形式
static thread_local Fiber::ptr t_thread_fiber = nullptr;

// 协程栈默认大小 128k
static uint32_t fiber_stack_size = 128 * 1024;

// malloc 栈内存分配器
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }
    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

/**
* @brief 无参构造函数，只用于创建线程的第一个协程，也就是线程主函数对应的协程
* @note 只能由 GetThis() 调用，创建线程的第一个协程，也就是线程主函数对应的
* @author zch
*/
Fiber::Fiber() {
    SetThis(this); // 设置 t_fiber
    m_state = RUNNING;
    // 获取主协程的上下文
    if (getcontext(&m_ctx)) {
        LOG_ERROR() << "getcontext error!";
        assert(false);
    }

    ++s_fiber_count;
    m_id = s_fiber_id++;  // 协程 id 从 0 开始，用完加 1

    LOG_INFO() << "Main fiber " << m_id << " created!";
}

/**
* @brief 构造函数
* @param[in] cb 协程入口函数
* @param[in] stacksize 栈大小
* @param[in] run_in_scheduler 本协程是否参与调度器调度
@ @author zch
*/ 
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler)
    : m_id(s_fiber_id++)
    , m_cb(cb)
    , m_runInScheduler(run_in_scheduler) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : fiber_stack_size;
    m_stack     = StackAllocator::Alloc(m_stacksize);

    if (getcontext(&m_ctx)) {
        LOG_ERROR() << "getcontext error!";
        assert(false);
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    // 将此上下文 m_ctx 与 MainFunc 函数绑定，当调度器让此上下文的协程
    // 运行时，就执行 MainFunc 函数，就会执行绑定在协程里面的 cb。在
    // 前面列表初始化中已经为 cb 初始化。
    // 为什么不是直接将 m_cb 绑定在这个 m_ctx 中呢？
    // 因为运行到该协程时，不单单就只是执行函数那么简单，你还要为
    // 执行完函数后的一系列处理，如状态变化，yield 等操作，都要协程
    // 执行完后自动管理，而不是后面通过用户管理。
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    LOG_INFO() << "Task fiber " << m_id << " created!";
}

/**
* @brief 析构函数，线程的主协程析构时需要特殊处理，因为主协程没有分配栈和 cb
@ @author zch
*/
Fiber::~Fiber() {
    LOG_INFO() << "Fiber " << m_id << " destroyed!";
    --s_fiber_count;
    if (m_stack) {
        // 有栈，说明是子协程，需要确保子协程一定是结束状态
        if (m_state != TERM) {
            LOG_ERROR() << "Fiber " << m_id << " not TERM state, can't destroy!";
            assert(false);
        }
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        // 没有栈，说明是线程的主协程
        // 主协程没有cb
        assert(!m_cb);
        // 主协程一定是执行状态
        assert(m_state == RUNNING); 

        // 当前协程就是自己
        Fiber *cur = t_fiber; 
        if (cur == this) {
            // 析构了就要设置为空
            SetThis(nullptr);
        }
    }
}

/**
* @brief 获取当前协程，同时充当初始化当前线程主协程的作用，这个函数在使用协程之前要调用一下
@ @author zch
*/ 
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    // 这里相当于调用了无参构造函数，创建了线程的主协程
    Fiber::ptr main_fiber(new Fiber);
    assert(t_fiber == main_fiber.get());
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}

/**
* @brief 重置协程状态和入口函数，复用栈空间，不重新创建栈
* @param[in] cb 协程入口函数
@ @author zch
*/
void Fiber::reset(std::function<void()> cb) {
    if (!m_stack) {
        LOG_ERROR() << "Fiber " << m_id << " want reset(), but m_stack, can't reset()!";
        assert(false);
    }

    // 这里为了简化状态管理，强制只有 TERM 状态的协程才可以重置，
    // 但其实刚创建好但没执行过的协程也应该允许重置的
    if (m_state != TERM) {
        LOG_ERROR() << "Fiber " << m_id << " want reset(), but it not TREM, can't reset()!";
        assert(false);
    }

    m_cb = cb;
    if (getcontext(&m_ctx)) {
        LOG_ERROR() << "getcontext error!";
        assert(false);
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = READY;
}

/**
* @brief 恢复协程运行
@ @author zch
*/
void Fiber::resume() {
    // resume 时协程的状态只能为 READY 
    if (m_state != READY) {
        LOG_ERROR() << "Fiber " << m_id << " resume not READY, can't resume, resume only is READY!";
        assert(false);
    }

    // 设置 t_fiber
    SetThis(this);
    m_state = RUNNING;

    // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (m_runInScheduler) {
        // 第一个参数为获取调度器协程,m_ctx为调用者的上下文，即调用 resume() 的协程。
        // 使用 swapcontext() 后就切换到调用者的上下文了。
        if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
            LOG_WARN() << "Fiber " << GetFiberId() << " and schedule main fiber swap fail!";
            assert(false);
        }
    } else {
        if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx)) {
            LOG_WARN() << "Fiber " << GetFiberId() << " and main fiber swap fail!";
            assert(false);
        }
    }
}

/**
* @brief 暂停当前协程运行，切换到主协程运行
@ @author zch
*/
void Fiber::yield() {
    //协程里面的函数执行完后会先将状态置为TERM,所以yield时，状态可以为 TERM
    //也可以为RUNNING，所以就是不能是READY
    if (m_state == READY) {
        LOG_ERROR() << "Fiber " << m_id << " yield is READY, can't yield, yield not is READY!";
        assert(false);
    }

    SetThis(t_thread_fiber.get());

    if (m_state != TERM) {
        //说明是函数执行到一半被切换掉的。
        m_state = READY;
    }

    // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (m_runInScheduler) {
        if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
            LOG_WARN() << "Fiber " << GetFiberId() << " and schedule main fiber swap fail!";
            assert(false);
        }
    } else {
        if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx))) {
            LOG_WARN() << "Fiber " << GetFiberId() << " and main fiber swap fail!";
            assert(false);
        }
    }
}

/**
* @brief 协程入口函数，所有协程都是从这里开始执行的
@ @author zch
*/
void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    if (!cur) {
        LOG_ERROR() << "Fiber::MainFunc cur is nullptr";
        assert(false);
    }

    // 执行该协程的任务
    cur->m_cb();
    cur->m_cb    = nullptr;
    cur->m_state = TERM;

    // 手动让 t_fiber 的引用计数减1
    auto raw_ptr = cur.get(); 
    cur.reset();
    raw_ptr->yield();
}

/**
* @brief 获取当前协程 id
@ @author zch
*/
uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

/**
* @brief 设置当前正在运行的协程，即设置线程局部变量t_fiber的值
* @param[in] f 要设置为当前正在运行的协程的指针
@ @author zch
*/
void Fiber::SetThis(Fiber *f) { 
    t_fiber = f; 
}
