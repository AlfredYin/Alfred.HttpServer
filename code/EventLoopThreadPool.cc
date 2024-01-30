#include "EventLoopThreadPool.h"

#include <cassert>

#include "EventLoop.h"
#include "macros.h"
#include "EventLoopThread.h"

#include "config.h"

namespace webserver
{
	
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
	: baseLoop_(baseLoop),
	  started_(false),
	  numThreads_(numThreads),
	  next_(0)
{
#ifdef EVENTLOOPTHREADPOOLBUG
	printf("EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads) \n");
#endif // EVENTLOOPTHREADPOOLBUG
}

EventLoopThreadPool::~EventLoopThreadPool()
{
	// Don't delete loop, it's stack variable
}	

// typedef std::function<void (EventLoop*)> ThreadInitCallback;
// 启动所有事件循环线程。首先确保线程池尚未启动，并且主事件循环在当前线程中。
// EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
	assert(unlikely(!started_));
	assert(unlikely(baseLoop_->isInLoopThread()));

#ifdef EVENTLOOPTHREADPOOLBUG
	printf("void EventLoopThreadPool::start(const ThreadInitCallback &cb) \n");
#endif // EVENTLOOPTHREADPOOLBUG
	
	// 然后，循环创建 EventLoopThread 对象，将其加入 threads_ 向量，并启动线程，将返回的事件循环指针加入 loops_ 向量。
	// 如果线程池中没有子线程，且提供了回调函数 cb，则直接在主事件循环上执行回调函数。
	started_ = true;
	
	// cb 默认为空
	for(int i=0; i<numThreads_; ++i)
	{
		// 在事件循环线程池 中 创建事件循环线程 
		EventLoopThread *t =new EventLoopThread(cb);
		
		// 线程和事件循环存入数组
		threads_.push_back(t);
		loops_.push_back(t->startLoop());
	}
	
	if(numThreads_ == 0 && cb)
	{
		cb(baseLoop_);
	}
}

//sub-reactor
// 函数用于获取下一个可用的事件循环对象，采用轮询（round-robin）方式选择。
// 如果线程池中存在子线程，则按顺序选择下一个线程的事件循环，实现了基本的负载均衡。
EventLoop* EventLoopThreadPool::getNextLoop()
{

#ifdef EVENTLOOPTHREADPOOLBUG
	printf("EventLoop* EventLoopThreadPool::getNextLoop() //sub-reactor \n");
#endif // EVENTLOOPTHREADPOOLBUG

	assert(unlikely(baseLoop_->isInLoopThread()));
	EventLoop *loop = baseLoop_;
	
	if(!loops_.empty())
	{
		//round-robin
		loop = loops_[next_++];
		if(static_cast<size_t>(next_) >= loops_.size())
		{
			next_ = 0;
		}
	}
	
	return loop;
}

} //namespace webserver
