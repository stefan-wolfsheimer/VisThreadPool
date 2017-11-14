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
  std::lock_guard<mutex_type> main_lock(mutex);
  state = State::Waiting;
  threadPoolSize = n;
  numDone = 0;
  numFailed = 0;
  taskCounter = 0;
  mainThreadId = std::this_thread::get_id();
  tasksInThreads.resize(n);
}

ThreadPool::~ThreadPool()
{
  condition.notify_all();
  for(auto & t : threads)
  {
    t.join();
  }
  threads.clear();
}

void ThreadPool::activate()
{
  if(state == State::Waiting)
  {
    if(mainThreadId == std::this_thread::get_id())
    {
      auto self = shared_from_this();
      {
        std::unique_lock<mutex_type> lock(mutex);
        state = State::Active;
	handleStateChange();
      }
      for(std::size_t id = 0; id < threadPoolSize; id++)
      {
        threads.push_back(std::thread([self, id]() {
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
                           ThreadPool::stateToString(state) +
                           " -> " +
                           ThreadPool::stateToString(State::Active));
  }
}

void ThreadPool::terminate()
{
  {
    std::unique_lock<mutex_type> lock(mutex);
    if(state != State::Active)
    {
      throw std::logic_error("Invalid Task transition " +
                             ThreadPool::stateToString(state) +
                             " -> " +
                             ThreadPool::stateToString(State::Terminated));
    }
    if(mainThreadId != std::this_thread::get_id())
    {
      throw std::logic_error("Attempt to terminate ThreadPool from thread ");
    }
    else
    {
      state = State::Terminated;
      handleStateChange();
      lock.unlock();
      condition.notify_all();
      for(auto & t : threads)
      {
        t.join();
      }
      threads.clear();
    }
  }
}

void ThreadPool::addTask(std::shared_ptr<Task> task)
{
  {
    std::unique_lock<mutex_type> lock(mutex);
    if(state == State::Terminated)
    {
      throw std::logic_error("ThreadPool already terminated");
    }
    task->taskId = taskCounter++;
    task->setState(Task::State::Ready);
    taskQueue.push_back(task);
    {
      auto self = shared_from_this();
      lock.unlock();
      task->handleStateChange(self);
      lock.lock();
    }
    condition.notify_all();
  }
}

std::size_t ThreadPool::size() const
{
  return threadPoolSize;
}

std::size_t ThreadPool::numTasks(Task::State s) const
{
  std::lock_guard<mutex_type> lock(mutex);
  switch(s)
  {
  case Task::State::Ready: return taskQueue.size();
  case Task::State::Done: return numDone;
  case Task::State::Failed: return numFailed;
  default:
    return 0u;
  }
}

ThreadPool::State ThreadPool::getState() const
{
  return state;
}

std::pair<std::vector<std::shared_ptr<Task> >,
	  std::vector<std::shared_ptr<Task> > > ThreadPool::getTasks() const
{
  std::unique_lock<mutex_type> lock(mutex);
  std::vector<std::shared_ptr<Task> > q(taskQueue.begin(),
					taskQueue.end());
  return std::make_pair(q, tasksInThreads);
}


void ThreadPool::runThread(std::size_t id)
{
  std::unique_lock<mutex_type> lock(mutex);
  do 
  {
    if(state != State::Terminated && taskQueue.empty())
    {
      condition.wait(lock);
    }
    if(!taskQueue.empty())
    {
      auto task = taskQueue.front();
      taskQueue.pop_front();
      task->threadId = id;
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
        numDone++;
        task->setState(Task::State::Done);
        {
          auto self = shared_from_this();
          lock.unlock();
          task->handleStateChange(self);
          lock.lock();
          task->promise.set_value();
        }
      }
      else
      {
        numFailed++;
        task->setState(Task::State::Failed);
        {
          auto self = shared_from_this();
          lock.unlock();
          task->handleStateChange(self);
          lock.lock();
          task->promise.set_value();
        }
      }
    }
  } while (state != State::Terminated ||
           !taskQueue.empty());
}

void ThreadPool::onStateChange(State s,
			       std::function<void(std::shared_ptr<ThreadPool>)> func)
{
  unsigned int ints = (unsigned int)s;
  stateChanges[ints].push_back(func);
}

void ThreadPool::handleStateChange()
{
  auto itr = stateChanges.find((unsigned int)state);
  if(itr != stateChanges.end())
  {
    auto self = shared_from_this();
    for(auto func : itr->second)
    {
      func(self);
    }
  }
}
