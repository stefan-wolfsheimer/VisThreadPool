#include "task.h"
#include <stdexcept>
#include <unordered_map>

const std::size_t Task::undefinedThreadId = std::size_t(-1);
const std::size_t Task::undefinedTaskId = std::size_t(-1);

std::shared_ptr<Task> Task::create(std::function<void()> func)
{
  return std::shared_ptr<Task>( new Task([func](){
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

Task::Task(std::function<bool()> func)
  : _function(func),
    _threadId(Task::undefinedThreadId),
    _taskId(Task::undefinedTaskId),
    _state(State::Waiting),
    _future(_promise.get_future())
{
}

Task::~Task()
{
}

std::size_t Task::getThreadId() const
{
  return _threadId;
}

std::size_t Task::getTaskId() const
{
  return _taskId;
}

Task::State Task::getState() const 
{
  return _state;
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
  _future.wait();
}

bool Task::run()
{
  bool ret = true;
  try
  {
    ret = _function();
  }
  catch(...)
  {
    ret = false;
  }
  return ret;
}

void Task::handleStateChange(std::shared_ptr<ThreadPool> pool)
{
  auto itr = stateChanges.find((unsigned int)_state);
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
      func(_state, self, pool);
    }
  }
}

void Task::setState(Task::State s)
{
  switch(_state)
  {
  case State::Waiting:
    if(s == State::Ready)
    {
      _state = State::Ready;
      return;
    }
    else if(s == State::Canceled)
    {
      _state = State::Canceled;
      return;
    }
    break;
  case State::Ready:
    if(s == State::Running)
    {
      _state = State::Running;
      return;
    }
    else if(s == State::Canceled)
    {
      _state = State::Canceled;
      return;
    }
    break;
  case State::Running:
    if(s == State::Done) 
    {
      _state = State::Done;
      return;
    }
    else if(s == State::Failed)
    {
      _state = State::Failed;
      return;
    }
    else if(s == State::CancelRequested)
    {
      _state = State::CancelRequested;
      return;
    }
    break;
  case State::CancelRequested:
    if(s == State::Canceled)
    {
      _state = State::Canceled;
      return;
    }
    break;
  default:
    break;
  }
  throw std::logic_error("Invalid Task transition " +
                         Task::stateToString(_state) +
                         " -> " +
                         Task::stateToString(s));
}
