#include "thread_pool.h"
#include "task.h"
#include <iostream>
#include <chrono>

ThreadPool::ThreadPool(std::size_t n)
{
  std::lock_guard<mutex_type> main_lock(_mutex);
  _done = false;
  _num_done = 0;
  _num_failed = 0;
  for(std::size_t id = 0; id < n; id++) 
  {
    _threads.push_back(std::thread([this,id]() {
       this->runThread(id);
    }));
  }
}

ThreadPool::~ThreadPool()
{
  {
    std::lock_guard<mutex_type> lock(_mutex);
    _done = true;
    _condition.notify_all();
  }
  for(auto & t : _threads) 
  {
    t.join();
  }
  if(_onDestroy) 
  {
    try
    {
      _onDestroy(this);
    }
    catch(...)
    {
    }
  }
}

void ThreadPool::addTask(std::shared_ptr<Task> task)
{
  {
    std::lock_guard<mutex_type> lock(_mutex);
    if(_done)
    {
      throw std::logic_error("ThreadPool already terminated");
    }
    task->setState(Task::State::Ready);
    _task_queue.push(task);
    _condition.notify_all();
  }
}

void ThreadPool::onDestroy(std::function<void(ThreadPool* pool)> cb)
{
  _onDestroy = cb;
}

std::size_t ThreadPool::size() const
{
  return _threads.size();
}

std::size_t ThreadPool::numTasks(Task::State s) const
{
  std::lock_guard<mutex_type> lock(_mutex);
  switch(s)
  {
  case Task::State::Ready: return _task_queue.size();
  case Task::State::Done: return _num_done;
  case Task::State::Failed: return _num_failed;
  default:
    return 0u;
  }
}

void ThreadPool::runThread(std::size_t id)
{
  std::unique_lock<mutex_type> lock(_mutex);
  do 
  {
    if(!_done)
    {
      _condition.wait(lock);
    }
    if(!_task_queue.empty())
    {
      auto task = _task_queue.front();
      _task_queue.pop();
      task->_thread_id = id;
      task->setState(Task::State::Running);
      lock.unlock();
      bool ret = task->run();
      lock.lock();
      if(ret)
      {
        _num_done++;
        task->setState(Task::State::Done);
      }
      else
      {
        _num_failed++;
        task->setState(Task::State::Failed);
      }
    }
  } while (!_done || !_task_queue.empty());
}
