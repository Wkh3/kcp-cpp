#include "test.h"


#include <stdio.h>
#include <stdlib.h>
#include <Trace.hpp>
#include "test.h"
#include <Kcp.hpp>
using namespace kcp;

// 模拟网络
LatencySimulator *vnet;

// 测试用例
void test()
{
	// 创建模拟网络：丢包率10%，Rtt 60ms~125ms
	vnet = new LatencySimulator(10, 60, 125);
    
    KcpOpt opt;
    Kcp<true>  *kcp1 = nullptr;
    Kcp<true>  *kcp2 = nullptr;
     
    auto func = [&kcp1,&kcp2](const void *buf,std::size_t len,void *p)
    {
        int id = p == kcp1 ? 0 : 1;
        vnet->send(id,buf,len);
        return 0;
    };
    
    opt.send_func = func;
    opt.conv 				= 1;
	opt.trigger_fast_resend = 3;
	opt.nodelay 			= true;
	opt.interval 		    = 10;
	opt.snd_wnd_size 		= 128;
    opt.snd_queue_max_size  = 1000;

    kcp1 = new Kcp<true>(opt);
    kcp2 = new Kcp<true>(opt);
	
	uint32_t current = iclock();
	uint32_t slap = current + 20;
	uint32_t index = 0;
	uint32_t next = 0;
	int64_t sumrtt = 0;
	int count = 0;
	int maxrtt = 0;


	char buffer[2000];
	int hr;

	uint32_t ts1 = iclock();
 
	while (1) {
		isleep(1);
    	
		current = iclock();

		// 每隔 20ms，kcp1发送数据
		for (; current >= slap; slap += 20) {
			((uint32_t*)buffer)[0] = index++;
			((uint32_t*)buffer)[1] = current;

			// 发送上层协议包
			kcp1->Send(buffer, 8);
		}
        kcp1->Update(iclock());
        kcp2->Update(iclock());

		// 处理虚拟网络：检测是否有udp包从p1->p2
		while (1) {
			
			hr = vnet->recv(1, buffer, 2000);
			if (hr <= 0) break;
			// 如果 p2收到udp，则作为下层协议输入到kcp2
			
            kcp2->Input(buffer,hr);
		}

		// 处理虚拟网络：检测是否有udp包从p2->p1
		while (1) {
		
			hr = vnet->recv(0, buffer, 2000);
			  if (hr <= 0) break;
			//TRACE("hr = ",hr);
			// 如果 p1收到udp，则作为下层协议输入到kcp1
		
            kcp1->Input(buffer,hr);
		}

		// kcp2接收到任何包都返回回去
		while (1) {
			
			hr = kcp2->Recv(buffer, 1000);
			// 没有收到包就退出
			if (hr <= 0) break;
			// 如果收到包就回射
			kcp2->Send(buffer,hr);
			  
		}

		// kcp1收到kcp2的回射数据
		while (1) {
			hr = kcp1->Recv(buffer, 1000);
			// 没有收到包就退出
		
			if (hr <= 0) break;

			uint32_t sn = *(uint32_t*)(buffer + 0);
			uint32_t ts = *(uint32_t*)(buffer + 4);
			uint32_t rtt = current - ts;
			
			if (sn != next) {
				// 如果收到的包不连续
				printf("ERROR sn %d<->%d\n", (int)count, (int)next);
				return;
			}

			next++;
			sumrtt += rtt;
			count++;
			if (rtt > (uint32_t)maxrtt) maxrtt = rtt;

			printf("[RECV] sn=%d rtt=%u\n", (int)sn, rtt);
		}
		//TRACE("---");
		if (next > 1000) break;
	}

	ts1 = iclock() - ts1;


	const char *names[3] = { "default", "normal", "fast" };
	printf("avgrtt=%d maxrtt=%d tx=%d\n", (int)(sumrtt / ++count), (int)maxrtt, (int)vnet->tx1);
}

int main()
{
	test();	

	return 0;
}