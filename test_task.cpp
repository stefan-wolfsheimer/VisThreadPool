#include "task.h"
#include "catch.hpp"
#include <unordered_set>
#include <list>
#include <utility>

class TaskFixture : public Task
{
public:
  static std::shared_ptr<TaskFixture> create(Task::State s)
  {
    return std::shared_ptr<TaskFixture>(new TaskFixture(s));
  }

  static std::list<std::pair<Task::State,
                             std::unordered_set<unsigned int> > >
  getTransitions()
  {
    return std::list<std::pair<Task::State,
                               std::unordered_set<unsigned int> > >({
      {Task::State::Waiting,
        {(unsigned int) Task::State::Ready,
         (unsigned int) Task::State::Canceled}},
      {Task::State::Ready,
        {(unsigned int) Task::State::Running,
         (unsigned int) Task::State::Canceled}},
      {Task::State::Running,
       {(unsigned int) Task::State::Done,
        (unsigned int) Task::State::Failed,
        (unsigned int) Task::State::CancelRequested}},
      {Task::State::CancelRequested,
        {(unsigned int) Task::State::Canceled}},
      {Task::State::Canceled, {}},
      {Task::State::Done, {}},
      {Task::State::Failed, {}}
    });
  }


  void setState(State s)
  {
     Task::setState(s);
  }

  void handleStateChange(std::shared_ptr<ThreadPool> pool)
  {
    Task::handleStateChange(pool);
  }

private:
  TaskFixture(Task::State s) 
    : Task([](){ return true; })
  {
    switch(s)
    {
    case Task::State::Waiting:
      break;
    case Task::State::Ready:
      setState(Task::State::Ready);
      break;
    case Task::State::Running:
      setState(Task::State::Ready);
      setState(Task::State::Running);
      break;
    case Task::State::CancelRequested:
      setState(Task::State::Ready);
      setState(Task::State::Running);
      setState(Task::State::CancelRequested);
      break;
    case Task::State::Canceled:
      setState(Task::State::Canceled);
      break;
    case Task::State::Done:
      setState(Task::State::Ready);
      setState(Task::State::Running);
      setState(Task::State::Done);
      break;
    case Task::State::Failed:
      setState(Task::State::Ready);
      setState(Task::State::Running);
      setState(Task::State::Failed);
      break;
    }
  }
};

TEST_CASE("Task_stateToString", "[Task]")
{
  REQUIRE(Task::stateToString(Task::State::Waiting) ==
          "Waiting");
  REQUIRE(Task::stateToString(Task::State::Ready) ==
          "Ready");
  REQUIRE(Task::stateToString(Task::State::Running) ==
          "Running");
  REQUIRE(Task::stateToString(Task::State::CancelRequested) ==
          "CancelRequested");
  REQUIRE(Task::stateToString(Task::State::Canceled) ==
          "Canceled");
  REQUIRE(Task::stateToString(Task::State::Done) ==
          "Done");
  REQUIRE(Task::stateToString(Task::State::Failed) ==
          "Failed");
}

TEST_CASE("Task_transitions", "[Task]")
{
  auto transitions = TaskFixture::getTransitions();
  for(auto & from : transitions)
  {
    for(auto & to : transitions) 
    {
      auto task = TaskFixture::create(from.first);
      if(from.second.find((unsigned int)to.first) == from.second.end())
      {
        REQUIRE_THROWS(task->setState(to.first));
        REQUIRE(task->getState() == from.first);
      }
      else
      {
        task->setState(to.first);
        REQUIRE(task->getState() == to.first);
      }
    }
  }
}

TEST_CASE("Task_onStateChange", "[Task]")
{
  auto task = TaskFixture::create(Task::State::Ready);
  std::vector<int> res;
  std::shared_ptr<ThreadPool> pool;
  task->onStateChange(Task::State::Running,
                      [&res](std::shared_ptr<Task>,
                            std::shared_ptr<ThreadPool>){res.push_back(1);});
  task->onStateChange(Task::State::Running,
                      [&res](std::shared_ptr<Task>,
                            std::shared_ptr<ThreadPool>){res.push_back(2);});
  task->onStateChange(Task::State::Done,
                      [&res](std::shared_ptr<Task>,
                            std::shared_ptr<ThreadPool>){res.push_back(3);});
  task->setState(Task::State::Running);
  REQUIRE(res == std::vector<int>({}));
  task->handleStateChange(pool);
  REQUIRE(res == std::vector<int>({1, 2}));
  task->setState(Task::State::Done);
  task->handleStateChange(pool);
  REQUIRE(res == std::vector<int>({1, 2, 3}));
}
