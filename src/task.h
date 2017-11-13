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
  void onStateChange(State s,
                     std::function<void(std::shared_ptr<Task>,
                                        std::shared_ptr<ThreadPool>)> func);
  void wait();
protected:
  void setState(State s);
  void handleStateChange(std::shared_ptr<ThreadPool> pool);
  Task(std::function<bool()> func);
private:
  typedef std::shared_ptr<Task> shared_self_type;
  typedef std::shared_ptr<ThreadPool> shared_pool_type;
  typedef std::function<void(shared_self_type,
			     shared_pool_type)> state_change_func_type;
  typedef std::list<state_change_func_type> state_change_func_list_type;

  bool run();
  std::function<bool()> _function;
  std::unordered_map<unsigned int, state_change_func_list_type> stateChanges;
  std::size_t _thread_id;
  State _state;
  std::promise<void> _promise;
  std::future<void> _future;
};