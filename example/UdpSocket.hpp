/*
 * @Author: wkh
 * @Date: 2021-11-15 01:04:30
 * @LastEditTime: 2021-11-15 19:26:33
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/UdpSocket.hpp
 * 
 */

#pragma once
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <Trace.hpp>
class UdpSocket
{
public:
        using ptr = std::shared_ptr<UdpSocket>;

        UdpSocket(const std::string &ip,uint16_t port,int family);

        ~UdpSocket();

        int SendTo(const void *buf,std::size_t size,sockaddr_in &addr,int flag = 0);

        int RecvFrom(void *buf,std::size_t size,sockaddr_in &addr,int flag = 0);

        bool Bind();

        bool SetNoblock();
private:
        std::string ip_;
        uint16_t    port_;
        int         fd_;
        int         family_;
        sockaddr_in addr_;
};

UdpSocket::UdpSocket(const std::string &ip,uint16_t port,int family) : ip_(ip),
                                                                       port_(port),
                                                                       family_(family)
{
     addr_.sin_family      = family_;
     addr_.sin_port        = htons(port_);
     addr_.sin_addr.s_addr = inet_addr(ip.c_str());

     TRACE("addr = ",inet_ntoa(addr_.sin_addr));
     fd_                   = socket(family_,SOCK_DGRAM,IPPROTO_UDP);

     if(fd_ == -1)
       throw strerror(errno);
}
UdpSocket::~UdpSocket()
{
     close(fd_);
}

bool UdpSocket::SetNoblock()
{
     int flag = fcntl(fd_,F_GETFL);
     return fcntl(fd_,F_SETFL,flag | O_NONBLOCK) == 0; 
}

bool UdpSocket::Bind()
{
     int ret = bind(fd_,(sockaddr*)&addr_,sizeof(addr_));
     
     if(ret == -1)
     {
          perror("bind error");
          return false;
     }
     
     return true;
}


int UdpSocket::RecvFrom(void *buf,std::size_t size,sockaddr_in &addr,int flag)
{
    socklen_t len = sizeof(addr);
    return recvfrom(fd_,buf,size,flag,(sockaddr*)&addr,&len);
}

int UdpSocket::SendTo(const void *buf,std::size_t size,sockaddr_in &addr,int flag)
{
    return sendto(fd_,buf,size,flag,(sockaddr*)&addr,sizeof(addr));    
}