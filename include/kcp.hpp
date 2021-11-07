/*
 * @Author: wkh
 * @Date: 2021-11-01 16:31:14
 * @LastEditTime: 2021-11-08 00:05:24
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
	 * @brief  Kcp wiil fragment data to KcpSeg
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

       void ReplyAck();

       std::size_t GetWndSize();
private:
       using AckType  = std::pair<int,int>;
       using AckList  = List<AckType,ThreadSafe>;
       using HdrList  = List<KcpHdr::ptr,ThreadSafe>;
       using SegList  = List<KcpSeg::ptr,ThreadSafe>;
       using BufList  = List<KcpHdr::ptr,ThreadSafe>;

       template<typename T>
       using Atomic   = typename std::conditional<ThreadSafe,std::atomic<T>,FakeAtomic<T>>::type;

private:
       
       uint8_t                         state_            {KcpAttr::KCP_ONLINE};
       uint32_t                        current_          {0};
       uint32_t                        flush_ts_         {0};
       uint32_t                        probe_wait_       {0};
       Atomic<uint32_t>                probe_            {0};
       Atomic<uint32_t>                rcv_next_         {0};
       KcpOpt                          opt_;
       HdrList                         rcv_queue_;
       HdrList                         rcv_buf_;
       SegList                         snd_queue_;
       SegList                         snd_buf_;
       AckList                         acks_;
       BufList                         buf_;
};

}
