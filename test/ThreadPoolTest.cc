#include "ThreadPool.h"
#include "CountDownLatch.h"

void print()
{
	printf("%s\n", "Hello,World");
}

void printString(const std::string &str)
{
	printf("%s\n", str.c_str());
}

int main()
{
	// 创建线程池对象，自动创建线程
	webserver::ThreadPool pool(6, 100);
	pool.start();
	
	/* no arguments function*/
	// 添加一个无参数的任务，多个线程竞争执行
	pool.addTask(print);
	
	/* function with arguments */
	// 添加多个函数带有参数，多个线程竞争执行
	for(int i=0; i<100; ++i)
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "task %d", i);

		// 带有参数添加
		pool.addTask(std::bind(&printString, std::string(buf)));
	}
	
	// 这段代码目的是 等待 消息清空，然后 停止线程池
	/* member functions with no argument */
	// 创建初始值为1的计数器
	webserver::CountDownLatch latch(1);

	// 这个任务是执行 CountDownLatch::countDown 函数，将 latch 计数减一。
	pool.addTask(std::bind(&webserver::CountDownLatch::countDown, &latch));
	
	//  等待 latch 计数变为零。由于前面的任务会执行 countDown，所以 latch 的计数会减为零。
	latch.wait();
	// 调用 pool.stop() 停止线程池。
	pool.stop();
	
	return 0;
}
