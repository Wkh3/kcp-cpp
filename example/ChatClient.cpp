/*
 * @Author: wkh
 * @Date: 2021-11-12 23:22:10
 * @LastEditTime: 2021-11-15 23:25:49
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/ChatClient.cpp
 * 
 */

#include "KcpClient.hpp"
#include <thread>
class ChatClient : KcpClient
{
public:
    using KcpClient::KcpClient;

    void Start()
    {
        std::thread t([this]()
                      {
                          while (true)
                          {
                              usleep(10);

                              if (!Run())
                              {
                                TRACE("error ouccur");
                                break;
                              }
                          }
                      });

        while (1)
        {
             std::string msg;
             std::getline(std::cin,msg);
             Send(msg.data(), msg.length());
        }
        t.join();
    }

    virtual void HandleMessage(const std::string &msg) override
    {
        std::cout << msg << std::endl;
    }
    virtual void HandleClose() override
    {
        std::cout << "close kcp connection!" << std::endl;
        exit(-1);
    }
};

int main(int argc, char *argv[])
{
    //arg : ip + port
    if (argc != 3)
    {
        TRACE("args error please input [ip] [port]");
        exit(-1);
    }

    std::string ip = argv[1];
    uint16_t port = atoi(argv[2]);

    srand(time(NULL));
    //Initialize conv randomly

    kcp::KcpOpt opt;

    opt.conv                = rand() * rand();
    opt.interval            = 10;
    opt.nodelay             = true;
    opt.trigger_fast_resend = 5;
    opt.offline_standard    = 100;
    opt.snd_queue_max_size  = 20000;
    TRACE("conv = ",opt.conv);
    ChatClient client(ip, port, opt);

    client.Start();

    return 0;
}