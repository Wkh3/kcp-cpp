/*
 * @Author: wkh
 * @Date: 2021-11-01 16:31:14
 * @LastEditTime: 2021-11-09 21:52:18
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/kcp.hpp
 */
#pragma once
#include <iostream>
#include <mutex>
#include <util.hpp>
#include <functional>

namespace kcp{

      class KcpHdr
      {
      public:
            using ptr = std::shared_ptr<KcpHdr>;
            uint16_t conv{0};
            uint16_t len {0};
            uint8_t  cmd {0};
            uint8_t  frg {0};
            uint16_t wnd {0};
            uint32_t ts  {0};
            uint32_t sn  {0};
            uint32_t una {0};
            char     buf [0];
      public:
            static KcpHdr::ptr create(uint16_t len);
            
            KcpHdr(uint16_t length);
      };

      class KcpSeg
      {
      public:
            using ptr = std::unique_ptr<KcpSeg>;
      public:
            uint32_t    rts {0};
            uint32_t    rto {0};
            uint32_t    mit {0};
            uint32_t    fack{0};
            KcpHdr::ptr data;
      };

      class KcpAttr
      {
      public:
            //min rto in no delay 
            constexpr static int32_t  KCP_RTO_NDL = 30;
            //min rto in normal
            constexpr static int32_t  KCP_RTO_MIN = 100;
            //default rto
            constexpr static int32_t  KCP_RTO_DEF = 200;
            //max rto
            constexpr static int32_t  KCP_RTO_MAX = 60000;
            //cmd : push data
            constexpr static uint32_t KCP_CMD_PUSH = 81;
            //cmd : ack
            constexpr static uint32_t KCP_CMD_ACK = 82;
            //cmd : window probe
            constexpr static uint32_t KCP_CMD_WASK = 83;
            //cmd : window tell
            constexpr static uint32_t KCP_CMD_WINS = 84;
            //update timer interval
            constexpr static uint32_t KCP_INTERVAL = 100;
            //maximum transmission unit
            constexpr static uint32_t KCP_MTU_DEF = 1400;
            //ssthresh init size
            constexpr static uint32_t KCP_THRESH_INIT = 0xffffffff;
            //ssthresh min size
            constexpr static uint32_t KCP_THRESH_MIN = 2;
            //7 secs to probe window size
            constexpr static uint32_t KCP_PROBE_INIT = 7000;
            //up to 120 secs to probe window
            constexpr static uint32_t KCP_PROBE_LIMIT = 120000;
            //max times to trigger fastack
            constexpr static uint32_t KCP_FAST_RESEND_LIMIT = 5;
            //start time send window size
            constexpr static uint32_t KCP_WND_SND = 32;
            //recv window size   must >= max fragment size
            constexpr static uint32_t KCP_WND_RCV = 128;
            //need send IKCP_CMD_ACK
            constexpr static uint32_t KCP_ASK_SEND = 1;
            //need send IKCP_ASK_TELL
            constexpr static uint32_t KCP_ASK_TELL = 2;
            //send queue max size
            constexpr static uint32_t KCP_SND_QUEUE_MAX_SIZE = 100;
            //Header size
            constexpr static uint32_t KCP_HEADER_SIZE = sizeof(KcpHdr);
            //connection state online
            constexpr static uint8_t  KCP_ONLINE  = 1;
            //connection state offline
            constexpr static uint8_t  KCP_OFFLINE = -1;
            //resend times greater than it will be judged offline
            constexpr static uint32_t KCP_OFFLINE_STANDARD = 20;
            //max frg length
            constexpr static uint32_t KCP_FRG_LIMIT = (1 << 8) - 1;
      };

      class KcpOpt
      {
      public:
            using SendFunc = std::function<int(const void *buf, int len, void *kcp)>;
      public:
            //conv id identify the same session
            uint16_t    conv;
            //maximum transmission unit
            uint16_t    mtu                 {KcpAttr::KCP_MTU_DEF};
            //maximum segment size = mtu - KCP_OVERHEAD
            uint16_t    mss                 {KcpAttr::KCP_MTU_DEF - KcpAttr::KCP_HEADER_SIZE};
            //send window size
            uint32_t    snd_wnd_size        {KcpAttr::KCP_WND_SND};
            //recv window size, the send window size of peer will not be greater it
            uint32_t    rcv_wnd_size        {KcpAttr::KCP_WND_RCV};
            //update timer in every interval
            uint32_t    interval            {KcpAttr::KCP_INTERVAL};
            //max fast resend times
            uint32_t    fast_resend_limit   {KcpAttr::KCP_FAST_RESEND_LIMIT};
            //max resend times,greater it will be judged offline
            uint32_t    offline_standard    {KcpAttr::KCP_OFFLINE_STANDARD};
            //send queue max size,avoid buffer overflow caused by large amount of data
            uint32_t    snd_queue_max_size  {KcpAttr::KCP_SND_QUEUE_MAX_SIZE};
            //if the number of recved ack which grater than it's sn is greater than this,fast resend will be trrigered  
            uint32_t    trigger_fast_resend {0};
            //Turning it on will increase RTO by 1.5 times;
            bool        nodelay             {false};
            //whether use congesition control
            bool        use_congesition     {true};   
            //send func callback
            SendFunc    send_func;
      };
       template<bool ThreadSafe = false>
       class Kcp
       {
       public:
              /**
               * @brief: call it every 10 - 100ms repeatedly
               * @param {uint32_t} current timestamp in millsec
               * @return {bool}    whether peer connection offline  
               */       
              bool Update(uint32_t current);
              /**
               * @brief input data parser to KcpSeg 
               * @return  -1 : input data is too short | -2 :
               */
              int Input(void *data, std::size_t size);
              /**
               * @brief  Kcp wiil atomicly fragment data to KcpSeg
               * @return  0 : success | -1 : failed 
               */
              int Send(const void *data, uint32_t size);
              /**
               * 
               */
              int Recv(void *buf,uint32_t size);
       public:
              Kcp(const KcpOpt &opt);

              bool Offline() const  { return state_ == KcpAttr::KCP_OFFLINE; }
              
              const KcpOpt& GetOpt() const { return opt_; }
       private:
              void Flush();
              /**
               *@brief relpy ack of received data to remote
               */
              void ReplyAck();
              /**
               *@brief check whther need to probe remote window size 
               */
              void CheckWnd();

              void HandleWndCmd();
              /**
               *@brief flush the data of send buf and send to remote  
               */
              void FlushBuf();
              /**
               *@brief move the right boundary of send window after receiving remote ack 
               */
              void MoveSndWnd();
              /**
               *@brief the KcpSeg may be lost,so check whther KcpSeg in the send window need to be resent      
               */
              void checkSndWnd();
              
              std::size_t GetWndSize();
       private:
              using AckType  = std::pair<int,int>;
              using AckList  = List<AckType,ThreadSafe>;
              using HdrList  = List<KcpHdr::ptr,ThreadSafe>;
              using SegList  = List<KcpSeg::ptr,ThreadSafe>;
              using BufList  = std::list<KcpHdr::ptr>; 

              template<typename T>
              using Atomic   = typename std::conditional<ThreadSafe,std::atomic<T>,FakeAtomic<T>>::type;

       private:
              uint8_t                         state_            {KcpAttr::KCP_ONLINE};
              uint32_t                        current_          {0};
              uint32_t                        flush_ts_         {0};
              Atomic<uint32_t>                probe_            {0};
              Atomic<uint32_t>                rcv_next_         {0};
              Atomic<uint32_t>                rmt_wnd_          {0};
              Atomic<uint32_t>                cwnd_             {0};
              Atomic<uint32_t>                snd_unack_        {0};
              Atomic<uint32_t>                snd_next_         {0};
              Atomic<uint32_t>                rx_rto_           {0};
              KcpOpt                          opt_;
              HdrList                         rcv_queue_;
              HdrList                         rcv_buf_;
              SegList                         snd_queue_;
              SegList                         snd_buf_;
              AckList                         acks_;
              BufList                         buf_;
       };

}
