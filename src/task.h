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
  static const std::size_t undefinedTaskId;
  virtual ~Task();

  static std::shared_ptr<Task> create(std::function<void()> func);
  static std::shared_ptr<Task> create(std::function<bool()> func);
  static std::shared_ptr<Task> create(std::function<void(std::shared_ptr<Task>)> func);
  static std::shared_ptr<Task> create(std::function<bool(std::shared_ptr<Task>)> func);
  static std::string stateToString(State s);

  std::size_t getThreadId() const;
  std::size_t getTaskId() const;
  State getState() const;
  void onStateChange(State s,
                     std::function<void(std::shared_ptr<Task>,
                                        std::shared_ptr<ThreadPool>)> func);
  void onStateChange(std::function<void(State s,
					std::shared_ptr<Task>,
                                        std::shared_ptr<ThreadPool>)> func);
  void wait();
protected:
  void setState(State s);
  void handleStateChange(std::shared_ptr<ThreadPool> pool);
  Task(std::function<bool(std::shared_ptr<Task>)> func);
private:
  typedef std::shared_ptr<Task> shared_self_type;
  typedef std::shared_ptr<ThreadPool> shared_pool_type;
  typedef std::function<void(shared_self_type,
			     shared_pool_type)> state_change_func_type;
  typedef std::function<void(State,
			     shared_self_type,
			     shared_pool_type)> gen_state_change_func_type;
  typedef std::list<state_change_func_type> state_change_func_list_type;

  bool run();
  std::function<bool(shared_self_type)> function;
  std::unordered_map<unsigned int, state_change_func_list_type> stateChanges;
  std::list<gen_state_change_func_type> genStateChanges;
  std::size_t threadId;
  std::size_t taskId;
  State state;
  std::promise<void> promise;
  std::future<void> future;
};
