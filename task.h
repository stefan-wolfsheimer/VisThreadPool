#pragma once
#include <unordered_map>
#include <list>

#include <functional>
#include <string>
#include <memory>

#include <future>

class ThreadPool;

class Task : public std::enable_shared_from_this<Task>
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
  virtual ~Task();

  static std::shared_ptr<Task> create(std::function<void()> func);
  static std::shared_ptr<Task> create(std::function<bool()> func);
  static std::string stateToString(State s);

  std::size_t getThreadId() const;
  State getState() const;



  // Todo return event
  void onStateChange(State s,
                     std::function<void(std::shared_ptr<Task>,
                                        std::shared_ptr<ThreadPool>)> func);
  void wait();
protected:
  void setState(State s);
  void handleStateChange(std::shared_ptr<ThreadPool> pool);
  Task(std::function<bool()> func);
private:
  typedef
  std::function<void(std::shared_ptr<Task>,
                     std::shared_ptr<ThreadPool>)> stateChangeFunctionType;
  typedef
  std::list<stateChangeFunctionType> stateChangeFunctionListType;

  bool run();
  std::function<bool()> _function;
  std::unordered_map<unsigned int, stateChangeFunctionListType> stateChanges;
  std::size_t _thread_id;
  State _state;
  std::promise<void> _promise;
  std::future<void> _future;
};
