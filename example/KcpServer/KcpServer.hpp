/*
 * @Author: wkh
 * @Date: 2021-11-11 21:27:17
 * @LastEditTime: 2021-11-16 19:09:25
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/KcpServer/KcpServer.hpp
 * 
 */
#pragma once
#include <iostream>
#include <unistd.h>
#include <functional>
#include <sstream>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <string.h>
#include <Kcp.hpp>
#include <vector>
#include <map>
#include <time.h>
#include <sys/time.h>
#include "util.hpp"
#include "UdpSocket.hpp"
#include "ThreadPool.hpp"

class KcpSession : public kcp::Kcp<true>
{
public:
    
      using ptr  = std::shared_ptr<KcpSession>;

      KcpSession(const kcp::KcpOpt &opt,const sockaddr_in &addr,const UdpSocket::ptr &socket);

      const sockaddr_in& GetAddr() const { return addr_;}

      std::string GetAddrToString() const;

      void SetAddr(const sockaddr_in &addr) {addr_ = addr;}
private:
      sockaddr_in          addr_;
      UdpSocket::ptr       socket_;
};

class KcpServer
{
public:
        KcpServer(const std::string &ip,uint16_t port);
        
        void Run(std::size_t threads);
private:
        void            Update();
        bool            Check(int ret);
        void            HandleSession(const KcpSession::ptr &session,int length);
        uint64_t        GetConv(const void *data);
        KcpSession::ptr GetSession(uint64_t conv,const sockaddr_in &addr);
        KcpSession::ptr NewSession(uint64_t conv,const sockaddr_in &addr);

        static constexpr uint32_t body_size = 1024 * 4;
protected:
        virtual void HandleClose(const KcpSession::ptr& session) = 0;
        virtual void HandleMessage(const KcpSession::ptr& session,const std::string &msg) = 0;
        virtual void HandleConnection(const KcpSession::ptr& session) = 0;

private:
        using SessionMap = std::unordered_map<uint64_t,KcpSession::ptr>;
private:
        UdpSocket::ptr     socket_;
        ThreadPool         pool_;
        SessionMap         sessions_;
        std::vector<char>  buf_;
};

KcpServer::KcpServer(const std::string &ip,uint16_t port) : socket_(std::make_shared<UdpSocket>(ip,port,AF_INET)),buf_(body_size)
{
    if(!socket_->Bind())
    {
        exit(-1);
    }
    socket_->SetNoblock();
}


void KcpServer::Update()
{
     for(auto it = sessions_.begin(); it != sessions_.end();)
     {
         if(!it->second->Update(iclock64()))
         {  
            ++it;
            continue;
         }
         HandleClose(it->second);
         it = sessions_.erase(it);
     } 
}

bool KcpServer::Check(int ret)
{
       if(ret == -1)
       {
          if(errno != EAGAIN)
             perror("recvform error!");
         
          return false;
       }

       if(ret < kcp::KcpAttr::KCP_HEADER_SIZE)
         return false;
      
       return true;
}

KcpSession::ptr KcpServer::GetSession(uint64_t conv,const sockaddr_in &addr)
{
      auto it = sessions_.find(conv);
       
      if(it == sessions_.end())
         sessions_[conv] = NewSession(conv,addr);
       
      KcpSession::ptr session = sessions_[conv];

      const sockaddr_in &session_addr = session->GetAddr();

      if(session_addr.sin_port != addr.sin_port || 
         session_addr.sin_addr.s_addr != addr.sin_addr.s_addr)
      {
          session->SetAddr(addr);
      }

      return session;
}

KcpSession::ptr KcpServer::NewSession(uint64_t conv,const sockaddr_in &addr)
{
     kcp::KcpOpt opt;
     opt.conv                = conv;
     opt.interval            = 10;
     opt.nodelay             = true;
     opt.trigger_fast_resend = 5;
     opt.offline_standard    = 1000;

     KcpSession::ptr session = std::make_shared<KcpSession>(opt,addr,socket_);  
     
     HandleConnection(session);

     return session;
}

uint64_t KcpServer::GetConv(const void* buf)
{
     return *(uint64_t*)(buf);
}
void KcpServer::Run(std::size_t threads)
{
     pool_.Start(threads);
     
     do
     {   
         usleep(10);

         Update();

         sockaddr_in addr;

         int len = socket_->RecvFrom(buf_.data(),body_size,addr);

         if(!Check(len))
           continue;
      
         uint64_t conv = GetConv(buf_.data());

         KcpSession::ptr session = GetSession(conv,addr);

         HandleSession(session,len);
         
     }while(1);
}

void KcpServer::HandleSession(const KcpSession::ptr &session,int length)
{
      int ret = session->Input(buf_.data(),length);
      
      if(ret != 0)
      {
         TRACE("Input error = ",ret);
         return;
      }
      
      do
      {
         int len = session->Recv(buf_.data(),body_size);

         if(len == -1)
         {
            TRACE("body size too small");
            exit(-1);
         }

         if(len == 0)
           break;
         
         std::string msg(buf_.data(),buf_.data() + len);
         
         TRACE(msg);
         
         pool_.Put([this,session,msg]()
         {
            HandleMessage(session,msg);
         });
      }while(1);

}

KcpSession::KcpSession(const kcp::KcpOpt &opt,const sockaddr_in &addr,const UdpSocket::ptr &socket) : kcp::Kcp<true>(opt),
                                                                                                      addr_(addr),
                                                                                                      socket_(socket)
{
     SetSendFunc([this](const void *buf,std::size_t size,void *)
     {
            if(socket_->SendTo(buf,size,addr_) == -1)
              perror("sendto error");
     });
}

std::string KcpSession::GetAddrToString() const
{
    std::stringstream ss;
    ss << inet_ntoa(addr_.sin_addr) << ":" << ntohs(addr_.sin_port);
    return ss.str();
}