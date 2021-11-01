/*
 * @Author: wkh
 * @Date: 2021-11-01 19:28:02
 * @LastEditTime: 2021-11-01 19:58:43
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/src/control.hpp
 * 
 */

#include <memory>

class CongestionControl
{
public:
       using ptr = std::unique_ptr<CongestionControl>;

       virtual void SlowStart(uint32_t &cwnd,uint32_t &ssthresh){};

       virtual void FastResend(uint32_t &cwnd,uint32_t &ssthresh){};

       virtual void CongestionAvoidance(uint32_t &cwnd,uint32_t &ssthresh){}; 

       virtual void QuickRecover(uint32_t &cwnd,uint32_t &ssthresh){};

       virtual ~CongestionControl(){}
protected:
};