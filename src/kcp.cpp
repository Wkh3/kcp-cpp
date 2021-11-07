/*
 * @Author: wkh
 * @Date: 2021-11-05 20:56:00
 * @LastEditTime: 2021-11-08 00:16:47
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

}

