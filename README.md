# Sliding Window

In order to slove the problem of reliable transmission and packet disorder,every Kcp object contains a send window and a receive window.Next,let's demonstate the whole sending and receiving process.

In the initial state,the sender send the packets 1 to 4.The left of the send window boundary is `snd_unack_`,which points to packet 1.The right of the send window boundary is
`snd_next_`,which points to packet 5 to be sent.

![image](https://img-blog.csdnimg.cn/b793d386f3f44350bf94d4e41c7e9d6e.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBAQEtI,size_20,color_FFFFFF,t_70,g_se,x_16)

Unfortunately,packet 1 was lost during transmission,so the receiver only received 
packets 2 to 4.the recevier inserts packets 2 to 4 into corresponding position and replies ack to sender.The left boundary of receive window can't move,because packet 1 
wasn't received.

![image](https://img-blog.csdnimg.cn/edc2495f3f1541529851d81378858453.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBAQEtI,size_20,color_FFFFFF,t_70,g_se,x_16)

