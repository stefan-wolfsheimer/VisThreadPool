#include "thread_pool.h"
#include "task.h"
#include "catch.hpp"
#include <unordered_map>
#include <mutex>
#include <exception>

TEST_CASE("ThreadPool_empty", "[ThreadPool]")
{
  auto pool = ThreadPool::create(0);
  REQUIRE( pool->size() == 0u);
  auto tasks = pool->getTasks();
  REQUIRE(tasks.first.empty());
  REQUIRE(tasks.second.empty());

  CHECK(pool.use_count() == 1u);
}

TEST_CASE("ThreadPool_size", "[ThreadPool]")
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

TEST_CASE("ThreadPool_adding_tasks_after_terminate_throws", "[ThreadPool]")
{
  auto pool = ThreadPool::create(0);
  pool->activate();
  pool->terminate();
  CHECK_THROWS_AS(pool->activate(), std::logic_error);
  CHECK(pool.use_count() == 1u);
}


TEST_CASE("ThreadPool_run_tasks_in_single_thread", "[ThreadPool]")
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
  auto tasks = pool->getTasks();
  CHECK(tasks.first.empty());
  CHECK(tasks.second.size() == 1);

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

TEST_CASE("ThreadPool_run_tasks_set_messages", "[ThreadPool]")
{
  auto task1 = Task::create([](std::shared_ptr<Task> t){ t->setMessage("task1"); });
  auto task2 = Task::create([](std::shared_ptr<Task> t){ t->setMessage("task2"); });
  auto task3 = Task::create([](std::shared_ptr<Task> t){ t->setMessage("task3"); });
  auto pool = ThreadPool::create(2);
  pool->activate();
  pool->addTask(task1);
  pool->addTask(task2);
  pool->addTask(task3);
  pool->terminate();
  CHECK(task1->getMessage() == "task1");
  CHECK(task2->getMessage() == "task2");
  CHECK(task3->getMessage() == "task3");

  CHECK(pool.use_count() == 1u);
}

TEST_CASE("ThreadPool_run_tasks_in_multi_threads", "[ThreadPool]")
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
    pool->addTask(Task::create([i, &_mutex, &_counter](std::shared_ptr<Task> task) {
          std::lock_guard<std::mutex> lock(_mutex);
          CHECK(task->getState() == Task::State::Running);
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

  CHECK(pool.use_count() == 1u);
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

  CHECK(pool.use_count() == 1u);
}

TEST_CASE( "ThreadPool_add_task_after_terminate_throws", "[ThreadPool]" )
{
  auto pool = ThreadPool::create(1);
  auto task = Task::create([](){});
  pool->activate();
  pool->terminate();
  CHECK_THROWS_AS(pool->addTask(task), std::logic_error);

  CHECK(pool.use_count() == 1u);
}

TEST_CASE( "ThreadPool_terminate_pool_on_done_throws", "[ThreadPool]" )
{
  auto pool = ThreadPool::create(1);
  auto task = Task::create([](){});
  task->onStateChange(Task::State::Done,
                      [](std::shared_ptr<Task> t,
                         std::shared_ptr<ThreadPool> pool)
                      {
                        CHECK_THROWS_AS(pool->terminate(), std::logic_error);
                      });
  pool->activate();
  pool->addTask(task);
  pool->terminate();

  CHECK(pool.use_count() == 1u);
}

TEST_CASE( "ThreadPool_wait_for_task", "[ThreadPool]" )
{
  auto pool = ThreadPool::create(1);
  auto task1 = Task::create([](){});
  auto task2 = Task::create([](){});
  auto task3 = Task::create([](){});
  auto task4 = Task::create([](){});
  pool->addTask(task1);
  pool->addTask(task2);
  pool->addTask(task3);
  pool->addTask(task4);
  pool->activate();
  task1->wait();
  task4->wait();
  task2->wait();
  task3->wait();
  pool->terminate();

  CHECK(pool.use_count() == 1u);
}

TEST_CASE( "ThreadPool_add_task_chain", "[ThreadPool]" )
{
  auto pool = ThreadPool::create(1);
  int c = 100;
  pool->activate();
  std::shared_ptr<Task> last_task = Task::create([](){
    });
  std::shared_ptr<Task> first_task = last_task;
  for(int i = 1; i < c; i++)
  {
    std::shared_ptr<Task> task = Task::create([](){});
    last_task->onStateChange(Task::State::Done,
                             [task](std::shared_ptr<Task> t,
                                    std::shared_ptr<ThreadPool> pool)
                             {
                               pool->addTask(task);
                             });
    last_task = task;
  }
  pool->addTask(first_task);
  last_task->wait();
  pool->terminate();
}

TEST_CASE( "ThreadPool_state_changes_handled", "[ThreadPool]" )
{
  auto pool = ThreadPool::create(1);
  auto task = Task::create([](){});
  bool activated = false;
  bool terminated = false;
  pool->onStateChange(ThreadPool::State::Active,
		      [&activated](std::shared_ptr<ThreadPool> pool){
			CHECK(pool->getState() == ThreadPool::State::Active);
			activated = true;
		      });
  pool->onStateChange(ThreadPool::State::Terminated,
		      [&terminated](std::shared_ptr<ThreadPool> pool){
			CHECK(pool->getState() == ThreadPool::State::Terminated);
			terminated = true;
		      });
  CHECK_FALSE(activated);
  CHECK_FALSE(terminated);
  pool->activate();
  CHECK(activated);
  CHECK_FALSE(terminated);
  pool->terminate();
  CHECK(activated);
  CHECK(terminated);

  CHECK(pool.use_count() == 1u);
}
