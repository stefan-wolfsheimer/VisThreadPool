#include "thread_pool.h"
#include "task.h"
#include <iostream>
#include <chrono>

std::shared_ptr<ThreadPool> ThreadPool::create(std::size_t n)
{
  return std::shared_ptr<ThreadPool>(new ThreadPool(n));
}

std::string ThreadPool::stateToString(State s)
{
  switch(s)
  {
  case State::Waiting: return "Waiting";
  case State::Active: return "Active";
  case State::Terminated: return "Terminated";
  default: return "";
  };
}

ThreadPool::ThreadPool(std::size_t n)
{
  std::lock_guard<mutex_type> main_lock(_mutex);
  _state = State::Waiting;
  _size = n;
  _num_done = 0;
  _num_failed = 0;
  _main_thread_id = std::this_thread::get_id();
}

ThreadPool::~ThreadPool()
{
  _condition.notify_all();
  for(auto & t : _threads)
  {
    t.join();
  }
  _threads.clear();
}

void ThreadPool::activate()
{
  if(_state == State::Waiting)
  {
    if(_main_thread_id == std::this_thread::get_id())
    {
      auto self = shared_from_this();
      {
        std::unique_lock<mutex_type> lock(_mutex);
        _state = State::Active;
      }
      for(std::size_t id = 0; id < _size; id++)
      {
        _threads.push_back(std::thread([self, id]() {
              self->runThread(id);
            }));
      }
    }
    else
    {
      throw std::logic_error("Attempt to activate ThreadPool from thread ");
    }
  }
  else
  {
    throw std::logic_error("Invalid Task transition " +
                           ThreadPool::stateToString(_state) +
                           " -> " +
                           ThreadPool::stateToString(State::Active));
  }
}

void ThreadPool::terminate()
{
  {
    std::unique_lock<mutex_type> lock(_mutex);
    if(_state != State::Active)
    {
      throw std::logic_error("Invalid Task transition " +
                             ThreadPool::stateToString(_state) +
                             " -> " +
                             ThreadPool::stateToString(State::Terminated));
    }
    if(_main_thread_id != std::this_thread::get_id())
    {
      throw std::logic_error("Attempt to terminate ThreadPool from thread ");
    }
    else
    {
      _state = State::Terminated;
      lock.unlock();
      _condition.notify_all();
      for(auto & t : _threads)
      {
        t.join();
      }
      _threads.clear();
    }
  }
}

void ThreadPool::addTask(std::shared_ptr<Task> task)
{
  {
    std::unique_lock<mutex_type> lock(_mutex);
    if(_state == State::Terminated)
    {
      throw std::logic_error("ThreadPool already terminated");
    }
    task->setState(Task::State::Ready);
    _task_queue.push(task);
    {
      auto self = shared_from_this();
      lock.unlock();
      task->handleStateChange(self);
      lock.lock();
    }
    _condition.notify_all();
  }
}

std::size_t ThreadPool::size() const
{
  return _size;
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

ThreadPool::State ThreadPool::getState() const
{
  return _state;
}

void ThreadPool::runThread(std::size_t id)
{
  std::unique_lock<mutex_type> lock(_mutex);
  do 
  {
    if(_state != State::Terminated && _task_queue.empty())
    {
      _condition.wait(lock);
    }
    if(!_task_queue.empty())
    {
      auto task = _task_queue.front();
      _task_queue.pop();
      task->_thread_id = id;
      task->setState(Task::State::Running);
      {
        auto self = shared_from_this();
        lock.unlock();
        task->handleStateChange(self);
      }
      bool ret = task->run();
      lock.lock();
      if(ret)
      {
        _num_done++;
        task->setState(Task::State::Done);
        {
          auto self = shared_from_this();
          lock.unlock();
          task->handleStateChange(self);
          lock.lock();
          task->_promise.set_value();
        }
      }
      else
      {
        _num_failed++;
        task->setState(Task::State::Failed);
        {
          auto self = shared_from_this();
          lock.unlock();
          task->handleStateChange(self);
          lock.lock();
          task->_promise.set_value();
        }
      }
    }
  } while (_state != State::Terminated ||
           !_task_queue.empty());
}
