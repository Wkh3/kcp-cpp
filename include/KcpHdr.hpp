/*
 * @Author: wkh
 * @Date: 2021-11-01 18:54:01
 * @LastEditTime: 2021-11-16 19:54:20
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/KcpHdr.hpp
 * 
 */
#pragma once
#include <functional>
#include <memory>
#include <iostream>
#include <atomic>
#include <mutex>
#include <util.hpp>
#include <list>

namespace kcp
{
      #pragma pack(1)
      class KcpHdr
      {
      public:
            using ptr = std::shared_ptr<KcpHdr>;
            uint64_t conv{0};
            uint32_t ts{0};
            uint32_t sn{0};
            uint32_t una{0};
            uint16_t len{0};
            uint8_t  cmd{0};
            uint8_t  frg{0};
            uint16_t wnd{0};
            char     buf[0];

      public:
            static KcpHdr::ptr Create(uint16_t len);

            static KcpHdr::ptr Dup(KcpHdr *rhs);

            KcpHdr(uint16_t length);

            void  Dump() const;
      };
      #pragma pack()

      class KcpSeg
      {
      public:
            using ptr = std::unique_ptr<KcpSeg>;

      public:
            uint32_t    rts{0};
            uint32_t    rto{0};
            uint32_t    mit{0};
            uint32_t    fack{0};
            KcpHdr::ptr data;
      };

      class KcpAttr
      {
      public:
            //min rto in no delay
            constexpr static uint32_t KCP_RTO_NDL = 30;
            //min rto in normal
            constexpr static uint32_t KCP_RTO_MIN = 100;
            //default rto
            constexpr static uint32_t KCP_RTO_DEF = 200;
            //max rto
            constexpr static uint32_t KCP_RTO_MAX  = 10000;
            //cmd : push data
            constexpr static uint8_t KCP_CMD_PUSH = 81;
            //cmd : ack
            constexpr static uint8_t KCP_CMD_ACK = 82;
            //cmd : window probe
            constexpr static uint8_t KCP_CMD_WASK = 83;
            //cmd : window tell
            constexpr static uint8_t KCP_CMD_WINS = 84;
            //cmd : ping
            constexpr static uint8_t KCP_CMD_PING = 85;
            //cmd : pong
            constexpr static uint8_t KCP_CMD_PONG = 86;
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
            constexpr static uint32_t IKCP_PROBE_LIMIT = 120000;
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
            constexpr static uint32_t KCP_SND_QUEUE_MAX_SIZE = 1024;
            //Header size
            constexpr static uint32_t KCP_HEADER_SIZE = sizeof(KcpHdr);
            //connection state online
            constexpr static int8_t   KCP_ONLINE  = 1;
            //connection state offline
            constexpr static int8_t   KCP_OFFLINE = -1;
            //resend times greater than it will be judged offline
            constexpr static uint32_t KCP_OFFLINE_STANDARD = 100;
            //max frg length
            constexpr static uint32_t KCP_FRG_LIMIT = (1 << 8) - 1;
            //the interval for sending heartbeat packets
            constexpr static uint32_t KCP_HEARTBEAT_CHECK = 1500;
            //timeout in millisecond
            constexpr static uint32_t KCP_TIMEOUT  = 1500 * 8;
      };

      class KcpOpt
      {
      public:
            using SendFunc = std::function<void(const void *buf, int len, void *kcp)>;
      public:
            //conv id identify the same session
            uint64_t conv;
            //maximum transmission unit
            uint16_t mtu{KcpAttr::KCP_MTU_DEF};
            //maximum segment size = mtu - KCP_OVERHEAD
            uint16_t mss{KcpAttr::KCP_MTU_DEF - KcpAttr::KCP_HEADER_SIZE};
            //send window size
            uint32_t snd_wnd_size{KcpAttr::KCP_WND_SND};
            //recv window size, the send window size of peer will not be greater it
            uint32_t rcv_wnd_size{KcpAttr::KCP_WND_RCV};
            //update timer in every interval
            uint32_t interval{KcpAttr::KCP_INTERVAL};
            //max fast resend times
            uint32_t fast_resend_limit{KcpAttr::KCP_FAST_RESEND_LIMIT};
            //max resend times,greater it will be judged offline
            uint32_t offline_standard{KcpAttr::KCP_OFFLINE_STANDARD};
            //send queue max size,avoid buffer overflow caused by large amount of data
            uint32_t snd_queue_max_size{KcpAttr::KCP_SND_QUEUE_MAX_SIZE};
            //if the number of recved ack which grater than it's sn is greater than this,fast resend will be trrigered
            uint32_t trigger_fast_resend{0};
            //the interval for sending heartbeat packets
            uint32_t heartbeat{KcpAttr::KCP_HEARTBEAT_CHECK};
            //Turning it on will increase RTO by 1.5 times;
            bool     nodelay{false};
            //whether use congesition control
            bool     use_congestion{false};
            //send func callback
            SendFunc send_func;
      };
      template <bool ThreadSafe = false>
      class Kcp
      {
      public:
            using ptr = std::shared_ptr<Kcp>;
            /**
             * @brief: call it every 10 - 100ms repeatedly
             * @param[in] current timestamp in millsec
             * @return {bool}     whether peer connection offline  
             */
            bool Update(uint32_t current);
            /**
             * @brief input data parser to KcpSeg
             * @param[in] data  
             * @param[in] size  
             * @return  0 : success |-1 : input data is too short | -2 : wrong conv | -3 : data error
             */
            int Input(void *data, std::size_t size);
            /**
             * @brief  Kcp wiil atomatically fragment data to KcpSeg
             * @param[in] data  
             * @param[in] size  
             * @return  0 : success | -1 : failed 
             */
            int Send(const void *data, uint32_t size);
            /**
             * @brief  receive a user packet from Kcp
             * @param[in,out] data  
             * @param[in]  size  
             * @return  -1 : the size of buf greater data size | 0 : empty | > 0 : data size
             */
            int Recv(void *buf, uint32_t size);

      public:
            Kcp(const KcpOpt &opt);

            bool Offline() { return state_.load(std::memory_order_relaxed) == KcpAttr::KCP_OFFLINE; }

            const KcpOpt &GetOpt() const { return opt_; }

            void SetControl(CongestionControl *p) { control_.reset(p); }

            void SetSendFunc(const KcpOpt::SendFunc &send_func){ opt_.send_func = send_func;}
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
             *@brief move the right boundary of send window when the send window can be expanded 
             */
            void MoveSndWndRight();
            /**
             *@brief move the left boundary of send window after receiving remote ack 
             */
            void MoveSndWndLeft(uint32_t unack);
            /**
             *@brief the KcpSeg may be lost,so check whther KcpSeg in the send window need to be resent      
             */
            void CheckSndWnd();
            /**
             * @brief: Check whther the KcpSeg need to be resent
             * @param[in]     seg   KcpSeg
             * @param[in,out] lost  whther the KcpSeg is lost
             * @param[in,out] fast_resend whther fast resend occur
             * @return        true : need to be resent | false : dont need to be resent
             */
            bool CheckSeg(KcpSeg::ptr &seg, bool &lost, bool &fast_resend);
            /**
             *@brief parser data to KcpSeg
             *@param[in] data
             *@param[in] size
             *@return 0 : success | -1 : faild
             */
            int ParserData(char *data,std::size_t size);
            /**
             *@brief  parser cmd and handle
             */ 
            void ParserCmd(uint32_t cmd,KcpHdr *hdr);
            /**
             *@brief check whther the cmd of KcpSeg is vaild
             */
            bool CheckCmd(uint32_t cmd);
            /**
             *@brief  set state_ offline
             */
            void Close(){ state_.store(KcpAttr::KCP_OFFLINE,std::memory_order_relaxed);};
            /**
             *@brief update rto according to rtt
             */
            void UpdateRto(uint32_t rtt);
            /**
             *@brief remove KcpSeg which sequance number equal sn and increase fack of KcpSeg which sequance number less than sn 
             */
            void RemoveSeg(uint32_t sn);
            /**
             * @brief: insert the KcpSeg into corresponding position and move left boundary of receiev window
             */            
            void InsertData(KcpHdr *hdr);

            void SetProbeFlag(uint32_t flag);
            /**
             * @brief: get the first size of whole user packet in recv_queue_ 
             */                     
            std::size_t PeekSize();
            /**
             * @brief: check heart beat
             */     
            void HeartBeat();

            std::size_t GetWndSize();

      private:
            using AckType = std::pair<int, int>;
            using AckList = List<AckType, ThreadSafe>;
            using HdrList = List<KcpHdr::ptr, ThreadSafe>;
            using SegList = List<KcpSeg::ptr, ThreadSafe>;
            using BufList = std::list<KcpHdr::ptr>;
            using Control = std::unique_ptr<CongestionControl>;
            using Lock    = std::unique_lock<std::mutex>;

            template <typename T>
            using Atomic = typename std::conditional<ThreadSafe, std::atomic<T>, FakeAtomic<T>>::type;

      private:
            uint32_t                current_          {0};
            uint32_t                flush_ts_         {0};
            uint32_t                cwnd_             {3};
            uint32_t                ssthresh_         {KcpAttr::KCP_THRESH_INIT};
            uint32_t                probe_ts_         {0};
            uint32_t                probe_wait_       {0};
            uint32_t                rx_srtt_          {0};
            uint32_t                rx_rttval_        {0};
            Atomic<int8_t>          state_            {KcpAttr::KCP_ONLINE};
            Atomic<uint32_t>        probe_            {0};
            Atomic<uint32_t>        rcv_next_         {0};
            Atomic<uint32_t>        rmt_wnd_          {KcpAttr::KCP_WND_RCV};
            Atomic<uint32_t>        snd_unack_        {0};
            Atomic<uint32_t>        snd_next_         {0};
            Atomic<uint32_t>        rcv_time          {0};
            Atomic<uint32_t>        rx_rto_           {KcpAttr::KCP_RTO_DEF};
            Atomic<bool>            pong_             {false};
            Control                 control_          {nullptr};
            std::mutex              control_mtx_;
            KcpOpt                  opt_;
            HdrList                 rcv_queue_;
            HdrList                 rcv_buf_;
            SegList                 snd_queue_;
            SegList                 snd_buf_;
            AckList                 acks_;
            BufList                 buf_;
      };

   
}
