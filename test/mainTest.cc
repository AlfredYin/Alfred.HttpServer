#include <iostream>
#include "HttpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"

/* 6核12线程 */
int main(int argc, char *argv[])
{
	webserver::InetAddress self_addr(8080);

	//  创建了一个事件循环对象 EventLoop，这是一个通常在异步网络编程中使用的概念，用于处理事件的循环。
	webserver::EventLoop mainLoop;
	
	// 创建了一个 HttpServer 对象，该对象用于处理 HTTP 请求。
	// 构造函数的参数包括之前创建的事件循环对象 mainLoop，服务器监听的地址 self_addr，以及似乎是服务器的线程池大小（可能是 3 个线程）。
	webserver::HttpServer server(&mainLoop, self_addr, 12);
	
	server.start();
	
	// 进入事件循环，程序会一直在这里等待并处理事件，直到程序被显式终止。在这里，事件循环主要用于处理异步操作，例如接收和处理来自客户端的 HTTP 请求。
	mainLoop.loop();
	
	return 0;
}