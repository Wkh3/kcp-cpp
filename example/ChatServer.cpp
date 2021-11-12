/*
 * @Author: wkh
 * @Date: 2021-11-12 23:22:23
 * @LastEditTime: 2021-11-13 01:56:53
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/ChatServer.cpp
 * 
 */

#include "KcpServer.hpp"
#include <unordered_set>

class ChatServer
{
public:
        ChatServer(const std::string &ip,uint16_t port,uint16_t conv) : server_(ip,port,conv)
        {
             using namespace std::placeholders;
             server_.SetCloseCallBack(std::bind(&ChatServer::CloseCallBack,this,_1,_2));
             server_.SetMessageCallBack(std::bind(&ChatServer::MessageCallBack,this,_1,_2,_3));
             server_.SetNewClientCallBack(std::bind(&ChatServer::NewClientCallBack,this,_1,_2));
        }

        void MessageCallBack(kcp::Kcp<true>::ptr pcon,KcpServer::Addr addr,std::string msg)
        {
            std::stringstream ss;
            ss << "user from [" << addr.first << ":" << addr.second << "] : " << msg;
            Notify(ss.str());
        }
        void NewClientCallBack(kcp::Kcp<true>::ptr pcon,KcpServer::Addr addr)
        {
            {
                Lock lock(mtx_);
                users_.insert(pcon);
            }

            std::stringstream ss;
            ss << "user from [" << addr.first << ":" << addr.second << "] : " << "join the KcpChatRoom!";
            Notify(ss.str());
        }
        void CloseCallBack(kcp::Kcp<true>::ptr pcon,KcpServer::Addr addr)
        {   
            {
                Lock lock(mtx_);
                users_.erase(pcon);
            }   
            std::stringstream ss;
            ss << "user from [" << addr.first << ":" << addr.second << "] : " << "left the KcpChatRoom!";
            Notify(ss.str());
        }
        void Notify(const std::string &str)
        {
             Lock lock(mtx_);
             
             for(auto &user : users_)
                user->Send(str.data(),str.size());
        }
        void Run(){ return server_.Run(); }
public:
       using Lock = std::unique_lock<std::mutex>;
private:
        KcpServer                               server_;
        std::unordered_set<kcp::Kcp<true>::ptr> users_;
        std::mutex                              mtx_;
};

int main(int argc,char *argv[])
{
    //arg : ip + port
    if(argc != 3)
    {
        TRACE("args error please input [ip] [port]");
        exit(-1);
    }

    std::string ip   = argv[1];
    uint16_t    port = atoi(argv[2]);
      
    ChatServer server(ip,port,666);
    
    server.Run();

    return 0;
} 