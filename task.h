#pragma once
#include <functional>
#include <string>

class Task
{
public:
  friend class ThreadPool;
  enum class State : unsigned int
  {
    Waiting         = 1,  //-> Ready, Canceled
    Ready           = 2,  //-> Canceled, Running
    Running         = 4,  //-> CancelRequested, Done, Failed
    CancelRequested = 8,  //-> Canceled
    Canceled        = 16,
    Done            = 32,
    Failed          = 64 
  };

  static const std::size_t undefinedThreadId;

  Task(std::function<void()> func);
  Task(std::function<bool()> func);
  virtual ~Task();

  std::size_t getThreadId() const;
  State getState() const;

  static std::string stateToString(State s);

protected:
  virtual bool run();
  void setState(State s);

private:
  std::function<bool()> _function;
  std::size_t _thread_id;
  State _state;
};
