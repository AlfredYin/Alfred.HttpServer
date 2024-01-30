#ifndef code_ThreadPool_h
#define code_ThreadPool_h

#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

#include "Thread.h"
#include "noncopyable.h"

namespace webserver
{
// 构造函数 ThreadPool(int threadNum, int maxQueueSize) 用于创建线程池对象，参数包括线程数量和任务队列的最大大小。
class ThreadPool : noncopyable
{
public:
	// 是一个任务的类型，是一个无参数无返回值的函数类型
	typedef std::function<void ()> Task;
	// 构造函数 ThreadPool(int threadNum, int maxQueueSize) 用于创建线程池对象，参数包括线程数量和任务队列的最大大小。
	ThreadPool(int threadNum, int maxQueueSize);
	~ThreadPool();
	
	// 启动线程池中的所有线程。
	void start();
	void stop();
	
	// 函数用于向线程池中添加任务。
	int addTask(const Task &task);
	
private:
	// 每个线程执行的函数，它会不断地从任务队列中取出任务并执行。
	void runInThread();
	// 从任务队列中取出一个任务。
	Task take();
	
private:
	// 用于实现线程安全的互斥锁和条件变量。
	std::mutex mutex_;
	std::condition_variable condition_;

	// 存储线程对象的容器。
	std::vector<std::unique_ptr<webserver::Thread>> threads_;

	// 存储任务的任务队列。
	std::deque<Task> queue_;	/* fifo */

	// 任务队列最大大小和线程池中线程的最大数量。
	int maxQueueSize_;
	int maxThreadSize_;
	// 线程池是否在运行状态.
	bool running_;
};

} //webserver

#endif
