/*
 * @Author: wkh
 * @Date: 2021-11-12 22:31:14
 * @LastEditTime: 2021-11-12 23:10:42
 * @LastEditors: wkh
 * @Description: 
 * @FilePath: /kcp-cpp/example/ThreadPool.hpp
 * 
 */
#pragma  once
#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>


class ThreadPool
{

public:
       using Task = std::function<void()>; 
       using Lock = std::unique_lock<std::mutex>;
       
       ThreadPool() = default; 
       ~ThreadPool(){ Stop();}

       void Put(const Task &task)
       {
           Lock lock(mtx_);
            
           tasks_.push_back(task);

           cv_.notify_one();
       }

       void Start(uint32_t num)
       {
           run_.store(true,std::memory_order_release);
           CreateThread(num);
       }

       void Stop()
       {
           if(!run_.load(std::memory_order_acquire))
             return;
           
           run_.store(false,std::memory_order_release);

           for(auto &t : threads_)
              t.join();
       }
       
private:

        void CreateThread(uint32_t num)
        {
            for(uint32_t i = 0; i < num; i++)
            {
                 threads_.emplace_back([this](){ run();});
            }
        }

        void run()
        {
             while(true)
             {
                Lock lock(mtx_);

                cv_.wait(lock,[this]()
                {
                    if(!run_.load(std::memory_order_relaxed))
                        return true;
                    
                    return !tasks_.empty(); 
                });

                if(!run_.load(std::memory_order_relaxed))
                     return;

                auto task = std::move(tasks_.front());

                tasks_.pop_front();

                lock.unlock();
                
                task();
             }
        }
public:
      std::list<std::thread>    threads_;
      std::list<Task>           tasks_;
      std::mutex                mtx_;
      std::condition_variable   cv_;
      std::atomic<bool>         run_{false};
};