/*
 * @Author: wkh
 * @Date: 2021-11-01 18:54:01
 * @LastEditTime: 2021-11-08 00:16:23
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/util.hpp
 * 
 */
#pragma once
#include <functional>
#include <memory>
#include <iostream>
#include <atomic>
#include <mutex>
#include <list>

namespace kcp
{

      class KcpHdr
      {
      public:
            using ptr = std::shared_ptr<KcpHdr>;
            //会话编号
            uint16_t conv{0};
            //数据长度
            uint16_t len {0};
            //操作码
            uint8_t  cmd {0};
            //分片id
            uint8_t  frg {0};
            //窗口
            uint16_t wnd {0};
            //时间戳
            uint32_t ts  {0};
            //序列号
            uint32_t sn  {0};
            //待接收包序列号
            uint32_t una {0};
            //数据
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
            //no delay min rto 无延迟最小重传时间
            constexpr static int32_t  KCP_RTO_NDL = 30;
            //正常模式最小超时重传时间
            constexpr static int32_t  KCP_RTO_MIN = 100;
            //默认超时重传时间
            constexpr static int32_t  KCP_RTO_DEF = 200;
            //最大超时时间
            constexpr static int32_t  KCP_RTO_MAX = 60000;
            //cmd : push data
            constexpr static uint32_t KCP_CMD_PUSH = 81;
            //cmd : ack
            constexpr static uint32_t KCP_CMD_ACK = 82;
            //cmd : window probe
            constexpr static uint32_t KCP_CMD_WASK = 83;
            //cmd : window tell
            constexpr static uint32_t KCP_CMD_WINS = 84;
            //刷新时间间隔
            constexpr static uint32_t KCP_INTERVAL = 100;
            //报文默认大小
            constexpr static uint32_t KCP_MUT_DEF = 1400;
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
            uint16_t    mtu                 {KcpAttr::KCP_MUT_DEF};
            //maximum segment size = mtu - KCP_OVERHEAD
            uint16_t    mss                 {KcpAttr::KCP_MUT_DEF - KcpAttr::KCP_HEADER_SIZE};
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

      
      class FakeLock
      {
      public:
            void lock()   {}
            void unlock() {}
      };

      template<typename T,bool ThreadSafe = false>
      class List
      {
      public:
            using Lock       = typename std::conditional<ThreadSafe,std::mutex,FakeLock>::type;
            using AutoLock   = std::unique_lock<List<T,ThreadSafe>>;
            using Container  = std::list<T>;
            using value_type = T;
      public:
            void lock()       { return lock_.lock();   }
            void unlock()     { return lock_.unlock(); }
            Container& get()  { return list_;          }
            
            std::size_t size()
            {
                  AutoLock lock(lock_);
                  return list_.size();
            }
            bool empty()
            {
                  AutoLock lock(lock_);
                  return list_.empty();
            }
            template<typename T>
            void push_back(T &&x)
            {
                  AutoLock lock(lock_);
                  list_.push_back(std::forward<T>(x));
            }

            template<typename ...Args>
            void emplace_back(Args&& ...args)
            {
                  AutoLock lock(lock_);
                  list_.emplace_back(std::forward<Args>(args)...);
            }

            template<typename Fn>
            void for_each(Fn &&fn)
            {
                  AutoLock lock(lock_);
                  for(auto &it : list_)
                     fn(it);
            }
      private:
            Container    list_;
            Lock         lock_;  
      };

      template<typename T>
      using AutoLock = typename T::AutoLock;


      template<typename T>
      class FakeAtomic
      {
      public:
            FakeAtomic(T &x) : x_(x){}

            FakeAtomic() noexcept = default;

            FakeAtomic() noexcept = default;
             
            T load(std::memory_order m = std::memory_order_acq_rel) { return x_;}
             
            T exchange(T x,std::memory_order m = std::memory_order_acq_rel)
            {
                   T tmp = x_;
                   x_ = x;
                   return tmp;
            }
             
            bool compare_exchange_weak(T &expected, T desired, 
                                       std::memory_order s = std::memory_order_acq_rel) noexcept
            {
                  if(expected != x_)
                  {
                        expected = _x;
                        return false;
                  }

                  x_ = desired;
                  return true;
            }

            void store(T x,std::memory_order m = std::memory_order_acq_rel) { x_ = x;}
      private:
            FakeAtomic(const atomic&) = delete;

            FakeAtomic& operator=(const Atomic&) = delete;

            FakeAtomic& operator=(const Atomic&) volatile = delete;
             
      private:
            T x_;
      };


}
