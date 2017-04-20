#include "task.h"
#include "catch.hpp"
#include <unordered_set>
#include <list>
#include <utility>

class TaskFixture : public Task
{
public:
  TaskFixture(Task::State s) 
    : Task([](){})
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

  void setState(State s)
  {
    Task::setState(s);
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
  std::list<std::pair<Task::State, std::unordered_set<unsigned int> > >
    transitions({
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
  for(auto & from : transitions)
  {
    for(auto & to : transitions) 
    {
      TaskFixture task(from.first);
      if(from.second.find((unsigned int)to.first) == from.second.end())
      {
        REQUIRE_THROWS(task.setState(to.first));
        REQUIRE(task.getState() == from.first);
      }
      else
      {
        task.setState(to.first);
        REQUIRE(task.getState() == to.first);
      }
    }
  }
}

