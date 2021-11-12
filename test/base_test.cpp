/*
 * @Author: wkh
 * @Date: 2021-11-10 14:53:35
 * @LastEditTime: 2021-11-12 11:49:56
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/test/main.cpp
 * 
 */
#include <Kcp.hpp>
#include <Trace.hpp>
using namespace kcp;
int main()
{
    kcp::KcpOpt opt;
    opt.use_congesition = false;
    opt.conv = 1;   
    opt.snd_queue_max_size = 1000;
    
    Kcp<> kcp1(opt);
    Kcp<> kcp2(opt);

    auto send_func1 = [&kcp2](const void *data,std::size_t size,void *)
    {
       KcpHdr *hdr = (KcpHdr*)data;
       //TRACE(size);
       int ret = kcp2.Input(const_cast<void*>(data),size);
       //TRACE(ret);
       return 0;
    }; 

    auto send_func2 = [&kcp1](const void *data,std::size_t size,void *)
    {
       //TRACE(size);
       int ret = kcp1.Input(const_cast<void*>(data),size);
       //TRACE(ret);
       return 0;
    }; 
    
    kcp1.SetSendFunc(send_func1);
    kcp2.SetSendFunc(send_func2);
    
    for(int i = 0; i < 1; i++)
    {
        kcp1.Send(&i,sizeof(i));
        kcp2.Send(&i,sizeof(i));
    }
    while(1)
    {
        clock_t now = clock();
        if(kcp1.Update(now))
        {
            TRACE("kcp1 offline");
        }
        if(kcp2.Update(now))
        {
            TRACE("kcp2 offline");
        }

        int tmp;
        while(1){
            int ret = kcp2.Recv(&tmp,sizeof(tmp));

            if(ret <= 0)
              break;

            kcp2.Send(&tmp,sizeof(tmp));

            TRACE(tmp);
        }
        while(1)
        {
            int ret = kcp1.Recv(&tmp,sizeof(tmp));

            if(ret <= 0)
              break;

            TRACE(tmp);
        }
    }
    return 0;   
}