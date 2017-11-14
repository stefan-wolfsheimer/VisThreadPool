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
  _taskCounter = 0;
  _main_thread_id = std::this_thread::get_id();
  tasksInThreads.resize(n);
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
	handleStateChange();
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
      handleStateChange();
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
    task->_taskId = _taskCounter++;
    task->setState(Task::State::Ready);
    _task_queue.push_back(task);
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

std::pair<std::vector<std::shared_ptr<Task> >,
	  std::vector<std::shared_ptr<Task> > > ThreadPool::getTasks() const
{
  std::unique_lock<mutex_type> lock(_mutex);
  std::vector<std::shared_ptr<Task> > q(_task_queue.begin(),
					_task_queue.end());
  return std::make_pair(q, tasksInThreads);
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
      _task_queue.pop_front();
      task->_threadId = id;
      tasksInThreads[id] = task;
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

void ThreadPool::onStateChange(State s,
			       std::function<void(std::shared_ptr<ThreadPool>)> func)
{
  unsigned int ints = (unsigned int)s;
  stateChanges[ints].push_back(func);
}

void ThreadPool::handleStateChange()
{
  auto itr = stateChanges.find((unsigned int)_state);
  if(itr != stateChanges.end())
  {
    auto self = shared_from_this();
    for(auto func : itr->second)
    {
      func(self);
    }
  }
}
