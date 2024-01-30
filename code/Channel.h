#ifndef code_Channel_h
#define code_Channel_h

#include <memory>
#include <functional>

#include <sys/epoll.h>

namespace webserver
{

class EventLoop;
struct channelHash;
struct channelCmp;

/* event dispatcher
 * obligation: handle activate events in this file descriptor 
 * owner: EventLoop, and each file descriptor corresponds to a Channel
 */
// 事件调度器
// 职责：在这个文件描述符中 处理这个活跃事件
// 拥有者：事件循环。 并且 每个文件描述符对应一个Channel

// Channel 类继承自 std::enable_shared_from_this<Channel>，
// 这是为了允许在类成员函数中使用 shared_from_this，以避免悬空指针问题。
class Channel : public std::enable_shared_from_this<Channel>
{
public:
	typedef std::function<void ()> EventCallback;

	Channel(int fd, EventLoop *loop);
	~Channel();
	
	/* event dispatcher */
	// 处理事件。
	void handleEvent();

	/*
	std::move 是 C++ 中的一个函数模板，位于头文件 <utility> 中。
	它用于将对象的所有权从一个对象转移到另一个对象，通常用于支持移动语义。
	移动语义是 C++11 引入的一个特性，旨在提高对资源的高效管理，特别是在处理动态分配的内存和移动语义可用的情况下。
	具体来说，std::move 接受一个对象作为参数，并返回一个右值引用，将该对象标记为“可移动”的。
	这并不实际移动任何数据，而只是告诉编译器该对象的所有权可以被转移。这对于实现移动构造函数和移动赋值运算符非常有用。
	移动语义的主要优势在于避免不必要的复制和内存分配，提高程序性能。
	当你有一个临时对象或即将被销毁的对象时，通过使用 std::move，你可以使用它的资源而不进行深层复制。这对于容器、智能指针等类的实现中尤为重要。
	*/
	
	// 设置事件回调函数的函数
	void setReadCallback(EventCallback cb)
	{ readCallback_ = std::move(cb); }
	void setWriteCallback(EventCallback cb)
	{ writeCallback_ = std::move(cb); }
	void setCloseCallback(EventCallback cb)
	{ closeCallback_ = std::move(cb); }
	void setErrorCallback(EventCallback cb)
	{ errorCallback_ = std::move(cb); }
	
	// 获取事件和状态信息的函数
	int getFd() const { return fd_; }
	int events() const { return events_; }
	int revents() const { return revents_; }
	void set_revents(int revt) { revents_ = revt; }
	bool isNoneEvent() const { return events_ == kNoneEvent; }
	
	// 启用或禁用事件的函数
	void enableReading() 
	{ events_ |= (kReadEvent | EPOLLET); update(); }
	
	void disableReading() 
	{ events_ &= ~kReadEvent; update(); }
	
	void enableWriting() 
	{ events_ |= (kWriteEvent | EPOLLET); update(); }
	
	void disableWriting() 
	{ events_ &= ~kWriteEvent; update(); }
	
	void disableAll() 
	{ events_ = kNoneEvent; update(); }
	
	// 查询事件状态的函数
	bool isWriting() const 
	{ return revents_ & kWriteEvent; }
	
	bool isReading() const 
	{ return revents_ & kReadEvent; }

	// 查询事件是否已启用的函数
	bool isEnableReading() const 
	{ return events_ & kReadEvent; }

	bool isEnableWriting() const
	{ return events_ & kWriteEvent; }

	// Channel 拥有者是哪个 事件循环
	EventLoop *ownerLoop() const 
	{ return loop_; }
	
private:
	// 更新事件状态
	void update();
	
private:
	const int fd_;
	// 目标事件。
	int events_;
	// 实际发生的事件。
	int revents_;
	EventLoop * const loop_;
	
	// 四个回调函数
	EventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
	
	/* ET mode */
	// 不同类型事件的常量。
	// 没有事件
	static const int kNoneEvent = 0;
	// 读事件
	static const int kReadEvent = EPOLLIN | EPOLLPRI;
	// 写事件
	static const int kWriteEvent = EPOLLOUT;
};

// 用于 std::unordered_set 中的哈希函数对象。
// 这两个函数对象都是为了在基于 std::shared_ptr<Channel> 类型的容器中进行元素操作而定义的。
// 在使用这些函数对象的地方，可以像下面这样使用它们：

// std::unordered_set<std::shared_ptr<Channel>, channelHash> myUnorderedSet;
// std::set<std::shared_ptr<Channel>, channelCmp> mySet;
// 在这里，myUnorderedSet 使用了哈希函数对象，而 mySet 使用了比较函数对象。

struct channelHash
{
	std::size_t operator()(const std::shared_ptr<Channel> &key) const
	{ 
		return std::hash<int>()(key->getFd());
	}
};

// 用于 std::set 中的比较函数对象。
struct channelCmp
{
	bool operator()(const std::shared_ptr<Channel> &lhs, 
	                const std::shared_ptr<Channel> &rhs) const
	{
		return lhs->getFd() < rhs->getFd();
	}
};

} //namespace webserver

#endif
