#include "EventLoop.h"

#include <cassert>

#include <sys/eventfd.h>
#include <unistd.h>

#include "Epoll.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "HttpHandler.h"
#include "HttpManager.h"

#include "config.h"

namespace webserver
{

// 是一个线程局部变量，用于保存当前线程的 EventLoop 实例指针。
thread_local EventLoop *t_loopInThisThread = nullptr;

// 轮询的超时时间。
static const int kEPollTimeMs = 10000;	//10s

// 该函数用于创建一个用于事件唤醒的文件描述符（eventfd）
int createEventFd()
{
	int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if(evtfd < 0)
	{
		fprintf(stderr, "failed in eventfd\n");
		abort();
	}
	return evtfd;
}

// 初始化了成员变量，包括事件循环状态、线程ID、事件轮询器、唤醒通道、HTTP 管理器等。
EventLoop::EventLoop()
	: looping_(false),		// 是否
	  quit_(false),
	  threadId_(CurrentThread::tid()),		// 记录事件循环的线程 pid
	  poller_(new Epoll()),
	  wakeupFd_(createEventFd()),		// 创建唤醒Fd
	  wakeupChannel_(new Channel(wakeupFd_, this)),		// 创建唤醒通道
	  callingPendingFucntors_(false),
	  manager_(new HttpManager(this))
{
	// 确保每个线程只能拥有一个 EventLoop 实例
	if(unlikely(t_loopInThisThread))
	{
		/* one threads only owns one eventloop */
		fprintf(stderr, "one threads only owns one eventloop\n");
		abort();
	}
	t_loopInThisThread = this;
	
	/* may be wakeuped from other thread */
	// 设置唤醒通道的回调函数和启用读事件监听
	wakeupChannel_->setReadCallback(
		std::bind(&EventLoop::wakeupRead, this));
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
	::close(wakeupFd_);
	t_loopInThisThread = nullptr;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

void EventLoop::loop()
{
	// 断言 没有开始进入循环
	assert(!looping_);
	assert(isInLoopThread());
	
	looping_ = true;

	// Channels 指针数组 活跃
	ChannelVector activeChannels;
	// 没有退出变量即执行
	while(!quit_)
	{
		activeChannels.clear();
		/* acquire activate events */
		// 获取活跃的事件
		activeChannels = poller_->poll(kEPollTimeMs);
		
		/* handle activate events */
		/* ?? may have a rece condition ?? */
		for(auto &it : activeChannels)
		{
			/* 处理读写，并更新状态 */
			it->handleEvent();

			/* 根据状态反馈Http */
			/* acceptFd_, wakeupfd是没有插入到httpMap */
			/* 上诉fd，不进入该分支 */
			manager_->handler(it);
		}
		
		/* handle extra functors */
		doPendingFunctors();
	}
	looping_ = false;
}

void EventLoop::quit()
{
	quit_ = true;
	/* non-eventloop thread quits the eventloop */
	if(!isInLoopThread())
	{
		wakeup();
	}
}

// 写空事件事件，来唤醒事件循环线程
void EventLoop::wakeup()
{
	// 写入一个值到唤醒文件描述符，唤醒事件循环线程
	uint64_t one = 1;
	ssize_t nbytes = ::write(wakeupFd_, &one, sizeof(one));
	if(unlikely(nbytes != sizeof(one))){
		fprintf(stderr, "wakeup failed\n");
	}
}

// wakeup的回调函数
void EventLoop::wakeupRead()
{
	// 从唤醒文件描述符读取一个值，用于清除事件循环线程的唤醒状态
	uint64_t one; 
	ssize_t nbytes = ::read(wakeupFd_, &one, sizeof(one));
	if(unlikely(nbytes != sizeof(one)))
	{
		fprintf(stderr, "wakeupRead failed\n");
	}
}

/* be used in eventloop */
void EventLoop::runInLoop(Functor &&cb)
{

pid_t pid=webserver::CurrentThread::tid();
printf("事件循环线程=%d pid = %d \n",threadId_,pid); // 事件线程是 处理listenfd的线程

	// 函数用于检查当前线程是否是记录事件循环线程。
	if(likely(isInLoopThread()))
	{
printf("isInLoopThread() 当前线程 是 记录事件循环线程 \n");
		// 直接执行传入的回调函数 cb()。
		cb();
	}
	else
	{
		// 如果当前线程不是记录事件循环线程，调用 queueInLoop 函数，
		// 将回调函数 cb 移动到事件循环线程的队列中等待执行。
		queueInLoop(std::move(cb));
	}
}

/* be used in other threads */
// 将回调函数放入事件循环线程的队列中，并在有需要的情况下唤醒事件循环线程。
// 这种设计可以确保在多线程环境中，当其他线程需要向事件循环线程添加任务时，能够正确地将任务加入队列并通知事件循环线程执行。
void EventLoop::queueInLoop(Functor &&cb)
{
	{
		// 临时加锁
		std::unique_lock<std::mutex> lock(mutex_);
		pendingFunctors_.push_back(cb);		// 放入要执行的回调函数的队列
	}

	// 如果当前线程不是事件循环线程 (!isInLoopThread()) 或者正在处理回调函数 (callingPendingFucntors_)，
	// 则调用 wakeup() 唤醒事件循环线程。
	if(!isInLoopThread() || callingPendingFucntors_)
	{
		// 唤醒事件循环线程
		wakeup();
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFucntors_ = true;
	
	/* use a local variable to reduce critical region */
	{
		std::unique_lock<std::mutex> lock(mutex_);
		functors.swap(pendingFunctors_);
	}
	for(auto &functor : functors)
	{
		functor();
	}
	
	callingPendingFucntors_ = false;
}

/* void EventLoop::updateChannel(SP_Channel &&channel) */
/* shared_from_this() return a temporary variable(rvalue) */
void EventLoop::updateChannel(SP_Channel channel)
{
	assert(isInLoopThread());
	assert(channel->ownerLoop() == this);
	
	poller_->updateChannel(channel);
}

void EventLoop::removeChannel(SP_Channel channel)
{ 
	poller_->removeChannel(channel);
	manager_->delHttpConnection(channel);
}

void EventLoop::addHttpConnection(SP_HttpHandler handler)
{
	// http请求的管理 和 新加上handler以便于管理所有 handler
	manager_->addNewHttpConnection(handler);
}

void EventLoop::flushKeepAlive(SP_Channel &channel, HttpManager::TimerNode &node)
{
	manager_->flushKeepAlive(channel, node);
}

} //namespace webserver
