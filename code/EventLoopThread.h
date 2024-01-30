#ifndef code_EventLoopThread_h
#define code_EventLoopThread_h

#include <mutex>
#include <condition_variable>
#include <functional>

#include "Thread.h"
#include "noncopyable.h"

namespace webserver
{

// 总体而言，这个类的目的是封装了一个事件循环线程的管理器，提供了启动线程、获取事件循环指针等功能。
// 用户可以通过传递回调函数来进行额外的初始化操作。这种设计有助于简化多线程编程中的事件循环的创建和管理。
class EventLoop;

// 事件循环线程管理器 EventLoopThread 的声明
class EventLoopThread : noncopyable
{
public:
	// EventLoopThread 类声明了一个类型别名 ThreadInitCallback，它是一个接受 EventLoop* 参数的函数回调。
	typedef std::function<void (EventLoop*)> ThreadInitCallback;
	
	// 接受一个可选的回调函数参数，该回调函数在线程初始化时被调用，默认为空。
	EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
	~EventLoopThread();
	
	// 方法用于启动事件循环线程，并返回事件循环的指针。
	EventLoop* startLoop();

private:

	// 方法是实际的线程执行函数，会被 startLoop() 调用，在该方法中会调用回调函数以及运行事件循环。
	void threadFunc();
	
	// 是事件循环指针，用于存储线程中创建的事件循环对象。
	EventLoop* loop_;
	// 是一个标志，表示线程是否退出。
	bool exit_;

	// 是一个线程对象，用于管理事件循环线程的创建和销毁。
	Thread thread_;

	// 互斥锁和条件变量，用于线程间的同步。
	std::mutex mutex_;
	std::condition_variable cond_;

	// 是在线程初始化时调用的回调函数，用于执行额外的初始化操作。
	ThreadInitCallback callback_;
};
	
} //namespace webserver


#endif
