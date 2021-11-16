/*
 * @Author: wkh
 * @Date: 2021-11-01 19:28:02
 * @LastEditTime: 2021-11-16 18:56:18
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/Control.hpp
 * 
 */
#pragma once
#include <memory>

namespace kcp
{
       class CongestionControl
       {
       public:
              using ptr = std::unique_ptr<CongestionControl>;
              /**
               * @brief: slow start algorithm 
               * @param[in,out] cwnd 
               * @param[in,out] ssthresh
               */
              virtual void SlowStart(uint32_t &cwnd, uint32_t &ssthresh)
              {
                   cwnd *= 2;
              }
              /**
               * @brief: algorithm when packet loss occurs
               * @param[in,out] cwnd 
               * @param[in,out] ssthresh
               */
              virtual void Lost(uint32_t &cwnd, uint32_t &ssthresh)
              {
                   cwnd = 1;
                   ssthresh = std::max(ssthresh / 2,1u) ;
              }
               /**
               * @brief: congesion avoidance algorithm 
               * @param[in,out] cwnd 
               * @param[in,out] ssthresh
               */
              virtual void CongestionAvoidance(uint32_t &cwnd, uint32_t &ssthresh)
              {
                  cwnd++;
              }
              /**
               * @brief: quick recover algorithm 
               * @param[in,out] cwnd 
               * @param[in,out] ssthresh
               * @param[in]     fast_resend_trigger
               */
              virtual void QuickRecover(uint32_t &cwnd, uint32_t &ssthresh,uint32_t fast_resend_trigger)
              {
                  ssthresh  = std::max(ssthresh / 2,1u) ;
                  cwnd      = ssthresh + fast_resend_trigger;
              }

              virtual void SetRtt(uint32_t rtt){}

              virtual void SetMss(uint32_t mss){}

              virtual ~CongestionControl() {}
       };
}