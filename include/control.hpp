/*
 * @Author: wkh
 * @Date: 2021-11-01 19:28:02
 * @LastEditTime: 2021-11-06 15:18:31
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/control.hpp
 * 
 */

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
              virtual void SlowStart(uint32_t &cwnd, uint32_t &ssthresh){};
              /**
               * @brief: fast resend algorithm 
               * @param[in,out] cwnd 
               * @param[in,out] ssthresh
               */
              virtual void FastResend(uint32_t &cwnd, uint32_t &ssthresh){};
               /**
               * @brief: congesion avoidance algorithm 
               * @param[in,out] cwnd 
               * @param[in,out] ssthresh
               */
              virtual void CongestionAvoidance(uint32_t &cwnd, uint32_t &ssthresh){};
              /**
               * @brief: quick recover algorithm 
               * @param[in,out] cwnd 
               * @param[in,out] ssthresh
               */
              virtual void QuickRecover(uint32_t &cwnd, uint32_t &ssthresh){};

              virtual ~CongestionControl() {}
       };
}