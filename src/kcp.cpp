/*
 * @Author: wkh
 * @Date: 2021-11-05 20:56:00
 * @LastEditTime: 2021-11-09 21:47:02
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/src/kcp.cpp
 * 
 */

#include <kcp.hpp>
#include <string.h>

namespace kcp
{

    KcpHdr::KcpHdr(uint16_t length) : len(length)
    {}

    KcpHdr::ptr KcpHdr::create(uint16_t len)
    {
         return KcpHdr::ptr(new (malloc(KcpAttr::KCP_HEADER_SIZE + len)) KcpHdr(len));
    }

    int Kcp<ThreadSafe>::Send(const void *data, uint32_t size)
    {
        if(size == 0)
		  return 0;
      
		uint32_t count = (size + opt_.mss - 1) / opt_.mss;
        
	 	AutoLock<SegList> lock(snd_queue_);
		 
        auto &snd_queue = snd_queue_.get();
        
		if(count + snd_queue.size() > opt_.snd_queue_max_size || count > KcpAttr::KCP_FRG_LIMIT)
		   return -1;

        const char *src = static_cast<const char*>(data);

        uint32_t offset = 0;

		for(uint32_t i = 0;i < count; i++)
		{
			uint32_t len = std::min(size,static_cast<uint32_t>(opt_.mss));
	 		
			KcpSeg *seg = new KcpSeg();

			seg->data = KcpHdr::create(len);
            
			seg->data->frg = count - i - 1;

			memcpy(seg->data->buf,src + offset,len);
            
			snd_queue.emplace_back(seg);

			size -= len;
			offset += len;
		}

		return 0;
    }
	
    int Kcp<ThreadSafe>::Input(void *data, std::size_t size)
	{

	}
    Kcp<ThreadSafe>::Kcp(const KcpOpt &opt) : opt_(opt)
	{
		if(!opt_.send_func)
		  throw "KcpOpt don't set send_func!";
		
		if(opt_.mss > opt_.mtu)
		  throw "It's illegal that mss greater than mtu";
		

	}
    bool Kcp<ThreadSafe>::Update(uint32_t current)
	{
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
			return;

		flush_ts_ += opt_.interval;

		//if next update time less than current_,fix it
		if (current_ - flush_ts_ >= 0)
			flush_ts_ = current_ + opt_.interval;
			
		return Offline();
	}
    void Kcp<ThreadSafe>::Flush()
	{
		// 1.reply ack to client
		ReplyAck();
		// 2.check remote window size
		CheckWnd();
		// 3.handle window cmd
		HandleWndCmd();
		// 4.flush buf
		FlushBuf();
		// 5.move send window
		MoveSndWnd();
		// 6.check whther KcpSeg in the send window need to be sent
		
	}
    void Kcp<ThreadSafe>::MoveSndWnd()
	{
		//the size of send window is the minimum in swnd,rmd_wnd and cwnd
		uint32_t swnd = std::min(opt_.snd_wnd_size,rmt_wnd_.load(std::memory_order_relaxed));
		
		if(opt_.use_congesition)
		  swnd = std::min(swnd,cwnd_.load(std::memory_order_relaxed));
		
		AutoLock<SegList> lock(snd_queue_);
		
		auto &snd_queue = snd_queue_.get();

		while (snd_next_.load(std::memory_order_acquire) < snd_unack_.load(std::memory_order_acquire) + swnd 
			   && !snd_queue_.empty())
		{
			auto &seg = snd_queue.front();

			seg->data->conv = opt_.conv;
			seg->data->cmd  = KcpAttr::KCP_CMD_PUSH;
			seg->data->wnd  = GetWndSize();
			seg->data->ts   = current_;
			seg->data->sn   = snd_next_++;
			seg->data->una  = rcv_next_.load(std::memory_order_relaxed);
			seg->rto 	    = rx_rto_.load(std::memory_order_relaxed);
			seg->rts 		= current_;
			seg->fack 		= 0;
			seg->mit 		= 0;

			snd_buf_.push_back(std::move(seg));

			snd_queue.pop_front();
		}

	}
	
	void Kcp<ThreadSafe>::HandleWndCmd()
	{

		uint32_t probe  = probe_.exchange(0,std::memory_order_relaxed);

		if(probe == 0)
		  return;
		
		KcpHdr hdr(0);
		hdr.conv = opt_.conv;
		hdr.wnd  = GetWndSize();
		hdr.una  = rcv_next_.load(std::memory_order_relaxed);

		if (probe_ & KcpAttr::KCP_ASK_SEND)
		{
			hdr.cmd = KcpAttr::KCP_CMD_WASK;
			buf_.emplace_back(std::make_shared<KcpHdr>(hdr));
		}

		if (probe_ & KcpAttr::KCP_ASK_TELL)
		{
			hdr.cmd = KcpAttr::KCP_CMD_WINS;
			buf_.emplace_back(std::make_shared<KcpHdr>(hdr));
		}
	}

    void Kcp<ThreadSafe>::FlushBuf()
	{
		if(!opt_.send_func)
		  throw "Kcp dont have send func";
		
		if(buf_.empty())
		  return;
		
		uint32_t offset = 0;

		static std::string str(opt_.mtu,'0');

	    char *data = const_cast<char*>(str.data()); 		
		// combine multi kcp packets into a frame of mtu size
		for (auto &hdr : buf_)
		{
			if (offset + KcpAttr::KCP_HEADER_SIZE + hdr->len > opt_.mtu)
			{
				opt_.send_func(data, offset, this);
				offset = 0;
			}
			
			memcpy(data + offset, hdr.get(), KcpAttr::KCP_HEADER_SIZE + hdr->len);

			offset += KcpAttr::KCP_HEADER_SIZE + hdr->len;
		}
         
		opt_.send_func(data, offset, this);
		
		buf_.clear();
	}
    void Kcp<ThreadSafe>::checkSndWnd()
	{
		
	}
	void Kcp<ThreadSafe>::ReplyAck()
	{
		 if(acks_.empty())
		   return;
		 
		 KcpHdr hdr(0);
		 hdr.conv = opt_.conv;
		 hdr.cmd  = KcpAttr::KCP_CMD_ACK;
		 hdr.wnd  = GetWndSize();
		 hdr.una  = rcv_next_.load(std::memory_order_relaxed);

		 AutoLock<AckList> lock(acks_);

		 for(auto &it : acks_.get())
		 {
			hdr.sn = it.first;
			hdr.ts = it.second;
			buf_.emplace_back(new KcpHdr(hdr));
		 }

		 acks_.get().clear();		 
	}
    std::size_t Kcp<ThreadSafe>::GetWndSize()
	{	 
		 int64_t wnd = opt_.rcv_wnd_size - rcv_queue_.size(); 
		 
		 return wnd < 0 ? 0 : wnd;
	}
	void Kcp<ThreadSafe>::CheckWnd()
	{
		 static uint32_t probe_ts   = 0;
		 static uint32_t probe_wait = 0;
		 
		 // if remote window size greater than 0,dont need to probe remote window
		 if(rmt_wnd_.load(std::memory_order_relaxed) != 0)
		 {
			probe_ts   = 0;
			probe_wait = 0;
			return;
		 }
		 //init probe_wait and probe_ts
		 if(probe_wait == 0)
		 {
			probe_wait = KcpAttr::KCP_PROBE_INIT;
			probe_ts   = current_ + probe_wait;
		 }

		 if(current_ - probe_ts < 0)
		   return;
		
		//add probe wait time 
		probe_wait += probe_wait / 2;

		probe_wait = std::min(KcpAttr::KCP_PROBE_LIMIT,probe_wait);

		probe_ts   = current_ + probe_wait;

		uint32_t probe = probe_.load(std::memory_order_acquire);
		//use cas to set KCP_ASK_SEND flag
		do
		{
			if(probe & KcpAttr::KCP_ASK_SEND)
			   break;
			   
		}while(!probe_.compare_exchange_weak(probe,probe | KcpAttr::KCP_ASK_SEND,std::memory_order_acq_rel));

	}

}

