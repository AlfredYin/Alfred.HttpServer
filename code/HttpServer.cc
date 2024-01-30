#include "HttpServer.h"

#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "HttpHandler.h"
#include "Channel.h"
#include "macros.h"
#include "utils.h"
#include "config.h"

// main loop need a acceptor
// dispatch new connections to other event loops
// HttpAck.cc HttpReq.cc
/* auto-machine, setStateTrackCallback: each http requests is divided to some stage handler ?? */
/* 应用层keepalive机制 */
namespace webserver
{
	
HttpServer::HttpServer(EventLoop *loop, const InetAddress &addr, int numThreads)
	: mainLoop_(loop),		// 将 主事件循环 传递给 HttpServer对象
	  numThreads_(numThreads),		// 最大线程数
	  threadPool_(new EventLoopThreadPool(mainLoop_, numThreads_)),	// 使用事件循环(同时将主事件循环(mainLoop)传递给 EventLoopThreadPool的对象) 和最大线程数来创建线程池
	  listenFd_(utils::SocketBindListen(addr)),	//使用 SocketBindListen 函数创建并绑定到指定地址的监听套接字。
	  acceptChannel_(new Channel(listenFd_, mainLoop_)),		// 负责监听的Channel，start绑定Read的回调函数 使用 监听fd和主循环 创建Channel
	  started_(false),
	  idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))	// 打开空闲文件描述符
{
	// 判断 fd > 0
	assert(listenFd_ > 0);
	assert(idleFd_ > 0);
	
	// 允许在同一端口上快速重用处于TIME_WAIT状态的套接字
	utils::setReuseAddr(listenFd_, true);

	// 设置信号处理以忽略 SIGPIPE。
	utils::IgnoreSigpipe();
}

HttpServer::~HttpServer()
{
	assert(!started_);	
}

void HttpServer::start()
{

#ifdef DEBUG
	printf("void HttpServer::start()\n");
#endif // DEBUG

	assert(!started_);
	started_ = true;
	
	// 设置Channel读的回调函数，并且启动 Reading
	// 将接受通道的读回调设置为 acceptor 方法，并启用通道进行读取。
	/* main loop be used to accept new connections */
	acceptChannel_->setReadCallback(std::bind(&HttpServer::acceptor, this));
	acceptChannel_->enableReading();
	
	// 启动线程池。
	threadPool_->start();
}

// 此方法负责接受新连接。
// 每次由 listenfd有Reading时，即调用一次
void HttpServer::acceptor()
{

#ifdef DEBUG
	printf("void HttpServer::acceptor() \n");
#endif // DEBUG	

	InetAddress addr(0);
	int connfd;
	
	// 接受新连接
	// 使用非阻塞的 AcceptNb 方法接受新连接，并以边缘触发模式进行处理。
	//edge trigger mode
	while((connfd = utils::AcceptNb(listenFd_, addr)) > 0 || errno == EMFILE)
	{
		// 在文件描述符耗尽的情况下（EMFILE 错误），通过关闭一个空闲文件描述符并重新打开它来应用解决方法。
		/* File descriptor exhausted */
		if(unlikely(errno == EMFILE))
		{
			::close(idleFd_);
			idleFd_ = ::accept(listenFd_, NULL, NULL);
			::close(idleFd_);
			idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
			continue;
		}
		
		// 对于每个接受的连接，创建一个新的 HttpHandler 实例，
		// 与线程池中的一个事件循环相关联，并将其添加到事件循环的队列中以进一步处理。

		// 从线程池中获取一个事件循环对象 EventLoop。
		// 每一个事件循环都有一个 httpManager
		// 将 处理连接上来的connsocket 交给 threadPool来处理；
		EventLoop *loop = threadPool_->getNextLoop();
		// 创建一个新的 HttpHandler 实例，该实例与上面获取的事件循环相关联，并传入新连接的文件描述符 connfd。
		std::shared_ptr<HttpHandler> handler(new HttpHandler(loop, connfd));
		
		// 将新的 HttpHandler 实例添加到事件循环的队列中以进行进一步处理。
		loop->queueInLoop(std::bind(&EventLoop::addHttpConnection, loop, handler));	

#ifdef DEBUG
	printf("fd=%d, %s1\n", connfd, addr.toIpPortString().c_str());
#endif // DEBUG	
	}
}

}//namespace webserver
