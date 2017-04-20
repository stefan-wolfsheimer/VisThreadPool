#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>
#include <list>
#include <queue>
#include "task.h"

class ThreadPool
{
public:
  ThreadPool(std::size_t n);
  ~ThreadPool();
  void addTask(std::shared_ptr<Task> task);
  void onDestroy(std::function<void(ThreadPool* pool)> cb);

  std::size_t size() const;
  std::size_t numTasks(Task::State s) const;

protected:
  void runThread(std::size_t id);
private:
  typedef std::mutex mutex_type;
  mutable mutex_type _mutex;
  std::list<std::thread> _threads;
  std::queue<std::shared_ptr<Task> > _task_queue;
  std::condition_variable _condition;
  bool _done;
  std::size_t _num_done;
  std::size_t _num_failed;
  std::function<void(ThreadPool* pool)> _onDestroy;
};
