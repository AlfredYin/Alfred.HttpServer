#include "HttpHandler.h"

#include <string>
#include <sys/socket.h>
#include <cassert>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
	   
#include "Channel.h"
#include "HttpConnection.h"
#include "EventLoop.h"
#include "macros.h"
#include "config.h"

namespace webserver
{

const char *HttpHandler::kMethod[] = {"GET", "POST", "HEAD", "Unknown"};
const char *HttpHandler::kVersion[] = {"HTTP/1.0", "HTTP/1.1", "Unknown"};

HttpHandler::HttpHandler(EventLoop *loop, int connfd)
	: loop_(loop),
	  connfd_(connfd),
	  connection_(new HttpConnection(loop_, connfd_)),	// 创建HttpConnection实例 负责与 Channel 通信，根据事件触发自动读写 HTTP 数据到缓冲区。
	  state_(kStart),
	  keepAlive_(false)
{
	assert(connfd_ > 0);
}

HttpHandler::~HttpHandler()
{
	//printf("dtor HttpHandler\n");
}

//在当前event loop中，仅被调用一次 第一次开始建立连接
void HttpHandler::newConnection()
{
#if DEBUG
	printf("void HttpHandler::newConnection()\n");
#endif

	std::shared_ptr<Channel> channel = connection_->getChannel();
	
	connection_->setDefaultCallback();
	connection_->setHolder(shared_from_this());
	channel->enableReading();
}

// GET / HTTP/1.1\r\n Host: localhost\r\n
/* HTTP 1.1: 多个Http请求，不能重叠执行 */
/* 解析Http协议时，使用正则子表达式，可能会更加清晰 */
void HttpHandler::handleHttpReq()
{
	/* bpos当前位置，epos下一行位置 */
	int bpos = 0, epos = 0;
	/* ROV优化, 不必忧心效率 */
	std::string buffer = connection_->getRecvBuffer();

#if DEBUG
	printf("void HttpHandler::handleHttpReq()\n");
	printf("buffer=%s\n",buffer.c_str());
#endif

	HttpConnection::ConnState connState = connection_->getState();

	if(connState == HttpConnection::kError)
	{

printf("connState == HttpConnection::kError");
		state_ = kStart;	/* 跳过解析环节，回复404 bad request */
		goto __err;
	}
	
	/* 请求到来，但无数据 */
	if(buffer.empty()) 
	{

printf("buffer.empty()");
		state_ = kStart;
		goto __err;
	}
	
	// 解析Url
	epos = praseUrl(buffer, bpos);
	if(unlikely(epos < 0))
	{
printf("(unlikely(epos < 0) state_ = kPraseUrl;");		
		state_ = kPraseUrl;	//错误处理, 设置状态 bad request
		goto __err;
	}
	
	// 解析Head
	bpos = epos;
	epos = praseHeader(buffer, bpos);
	if(unlikely(epos < 0))
	{
printf("(unlikely(epos < 0) state_ = kPraseHeader;");		
		state_ = kPraseHeader;
		goto __err;
	}
	
	// Post请求 解析 Body
	bpos = epos;
	epos = praseBody(buffer, bpos);
	if(unlikely(epos < 0))
	{
printf("(unlikely(epos < 0) state_ = kPraseBody;");		
		state_ = kPraseBody;
		goto __err;
	}
	state_ = kPraseDone;

#ifdef DEBUG
	printf("state_ kPraseDone \n");
#endif // DEBUG
	
__err:
	/* 根据解析状态，返回结果 */
	responseReq();
	
	/* 连接处理：断开 or 保持 */
#if DEBUGKeepAlive
	keepAlive_=false;
#endif
	keepAliveHandle();
}

/* 解析请求行，发生错误时返回-1，否则返回Header的索引位置 */
int HttpHandler::praseUrl(std::string &buf, int bpos)
{
	/* 提取请求行 */
	std::string::size_type epos = buf.find("\r\n", bpos);
	if(epos == std::string::npos) return -1;
	
	/* 解析请求方法 */
	std::string::size_type space = buf.find(" ", bpos);
	if(space == std::string::npos || space > epos) return -1;
	setMethod(buf.substr(bpos, space-bpos));
	if(kMethod[method_] == std::string("Unknown")) return -1;
	
	/* 解析请求资源路径 */
	bpos = space+1;
	space = buf.find(" ", bpos);
	if(space == std::string::npos || space > epos) return -1;
	setPath(buf.substr(bpos, space-bpos));
	if(path_.empty() || path_[0] != '/') return -1;
	
	/* 解析Http版本号 */
	bpos = space+1;
	setVersion(buf.substr(bpos, epos-bpos));
	if(kVersion[version_] == std::string("Unknown")) return -1;
	if(version_ == kHttpV11) keepAlive_=true;

#if DEBUG
	printf("method:%s ", kMethod[method_]);
	printf("path:%s ", path_.c_str());
	printf("version:%s \n", kVersion[version_]);
#endif
	
	return epos+2;
}

/* 解析Header，发生错误时返回-1，否则返回Body的索引位置 */
int HttpHandler::praseHeader(std::string &buf, int bpos)
{
	std::string::size_type epos = bpos;
	
	while(static_cast<int>(epos = buf.find("\r\n", bpos)) != bpos)
	{
		std::string::size_type sep = buf.find(":", bpos);
		if(sep == std::string::npos || sep > epos) return -1;
		
		while(buf[bpos] == ' ') bpos++;
		std::string key(buf.substr(bpos,sep-bpos));
		
		sep += 1;
		while(buf[sep] == ' ') sep++;
		std::string value(buf.substr(sep, epos-sep));
		
		setHeader(key, value);
		
		bpos = epos+2;
	}
	
	/* Keepalive判断 */
	if(header_.count("Connection"))
	{
		if(header_["Connection"] == "keep-alive" || 
	       header_["Connection"] == "Keep-Alive") 
		{
			keepAlive_ = true;
		}
		else if(header_["Connection"] == "close" ||
		        header_["Connection"] == "Close")
		{
			keepAlive_ = false;
		}
	}

#if DEBUG
	for(auto &p : header_)
	{
		printf("%s: %s\n", p.first.c_str(), p.second.c_str());
	}
#endif

	return epos+2;
}

/* 解析Body，发生错误时返回-1，否则返回0 */
// 非Post 不解析Body
int HttpHandler::praseBody(std::string &buf, int bpos)
{

#if DEBUG
	printf("int HttpHandler::praseBody(%s, %d) \n", body_.c_str(),bpos);
#endif

	/* 非Post，不解析body */
	if(method_ != kPost) return 0;
	
	/* Body长度 */
	if(!header_.count("Content-length")){

#if DEBUG
	printf("header_.count(\"Content-length\") \n");
#endif				
		//return -1;
	}
	
// 	int bodyLen = stoi(header_["Content-length"]);
// 	if(bodyLen < static_cast<int>(buf.size())-bpos){

// #if DEBUG
// 	printf("bodyLen < static_cast<int>(buf.size())-bpos \n");
// #endif	
// 		//return -1;
// 	}
	
	/* inefficient!! */
	//body_ = buf.substr(bpos);
	body_="Post:请求已经处理";
	
#if DEBUG
	printf("body: %s\n", body_.c_str());
#endif

	return 0;
}

/* 应答异常请求 */
void HttpHandler::badRequest(int num, const std::string &note)
{
#ifdef DEBUG
	printf("void HttpHandler::badRequest(%d,%s)\n",num,note.c_str());
#endif // DEBUG

	std::string body;
	std::string header;
	
	header += "HTTP/1.1 " + std::to_string(num) + 
	          " " + note + "\r\n";
	header += "Content-Type: text/html\r\n";
	
	if(! keepAlive_) header += "Connection: close\r\n";
	else             header += "Connection: Keep-Alive\r\n";
	
	body += "<html><meta charset=\"UTF-8\"><title>"+note+"</title>";
	body += "<body>" + std::to_string(num) + " " + note;
	body += "<hr /><em>Alfred WebServer</em>";
	body += "</body></html>";
	
	header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	header += "Server: Alfred WebServer\r\n\r\n";
	
	connection_->send(header);
	connection_->send(body);
}

/* 应答正常请求 */
void HttpHandler::onRequest(const std::string &body)
{	
#ifdef DEBUG
	printf("void HttpHandler::onRequest(%s) \n",body.c_str());
#endif // DEBUG

	std::string header;
	
	header += "HTTP/1.1 200 OK\r\n";
	header += "Content-Type: text/html\r\n";
	
	if(! keepAlive_) header += "Connection: close\r\n";
	else             header += "Connection: Keep-Alive\r\n";
	
	// 仅有头的方法
	if(method_ != kHead)
	{
		header += "Content-Length: " + 
	              std::to_string(body.size()) + "\r\n";
	}
	header += "Server: Alfred WebServer\r\n\r\n";
	
	connection_->send(header);
	if(method_ != kHead) 
	{
		connection_->send(body);
	}
#ifdef DEBUG
	printf("head=%s\n",header.c_str());
#endif // DEBUG
}

// 准备请求的文件
void HttpHandler::responseReq()
{

#ifdef DEBUG
	printf("void HttpHandler::responseReq() \n");
#endif // DEBUG

	std::string filename("../../source/");
	std::string context;
	
	/* 根据解析状态，响应Http请求 */
	// 错误的请求
	if(state_ != kPraseDone)
	{
		//bad request 400
		badRequest(400, "Bad Request");
		return ;
	}
	
	/* 解析请求资源 */
	// 正常的请求 解析出来请求的文件路径
	if(path_ == "/")
	{
		//默认返回index.html页面
		filename += "index.html";
	}
	else 
	{
		/* 不能原地赋值!! */
		std::string path = path_.substr(1);
		
		//for webbench test!
		if(path == "hello")
		{
			std::string hello("Hello, Alfred WebServer.");
			onRequest(hello);
			return ;
		}
		else if(::access((filename+path).c_str(), F_OK) < 0)
		{
			//404 Not Found
			badRequest(404, "Not Found");
			return ;
		}
		filename += path;
	}

	if(method_== kPost){
		context=body_;
		
		onRequest(context);
		return ;
	}

#ifdef DEBUG
	printf("filename=%s \n",filename.c_str());
#endif // DEBUG
	/* 返回页面 */
	// 查找文件
	struct stat st;
	if(unlikely(::stat(filename.c_str(), &st)<0))
	{
		perror("stat");
		//404 Not Found
		badRequest(404, "Not Found");
		return ;
	}
	
	// 读取文件的fd
	int fd = ::open(filename.c_str(), O_RDONLY);
	if(unlikely(fd < 0))
	{
		perror("open");
		//404 Not Found
		badRequest(404, "Not Found");
		return ;
	}
	
	void *mapFile = ::mmap(NULL, st.st_size, PROT_READ, 
	                       MAP_PRIVATE, fd, 0);
	if(mapFile == MAP_FAILED)
	{
		perror("mmap");
		//404 Not Found
		badRequest(404, "Not Found");
		return ;
	}
	
	char *pf = static_cast<char *>(mapFile);
	context = std::string(pf, pf + st.st_size);

	// 响应请求
	onRequest(context);
	
	::close(fd);
	::munmap(mapFile, st.st_size);
}

// KeepAlive
void HttpHandler::keepAliveHandle(void)
{
	if(! keepAlive_) 
	{
		/* 关闭非keepalive连接，并返回 */
		connection_->setState(HttpConnection::kDisConnecting);
		connection_->getChannel()->disableReading();
		connection_->shutdown(SHUT_RD);	/* 关闭读半部 */
		
		/* 正常关闭流程 */
		/* HttpConnnection::handleWrite写完时，关闭连接 */
		return ;
	}
	
	/* keepalive预关闭 */
	HttpConnection::ConnState connState = connection_->getState();
	if(connState == HttpConnection::kDisConnecting) return ;
	
	/* 刷新keepalive时间 */
	loop_->flushKeepAlive(connection_->getChannel(), timerNode_);
	
	/* 清理工作，为下次接受请求做准备 */
	header_.clear();
	body_.clear();
	path_.clear();
	state_ = kStart;
	
	/* 重置Httpconnection状态 */
	connection_->setState(HttpConnection::kHandle);
}

}//namespace webserver