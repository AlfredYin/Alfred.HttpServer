#include <iostream>
#include <cassert>
#include <functional>
#include "EventLoopThread.h"

void printStr(const std::string &str)
{
	printf("%s\n", str.c_str());
}

class Foo
{
public:
	void printStr()
	{
		printf("this is foo\n");
	}
};

int main(int argc, char *argv[])
{

	// EventLoopThread 类的实现可能包含了在单独线程中运行事件循环的逻辑。

	// 这个类的实例可以通过 startLoop 方法启动事件循环线程，并返回指向事件循环对象的指针。
	// 两个不同的事件循环对象（loop1 和 loop2）在两个不同的线程中运行，并由 assert 语句确保它们不相等。


	// 创建一个 EventLoopThread 对象 elt1，用于运行 printStr 函数
	webserver::EventLoopThread elt1(std::bind(&printStr, "Hello, World"));
	// 启动 elt1 的事件循环线程，并获取指向事件循环对象的指针 loop1
	webserver::EventLoop *loop1 = elt1.startLoop();
	
	// 创建一个 Foo 对象 foo
	Foo foo;
	// 创建一个 EventLoopThread 对象 elt2，用于运行 Foo::printStr 函数
	webserver::EventLoopThread elt2(std::bind(&Foo::printStr, &foo));
	// 启动 elt2 的事件循环线程，并获取指向事件循环对象的指针 loop2
	webserver::EventLoop *loop2 = elt2.startLoop();
	
	// 断言：确保 loop1 和 loop2 不相等
	assert(loop1 != loop2);
	printf("Done!\n");
	
	return 0;
}
