#ifndef code_HttpServer_h
#define code_HttpServer_h

#include <memory>
#include <vector>

#include "InetAddress.h"

namespace webserver
{
	
class Channel;
class EventLoop;
class EventLoopThreadPool;

class HttpServer
{
public:
	// 处理事件的循环，网络配置的结构体，最大线程数
	// webserver::HttpServer server(&mainLoop, self_addr, 12); server.start(); mainLoop.loop();
	HttpServer(EventLoop *loop, const InetAddress &addr, int numThreads);
	~HttpServer();
	
	// 开始服务器
	void start();

	// 此方法处理socket新连接
	void acceptor();
	
private:
	// 指向主事件循环的指针。
	EventLoop *mainLoop_;

	// 服务器要使用的线程数。
	int numThreads_;

	// 指向 EventLoopThreadPool 对象的独特指针。
	std::unique_ptr<EventLoopThreadPool> threadPool_;

	// 表示用于监听的文件描述符的整数。
	int listenFd_;

	// 指向 Channel 对象的共享指针。
	// 监听fd的Channel ，在里面设置 监听处理的回调函数，此处应为单线程的处理
	std::shared_ptr<Channel> acceptChannel_;

	// 服务器是否已启动。
	bool started_;
	// 表示空闲文件描述符的整数。
	int idleFd_;
};

}//namespace webserver

#endif
