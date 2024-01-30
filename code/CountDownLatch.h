#ifndef code_CountDownLatch_h
#define code_CountDownLatch_h

#include <mutex>
#include <condition_variable>

#include "noncopyable.h"

namespace webserver
{

class CountDownLatch : noncopyable
{
public:
	// explicit 用于声明构造函数，防止编译器进行隐式类型转换。
	// // 显式调用构造函数
	// CountDownLatch latchExplicit(5);

	// 构造函数，接受一个初始计数值作为参数，初始化计数器。
	explicit CountDownLatch(int count);
	
	// 等待计数器归零的函数。如果计数器不为零，将阻塞当前线程，直到有其他线程调用 countDown 函数使得计数器归零。
	void wait();
	// 将计数器减一。通常由其他线程调用，用于递减计数器的值。
	void countDown();
	// 获取当前计数器的值。
	int getCount();
	
private:
	// 互斥锁，用于保护对计数器的操作。 
	mutable std::mutex mutex_;
	// 条件变量，用于实现线程的等待和唤醒。
	std::condition_variable condition_;
	// 计数器的值。
	int count_;
};

}

#endif
