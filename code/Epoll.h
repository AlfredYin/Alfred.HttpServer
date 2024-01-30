#ifndef code_Epoll_h
#define code_Epoll_h

#include <vector>
#include <memory>
#include <unordered_map>

#include <sys/epoll.h>

#include "noncopyable.h"

namespace webserver
{

class Channel;

/* 
 * io-multiplex
 * oligation: add/modify/delete concerned events and 
 *            return activate events
 * owner: EventLoop, guaranteed by unique_ptr
*/
// IO 多路复用
// 职责： 添加/修改/删除 关注的事件 并且 返回激活的事件
// 拥有者： EventLoop ， 由唯一指针保障

// Epoll 类继承自 noncopyable，表明该类是不可拷贝的，防止通过拷贝构造函数和赋值运算符进行复制。
class Epoll : noncopyable
{
public:
	typedef std::shared_ptr<Channel> SP_Channel;
	typedef std::vector<SP_Channel> ChannelVector;
	
	Epoll();
	~Epoll();
	
	// poll 函数用于进行事件轮询，返回激活的事件。
	ChannelVector poll(int timeout);
	
	// 更新和删除通道。
	void updateChannel(SP_Channel &channel);
	void removeChannel(SP_Channel &channel);

private:
	/* internel function, be invoked by update and remove channels */
	// 内部函数，由更新和删除通道调用，实时更新。
	int updateEvent(SP_Channel &channel, int operation);
	
private:
	// epoll 实例的文件描述符。
	int epollFd_;
	
	/* be used for epoll */
	// 存储 epoll 事件的数组。
	std::vector<epoll_event> events_;	
	
	/* The mapping of the file descriptor to the channel */
	/* record those monitored file descriptors */
	// 文件描述符到 SP_Channel 的映射。
	std::unordered_map<int, SP_Channel> channelMap_;
	
	// 初始化时用于事件数组的大小。
	static const int kInitEventSize = 16;
};

} //namespace webserver 


#endif
