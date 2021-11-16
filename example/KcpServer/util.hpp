/*
 * @Author: wkh
 * @Date: 2021-11-16 16:21:51
 * @LastEditTime: 2021-11-16 16:21:51
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/util.hpp
 * 
 */
#pragma once
#include <unistd.h>
#include <sys/time.h>

/* get system time */
static inline void itimeofday(long *sec, long *usec)
{
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
}

/* get clock in millisecond 64 */
static inline int64_t iclock64()
{
	long s, u;
	int64_t value;
	itimeofday(&s, &u);
	value = ((int64_t)s) * 1000 + (u / 1000);
	return value;
}