#include "task.h"
#include <stdexcept>
#include <unordered_map>

const std::size_t Task::undefinedThreadId = std::size_t(-1);
const std::size_t Task::undefinedTaskId = std::size_t(-1);

std::shared_ptr<Task> Task::create(std::function<void()> func)
{
  return std::shared_ptr<Task>( new Task([func](std::shared_ptr<Task> t){
        try
        {
          func();
        }
        catch(...)
        {
          return false;
        }
        return true;
      }));
}

std::shared_ptr<Task> Task::create(std::function<bool()> func)
{
  return std::shared_ptr<Task>( new Task([func](std::shared_ptr<Task> task){ return func(); }));
}

std::shared_ptr<Task> Task::create(std::function<void(std::shared_ptr<Task>)> func)
{
  return std::shared_ptr<Task>( new Task([func](std::shared_ptr<Task> task){
        try
        {
          func(task);
        }
        catch(...)
        {
          return false;
        }
        return true;
      }));
}

std::shared_ptr<Task> Task::create(std::function<bool(std::shared_ptr<Task>)> func)
{
  return std::shared_ptr<Task>( new Task(func) );
}


std::string Task::stateToString(State s)
{
  switch(s)
  {
  case State::Waiting: return "Waiting";
  case State::Ready: return "Ready";
  case State::Running: return "Running";
  case State::CancelRequested: return "CancelRequested";
  case State::Canceled: return "Canceled";
  case State::Done: return "Done";
  case State::Failed: return "Failed";
  default: return "";
  };
}

Task::Task(std::function<bool(std::shared_ptr<Task> task)> func)
  : function(func),
    threadId(Task::undefinedThreadId),
    taskId(Task::undefinedTaskId),
    state(State::Waiting),
    future(promise.get_future())
{
}

Task::~Task()
{
}

std::size_t Task::getThreadId() const
{
  return threadId;
}

std::size_t Task::getTaskId() const
{
  return taskId;
}

Task::State Task::getState() const 
{
  return state;
}

std::string Task::getMessage() const
{
  return message;
}

void Task::setMessage(const std::string & msg)
{
  std::lock_guard<std::mutex> main_lock(taskMutex);
  message = msg;
}

void Task::onStateChange(Task::State s,
                         std::function<void(std::shared_ptr<Task>,
                                            std::shared_ptr<ThreadPool>)> func)
{
  unsigned int ints = (unsigned int)s;
  stateChanges[ints].push_back(func);
}

void Task::onStateChange(std::function<void(State s,
					    std::shared_ptr<Task>,
					    std::shared_ptr<ThreadPool>)> func)
{
  genStateChanges.push_back(func);
}

void Task::wait()
{
  future.wait();
}

bool Task::run()
{
  bool ret = true;
  auto self = shared_from_this();
  try
  {
    ret = function(self);
  }
  catch(...)
  {
    ret = false;
  }
  return ret;
}

void Task::handleStateChange(std::shared_ptr<ThreadPool> pool)
{
  auto itr = stateChanges.find((unsigned int)state);
  if(itr != stateChanges.end())
  {
    auto self = shared_from_this();
    for(auto func : itr->second)
    {
      func(self, pool);
    }
  }
  if(!genStateChanges.empty())
  {
    auto self = shared_from_this();
    for(auto func : genStateChanges)
    {
      func(state, self, pool);
    }
  }
}

void Task::setState(Task::State s)
{
  switch(state)
  {
  case State::Waiting:
    if(s == State::Ready)
    {
      state = State::Ready;
      return;
    }
    else if(s == State::Canceled)
    {
      state = State::Canceled;
      return;
    }
    break;
  case State::Ready:
    if(s == State::Running)
    {
      state = State::Running;
      return;
    }
    else if(s == State::Canceled)
    {
      state = State::Canceled;
      return;
    }
    break;
  case State::Running:
    if(s == State::Done) 
    {
      state = State::Done;
      return;
    }
    else if(s == State::Failed)
    {
      state = State::Failed;
      return;
    }
    else if(s == State::CancelRequested)
    {
      state = State::CancelRequested;
      return;
    }
    break;
  case State::CancelRequested:
    if(s == State::Canceled)
    {
      state = State::Canceled;
      return;
    }
    break;
  default:
    break;
  }
  throw std::logic_error("Invalid Task transition " +
                         Task::stateToString(state) +
                         " -> " +
                         Task::stateToString(s));
}
