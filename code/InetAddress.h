#ifndef code_InetAddress_h
#define code_InetAddress_h

#include <string>

#include <netinet/in.h>

namespace webserver
{

class InetAddress
{
public:
	// 类声明，用于表示网络地址。
	explicit InetAddress(uint16_t port);
	InetAddress(const std::string &ip, uint16_t port);
	InetAddress(const sockaddr_in &addr) { addr_ = addr; }
	
	// 获取保存在类中的 sockaddr_in 结构。
	const struct sockaddr_in &get() const { return addr_; }
	// 设置类中的 sockaddr_in 结构。
	void set(const struct sockaddr_in &addr) { addr_ = addr; }
	
	// 将 IP 地址转换为字符串并返回。
	std::string toIpString() const;
	// 将 IP 地址和端口号转换为字符串并返回。
	std::string toIpPortString() const;
	
private:
	struct sockaddr_in addr_;
};

}

#endif
