/*
 * @Author: wkh
 * @Date: 2021-11-01 16:31:14
 * @LastEditTime: 2021-11-16 16:18:49
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/Kcp.hpp
 */
#pragma once
#include <iostream>
#include <mutex>
#include <string.h>
#include <functional>
#include <Trace.hpp>
#include <Control.hpp>
#include <algorithm>
#include <KcpHdr.hpp>

namespace kcp{
      KcpHdr::KcpHdr(uint16_t length) : len(length)
      {
      }

      KcpHdr::ptr KcpHdr::Create(uint16_t len)
      {
            return KcpHdr::ptr(new (malloc(KcpAttr::KCP_HEADER_SIZE + len)) KcpHdr(len));
      }
      
      inline std::string CmdToString(uint8_t cmd)
      {
           if(cmd == KcpAttr::KCP_CMD_PONG)
             return "KCP_CMD_PONG";
           if(cmd == KcpAttr::KCP_CMD_PING)
             return "KCP_CMD_PIMG";
           if(cmd == KcpAttr::KCP_CMD_ACK)
             return "KCP_CMD_ACK";
           if(cmd == KcpAttr::KCP_CMD_PUSH)
             return "KCP_CMD_PUSH";
           if(cmd == KcpAttr::KCP_CMD_WINS)
             return "KCP_CMD_WINS";
           if(cmd == KcpAttr::KCP_CMD_WASK)
             return "KCP_CMD_WASK";
           
           return "unknown cmd";
      }
      KcpHdr::ptr KcpHdr::Dup(KcpHdr *rhs)
      {
            if(!rhs)
              return nullptr;
            
            KcpHdr::ptr hdr = Create(rhs->len);

            memcpy(hdr.get(),rhs,KcpAttr::KCP_HEADER_SIZE + rhs->len);

            return hdr;
      }
      
       template<bool ThreadSafe>
       int Kcp<ThreadSafe>::Input(void *data, std::size_t size)
       {
            if(!data || size < KcpAttr::KCP_HEADER_SIZE)
                return -1;
            
            char *src = static_cast<char*>(data);

            int err = ParserData(src,size);

            //TRACE(this,rcv_queue_.size());
            if(err != 0)
               return err;

            if(!opt_.use_congesition || !control_)
              return 0;

            Lock lock(control_mtx_);

            if(cwnd_ < ssthresh_)
            {
              control_->SlowStart(cwnd_,ssthresh_);
            }else
            {
              control_->CongestionAvoidance(cwnd_,ssthresh_);
            }

            return 0;
       }
       template<bool ThreadSafe>
       bool Kcp<ThreadSafe>::CheckCmd(uint32_t cmd)
       {
             if(cmd == KcpAttr::KCP_CMD_ACK ||
                cmd == KcpAttr::KCP_CMD_PUSH||
                cmd == KcpAttr::KCP_CMD_WASK||
                cmd == KcpAttr::KCP_CMD_WINS||
                cmd == KcpAttr::KCP_CMD_PING||
                cmd == KcpAttr::KCP_CMD_PONG)
             {
                  return true;
             }
             return false;
       }

       void KcpHdr::dump() const
       {
            TRACE("hdr->conv",conv);
            TRACE("hdr->cmd",CmdToString(cmd));
            TRACE("hdr->ts",ts);
            TRACE("hdr->una",una);
            TRACE("hdr->wnd",wnd);
            TRACE("hdr->len",len);
            TRACE("hdr->sn",sn);
            TRACE("hdr->frg",frg);
            TRACE("--------------------");
       }
       template<bool ThreadSafe>
       int Kcp<ThreadSafe>::ParserData(char *data,std::size_t size)
       {
             if(!data || size == 0)
               return -1;
             
             while(size >= KcpAttr::KCP_HEADER_SIZE)
             {
                   KcpHdr *hdr = reinterpret_cast<KcpHdr*>(data);

                   data   += KcpAttr::KCP_HEADER_SIZE;
                   size   -= KcpAttr::KCP_HEADER_SIZE;

                   if(hdr->conv != opt_.conv)
                   {
                      Close();
                      return -2;
                   }

                   if(size < hdr->len)
                   {
                      Close();
                      return -1;
                   }
                  
                   data  += hdr->len;
                   size  -= hdr->len;

                   if(!CheckCmd(hdr->cmd))
                   {
                      Close();
                      return -3;
                   }
                  
                  rmt_wnd_.store(hdr->wnd,std::memory_order_relaxed);

                  MoveSndWndLeft(hdr->una);
                  
                  ParserCmd(hdr->cmd,hdr);
             }
             return 0;
       }

       template<bool ThreadSafe>
       int Kcp<ThreadSafe>::Recv(void *buf, uint32_t size)
       {
            if(!buf || size == 0)
               return -1;
             
            if(rcv_queue_.empty())
               return 0;

            AutoLock<HdrList> lock(rcv_queue_);

            uint32_t length = PeekSize();

            if(length > size)
              return -1;
            
            auto &rcv_queue = rcv_queue_.get();

            //ths size of recieve window euqal 0 now,need to tell remote window size after recing
            if(rcv_queue.size() > opt_.rcv_wnd_size)
              SetProbeFlag(KcpAttr::KCP_ASK_TELL);
            
            char *des = reinterpret_cast<char*>(buf);

            auto it = rcv_queue.begin();

            int len = 0;

            do
            {
                memcpy(des + len,it->get()->buf,it->get()->len);

                len += it->get()->len;
                
                it = rcv_queue.erase(it);

            }while(it != rcv_queue.end() && it->get()->frg != 0);

            return len;
       }

      template<bool ThreadSafe>
      std::size_t Kcp<ThreadSafe>::PeekSize()
      {
            auto &rcv_queue = rcv_queue_.get();

            if(rcv_queue.empty())
              return 0;
            
            auto it = rcv_queue.begin();

            uint8_t frg = it->get()->frg;

            if(frg == 0)
              return it->get()->len;
            
            if(rcv_queue.size() < frg + 1)
              return 0;
            
            uint32_t length = 0;

            for(auto &hdr : rcv_queue)
            {
                  length += hdr->len;

                  if(hdr->frg == 0)
                     break;
            }

            return length;
      }
     
       template<bool ThreadSafe>
       void Kcp<ThreadSafe>::UpdateRto(uint32_t rtt)
       {
            if (rx_srtt_ == 0)
		{
                rx_srtt_   = rtt;
                rx_rttval_ = rtt / 2;
            }
            else
            {
                int32_t delta = abs(static_cast<int64_t>(rtt) - static_cast<int64_t>(rx_srtt_));
                rx_rttval_    = (3 * rx_rttval_ + delta) / 4;
                rx_srtt_      = (7 * rx_srtt_ + rtt) / 8;
                rx_srtt_      = std::max(rx_srtt_, 1u);
            }
            
            uint32_t rto = rx_srtt_ + std::max(opt_.interval,4 * rx_rttval_);

            rx_rto_.store(bound(KcpAttr::KCP_RTO_NDL,rto,KcpAttr::KCP_RTO_MAX),std::memory_order_release);
       }

       template<bool ThreadSafe>
       void Kcp<ThreadSafe>::RemoveSeg(uint32_t sn)
       {
             if(sn < snd_unack_.load(std::memory_order_acquire) 
             || sn >= snd_next_.load(std::memory_order_acquire))
             {
                   return;
             }

             AutoLock<SegList> lock(snd_buf_);
             auto &snd_buf = snd_buf_.get();

             auto it = std::find_if(snd_buf.begin(),snd_buf.end(),[sn](KcpSeg::ptr &seg)
             {
                  seg->fack++;
                  return seg->data->sn >= sn;
             });

             if(it != snd_buf.end() && it->get()->data->sn == sn)
                snd_buf.erase(it);
       }

       template<bool ThreadSafe>
       void Kcp<ThreadSafe>::ParserCmd(uint32_t cmd,KcpHdr *hdr)
       {
             rcv_time.store(current_,std::memory_order_relaxed);
             switch (cmd)
             {
             case KcpAttr::KCP_CMD_ACK:
             {
                  int32_t rtt = static_cast<int64_t>(current_) - static_cast<int64_t>(hdr->ts);

                  if(rtt >= 0)
                    UpdateRto(rtt);
                  
                  RemoveSeg(hdr->sn);
                   
                  break;
             }
             case KcpAttr::KCP_CMD_PUSH:
             {
                  //over recv window
                  acks_.emplace_back(hdr->sn,hdr->ts);

                  InsertData(hdr);

                  break;
             }
             case KcpAttr::KCP_CMD_WASK:
             {
                  SetProbeFlag(KcpAttr::KCP_ASK_TELL);
                  break;
             }
             case KcpAttr::KCP_CMD_PING:
             {
                  pong_.store(true,std::memory_order_relaxed);
                  break;
             }
             case KcpAttr::KCP_CMD_PONG:
             case KcpAttr::KCP_CMD_WINS:
                   break;
             default:
                   break;
             }
       }
       
       template<bool ThreadSafe>
       void Kcp<ThreadSafe>::InsertData(KcpHdr *hdr)
       {
            //check it is null or expired
            if(!hdr || hdr->sn < rcv_next_.load(std::memory_order_relaxed))
              return;
            
            AutoLock<HdrList> lock(rcv_buf_);

            auto &rcv_buf = rcv_buf_.get();

            auto it = std::find_if(rcv_buf.begin(),rcv_buf.end(),[hdr](KcpHdr::ptr &p)
            {
                  return p->sn >= hdr->sn;
            });

            //repeat data dont need insert
            if(it != rcv_buf.end() && it->get()->sn == hdr->sn)
               return;
            
            rcv_buf.insert(it,KcpHdr::Dup(hdr));
            
            //sn != window left boundary,dont need to move recv window
            if(hdr->sn != rcv_next_.load(std::memory_order_acquire))
              return;

            while(!rcv_buf.empty())
            {
                  auto &first = rcv_buf.front();

                  if(first->sn != rcv_next_.load(std::memory_order_acquire) ||
                     rcv_queue_.size() > opt_.rcv_wnd_size)
                  {
                      return; 
                  }

                  rcv_queue_.push_back(std::move(first));

                  rcv_buf.pop_front();

                  ++rcv_next_;
            }
       }
       template<bool ThreadSafe>
       void Kcp<ThreadSafe>::MoveSndWndLeft(uint32_t unack)
       {
            if(unack <= snd_unack_.load(std::memory_order_acquire))
                return;

            AutoLock<SegList> lock(snd_buf_);

            auto &snd_buf = snd_buf_.get();

            auto it = std::find_if(snd_buf.begin(),snd_buf.end(),[this,unack](KcpSeg::ptr &p)
            {
                  return p->data->sn >= unack;
            });
            
            snd_buf.erase(snd_buf.begin(),it);

            unack = snd_buf.empty() ? snd_next_.load(std::memory_order_acquire) : snd_buf.front()->data->sn;

            snd_unack_.store(unack,std::memory_order_relaxed);
       }
       
       template <bool ThreadSafe>
       int Kcp<ThreadSafe>::Send(const void *data, uint32_t size)
       {
             if (size == 0)
                   return 0;

             uint32_t count = (size + opt_.mss - 1) / opt_.mss;

             AutoLock<SegList> lock(snd_queue_);

             auto &snd_queue = snd_queue_.get();

             if (count + snd_queue.size() > opt_.snd_queue_max_size || count > KcpAttr::KCP_FRG_LIMIT)
                   return -1;

             const char *src = static_cast<const char *>(data);

             uint32_t offset = 0;

             for (uint32_t i = 0; i < count; i++)
             {
                   uint32_t len = std::min(size, static_cast<uint32_t>(opt_.mss));

                   KcpSeg *seg = new KcpSeg();
                   
                   seg->data = KcpHdr::Create(len);
                   
                   seg->data->frg = count - i - 1;

                   memcpy(seg->data->buf, src + offset, len);

                   snd_queue.emplace_back(seg);

                   size   -= len;
                   offset += len;
             }

             return 0;
       }

       template <bool ThreadSafe>
       Kcp<ThreadSafe>::Kcp(const KcpOpt &opt) : opt_(opt),
                                                 control_(new CongestionControl())
       {

             if (opt_.mss > opt_.mtu)
                   throw "It's illegal that mss greater than mtu";
       }

       template <bool ThreadSafe>
       bool Kcp<ThreadSafe>::Update(uint32_t current)
       {
             if(Offline())
               return true;
            
             current_ = current;

             if (flush_ts_ == 0)
                   flush_ts_ = current_;

             int slap = current_ - flush_ts_;

             if (slap >= 10000 || slap < -10000)
             {
                   flush_ts_ = current_;
                   slap = 0;
             }

             if (slap < 0)
                   return Offline();

             flush_ts_ += opt_.interval;

             //if next update time less than current_,fix it
             if (current_ - flush_ts_ >= 0)
                   flush_ts_ = current_ + opt_.interval;

             Flush();

             return Offline();
       }
       template <bool ThreadSafe>
       void Kcp<ThreadSafe>::Flush()
       {
             // 1.reply ack to client
             ReplyAck();
             // 2.check remote window size
             CheckWnd();
             // 3.handle window cmd
             HandleWndCmd();
             // 4.check heart beta
             HeartBeat();
             // 5.flush buf
             FlushBuf();
             // 6.move the right boundary of send window
             MoveSndWndRight();
             // 7.check whther KcpSeg in the send window need to be sent
             CheckSndWnd();
       }
       template<bool ThreadSafe>
       void Kcp<ThreadSafe>::HeartBeat()
       {
            
            if(rcv_time.load(std::memory_order_acquire) == 0)
               rcv_time.store(current_,std::memory_order_release);

            if(current_ > rcv_time.load(std::memory_order_relaxed) + KcpAttr::KCP_TIMEOUT)
            {
               Close();
               return;
            }

            KcpHdr hdr(0);
            hdr.conv = opt_.conv;
            hdr.wnd  = GetWndSize();
            hdr.una  = rcv_next_.load(std::memory_order_relaxed);


            if(current_ > rcv_time.load(std::memory_order_relaxed) + KcpAttr::KCP_HEARTBEAT_CHECK)
            {
                  hdr.cmd = KcpAttr::KCP_CMD_PING;
                  buf_.emplace_back(std::make_shared<KcpHdr>(hdr));
            }

            bool pong = pong_.exchange(false,std::memory_order_relaxed);

            if(pong)
            {
                  hdr.cmd = KcpAttr::KCP_CMD_PING;
                  buf_.emplace_back(std::make_shared<KcpHdr>(hdr));
            }
       }
       template <bool ThreadSafe>
       void Kcp<ThreadSafe>::MoveSndWndRight()
       {
             //the size of send window is the minimum in swnd,rmd_wnd and cwnd
             uint32_t swnd = std::min(opt_.snd_wnd_size, rmt_wnd_.load(std::memory_order_relaxed));

             if (opt_.use_congesition)
             {
                  Lock lock(control_mtx_);
                  swnd = std::min(swnd, cwnd_);
             }
             AutoLock<SegList> lock(snd_queue_);

             auto &snd_queue = snd_queue_.get();

             while (snd_next_.load(std::memory_order_acquire) < snd_unack_.load(std::memory_order_acquire) + swnd && !snd_queue.empty())
             {
                   auto &seg = snd_queue.front();

                   seg->data->conv = opt_.conv;
                   seg->data->cmd  = KcpAttr::KCP_CMD_PUSH;
                   seg->data->wnd  = GetWndSize();
                   seg->data->ts   = current_;
                   seg->data->sn   = snd_next_++;
                   seg->data->una  = rcv_next_.load(std::memory_order_relaxed);
                   seg->rto        = rx_rto_.load(std::memory_order_relaxed);
                   seg->rts        = current_;
                   seg->fack       = 0;
                   seg->mit        = 0;
                   snd_buf_.push_back(std::move(seg));

                   snd_queue.pop_front();
             }
       }

       template <bool ThreadSafe>
       void Kcp<ThreadSafe>::HandleWndCmd()
       {
             uint32_t probe = probe_.exchange(0, std::memory_order_relaxed);

             if (probe == 0)
                   return;

             KcpHdr hdr(0);
             hdr.conv = opt_.conv;
             hdr.wnd  = GetWndSize();
             hdr.una  = rcv_next_.load(std::memory_order_relaxed);

             if (probe & KcpAttr::KCP_ASK_SEND)
             {
                   hdr.cmd = KcpAttr::KCP_CMD_WASK;
                   buf_.emplace_back(std::make_shared<KcpHdr>(hdr));
             }

             if (probe & KcpAttr::KCP_ASK_TELL)
             {
                   hdr.cmd = KcpAttr::KCP_CMD_WINS;
                   buf_.emplace_back(std::make_shared<KcpHdr>(hdr));
             }
       }
       template <bool ThreadSafe>
       void Kcp<ThreadSafe>::FlushBuf()
       {
             if (!opt_.send_func)
                   throw "Kcp dont have send func";

             if (buf_.empty())
                   return;

             uint32_t offset = 0;

             static std::string str(opt_.mtu, '0');

             char *data = const_cast<char *>(str.data());
             // combine multi kcp packets into a frame of mtu size
             for (auto &hdr : buf_)
             {
                  
                   //TraceHdr(hdr.get());
                   if (offset + KcpAttr::KCP_HEADER_SIZE + hdr->len > opt_.mtu)
                   {
                         opt_.send_func(data, offset, this);
                         offset = 0;
                   }

                   memcpy(data + offset, hdr.get(), KcpAttr::KCP_HEADER_SIZE + hdr->len);

                   offset += KcpAttr::KCP_HEADER_SIZE + hdr->len;
             }

            //  for(uint32_t i = 0;i < offset;)
            //  {
            //       KcpHdr *hdr = (KcpHdr*)(data + i);
            //       TraceHdr(hdr);
            //       i += KcpAttr::KCP_HEADER_SIZE;
            //       i += hdr->len;
            //  }

             opt_.send_func(data, offset, this);

             buf_.clear();
       }

       template <bool ThreadSafe>
       void Kcp<ThreadSafe>::CheckSndWnd()
       {
             bool lost        = false;
             bool fast_resend = false;

             snd_buf_.for_each([&lost, &fast_resend, this](KcpSeg::ptr &seg)
                               {
                                     if (CheckSeg(seg, lost, fast_resend))
                                     {
                                           seg->data->ts  = current_;
                                           seg->mit++;
                                           seg->data->wnd = GetWndSize();
                                           seg->data->una = rcv_next_.load(std::memory_order_relaxed);
                                           buf_.push_back(seg->data);

                                           
                                           if (seg->mit >= opt_.offline_standard)
                                           {
                                              Close(); 
                                           }
                                     }
                               });
            
            FlushBuf();

            {
                  if(!opt_.use_congesition || (!lost && !fast_resend) || !control_)
                        return;
                  
                  Lock lock(control_mtx_);
                  //fast resend
                  if(fast_resend)
                     control_->QuickRecover(cwnd_,ssthresh_,opt_.trigger_fast_resend);
                  //lost KcpSeg
                  if(lost)
                     control_->Lost(cwnd_,ssthresh_);
            }

       }
       template <bool ThreadSafe>
       bool Kcp<ThreadSafe>::CheckSeg(KcpSeg::ptr &seg, bool &lost, bool &fast_resend)
       {
             uint32_t rto_min = opt_.nodelay ? 0 : rx_rto_.load(std::memory_order_relaxed) >> 3;
             
             //TRACE("seg->rto",seg->rto);
             //first send
             if (seg->mit == 0)
             {
                  
                   seg->rto = rx_rto_.load(std::memory_order_relaxed);
                   seg->rts = current_ + seg->rto + rto_min;

                   return true;
             }

             //timeout lead to resend
             if (current_ >= seg->rts)
             {
                   lost = true;

                   seg->rto += opt_.nodelay ? seg->rto / 2 : std::max(seg->rto, rx_rto_.load(std::memory_order_relaxed));
                   seg->rts = current_ + seg->rto;
                   return true;
             }

             //dont use fast resend
             if (opt_.trigger_fast_resend == 0)
                   return false;

             if (seg->fack >= opt_.trigger_fast_resend && seg->mit < opt_.fast_resend_limit)
             {
                   seg->fack = 0;
                   seg->rts = current_ + seg->rto;
                   fast_resend = true;

                   return true;
             }

             return false;
       }

       template <bool ThreadSafe>
       void Kcp<ThreadSafe>::ReplyAck()
       {
             if (acks_.empty())
                   return;

             KcpHdr hdr(0);
             hdr.conv = opt_.conv;
             hdr.cmd = KcpAttr::KCP_CMD_ACK;
             hdr.wnd = GetWndSize();
             hdr.una = rcv_next_.load(std::memory_order_relaxed);

             AutoLock<AckList> lock(acks_);

             for (auto &it : acks_.get())
             {
                   hdr.sn = it.first;
                   hdr.ts = it.second;
                   buf_.emplace_back(new KcpHdr(hdr));
             }

             acks_.get().clear();
       }
       template <bool ThreadSafe>
       std::size_t Kcp<ThreadSafe>::GetWndSize()
       {
             int64_t wnd = static_cast<int64_t>(opt_.rcv_wnd_size) -  static_cast<int64_t>(rcv_queue_.size());
             return wnd < 0 ? 0 : wnd;
       }
       
       template<bool ThreadSafe>
       void Kcp<ThreadSafe>::SetProbeFlag(uint32_t flag)
       {
             uint32_t probe = probe_.load(std::memory_order_acquire);
             //use cas to set KCP_ASK_SEND flag
             do
             {
                   if (probe & flag)
                         break;

             } while (!probe_.compare_exchange_weak(probe, probe | flag, std::memory_order_acq_rel));
       }
       
       template <bool ThreadSafe>
       void Kcp<ThreadSafe>::CheckWnd()
       {
             // if remote window size greater than 0,dont need to probe remote window
             if (rmt_wnd_.load(std::memory_order_relaxed) != 0)
             {
                   probe_ts_ = 0;
                   probe_wait_ = 0;
                   return;
             }
             //init probe_wait and probe_ts
             if (probe_wait_ == 0)
             {
                   probe_wait_ = KcpAttr::KCP_PROBE_INIT;
                   probe_ts_   = current_ + probe_wait_;
             }

             if (current_ - probe_ts_ < 0)
                   return;

             //add probe wait time
             probe_wait_ += probe_wait_ / 2;
             
             probe_wait_ = std::min(2u, probe_wait_);

             probe_ts_   = current_ + probe_wait_;

             SetProbeFlag(KcpAttr::KCP_ASK_SEND);
       }

}
