/*
 * @Author: wkh
 * @Date: 2021-11-10 15:28:30
 * @LastEditTime: 2021-11-16 18:18:57
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/include/util.hpp
 * 
 */
#pragma once
#include <functional>
#include <memory>
#include <iostream>
#include <atomic>
#include <mutex>
#include <list>

namespace kcp
{

    class FakeLock
    {
    public:
        void lock() {}
        void unlock() {}
    };

    template <typename T, bool ThreadSafe = false>
    class List
    {
    public:
        using Lock       = typename std::conditional<ThreadSafe, std::mutex, FakeLock>::type;
        using AutoLock   = std::unique_lock<List<T,ThreadSafe>>;
        using Container  = std::list<T>;
        using value_type = T;
        
    public:
        void lock() { return lock_.lock(); }
        void unlock() { return lock_.unlock(); }
        Container &get() { return list_; }

        std::size_t size()
        {
            std::unique_lock<Lock> lock(lock_);
            return list_.size();
        }
        bool empty()
        {
            std::unique_lock<Lock> lock(lock_);
            return list_.empty();
        }
        template <typename U>
        void push_back(U &&x)
        {
            std::unique_lock<Lock> lock(lock_);
            list_.push_back(std::forward<U>(x));
        }

        template <typename... Args>
        void emplace_back(Args &&...args)
        {
            std::unique_lock<Lock> lock(lock_);
            list_.emplace_back(std::forward<Args>(args)...);
        }

        template <typename Fn>
        void for_each(Fn &&fn)
        {
            std::unique_lock<Lock> lock(lock_);
            for (auto &it : list_)
                fn(it);
        }

    private:
        Container list_;
        Lock lock_;
    };

    template <typename T>
    using AutoLock = typename T::AutoLock;

    template <typename T>
    class FakeAtomic
    {
    public:
        FakeAtomic(T x) : x_(x) {}

        FakeAtomic() noexcept = default;

        T operator++(int) { return x_++; }

        T operator++() { return ++x_; }

        T operator--(int) { return x_--; }

        T operator--() { return --x_; }

        T load(std::memory_order m = std::memory_order_acq_rel) { return x_; }

        T exchange(T x, std::memory_order m = std::memory_order_acq_rel)
        {
            T tmp = x_;
            x_ = x;
            return tmp;
        }

        bool compare_exchange_weak(T &expected, T desired,
                                   std::memory_order s = std::memory_order_acq_rel) noexcept
        {
            if (expected != x_)
            {
                expected = x_;
                return false;
            }

            x_ = desired;
            return true;
        }

        void store(T x, std::memory_order m = std::memory_order_acq_rel) { x_ = x; }

    private:
        FakeAtomic(const FakeAtomic &) = delete;

        FakeAtomic &operator=(const FakeAtomic &) = delete;

        FakeAtomic &operator=(const FakeAtomic &) volatile = delete;

    private:
        T x_;
    };

    inline uint32_t bound(const uint32_t lower,const uint32_t middle,const uint32_t upper)
    {
            return std::min(std::max(lower,middle),upper);
    }

}
