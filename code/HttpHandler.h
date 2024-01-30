#ifndef code_HttpHandler_h
#define code_HttpHandler_h

#include <memory>
#include <map>
#include <string>
#include <stdexcept> // If you decide to throw an exception
#include <iostream>
#include <string.h>

#include "HttpManager.h"

namespace webserver
{

class EventLoop;
class HttpConnection;
class HttpManager;

/* 持有HttpConnection */
/* 负责解析Http协议，并给予Http应答 */
// HttpHandler 类继承自 std::enable_shared_from_this，用于支持在成员函数中安全地获取 shared_ptr 实例。
// HttpHandler 负责管理与客户端的连接，并在事件循环中处理来自客户端的HTTP请求。
// 通过将连接的管理和请求处理分离，可以更好地实现服务器的可扩展性和并发处理能力。
class HttpHandler : public std::enable_shared_from_this<HttpHandler>
{
public:
	// 这些枚举类型定义了 HTTP 协议的版本、请求方法和处理状态。。
	// 例如，kHttpV10 表示 HTTP 1.0，kGet 表示 GET 请求方法，kStart 表示处理的起始状态等。
	/* google c++ format */
	enum HttpVersion { kHttpV10, kHttpV11, kHttpUnkown, kVersionSize };
	enum HttpMethod { kGet, kPost, kHead, kOtherMethods, kMethodSize };

	// 表示处理的起始状态等。
	enum HttpState { kStart, kPraseUrl, kPraseHeader, kPraseBody, kPraseDone, kResponse, kStateSize };
	
	// 这些数组定义了 HTTP 方法和版本的字符串表示。
	// 例如，kMethod 包含了 "GET"、"POST"、"HEAD" 和 "Unknown" 等字符串。
	static const char *kMethod[];
	static const char *kVersion[];
	
	HttpHandler(EventLoop *loop, int connfd);
	~HttpHandler();

	// 被主事件循环调用，处理新的连接。
	void newConnection(); /* 被main loop调用 */
	// 处理 HTTP 请求的入口函数。
	void handleHttpReq();

private:
	// 用于解析 HTTP 请求的 URL、头部和请求体。
	int praseUrl(std::string &buf, int bpos);
	int praseHeader(std::string &buf, int bpos);
	int praseBody(std::string &buf, int bpos);

	// 处理 HTTP 请求并返回响应。
	void responseReq();
	// 处理保持连接的逻辑。
	void keepAliveHandle();
	// 处理错误请求。
	void badRequest(int num, const std::string &note);
	// 处理完整的 HTTP 请求。
	void onRequest(const std::string &body);
	
	// 设置 HTTP 请求的方法、路径、版本和头部。
	void setMethod(const std::string &method)
	{

#if DEBUG
	printf("void setMethod(%s)\n", method.c_str());
#endif
		method_ = kOtherMethods;
		for(int i=0; i<kMethodSize-1; ++i)
		{
			// enum HttpMethod { kGet, kPost, kHead, kOtherMethods, kMethodSize };
			// const char *HttpHandler::kMethod[] = {"GET", "POST", "HEAD", "Unknown"};
			//if(method.c_str()== kMethod[i])
			// if (static_cast<const char*>(method_) == kMethod[i])
			if(strcmp(method.c_str(),kMethod[i])==0)
			{
				method_ = static_cast<HttpMethod>(i);
				return ;
			}
		}
		method_=static_cast<HttpMethod>(0);
	}
	
	void setPath(const std::string &path) { path_ = path; }
	void setVersion(const std::string &version)
	{
		version_ = kHttpUnkown;
		for(int i=0; i<kVersionSize-1; ++i)
		{
			if(version == kVersion[i])
			{
				version_ = static_cast<HttpVersion>(i);
				return ;
			}
		}
	}
	
	void setHeader(const std::string &key, const std::string &value) 
	{ 
		header_[key] = value; 
	}
	
private:
	EventLoop *loop_;
	// 与客户端建立的连接的文件描述符。
	int connfd_;
	// 持有一个 HttpConnection 对象，用于处理具体的 HTTP 连接。
	std::unique_ptr<HttpConnection> connection_;
	
	// 当前 HTTP 处理的状态，如解析 URL、解析头部、解析请求体等。
	HttpState state_;
	// HTTP 请求的方法，如 GET、POST 等。
	HttpMethod method_;
	// HTTP 协议的版本。
	HttpVersion version_;
	
	// 保存 HTTP 请求头的键值对。
	std::map<std::string, std::string> header_;
	// HTTP 请求的路径。
	std::string path_;
	// HTTP 请求体的内容。
	std::string body_;
	// 表示是否需要保持连接。
	bool keepAlive_;
	
	/* 变量类型不大理想 */
	// 用于处理 HTTP 连接的定时器节点。
	HttpManager::TimerNode timerNode_;
	
	friend class HttpManager;
};

}

#endif
