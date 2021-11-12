/*
 * @Author: wkh
 * @Date: 2021-11-12 23:22:10
 * @LastEditTime: 2021-11-13 01:50:29
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/ChatClient.cpp
 * 
 */

#include "KcpClient.hpp"
#include <thread>
class ChatClient
{
public:
      ChatClient(const std::string &ip,uint16_t port,uint16_t conv) : client_(ip,port,conv)
      {
          client_.SetMessageCallBack(std::bind(&ChatClient::MessageCallBack,this,std::placeholders::_1));
      }
      void Start()
      {
          std::thread t([this](){client_.Run();});
          
          while(1)
          {
              std::string msg;
              std::cin >> msg;
              client_.Send(msg.data(),msg.length());
          }         
          t.join();
      }

      void MessageCallBack(std::string msg)
      {
          std::cout << msg << std::endl;
      }
private:
      KcpClient client_;

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
      
    ChatClient client(ip,port,666);

    client.Start();

    return 0;
}