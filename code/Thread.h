#ifndef code_Thread_h
#define code_Thread_h

#include <pthread.h>

#include <functional>
#include <atomic>

#include "CountDownLatch.h"
#include "noncopyable.h"
#include "CurrentThread.h"

namespace webserver
{

// 以防止拷贝和赋值操作
// 接受一个线程执行的函数和一个线程名称，初始化线程对象的一些属性。
class Thread : noncopyable
{
public:
	// 表示线程运行的函数对象类型。
	typedef std::function<void()> ThreadFunc;

	// 构造函数，接受一个函数对象和线程名称作为参数，可选地提供线程名称，默认为空字符串。
	explicit Thread(const ThreadFunc &func, const std::string &name = std::string());
	~Thread();
	
	// 启动线程的函数。
	void start();
	// 检查线程是否已经启动。
	bool started() const { return started_; }
	
	// 等待线程结束。
	int join();
	
	// 获取线程ID。
	pid_t tid() const { return tid_; }

	// 获取线程名称。
	const std::string &name() const { return name_; }
	
	// 获取已创建的线程数量。
	static int numCreated() { return threadNum_; }
	
private:
	// 设置线程的默认名称。
	void setDefaultName();
	
private:
	pthread_t pthreadId_;
	pid_t tid_;
	ThreadFunc func_;
	std::string name_;
	bool started_;
	bool joined_;
	/* make sure to get the correct tid after the thread runs */
	// 用于确保在获取线程ID之前线程已经成功运行。
	CountDownLatch latch_;
	
	// 已创建的线程数量。
	static std::atomic<int> threadNum_;
};
	
} //namespace webserver


#endif