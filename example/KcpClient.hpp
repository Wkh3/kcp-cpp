/*
 * @Author: wkh
 * @Date: 2021-11-13 01:06:27
 * @LastEditTime: 2021-11-13 12:46:19
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/KcpClient.hpp
 * 
 */

#pragma once
#include <Kcp.hpp>
#include <functional>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class KcpClient
{
public:
        using MessageCallBack = std::function<void(std::string msg)>;
        using CloseCallBack   = std::function<void()>;

        KcpClient(const std::string &ip,uint16_t port,uint16_t conv) : ip_(ip), port_(port),conv_(conv)
        {
            Init();
        }
        
        void SetMessageCallBack(const MessageCallBack &cb){msg_cb_ = cb;}
        void SetCoseCallBack(const CloseCallBack &cb){close_cb_ = cb;}
        void Run()
        {
            while(1)
            {
                 usleep(50);

                 if(kcp_->Update(clock()))
                 {
                     if(close_cb_)
                        close_cb_();
                     
                     break;
                 }
                 
                 std::string buf(1024 * 4,'a');
                 
                 socklen_t socklen;

                 int len = recvfrom(fd_,const_cast<char*>(buf.data()),buf.length(),0,(sockaddr*)&addr_,&socklen);

                 if(CheckError(len))
                   continue;
                 
                 int ret = kcp_->Input(const_cast<char*>(buf.data()),len);

                 if(ret < 0)
                 {
                     TRACE("Kcp Input error = ",ret);
                     exit(-1);
                 }

                 len = kcp_->Recv(const_cast<char*>(buf.data()),buf.length());

                 if(len == 0)
                   continue;
                 
                 if(len == -1)
                 {
                     TRACE("buf size is too small");
                     exit(-1);
                 }
                 
                 buf.resize(len);

                 if(msg_cb_)
                 msg_cb_(buf);
            }
        }
        void Send(const void *data,std::size_t size)
        {
             if(kcp_->Send(data,size) == -1)
                TRACE("kcp Send error = -1");
        }
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
        void Init()
        {
            fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

            if (fd_ == -1)
                perror("create socket failed!");

            addr_.sin_family = AF_INET;
            addr_.sin_port = htons(port_);
            addr_.sin_addr.s_addr = inet_addr(ip_.c_str());

            int flag = fcntl(fd_, F_GETFL);
            fcntl(fd_, F_SETFL, flag | O_NONBLOCK);

            kcp::KcpOpt opt;
            opt.conv                = conv_;
            opt.interval            = 50;
            opt.trigger_fast_resend = 3;
            opt.nodelay             = true;
            opt.offline_standard    = 500;

            opt.send_func           = [this](const void *data,std::size_t size,void *)
            {
                    int len = sendto(fd_,data,size,0,(sockaddr*)&addr_,sizeof(addr_));

                    if(len == -1)
                      perror("sendto error!");
            };

            kcp_ = std::make_shared<kcp::Kcp<true>>(opt);
        }

private:
        MessageCallBack      msg_cb_;
        CloseCallBack        close_cb_;
        int                  fd_;
        std::string          ip_;
        sockaddr_in          addr_;
        uint16_t             port_;
        uint16_t             conv_;
        kcp::Kcp<true>::ptr  kcp_;

};