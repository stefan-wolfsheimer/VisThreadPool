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

protected:
  void runThread(std::size_t id);
private:
  ThreadPool(std::size_t n);

  typedef std::mutex mutex_type;
  mutable mutex_type _mutex;
  std::list<std::thread> _threads;
  std::queue<std::shared_ptr<Task> > _task_queue;
  std::condition_variable _condition;
  State _state;
  std::size_t _size;
  std::size_t _num_done;
  std::size_t _num_failed;
};
