#include "Thread.h"

#include <thread>
#include <cassert>

#include <sys/prctl.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace webserver
{

std::atomic<int> Thread::threadNum_(0);

// 获取系统id
pid_t gettid()
{ 
	return static_cast<pid_t>(::syscall(SYS_gettid)); 
}

/* thread private data */
// 线程私有的数据，如线程ID、线程ID字符串和线程名称。
namespace CurrentThread
{
	
thread_local int t_cachedTid = 0;
/* maybe string is fine */
thread_local char t_tidString[32] = {0};
thread_local int t_tidStringLength = 0;
thread_local const char *t_threadName = nullptr;

}

// 实现，获取缓存
void CurrentThread::cacheTid()
{
	if(likely(t_cachedTid == 0))
	{
		t_cachedTid = webserver::gettid();
		t_tidStringLength = 
			snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
	}
}

// 这个结构体的设计使得线程启动函数 startThread 能够接收一些参数，并且通过这些参数执行用户定义的线程操作。
// 这种方式可以更灵活地传递数据给新线程，而不是直接通过全局变量或其他方式传递参数。
// 结构体：用于传递给 pthread_create 函数的数据结构，
// 包括线程执行的函数、线程名称、线程ID和一个用于通知主线程的 CountDownLatch。
struct ThreadData
{
	typedef Thread::ThreadFunc ThreadFunc;	// ThreadFunc 是一个函数类型，用于表示线程执行的函数
	ThreadFunc func_;	// 线程执行的函数
	std::string name_;	// 线程名称
	pid_t *tid_;	// 用于存储线程ID的指针
	CountDownLatch *latch_;	// 用于通知主线程的计数锁
	
	// 构造函数，初始化 ThreadData 的各个成员
	explicit ThreadData(const ThreadFunc &func, const std::string name, 
	                    pid_t *tid, CountDownLatch *latch)
		: func_(func),
		  name_(name),
		  tid_(tid),
		  latch_(latch)
	{}
	
	// 线程实际执行的函数，通过 func_ 调用用户提供的线程执行函数
	void runInThread()
	{
		*tid_ = CurrentThread::tid();	// 获取当前线程的ID
		/* wakeup main thread */
		latch_->countDown();	// 通知主线程，当前线程已经启动
		
		/* set name of thread */
		// 设置线程名称
		CurrentThread::t_threadName = name_.data();
		prctl(PR_SET_NAME, CurrentThread::t_threadName);
		
		func_();	// 执行用户提供的线程执行函数
		CurrentThread::t_threadName = "finished";
	}
	
};
// start 函数是 Thread 类的成员函数，负责启动新线程并设置一些线程的相关信息。
// 而 startThread 函数是一个全局的自由函数，用于作为新线程的启动函数，其中执行的操作是通过 ThreadData 传递的，以实现线程的具体行为。
// 同时通过 ThreadData 结构体和 startThread 函数来处理线程的执行函数和数据传递。
// 作为线程启动函数，用于执行 ThreadData 中的 runInThread 函数。
void *startThread(void *arg)
{
	ThreadData *data = static_cast<ThreadData *>(arg);
	data->runInThread();
	delete data;
	
	return nullptr;
}

// 接受一个线程执行的函数和一个线程名称，初始化线程对象的一些属性。
Thread::Thread(const ThreadFunc &func, const std::string &name)
	: pthreadId_(0),
	  tid_(0),
	  func_(std::move(func)),
	  name_(std::move(name)),
	  started_(false),
	  joined_(false),
	  latch_(1)		/* each thread waits one second */
{
	// 设置默认的线程名称
	setDefaultName();
}

// 在析构时，如果线程已经启动但未被 join，则调用 pthread_detach 以确保线程资源能够被释放。
Thread::~Thread()
{
	if(started_ && !joined_)
	{
		pthread_detach(pthreadId_);
	}
}

// 启动线程，并在启动后等待一秒钟，以确保线程ID被成功获取。
void Thread::start()
{
	assert(!started_);
	started_ = true;
	
	// 需要传递的 结构体data
	ThreadData *data = new ThreadData(func_, name_, &tid_, &latch_);
	// 使用 startthread 创建并传参
	if(pthread_create(&pthreadId_, nullptr, &startThread, data))
	{
		started_ = false;
		delete data; 	// 如果线程创建失败，释放 ThreadData 对象的内存
	}
	else 
	{
		latch_.wait();	// 主线程等待新线程成功启动
		assert(tid_ > 0); 	// 断言确保新线程的ID大于0，即启动成功
	}
}	

// 等待线程结束，调用 pthread_join。
int Thread::join()
{
	assert(started_);
	assert(!joined_);
	
	joined_ = true;
	
	return pthread_join(pthreadId_, NULL);
}

// 设置默认的线程名称，如果用户未指定名称的话。
void Thread::setDefaultName()
{
	int num = threadNum_++;	/* atomic operation */
	if(name_.empty())
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "Thread%d", num);
		name_ = buf;
	}
}

} 
