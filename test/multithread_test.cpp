/*
 * @Author: wkh
 * @Date: 2021-11-12 14:46:21
 * @LastEditTime: 2021-11-12 21:31:44
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/test/multithread_test.cpp
 * 
 */
#include <Kcp.hpp>
#include <bits/types/clock_t.h>
#include <thread>
#include <list>
#include <unistd.h>

using namespace kcp;
Kcp<true>::ptr kcp1;
Kcp<true>::ptr kcp2;
int start1 = 0;
int start2 = 0;
void update()
{
    while(1)
    {
        usleep(500);
        clock_t now = clock();
        kcp1->Update(now);
        kcp2->Update(now);

        int tmp = 0;
        int ret = kcp1->Recv(&tmp, sizeof(int));
          
        if (ret == 4)
        {
            if (tmp == start2)
            {
                start2++;
                if(start2 % 1000 == 0)
                  TRACE(start2);
            }
            else
            {
                TRACE("data error!");
            }
        }

        ret = kcp2->Recv(&tmp, sizeof(int));

        if (ret == 4)
        {
            if (tmp == start1)
            {
                start1++;
                if(start1 % 1000 == 0)
                  TRACE(start1);
            }
            else
            {
                TRACE("data error!");
            }
        }
    }
   
}
void run()
{
        for(int i = 0;i < 100000;i++)
        {
            usleep(300);
            kcp1->Send(&i, sizeof(int));
            kcp2->Send(&i, sizeof(int));
        }
        
     //TRACE(start);
}
int main()
{
    KcpOpt opt;
    opt.conv = 1;
    opt.interval = 500;
    opt.trigger_fast_resend = 3;
    opt.snd_queue_max_size = 100000;
    auto kcp1_func = [](const void *data,std::size_t size,void*)
    {
	    kcp2->Input(const_cast<void*>(data),size);
    };
    auto kcp2_func = [](const void *data,std::size_t size,void*)
    {
	    kcp1->Input(const_cast<void*>(data),size);
    };
    kcp1.reset(new Kcp<true>(opt));
    kcp2.reset(new Kcp<true>(opt));
    kcp1->SetSendFunc(kcp1_func);
    kcp2->SetSendFunc(kcp2_func);

    std::thread t1(update);
    std::thread t2(run);

    t1.join();
    t2.join();

    return 0;

}
