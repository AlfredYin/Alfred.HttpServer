#include "EventLoopThread.h"

#include <cassert>

#include "EventLoop.h"
#include "config.h"

namespace webserver
{
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb)
	: loop_(nullptr),	// 在事件循环线程中初始化 事件循环对象为nullptr
	  exit_(false),
	  thread_(std::bind(&EventLoopThread::threadFunc, this)),	// 线程对象
	  callback_(cb)	// 在线程创建EventLoop 对象后，通过回调函数进行额外的初始化
{
#ifdef EVENTLOOPTHREADDEBUG
	printf("EventLoopThread::EventLoopThread(const ThreadInitCallback &cb) \n");
#endif // EVENTLOOPTHREADDEBUG
}

// 设置 exit_ 为 true，然后调用 quit() 终止 EventLoop 的事件循环。
// 使用 thread_.join() 等待线程结束。
EventLoopThread::~EventLoopThread()
{
#ifdef EVENTLOOPTHREADDEBUG
	printf("EventLoopThread::~EventLoopThread() \n");
#endif // EVENTLOOPTHREADDEBUG
	exit_ = true;
	loop_->quit();
	thread_.join();
}

// 启动线程，并通过 cond_.wait(lock) 等待 EventLoop 对象的创建和初始化。
// 返回指向创建的 EventLoop 对象的指针。
// 如果已经返回了loop_,
EventLoop *EventLoopThread::startLoop()
{

#ifdef EVENTLOOPTHREADDEBUG
	printf("EventLoop *EventLoopThread::startLoop() \n");
#endif // EVENTLOOPTHREADDEBUG
	
	// 断言确保线程尚未启动。
	assert(!thread_.started());
	// 线程启动
	thread_.start();
	
	{
		// 通过 cond_.wait(lock) 等待 EventLoop 对象的创建和初始化。
		// 这是一个独占锁，它会在构造时锁住 mutex_，在析构时释放锁。
		std::unique_lock<std::mutex> lock(mutex_);
		
		// 防止虚假唤醒
		while(loop_ == nullptr)
		{
			// 如果 loop_ 是 nullptr，则调用 cond_.wait(lock) 进入等待状态。
			// 这会释放锁，并等待条件变量 cond_ 被通知。
			// 当其他地方调用了 cond_.notify_one()，当前线程会重新获取锁，并继续执行。
			cond_.wait(lock);

#ifdef EVENTLOOPTHREADDEBUG
	printf("EventLoop *EventLoopThread::startLoop() { cond_.wait(lock); } \n");
#endif // EVENTLOOPTHREADDEBUG
		}
	}
	
	// 创建好的 EventLoop 对象的指针返回给调用者
	return loop_;
}

// EventLoopThread 类中的 threadFunc 函数，该函数运行在新线程中。
void EventLoopThread::threadFunc()
{

#ifdef EVENTLOOPTHREADDEBUG
	printf("void EventLoopThread::threadFunc() \n");
#endif // EVENTLOOPTHREADDEBUG

	// 在 事件循环线程中 创建 事件循环对象 在栈上分配内存，对象执行析构函数释放内存
	EventLoop loop;
	
	// 如果在构造 EventLoopThread 对象时提供了回调函数 callback_，
	// 则调用该回调函数，并将新创建的 EventLoop 对象的指针传递给回调函数。
	// 这允许用户在新线程中执行额外的初始化工作。
	if(callback_)
	{
		callback_(&loop);
	}
	
	{
		// 创建一个独占锁，并使用 mutex_ 进行加锁。
		std::unique_lock<std::mutex> lock(mutex_);
		loop_ = &loop;
		// 通过条件变量 cond_ 通知正在等待的线程，即 EventLoopThread::startLoop 函数中的等待条件。
		cond_.notify_one();

#ifdef EVENTLOOPTHREADDEBUG
	printf("void EventLoopThread::threadFunc() { cond_.notify_one(); } \n");
#endif // EVENTLOOPTHREADDEBUG
	}
	
	// 调用 EventLoop 对象的 loop() 函数，启动事件循环。
	// 这将使新线程一直处于事件循环中，直到 EventLoop 对象被销毁或事件循环被终止。

#ifdef EVENTLOOPTHREADDEBUG
	printf("void EventLoopThread::threadFunc() { loop.loop(); // 开始进入事件循环 (即 开始事件监听) } \n");
#endif // EVENTLOOPTHREADDEBUG

	loop.loop();
}

} //namespace webserver