/*
 * @Author: wkh
 * @Date: 2021-11-12 16:22:45
 * @LastEditTime: 2021-11-12 16:22:45
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/Trace.hpp
 * 
 */
#pragma once
#include <iostream>

#define TRACE(...) trace("[",__FILE__,":",__func__,":",__LINE__,"] :",__VA_ARGS__);

#if __cplusplus >= 201703L
template<typename T ,typename... Args>
inline void trace(T&& tmp , Args&&... args)
{
    if constexpr(sizeof...(args)  > 0)
    {
        std::cout << tmp << " ";
        trace(std::forward<Args>(args)...);
    }else
    {
        
        std::cout << tmp <<std::endl;
       
    }
}
#elif __cplusplus >= 201103L
inline void trace()
{
    std::cout <<  std::endl;
}

template<typename T,typename... Args>
inline void trace(T &&tmp,Args&& ...args)
{
    std::cout << tmp << " ";
    trace(std::forward<Args>(args)...);
}

#endif
