#include "ThreadPool.h"

#include <cassert>

#include "CurrentThread.h"

namespace webserver
{

ThreadPool::ThreadPool(int threadNum, int maxQueueSize)
	: maxQueueSize_(maxQueueSize), 
	  maxThreadSize_(threadNum),
	  running_(false)
{
	assert(maxQueueSize > 0);
	assert(threadNum > 0);
	
	// 调用 std::vector 的 reserve 函数，预分配 threads_ 向量的存储空间，避免在后续添加线程时多次分配内存，提高性能。
	threads_.reserve(threadNum);
	
	// 表示先前作者可能考虑过预分配 queue_ 队列的存储空间，但是最终没有启用。
	// 这可能是因为队列是一个动态数据结构，会根据需要自动扩展。
	//queue_.reserve(maxQueueSize);
}

ThreadPool::~ThreadPool()
{
	if(running_)
	{
		stop();
	}
}

// start 函数的作用是启动线程池中的所有线程。每个线程都执行 
// ThreadPool::runInThread 函数，该函数是线程池中线程实际执行的内容。此函数会不断从任务队列中取出任务并执行。
void ThreadPool::start()
{
	assert(!running_);
	running_ = true;
	
	// 遍历线程池中的每一个线程。
	for(int i=0; i<maxThreadSize_; ++i)
	{
		char id[32];
		snprintf(id, sizeof(id), "%d", i+1);

		// emplace_back 函数向 threads_ 向量中添加一个新的 Thread 对象
		threads_.emplace_back(new webserver::Thread(
		                     std::bind(&ThreadPool::runInThread, this), id));
		threads_[i]->start();
	}
}

void ThreadPool::stop()
{
	{
		std::unique_lock<std::mutex> lock(mutex_);
		running_ = false;
		condition_.notify_all(); /* wakeup all threads */
	}
	for(auto &thread : threads_)
	{
		thread->join();
	}
}

/* producer */
int ThreadPool::addTask(const ThreadPool::Task &task)
{
	// 判断任务队列是否已满，如果已满，表示无法再添加任务，直接返回 -1。
	if(static_cast<int>(queue_.size()) >= maxQueueSize_)
		return -1;	/* the task queue is filled */
	
	// 创建一个 std::unique_lock 对象 lock，使用线程池的互斥锁 mutex_ 进行上锁。
	// 这是为了确保对任务队列的操作是线程安全的。
	std::unique_lock<std::mutex> lock(mutex_);
	
	// 将任务 task 添加到任务队列中。这里使用 std::move 将任务对象的所有权转移到队列中，避免不必要的复制开销。
	queue_.push_back(std::move(task));
	
	// 通知一个等待在条件变量上的线程，表示有新的任务可以执行。这会唤醒一个等待中的线程，使其尝试获取任务执行。
	condition_.notify_one();
	
	return 0;
}

/* consumer */
void ThreadPool::runInThread()
{
	printf("new thread, tid=%d\n", webserver::CurrentThread::tid());
	
	// 进入一个无限循环，只要 running_ 为 true，线程就会不断执行。
	while(running_)
	{
		// 从任务队列中取出一个任务，并将其存储在 task 变量中.
		Task task(take());

		// 判断是否成功取出了一个有效的任务，如果是，则执行任务。
		if(task)
		{
			task();
		}
	}
}

ThreadPool::Task ThreadPool::take()
{
	// 加锁
	std::unique_lock<std::mutex> lock(mutex_);
	/* spurious wakeup */
	
	// 竞争等待生产事件
	while(queue_.empty() && running_)
	{
		condition_.wait(lock);
	}
	
	// 从消息队列，获取生产资料，返回获取事件
	Task task;
	if(!queue_.empty())
	{
		task = queue_.front();
		queue_.pop_front();
	}
	
	return task;
}

} //webserver
