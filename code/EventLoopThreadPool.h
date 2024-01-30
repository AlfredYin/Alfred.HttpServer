#ifndef code_EventLoopThreadPool_h
#define code_EventLoopThreadPool_h

#include <functional>
#include <vector>

#include "noncopyable.h"

namespace webserver
{

class EventLoop;
class EventLoopThread;

// EventLoopThreadPool 类的目的是管理多个事件循环线程，为主事件循环分配工作，
// 并提供获取下一个事件循环的功能。这种设计有助于提高服务器的并发性和性能。
class EventLoopThreadPool : noncopyable
{
public:
	// EventLoopThreadPool 类声明了一个类型别名 ThreadInitCallback，它是一个接受 EventLoop* 参数的函数回调。
	typedef std::function<void (EventLoop*)> ThreadInitCallback;
	
	// 接受一个指向主事件循环 baseLoop 的指针，以及线程池中的线程数量 numThreads。
	EventLoopThreadPool(EventLoop* baseLoop, int numThreads = 0);
	~EventLoopThreadPool();
	
	// 方法用于启动所有事件循环线程，并可以通过可选的回调函数 ThreadInitCallback 进行额外的初始化。
	void start(const ThreadInitCallback &cb = ThreadInitCallback());

	// 获取下一个要处理事件的事件循环对象。
	EventLoop* getNextLoop();
	
private:
	// 存储主事件循环的指针。
	EventLoop* baseLoop_;	/* main loop */
	bool started_;
	int numThreads_;

	// 表示下一个要获取事件循环对象的线程索引。
	int next_;
	// 是一个存储事件循环线程的指针的向量。
	std::vector<EventLoopThread *> threads_;
	// 是一个存储事件循环对象的指针的向量。
	std::vector<EventLoop *> loops_;	
};

}//namespace webserver

#endif
