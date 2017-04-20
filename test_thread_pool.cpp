#include "thread_pool.h"
#include "task.h"
#include "catch.hpp"
#include <unordered_map>
#include <mutex>
#include <exception>

TEST_CASE("ThreadPool_Empty", "[ThreadPool]")
{
  ThreadPool pool(0);
  REQUIRE( pool.size() == 0u);
}

TEST_CASE("ThreadPool_Size", "[ThreadPool]")
{
  ThreadPool pool(4);
  REQUIRE( pool.size() == 4u);
}

TEST_CASE("ThreadPool_RunTasksInSingleThread", "[ThreadPool]")
{
  auto task1 = std::make_shared<Task>([](){});
  auto task2 = std::make_shared<Task>([](){ throw std::exception(); });
  auto task3 = std::make_shared<Task>([](){});
  CHECK(task1->getThreadId() == Task::undefinedThreadId);
  CHECK(task2->getThreadId() == Task::undefinedThreadId);
  CHECK(task3->getThreadId() == Task::undefinedThreadId);
  CHECK(task1->getState() == Task::State::Waiting);
  CHECK(task2->getState() == Task::State::Waiting);
  CHECK(task3->getState() == Task::State::Waiting);
  {
    ThreadPool pool(1);
    CHECK(pool.numTasks(Task::State::Ready) == 0);
    pool.onDestroy([task1, task2, task3](ThreadPool * pool)
                   {
                     CHECK(pool->numTasks(Task::State::Ready) == 0u);
                     CHECK(pool->numTasks(Task::State::Running) == 0u);
                     CHECK(pool->numTasks(Task::State::Done) == 2u);
                     CHECK(pool->numTasks(Task::State::Failed) == 1u);
                     CHECK(task1->getState() == Task::State::Done);
                     CHECK(task2->getState() == Task::State::Failed);
                     CHECK(task3->getState() == Task::State::Done);
                   });
    pool.addTask(task1);
    pool.addTask(task2);
    pool.addTask(task3);
  }
  CHECK(task1->getState() == Task::State::Done);
  CHECK(task2->getState() == Task::State::Failed);
  CHECK(task3->getState() == Task::State::Done);

  CHECK(task1->getThreadId() == 0u);
  CHECK(task2->getThreadId() == 0u);
  CHECK(task3->getThreadId() == 0u);
}

TEST_CASE("ThreadPool_RunTasks", "[ThreadPool]")
{
  std::unordered_map<std::size_t, std::size_t> _counter;
  std::mutex _mutex;
  std::size_t n = 417;
  for(std::size_t i = 0; i < n; i++)
  {
    _counter[i] = 0;
  }
  {
    ThreadPool pool(4);
    pool.onDestroy([n](ThreadPool * p)
                   {
                     CHECK(p->numTasks(Task::State::Ready) == 0u);
                     CHECK(p->numTasks(Task::State::Running) == 0u);
                     CHECK(p->numTasks(Task::State::Done) == n);
                     CHECK(p->numTasks(Task::State::Failed) == 0u);
                   });

    for(std::size_t i = 0; i < n; i++)
    {
      pool.addTask(std::make_shared<Task>([i, &_mutex, &_counter]() {
            std::lock_guard<std::mutex> lock(_mutex);
            _counter[i]++;
      }));
    }
  }
  for(std::size_t i = 0; i < n; i++)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    REQUIRE( _counter.find(i) != _counter.end() );
    REQUIRE(_counter.find(i)->second == 1u );
  }
}

TEST_CASE( "ThreadPool_add_task_multiple_times_throws", "[ThreadPool]" )
{
  ThreadPool pool(0);
  auto task = std::make_shared<Task>([](){});
  CHECK(task->getState() == Task::State::Waiting);
  pool.addTask(task);
  CHECK(task->getState() == Task::State::Ready);
  CHECK(pool.numTasks(Task::State::Ready) == 1);
  CHECK_THROWS_AS(pool.addTask(task), std::logic_error);
  CHECK(pool.numTasks(Task::State::Ready) == 1);
}

TEST_CASE( "ThreadPool_add_task_before_destroy_throws", "[ThreadPool]" )
{
  ThreadPool pool(1);
  auto task = std::make_shared<Task>([](){});
  pool.onDestroy([task](ThreadPool * p)
                 {
                   CHECK_THROWS_AS(p->addTask(task), std::logic_error);
                 });
}

