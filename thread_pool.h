#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>
#include <list>
#include <queue>
#include <memory>
#include "task.h"

class ThreadPool : public std::enable_shared_from_this<ThreadPool>
{
public:
  enum class State : unsigned int
  {
    Waiting            = 1,  // Waiting -> Active
    Active             = 2,  // Active -> Terminated
    Terminated         = 4
  };

  ~ThreadPool();

  static std::shared_ptr<ThreadPool> create(std::size_t n);
  static std::string stateToString(State s);

  void activate();
  void terminate();
  void addTask(std::shared_ptr<Task> task);

  std::size_t size() const;
  std::size_t numTasks(Task::State s) const;
  State getState() const;

  void onStateChange(State s,
                     std::function<void(std::shared_ptr<ThreadPool>)> func);

protected:
  void runThread(std::size_t id);

private:
  ThreadPool(std::size_t n);
  void handleStateChange();

  typedef std::mutex mutex_type;
  typedef std::shared_ptr<ThreadPool> shared_self_type;
  typedef std::function<void(shared_self_type)> state_change_func_type;
  typedef std::list<state_change_func_type> state_change_func_list_type;

  mutable mutex_type _mutex;
  std::thread::id _main_thread_id;
  std::list<std::thread> _threads;
  std::queue<std::shared_ptr<Task> > _task_queue;
  std::condition_variable _condition;
  State _state;
  std::size_t _size;
  std::size_t _num_done;
  std::size_t _num_failed;
  std::unordered_map<unsigned int,
		     state_change_func_list_type> stateChanges;
};
