/*
 * @Author: wkh
 * @Date: 2021-11-12 23:22:23
 * @LastEditTime: 2021-11-15 23:19:43
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/ChatServer.cpp
 * 
 */

#include "KcpServer.hpp"
#include <unordered_set>

class ChatServer : public KcpServer
{
public:
        using KcpServer::KcpServer;

        virtual void HandleMessage(const KcpSession::ptr& session,const std::string &msg) override
        {
            //TRACE(msg);
            std::stringstream ss;
            ss << "user from [" << session->GetAddrToString() << "] "<< msg;
            Notify(ss.str());
        }
        void HandleConnection(const KcpSession::ptr &session)
        {
            {
                Lock lock(mtx_);
                users_.insert(session);
            }

            std::stringstream ss;
            ss << "user from [" << session->GetAddrToString() << "] join the KcpChatRoom!";
            Notify(ss.str());
        }
        void HandleClose(const KcpSession::ptr &session)
        {   
            //TRACE("close ",session->GetAddrToString());
            {
                Lock lock(mtx_);
                users_.erase(session);
            }
            std::stringstream ss;
            ss << "user from [" << session->GetAddrToString() << "] left the KcpChatRoom!";
            Notify(ss.str());
        }
        void Notify(const std::string &str)
        {
             Lock lock(mtx_);
             
             for(auto &user : users_)
                user->Send(str.data(),str.size());
        }
public:
       using Lock = std::unique_lock<std::mutex>;
private:
        std::unordered_set<KcpSession::ptr>          users_;
        std::mutex                                   mtx_;
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
      
    ChatServer server(ip,port);
    
    server.Run(4);

    return 0;
} 