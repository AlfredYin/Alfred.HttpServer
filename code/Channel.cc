#include "Channel.h"

#include "EventLoop.h"
#include "utils.h"

#include "config.h"

namespace webserver
{

/* The file descriptor is bound to the Channel */
Channel::Channel(int fd, EventLoop *loop)
	: fd_(fd),
	  events_(0),
	  revents_(0),
	  loop_(loop)	// 是Main函数中的 mainLoop_主循环 将监听的任务交给主函数 传入 Channel对象
{}

/* Channel负责关闭文件描述符 */
Channel::~Channel()
{
	//printf("dtor channel\n");
	utils::Close(fd_);
}

/* event dispatcher */
// 事件处理函数：
// 根据发生的不同类型的事件调用相应的回调函数。
void Channel::handleEvent()
{

#ifdef CHANNELDEBUG
	printf("void Channel::handleEvent() \n");
#endif // CHANNELDEBUG

	// 如果 revents_ 处于 EPOLLHUP IN 状态 调用 关闭的回调函数
	if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
	{
		if(closeCallback_) closeCallback_();
		return ;
	}
	
	if(revents_ & EPOLLERR)
	{
		if(errorCallback_) errorCallback_();
		return ;
	}
	
	if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
	{
		if(readCallback_) readCallback_();
	}
	
	if(revents_ & EPOLLOUT)
	{
		if(writeCallback_) writeCallback_();
	}
}

/* update state of the Channel in poller */
/* may be ineffecient */
// 更新函数
// 调用 EventLoop 对象的 updateChannel 方法，以更新通道在轮询器中的状态。
// 该函数使用 shared_from_this() 返回当前对象的 shared_ptr，以确保对象在更新期间不被销毁。
// 注释中提到这可能是不高效的，因为这个操作可能会导致频繁的共享指针的创建和销毁。
void Channel::update(void)
{

#ifdef CHANNELDEBUG
	printf("void Channel::update(void) \n");
#endif // CHANNELDEBUG

	// 调用更新Channel 参数是 本身对象
	// 在这个里面调用 处理 handleEvent
	loop_->updateChannel(shared_from_this());
}
 
} //webserver
