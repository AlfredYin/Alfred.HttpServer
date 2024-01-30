#ifndef code_CurrentThread_h
#define code_CurrentThread_h

#include "macros.h"

namespace webserver
{

// 用于获取当前线程的信息。
// 这个头文件通过使用 thread_local 关键字声明了一些线程局部存储的变量，以确保这些变量在线程之间是独立的。
namespace CurrentThread
{

//internel
//may be able to use non-POD types
extern thread_local int t_cachedTid;
extern thread_local char t_tidString[32];
extern thread_local int t_tidStringLength;
extern thread_local const char *t_threadName;

// 缓存当前线程ID的函数。在 tid() 函数中，如果当前线程的 t_cachedTid 为零（未缓存），则调用此函数来获取并缓存线程ID。
void cacheTid();

// 返回当前线程的线程ID。如果线程ID未缓存，会调用 cacheTid 函数获取并缓存。
inline int tid()
{
	if(unlikely(t_cachedTid == 0))
	{
		cacheTid();
	}
	return t_cachedTid;
}

// 返回当前线程的线程ID的字符串表示。
inline const char *tidString()
{
	return t_tidString;	
}

// 返回当前线程的线程ID字符串的长度。
inline int tidStringLength()
{
	return t_tidStringLength;
}

// 返回当前线程的名称。
inline const char *name()
{
	return t_threadName;
}

} //namespace CurrentThread

} //namespace webserver

#endif
