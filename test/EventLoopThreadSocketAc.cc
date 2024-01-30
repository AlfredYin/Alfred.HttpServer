#include <iostream>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "CurrentThread.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "HttpHandler.h"
#include "Channel.h"
#include "macros.h"
#include "utils.h"

using namespace std;

void acceptor(int listenfd,webserver::EventLoopThreadPool *threadPool);

void handleSocket(int connfd);

int main(){

pid_t pid=webserver::CurrentThread::tid();
printf("Main pid = %d \n",pid);

    webserver::InetAddress self_addr(8888);

    webserver::EventLoop mainLoop;

    // 事件循环线程池  参数主线程 mainLoop
    webserver::EventLoopThreadPool *threadPool = new webserver::EventLoopThreadPool(&mainLoop,4);
    // 监听socketfd
    int listenfd=webserver::utils::SocketBindListen(self_addr);

    // Channel
    std::shared_ptr<webserver::Channel> acceptChannel = std::make_shared<webserver::Channel>(listenfd, &mainLoop);
    acceptChannel->setReadCallback(bind(&acceptor,listenfd,threadPool));
    acceptChannel->enableReading();

    // 开始事件循环线程
    threadPool->start();

    mainLoop.loop();

    return -1;
}

// 
void acceptor(int listenfd,webserver::EventLoopThreadPool *threadPool){

    printf("acceptor(%d)\n",listenfd);

    webserver::InetAddress addr(0);
    int connfd;

    while ((connfd=webserver::utils::AcceptNb(listenfd,addr))>0)
    {
        // Handler SocketConnection
        printf("Handler SocketConnection(%d) InetAddress(%s) \n",connfd,addr.toIpPortString().c_str());

        webserver::EventLoop *loop=threadPool->getNextLoop();
        
        //loop->queueInLoop(bind(&handleSocket,connfd));
        
// 监听仍然是还在主线程中     
// pid_t pid=webserver::CurrentThread::tid();
// printf("acceptor pid = %d \n",pid);

        loop->runInLoop(bind(&handleSocket,connfd));
    }
    
    printf("acceptor(%d) end !!!\n",listenfd);

    return;
}

void handleSocket(int connfd){

    printf("handleSocket(%d) start !!! \n",connfd);

pid_t pid=webserver::CurrentThread::tid();
printf("handleSocket pid = %d \n",pid);

    int i=5;

    while(i>0){

        printf("handleSocket(%d) \n",connfd);
        sleep(2);
        i--;
    }
}