/*
 * @Author: wkh
 * @Date: 2021-11-13 01:06:27
 * @LastEditTime: 2021-11-15 22:46:25
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/KcpClient.hpp
 * 
 */

#pragma once
#include <Kcp.hpp>
#include "UdpSocket.hpp"
#include <sys/time.h>



class KcpClient : public kcp::Kcp<true>
{
public:
        KcpClient(const std::string &ip,uint16_t port,const kcp::KcpOpt &opt);

        bool Run();
private:  
        bool CheckRet(int len);

        static constexpr uint32_t max_body_size = 1024 * 4;
protected:
        virtual void HandleMessage(const std::string &msg) = 0;
        virtual void HandleClose() = 0;

private:
        UdpSocket::ptr socket_;
        sockaddr_in    server_addr_;
        
};
KcpClient::KcpClient(const std::string &ip,uint16_t port,const kcp::KcpOpt &opt) : kcp::Kcp<true>(opt),
                                                                                   socket_(std::make_shared<UdpSocket>("0.0.0.0",0,AF_INET))                                                     
{
    server_addr_.sin_family      = AF_INET;
    server_addr_.sin_port        = htons(port);
    server_addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    socket_->SetNoblock();
    
    
    SetSendFunc([this](const void *buf,std::size_t size,void*)
    {
        int len = socket_->SendTo(buf,size,server_addr_);
        if(len == -1)
           perror("sendto error");
    });
}

bool KcpClient::Run()
{

    if(Update(iclock64()))
    {
        HandleClose();
        return false;
    }

    std::string buf(1024 * 4,'a');

    sockaddr_in addr;

    int len = socket_->RecvFrom(const_cast<char*>(buf.data()),buf.length(),addr);

    if(addr.sin_port        != server_addr_.sin_port || 
       addr.sin_addr.s_addr != server_addr_.sin_addr.s_addr)
       return true;

    if(!CheckRet(len))
      return true;
    
    buf.resize(len);

    len = Input(const_cast<char*>(buf.data()),len);

    if(len != 0)
    {
       TRACE("input error = ",len);  
       return false;
    }
    
    std::string body(max_body_size,'a');
    
    do
    {
        len = Recv(const_cast<char*>(body.data()),max_body_size);
        
        if(len == -1)
        {
            TRACE("body_size too small");
            exit(-1);
        }

        if(len == 0)
          break;
        
        body.resize(len);

        HandleMessage(body);
    }while(1);

    return true;
}

bool KcpClient::CheckRet(int len)
{
    
    if(len == -1)
    {
        if(errno != EAGAIN)
            perror("recvform error!");
         
        return false;
    }

    if(len < kcp::KcpAttr::KCP_HEADER_SIZE)
         return false;
      
    return true;
}