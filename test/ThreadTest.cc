#include <iostream>

#include <sys/types.h>
#include <unistd.h>

#include "Thread.h"

void threadFunc1()
{
	printf("threadFunc1() tid=%d\n", webserver::CurrentThread::tid());
}

void threadFunc2(int x)
{
	printf("threadFunc2() tid=%d, x=%d\n", webserver::CurrentThread::tid(), x);
}

class Foo
{
public:
	explicit Foo(double x)
		: x_(x)
	{}
	
	void memberFunc1()
	{
		printf("memberFunc1() tid=%d, Foo::x_=%f\n", webserver::CurrentThread::tid(), x_);
	}
	
	void memberFunc2(const std::string &text)
	{
		printf("memberFunc2() tid=%d, Foo::x_=%f, text=%s\n", 
			   webserver::CurrentThread::tid(), x_, text.data());
	}
	
private:
	double x_;
};

int main(int argc, char *argv[])
{
	// 获取主进程的pid tid
	printf("pid=%d, tid=%d\n\n", ::getpid(), webserver::CurrentThread::tid());
	
	// 传参是全局函数
	webserver::Thread t1(threadFunc1);
	t1.start();
	printf("t1.tid=%d\n", t1.tid());
	t1.join();

	printf("\n");
	
	// 创建并启动线程 t2，执行带参数的全局函数 threadFunc2，参数为 42
	webserver::Thread t2(std::bind(threadFunc2, 42), "function with arguments");
	t2.start();
	printf("t2.tid=%d\n", t2.tid());
	t2.join();
	
	printf("\n");

	// 创建 Foo 对象 foo，创建并启动线程 t3，执行对象的成员函数 memberFunc1
	// 在这里，std::bind(&Foo::memberFunc1, &foo) 创建了一个函数对象，它绑定到 Foo::memberFunc1 成员函数，
	// 并将 foo 对象作为调用成员函数时的对象。这个绑定的函数对象被传递给 webserver::Thread 的构造函数，
	// 因此线程 t3 在执行时会调用 Foo::memberFunc1 函数，而 foo 对象被作为参数传递给了这个函数。
	Foo foo(87.53);
	webserver::Thread t3(std::bind(&Foo::memberFunc1, &foo), 
		                 "object function with arguments");
	t3.start();
	printf("t3.tid=%d\n", t3.tid());
	t3.join();
	
	printf("\n");

	// 线程t4 执行对象的成员函数 2
	// webserver::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo),
	//                      std::string("Hello, World")));
	webserver::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo),
	                     std::string("Hello, World")),"abc");					 
	t4.start();
	printf("t4.tid=%d\n", t4.tid());
	printf("t4.pthreadName=%s\n", t4.name().c_str());
	// t4.join() 的作用是等待线程 t4 的执行结束。
	// 调用 join 函数会使当前线程阻塞，直到被调用的线程（在这里是 t4 线程）执行完成
	t4.join();

	printf("\n");

	// 如果不调用 join，主线程可能会在子线程执行完之前结束，这可能导致一些问题，
	// 尤其是在子线程分配了资源或执行了一些关键操作时。
	// 通过调用 join，主线程会等待子线程执行完成，然后再继续执行后续的代码。
	
	// 这里的主要目的是演示 Thread 对象在其生命周期结束时，会调用析构函数。
	// 在 Thread 类的析构函数中，如果线程已经启动但没有被 join，
	// 它会调用 pthread_detach 函数，将线程设置为可分离状态，以确保线程资源会在线程退出时被正确释放。

	// 在 C++ 中，花括号 {} 可以用来创建一个作用域，也称为块或者局部作用域。这被称为局部作用域或块作用域，
	// 在这个范围内定义的变量和对象在离开这个范围时会被销毁。这对于控制变量的生命周期以及资源管理是非常有用的。
	{
		// 在作用域内创建并启动线程 t5，执行全局函数 threadFunc1
		webserver::Thread t5(threadFunc1);
		t5.start();
		::sleep(1);

		// 因为 t5 是在作用域内创建的局部对象，当它离开作用域时，其析构函数会被调用。
		/* t5 will destruct */
	}
	
	// 理解错误，没有这种创建方式
	// // 使用全局的 startThread() 创建 
	// webserver::Thread::startThread();

	/* wait for t5 destruction */
	// 等待t5 析构
	::sleep(2);
	
	printf("\n");

	// 线程新建 线程count++ 线程结束 count不变 
	printf("number of created threads %d\n", webserver::Thread::numCreated());
	
	return 0;
}
