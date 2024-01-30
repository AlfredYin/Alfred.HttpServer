#include <cassert>

#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#include "EventLoop.h"
#include "Thread.h"
#include "CurrentThread.h"

#include "Channel.h"

using namespace webserver;

// 处理事件的回调函数
void WriteCallback(int listenFd){
	printf("WriteCallback(%d) \n",listenFd);

	int connFd=accept(listenFd,nullptr,nullptr);
	printf("connFd=%d\n",connFd);

	char buf[30]="";
	sprintf(buf,"connected fd=%d",connFd);
	write(connFd,buf,sizeof(buf));
	
	// 处理业务 延迟10s 因为单线程会阻塞 不会接受其他连接。如果有其他的connfd连接上来将会等待。因此这个要使用多线程
	sleep(10);
	close(connFd);
}

void threadFunc()
{
	printf("threadFunc(): pid=%d, tid=%d\n", getpid(), CurrentThread::tid());
	
	// 这个断言检查当前线程是否已经关联了任何 EventLoop 对象。在主线程开始时，应该尚未关联任何事件循环对象。
	assert(EventLoop::getEventLoopOfCurrentThread() == nullptr);

	// 创建一个 EventLoop 对象，表示一个事件循环。
	EventLoop loop;

	// 这个断言检查当前线程是否已经成功关联到刚刚创建的 EventLoop 对象。在这里，主线程关联到了新创建的事件循环。
	assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

	// 在事件循环中调用 runInLoop 方法，该方法用于在事件循环中执行指定的任务。
	// 这里使用 std::bind 绑定了 functor 函数和参数，表示在事件循环中执行打印 "Hello, World" 的任务。
	
	// FILE *file = fopen("example.txt", "w");
	
	// loop.runInLoop(std::bind(&functor, "Hello World said Alfred !!!",file));

	// 创建套接字
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    // 设置服务器地址信息
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // 使用8080端口
    serverAddr.sin_addr.s_addr = INADDR_ANY;

	// 绑定地址
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding address" << std::endl;
        close(serverSocket);
        return;
    }

	// 监听连接
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening for connections" << std::endl;
        close(serverSocket);
        return;
    }

	std::cout << "Server listening on port 8080..." << std::endl;

	std::shared_ptr<Channel> acceptChannel(new Channel(serverSocket,&loop));

	acceptChannel->setReadCallback(std::bind(&WriteCallback,serverSocket));
	acceptChannel->enableReading();

	// 启动事件循环，使之进入等待事件的状态。
	// 在这个例子中，由于之前注册了一个任务，因此事件循环会执行这个任务，即执行 functor("Hello, World")。
	loop.loop();
}

int main(int argc, char *argv[])
{
	printf("main(): pid=%d, tid=%d\n", getpid(), CurrentThread::tid());
	
	// 创建事件循环并且绑定当前线程 t_loopInThisThread = this;
	EventLoop mainloop;
	// 断言 判断是否绑定的是本身线程
	// assert(EventLoop::getEventLoopOfCurrentThread() == &mainloop);
	
	// 开启子线程
	Thread t1(threadFunc);

	printf("t1.start();");
	t1.start();
	
	printf("loop.loop();");
	// 开启事件循环
	mainloop.loop();

	printf("t1.join();");
	t1.join();
	
	return 0;
}