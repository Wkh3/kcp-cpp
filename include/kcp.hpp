/*
 * @Author: wkh
 * @Date: 2021-11-01 16:31:14
 * @LastEditTime: 2021-11-01 19:41:46
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/src/kcp.hpp
 */
#pragma once
#include <iostream>
#include <mutex>
#include <lock.hpp>
class KcpHdr
{
public:
    //会话编号
    uint16_t conv{0};
    //数据长度
    uint16_t len{0};
    //操作码
    uint8_t cmd;
    //分片id
    uint8_t frg{0};
    //窗口
    uint16_t wnd{0};
    //时间戳
    uint32_t ts{0};
    //序列号
    uint32_t sn{0};
    //待接收包序列号
    uint32_t una{0};
    //数据
    char buf[0];
};


template<bool ThreadSafe = false>
class Kcp
{
public:
       using Lock     = std::conditional<ThreadSafe,std::mutex,FackLock>;
       using AutoLock = std::unique_lock<Lock>; 
private:
       
};
