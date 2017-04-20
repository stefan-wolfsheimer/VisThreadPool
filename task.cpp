#include "task.h"
#include <stdexcept>
#include <unordered_map>

const std::size_t Task::undefinedThreadId = std::size_t(-1);

Task::Task(std::function<void()> func)
  : _function([func](){
      try
      {
        func();
      }
      catch(...)
      {
        return false;
      }
      return true;
    }),
    _thread_id(Task::undefinedThreadId),
    _state(State::Waiting)
{
}

Task::Task(std::function<bool()> func)
  : _function(func),
    _thread_id(Task::undefinedThreadId),
    _state(State::Waiting)
{
}

Task::~Task()
{
}

std::size_t Task::getThreadId() const
{
  return _thread_id;
}

Task::State Task::getState() const 
{
  return _state;
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
