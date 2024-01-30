#ifndef code_EventLoop_h
#define code_EventLoop_h

#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

#include "CurrentThread.h"
#include "HttpManager.h"

namespace webserver
{

class Epoll;
class Channel;
class HttpHandler;
class HttpManager;

class EventLoop
{
public:
	typedef std::function<void ()> Functor;
	// SP_Channel 是一个指向 Channel 类对象的共享指针类型。
	typedef std::shared_ptr<Channel> SP_Channel;
	// 这个向量用于存储多个 Channel 对象的共享指针，通常用于表示一组通道。
	typedef std::vector<SP_Channel> ChannelVector;
	// 管理 HttpHandler 对象的生命周期。
	typedef std::shared_ptr<HttpHandler> SP_HttpHandler;
	
	EventLoop();
	~EventLoop();
	
	// 启动事件循环。
	void loop();
	// 终止事件循环。
	void quit();

	/* run callback immdiately in the loop thread */
	// 在事件循环线程中立即运行回调函数。
	void runInLoop(Functor &&cb);
	
	/* queue callback in the loop thread */
	// 将回调函数排队到事件循环线程中执行。
	void queueInLoop(Functor &&cb);
	
	/* assert whether in the loop thread or not */
	// 检查当前线程是否为事件循环线程。
	bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
	
	/* internel usage */
	// 内部使用的方法，用于更新和移除事件循环的通道。
	void updateChannel(SP_Channel channel);
	void removeChannel(SP_Channel channel);
	
	//  执行排队的回调函数。
	void doPendingFunctors();
	
	// 唤醒事件循环线程的方法。
	void wakeupRead();
	void wakeup();
	
	// 获取当前线程的事件循环对象。
	static EventLoop* getEventLoopOfCurrentThread();
	
	// 支持 HTTP 的方法，用于添加 HTTP 连接和刷新保持连接。
	/* support Http */
	void addHttpConnection(SP_HttpHandler handler);
	void flushKeepAlive(SP_Channel &channel, HttpManager::TimerNode &node);
	
private:
	// 标志着事件循环是否处于运行状态。
	bool looping_;
	// 标志着是否需要退出事件循环。
	bool quit_;
	// 记录事件循环所属的线程 ID。
	pid_t threadId_;
	// 持有一个 Epoll 对象，用于事件的轮询和管理。
	std::unique_ptr<Epoll> poller_;
	// 用于唤醒事件循环线程的文件描述符。
	int wakeupFd_;
	
	// 持有一个 Channel 对象，该通道与 wakeupFd_ 相关联，用于处理唤醒事件。在构造函数中，设置了读事件的回调函数，并启用了读事件监听。
	std::shared_ptr<Channel> wakeupChannel_;
	// 存储当前轮询到的活跃通道，即有事件发生的文件描述符集合。在 loop() 函数中，用于处理这些活跃通道的事件。
	ChannelVector activateChannels_;
	
	// 标志着是否正在执行排队的回调函数。在 doPendingFunctors() 函数中，用于防止在处理回调函数时再次调用 wakeup()。
	bool callingPendingFucntors_;
	// 存储待执行的回调函数。在 queueInLoop() 函数中，将回调函数放入这个队列中，在 doPendingFunctors() 函数中执行这些回调函数。
	std::vector<Functor> pendingFunctors_;
	
	// 互斥锁，用于保护 pendingFunctors_ 队列的访问。
	std::mutex mutex_;	/* be used by functor vector */
	
	/* 由事件循环处理各个Http请求 */
	/* 先调用Channel的handleEvent，接受数据 */
	/* 各个事件循环管理Http连接（通断，清理） */
	std::unique_ptr<HttpManager> manager_;
};

}

#endif
