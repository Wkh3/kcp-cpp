/*
 * @Author: wkh
 * @Date: 2021-11-11 21:27:17
 * @LastEditTime: 2021-11-13 13:59:33
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/KcpServer.hpp
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
#include <string.h>
#include <Kcp.hpp>
#include <map>
#include "ThreadPool.hpp"
class KcpServer
{
   
public:
        using Addr               = std::pair<std::string,uint16_t>;
        using MessageCallBack    = std::function<void(kcp::Kcp<true>::ptr,KcpServer::Addr,std::string)>;
        using NewClientCallBack  = std::function<void(kcp::Kcp<true>::ptr,KcpServer::Addr)>;
        using CloseCallBack      = std::function<void(kcp::Kcp<true>::ptr,KcpServer::Addr)>;
public:
      KcpServer(const std::string &ip, uint16_t port, uint16_t conv) : ip_(ip),
                                                                       port_(port),
                                                                       conv_(conv)
      {
          Init();
      }

      void Run()
      {
         char buf[1024 * 4];
         sockaddr_in addr;
         memset(&addr,0,sizeof(addr));
         socklen_t   sock_len = sizeof(addr);
         pool_.Start(4);

         while (1)
         {
            usleep(50);

            Update();

            int len = recvfrom(fd_, buf, 1024 * 4, 0, reinterpret_cast<sockaddr *>(&addr), &sock_len);

            if (CheckError(len))
               continue;
               
            Handle(addr,sock_len,std::string(buf,len));
         }
      }

      void SetMessageCallBack(const MessageCallBack &cb)    { msg_cb_ = cb;       }
      void SetNewClientCallBack(const NewClientCallBack &cb){ new_client_cb_ = cb;}
      void SetCloseCallBack(const CloseCallBack &cb)        { close_cb_ = cb;     }

private:
      bool CheckError(int ret)
      {
         if (ret == -1)
         {
            if (errno != EAGAIN)
               perror("recvfrom error!");
            return true;
         }
         return false;
      }
      
      void Update()
      {
         for (auto it = kcp_map_.begin(); it != kcp_map_.end();)
         {
            if (it->second->Update(clock()))
            {
               close_cb_(it->second,it->first);
               it = kcp_map_.erase(it);
            }else
            {
               ++it;
            }
         }
      }
      
      void Handle(sockaddr_in &addr,socklen_t socklen,const std::string &str)
      {
            std::stringstream ss;

            Addr user_addr{inet_ntoa(addr.sin_addr),ntohs(addr.sin_port)};


            if (kcp_map_.find(user_addr) == kcp_map_.end())
            {
                auto pcon = NewClient(addr,socklen);
                TRACE("new client from ",user_addr.first,user_addr.second);
                kcp_map_[user_addr] = pcon;
                new_client_cb_(pcon,user_addr);
            }

            auto pcon = kcp_map_[user_addr];
            
            int ret = pcon->Input(const_cast<char*>(str.data()),str.length()); 

            if(ret < 0)
            {
                TRACE("Input error = ",ret);
                return;
            }

            std::string msg(1024 * 4,'a');
            
            int len = pcon->Recv(const_cast<char*>(msg.data()),msg.length());
            
            if(len == 0)
              return;

            if(len == -1)
            {
               TRACE("user data too long");
               exit(-1);
            }

            msg.resize(len);

            pool_.Put([this,pcon,msg,user_addr]()
            {
                  msg_cb_(pcon,user_addr,msg);
            });
      }

      kcp::Kcp<true>::ptr NewClient(sockaddr_in &addr,socklen_t socklen)
      {
         kcp::KcpOpt opt;
         opt.interval            = 50;
         opt.conv                = conv_;
         opt.offline_standard    = 1000;

         opt.send_func = [addr,socklen,this](const void *data, std::size_t size, void *kcp)
         {
            int len = sendto(fd_, data, size, 0, (sockaddr *)&addr,socklen);
            if (len == -1)
            {
                  perror("sendto error!");
                  TRACE("errno =",errno);
            }
         };

         return std::make_shared<kcp::Kcp<true>>(opt);
      }
      void Init()
      {
         fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

         if (fd_ == -1)
            perror("create socket failed!");

         sockaddr_in addr;
         addr.sin_family = AF_INET;
         addr.sin_port = htons(port_);
         addr.sin_addr.s_addr = inet_addr(ip_.c_str());

         if (bind(fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
            perror("bind error!");

         int flag = fcntl(fd_, F_GETFL);
         fcntl(fd_, F_SETFL, flag | O_NONBLOCK);
      }

private:
         int                                 fd_;
         uint16_t                            port_;
         uint16_t                            conv_;
         std::string                         ip_;
         ThreadPool                          pool_;
         MessageCallBack                     msg_cb_;
         NewClientCallBack                   new_client_cb_;
         CloseCallBack                       close_cb_;
         std::map<Addr, kcp::Kcp<true>::ptr> kcp_map_;
};