#include "Epoll.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <unistd.h>

#include "macros.h"
#include "Channel.h"

/* may use 'fd' instead of 'channel *' is better */
namespace webserver
{
	
Epoll::Epoll()
	: epollFd_(::epoll_create(EPOLL_CLOEXEC)),
	  events_(kInitEventSize)
{
	assert(epollFd_ > 0);
}

Epoll::~Epoll()
{
	::close(epollFd_);
}

/* ET mode */
Epoll::ChannelVector Epoll::poll(int timeout)
{
	// 调用 epoll_wait 函数等待事件，将结果存储在 events_ 中
	int numEvents = ::epoll_wait(epollFd_, 
	                             events_.data(),
								 static_cast<int>(events_.size()),
								 timeout);	/* ms */

	// 如果 epoll_wait 返回值小于 0，表示发生错误，输出错误信息
	if(unlikely(numEvents < 0))
	{
		perror("epoll_wait");
	}
		
	ChannelVector activeChannels;
	activeChannels.reserve(numEvents);

	// 遍历 events_ 数组，将每个活跃通道添加到 activeChannels 中
	for(int i=0; i<numEvents; ++i)
	{
		int fd = events_[i].data.fd;
		channelMap_[fd]->set_revents(events_[i].events);
		activeChannels.push_back(channelMap_[fd]);
	}
	
	/* enlarge space to try to read all events once */
	// 如果 events_ 数组的大小等于 numEvents，表示空间不足，将其扩大为原来的两倍
	if(unlikely(static_cast<int>(events_.size()) == numEvents))
	{
		events_.resize(2*events_.size());
	}
	
	// 返回存储了活跃通道的 vector
	return activeChannels;
}

void Epoll::updateChannel(SP_Channel &channel)
{
	int fd = channel->getFd();
	if(channelMap_.count(fd))
	{
		/* already exists, then modify it */
		updateEvent(channel, EPOLL_CTL_MOD);

		//channelMap_[fd] = channel;
	}
	else
	{
		/* add a new one */
		updateEvent(channel, EPOLL_CTL_ADD);
		channelMap_.insert(std::make_pair(fd, channel));
	}
}

void Epoll::removeChannel(SP_Channel &channel)
{
	int fd = channel->getFd();
	if(unlikely(!channelMap_.count(fd)))
	{
		fprintf(stderr, "channel no found\n");
		return ;
	}
	/* delete a monitored event */
	updateEvent(channel, EPOLL_CTL_DEL);
	channelMap_.erase(fd);
}

int Epoll::updateEvent(SP_Channel &channel, int operation)
{
	int fd = channel->getFd();
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	
	ev.events = channel->events();
	ev.data.fd = fd;
	if(unlikely(::epoll_ctl(epollFd_, operation, fd, &ev) < 0))
	{
		fprintf(stderr, "epoll_ctl");
		return -1;
	}
	
	return 0;
}

} //namespace webserver