#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <list>
#include <queue>
#include <chrono>

class ThreadPool
{
public:
  ThreadPool(std::size_t n);
  ~ThreadPool();
  void addTask(int id);
protected:
  void runThread(std::size_t id);
private:
  std::list<std::thread> _threads;
  std::queue<int> _produced_nums;
  std::mutex _mutex;
  std::condition_variable _condition;
  bool _done;
};

ThreadPool::ThreadPool(std::size_t n)
{
  std::lock_guard<std::mutex> main_lock(_mutex);
  _done = false;
  for(std::size_t id = 0; id < n; id++) 
  {
    _threads.push_back(std::thread([this,id]() {
       this->runThread(id);
    }));
  }
  std::cout << "threads created" << std::endl;
}

ThreadPool::~ThreadPool()
{
  {
    std::lock_guard<std::mutex> lock(_mutex);  
    _done = true;
    _condition.notify_all();
  }
  for(auto & t : _threads) 
  {
    t.join();
  }
  std::cout << "threads done" << std::endl;
}

void ThreadPool::runThread(std::size_t id)
{
  std::unique_lock<std::mutex> lock(_mutex);
  do 
  {
    if(!_done)
    {
      _condition.wait(lock);
    }
    if(!_produced_nums.empty())
    {
      std::cout << "consuming " << _produced_nums.front() 
                << " in thread " << id 
                << " done:" << _done << std::endl;
      _produced_nums.pop();
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::seconds(15 + id));
      lock.lock();
    }
  } while (!_done || !_produced_nums.empty());
}

void ThreadPool::addTask(int i)
{
  {
    std::lock_guard<std::mutex> lock(_mutex);
    std::cout << "producing " << i << '\n';
    _produced_nums.push(i);
    _condition.notify_all();
  }
}

int main()
{
  ThreadPool pool(10);
  for (int i = 0; i < 100; ++i) 
  {
    pool.addTask(i);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
