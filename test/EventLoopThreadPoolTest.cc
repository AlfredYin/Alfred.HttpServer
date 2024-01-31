#include <iostream>
#include <cassert>
#include <functional>

#include <sys/types.h>
#include <unistd.h>

#include "EventLoopThreadPool.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "EventLoop.h"

void print(webserver::EventLoop *p = nullptr)
{
	printf("main(): pid=%d, tid=%d, loop=%p\n",
	       getpid(), webserver::CurrentThread::tid(), p);
}

void init(webserver::EventLoop *p)
{
	printf("init(): pid=%d, tid=%d, loop=%p\n",
	       getpid(), webserver::CurrentThread::tid(), p);
}

int main(int argc, char *argv[])
{
	print();

	webserver::EventLoop loop;
	
	// 主线程开启线程池
	// {
	// 	printf("Single thread %p:\n", &loop);

	// 	// 创建线程池，将主事件循环传入
	// 	webserver::EventLoopThreadPool model(&loop, 0);

	// 	// 在主线程中执行 init函数
	// 	model.start(init);

	// 	// 没有创建子线程 仍然是返回的是 主线程的 事件循环
	// 	assert(model.getNextLoop() == &loop);
	// 	assert(model.getNextLoop() == &loop);
	// 	assert(model.getNextLoop() == &loop);
	// }
	
	{
		printf("Another thread:\n");
		webserver::EventLoopThreadPool model(&loop, 1);
		model.start(init);
		webserver::EventLoop* nextLoop = model.getNextLoop();
		
		// 子线程的事件循环
		assert(nextLoop != &loop);

		// 主线程的事件循环
		assert(nextLoop == model.getNextLoop());
	}
	
	// {
	// 	printf("Three threads:\n");
	// 	webserver::EventLoopThreadPool model(&loop, 3);
	// 	model.start(init);
	// 	webserver::EventLoop* nextLoop = model.getNextLoop();
	// 	nextLoop->runInLoop(std::bind(print, nextLoop));
	// 	assert(nextLoop != &loop);
	// 	assert(nextLoop != model.getNextLoop());
	// 	assert(nextLoop != model.getNextLoop());
	// 	assert(nextLoop == model.getNextLoop());
	// }
	
	loop.loop();
	return 0;
}