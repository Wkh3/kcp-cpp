
# 🌟 kcp-cpp

A C++11 header-only kcp library,It has been heavily optimized to support native heartbeat packets and multithreading


---


There are a lot of instructions in `KcpOpt` on how to configure it.

Configure speed mode
```
    kcp::KcpOpt opt;
    opt.conv                = conv;
    opt.interval            = 10;
    opt.nodelay             = true;
    opt.trigger_fast_resend = 5;
```
Use KCP with a single thread
```
    kcp::Kcp<> kcp(opt);
```
Call `Update` every 10 - 100ms repeatedly
```
    kcp.Update(current);
```
# 💥 multi-thread
Benefit by the c++11 feature, kcp-cpp can be used with multiple threads, but with no additional overhead when used in only single thread.  

You only need to set the template parameter to true to use it in multi threads.
```
    kcp::Kcp<true> kcp(opt);
```

Thie means you can call `Update`,`Send`,`Recv`,`Input` with multi threads


```
// thread1
   while(1){ kcp.Update(current);}
```
```
// thread2
   kcp.Send(buf,size);
```

# 🚀 ChatRoom

In the example directory, a chat room example shows you how to write a concurrent kcpServer on Linux

Build it

```
./autobuild
```

Start ChatServer
```
cd bin
./ChatServer server_ip server_port
```
Start ChatClient
```
./ChatClient server_ip server_port
```

This example will teach you how to use kcp-cpp to write an application, use conv to identify the client without reconnecting even if the client's network changes  


# 🎆 Congesition Control

In order to make it more convenient for users to customize the congestion control algorithm, so I separate its interface 

```
    class CongestionControl{
        //@brief: slow start algorithm 
        virtual void SlowStart(uint32_t &cwnd, uint32_t &ssthresh);
        //@brief: algorithm when packet loss occurs
        virtual void Lost(uint32_t &cwnd, uint32_t &ssthresh);
        //@brief: congesion avoidance algorithm
        virtual void CongestionAvoidance(uint32_t &cwnd, uint32_t &ssthresh);
        //@brief: quick recover algorithm 
        virtual void QuickRecover(uint32_t &cwnd, uint32_t &ssthresh,uint32_t fast_resend_trigger);
    };
```

Users can easily customize congestion control by simply inheriting it and implementing these interfaces   

```
    class Control : public CongestionContron{};
    KcpOpt opt;
    opt.use_congestion = true;
    Kcp<> kcp(opt);
    kcp.SetControl(new Control());
```

# 🌔 Sliding Window

In order to slove the problem of reliable transmission and packet disorder,every Kcp object contains a send window and a receive window.Next,let's demonstate the whole sending and receiving process.

In the initial state,the sender send the packets 1 to 4.The left of the send window boundary is `snd_unack_`,which points to packet 1.The right of the send window boundary is
`snd_next_`,which points to packet 5 to be sent.

![image](https://img-blog.csdnimg.cn/b793d386f3f44350bf94d4e41c7e9d6e.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBAQEtI,size_20,color_FFFFFF,t_70,g_se,x_16)

Unfortunately,packet 1 was lost during transmission,so the receiver only received 
packets 2 to 4.the recevier inserts packets 2 to 4 into corresponding position and replies ack to sender.The left boundary of receive window can't move,because packet 1 
wasn't received.

![image](https://img-blog.csdnimg.cn/edc2495f3f1541529851d81378858453.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBAQEtI,size_20,color_FFFFFF,t_70,g_se,x_16)

After the sender receive the ack of packets 2 to 4,it wiil delete packets 2 to 4 in buffer.However packet 1 will be resent due to timeout.

![image](https://img-blog.csdnimg.cn/490c6b7d03b242b4aa10f193803ceb09.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBAQEtI,size_20,color_FFFFFF,t_70,g_se,x_16)

The winow of receiver can move right after receive packet 1 and hand the data to application orderly.Then start the transmission next.

![image](https://img-blog.csdnimg.cn/885b29fd98734a16be2f0e01f85f67cc.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBAQEtI,size_20,color_FFFFFF,t_70,g_se,x_16)


