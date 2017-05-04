#include "thread_pool.h"
#include "task.h"
#include "catch.hpp"
#include <unordered_map>
#include <mutex>
#include <exception>

TEST_CASE("ThreadPool_Empty", "[ThreadPool]")
{
  auto pool = ThreadPool::create(0);
  REQUIRE( pool->size() == 0u);
  CHECK(pool.use_count() == 1u);
}

TEST_CASE("ThreadPool_Size", "[ThreadPool]")
{
  auto pool = ThreadPool::create(4);
  REQUIRE( pool->size() == 4u);
  CHECK(pool.use_count() == 1u);
}

TEST_CASE("ThreadPool_terminate_before_activate_throws", "[ThreadPool]")
{
  auto pool = ThreadPool::create(0);
  CHECK_THROWS_AS(pool->terminate(), std::logic_error);
  CHECK(pool.use_count() == 1u);
}

TEST_CASE("ThreadPool_activate_after_terminate_throws", "[ThreadPool]")
{
  auto pool = ThreadPool::create(0);
  pool->activate();
  pool->terminate();
  CHECK_THROWS_AS(pool->activate(), std::logic_error);
  CHECK(pool.use_count() == 1u);
}


TEST_CASE("ThreadPool_RunTasksInSingleThread", "[ThreadPool]")
{
  auto task1 = Task::create([](){});
  auto task2 = Task::create([](){ throw std::exception(); });
  auto task3 = Task::create([](){});
  CHECK(task1->getThreadId() == Task::undefinedThreadId);
  CHECK(task2->getThreadId() == Task::undefinedThreadId);
  CHECK(task3->getThreadId() == Task::undefinedThreadId);
  CHECK(task1->getState() == Task::State::Waiting);
  CHECK(task2->getState() == Task::State::Waiting);
  CHECK(task3->getState() == Task::State::Waiting);
  auto pool = ThreadPool::create(1);
  CHECK(pool->numTasks(Task::State::Ready) == 0);
  pool->activate();
  pool->addTask(task1);
  pool->addTask(task2);
  pool->addTask(task3);
  pool->terminate();
  CHECK(pool->numTasks(Task::State::Ready) == 0u);
  CHECK(pool->numTasks(Task::State::Running) == 0u);
  CHECK(pool->numTasks(Task::State::Done) == 2u);
  CHECK(pool->numTasks(Task::State::Failed) == 1u);
  CHECK(task1->getState() == Task::State::Done);
  CHECK(task2->getState() == Task::State::Failed);
  CHECK(task3->getState() == Task::State::Done);

  CHECK(task1->getThreadId() == 0u);
  CHECK(task2->getThreadId() == 0u);
  CHECK(task3->getThreadId() == 0u);

  CHECK(pool.use_count() == 1u);
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
  auto pool = ThreadPool::create(4);
  pool->activate();
  for(std::size_t i = 0; i < n; i++)
  {
    pool->addTask(Task::create([i, &_mutex, &_counter]() {
          std::lock_guard<std::mutex> lock(_mutex);
          _counter[i]++;
        }));
  }
  pool->terminate();
  CHECK(pool->numTasks(Task::State::Ready) == 0u);
  CHECK(pool->numTasks(Task::State::Running) == 0u);
  CHECK(pool->numTasks(Task::State::Done) == n);
  CHECK(pool->numTasks(Task::State::Failed) == 0u);
  CHECK(pool.use_count() == 1u);
  for(std::size_t i = 0; i < n; i++)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    REQUIRE( _counter.find(i) != _counter.end() );
    REQUIRE(_counter.find(i)->second == 1u );
  }
}

TEST_CASE( "ThreadPool_add_task_multiple_times_throws", "[ThreadPool]" )
{
  auto pool = ThreadPool::create(0);
  auto task = Task::create([](){});
  CHECK(task->getState() == Task::State::Waiting);
  pool->addTask(task);
  CHECK(task->getState() == Task::State::Ready);
  CHECK(pool->numTasks(Task::State::Ready) == 1);
  CHECK_THROWS_AS(pool->addTask(task), std::logic_error);
  CHECK(pool->numTasks(Task::State::Ready) == 1);
}

TEST_CASE( "ThreadPool_add_task_after_terminate_throws", "[ThreadPool]" )
{
  auto pool = ThreadPool::create(1);
  auto task = Task::create([](){});
  pool->activate();
  pool->terminate();
  CHECK_THROWS_AS(pool->addTask(task), std::logic_error);
}

