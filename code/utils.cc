#include "utils.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <strings.h>

#include "config.h"
#include "macros.h"

namespace webserver
{

namespace utils
{
	
typedef struct sockaddr SA;
const SA* sockaddr_cast(const struct sockaddr_in* addr)
{
	return static_cast<const SA*>(reinterpret_cast<const void*>(addr));
}

SA* sockaddr_cast(struct sockaddr_in* addr)
{
	return static_cast<SA*>(reinterpret_cast<void*>(addr));
}

int Socket(int family, int type, int proto)
{
	int sockfd = -1;
	sockfd = ::socket(family, type, proto);
	if(unlikely(sockfd < 0))
	{
		perror("Socket");
	}
	return sockfd;
}

void Bind(int sockfd, const struct sockaddr_in &addr)
{
	int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof(addr));
	if(unlikely(ret < 0))
	{
		perror("Bind");
	}
}

void Listen(int sockfd, int backlog)
{
	int ret = ::listen(sockfd, backlog);
	if(unlikely(ret < 0))
	{
		perror("Listen");
	}
}

int SocketBindListen(const webserver::InetAddress &addr)
{
	int sockfd = Socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | 
	                    SOCK_CLOEXEC, IPPROTO_TCP);
						
	Bind(sockfd, addr.get());
	Listen(sockfd, SOCKET_MAXBACKLOG);
	
	return sockfd;
}

int AcceptNb(int sockfd, webserver::InetAddress &addr)
{
	struct sockaddr_in acceptAddr;
	socklen_t addrLen = sizeof(acceptAddr);
	
	int connfd = ::accept4(sockfd, sockaddr_cast(&acceptAddr), 
	                       &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	
	addr.set(acceptAddr);
	
	return connfd;
}

void Close(int sockfd)
{
	int ret = ::close(sockfd);
	if(unlikely(ret < 0))
	{
		perror("Close");
	}
}

void setTcpNoDelay(int sockfd, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
               &optval, sizeof optval);
}

// 用于设置套接字（socket）的SO_REUSEADDR选项。这个选项通常用于允许在同一端口上快速重用处于TIME_WAIT状态的套接字。
void setReuseAddr(int sockfd, bool on)
{
  int optval = on ? 1 : 0;

  // 通过启用SO_REUSEADDR选项，可以在TIME_WAIT状态结束之前，允许新的套接字绑定到相同的端口上。
  // 这在一些特定情况下是有用的，例如在服务器程序需要频繁重启时，或者在开发和调试阶段，以避免端口占用问题。
  ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof optval);
}

void Shutdown(int sockfd, int how)
{
	int ret = ::shutdown(sockfd, how);
	if(unlikely(ret<0))
	{
		perror("shutdown");
	}
}

// SIGPIPE 是一种信号，当进程尝试向一个已经被关闭的管道（或者 socket）
// 写入数据时，系统会发送这个信号给进程，通知它写入一个已经被关闭的管道。忽略 SIGPIPE 信号通常是为了避免进程在这种情况下被终止。
void IgnoreSigpipe()
{
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if(unlikely(sigaction(SIGPIPE, &sa, NULL) < 0))
	{
		perror("sigaction");
	}
}

/* ET mode */
/* 读写直到EAGAIN */
int readn(int sockfd, std::string &io_buf, bool &isZero)
{
	char buf[MAX_BUFSIZE];
	int nbytes;
	int totalSize = 0;
	
	while(true)
	{
		if((nbytes = ::read(sockfd, buf, MAX_BUFSIZE)) <= 0)
		{
			if(errno == EINTR) continue;
			if(errno == EAGAIN) return totalSize;
			if(nbytes == 0)	/* 读0 */
			{
				isZero = true;
				break;
			}
			
			return -1;
		}
		
		totalSize += nbytes;
		io_buf += std::string(buf, buf+nbytes);
	}

	return totalSize;
}

int writen(int sockfd, std::string &io_buf)
{
	int nbytes;
	int totalSize = 0;
	int bufSize = static_cast<int>(io_buf.size());;
	const char *pstr = reinterpret_cast<const char *>(io_buf.data());
	
	while(totalSize < bufSize)
	{
		if((nbytes = ::write(sockfd, pstr +	totalSize, 
		                     bufSize-totalSize)) <= 0)
		{
			if(errno == EINTR) continue;
			if(errno == EAGAIN) break;
			//if(errno == EPIPE)
			
			io_buf.clear();
			return -1;
		}
		totalSize += nbytes;
	}
	
	if(totalSize == bufSize) 
	{
		io_buf.clear();
	}
	else 
	{
		io_buf = io_buf.substr(totalSize);
	}
	
	return totalSize;
}

}//namespace utils

}//namespace webserver
