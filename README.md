# ThreadPool

VisThreadPool is an C++11 implementation of the [thread pool design pattern](https://en.wikipedia.org/wiki/Thread_pool).

## Features
- The **Task** class encapsulate a lambda function to be executed
- Tasks are added to an **ThreadPool**. The tasks are stored in a task queue and distributed on a fixed number of threads.
- State changes such as starting of finishing a tasks can be tracked by registering call back lambda functions ([Observer pattern](https://en.wikipedia.org/wiki/Observer_pattern))
- An example webserver application is included:
A [complex calculation](https://en.wikipedia.org/wiki/Eight_queens_puzzle) is scheduled asynchronously and distributed.

![class diagram](https://github.com/stefan-wolfsheimer/VisThreadPool/raw/master/doc/classdiagram.png)
![sequence diagram](https://github.com/stefan-wolfsheimer/VisThreadPool/raw/master/doc/sequenceDiagram.png)

## Using the ThreadPool and Task classes

```C++
// Create a pool with 2 threads
auto pool = ThreadPool::create(2);

// Create 3 tasks using lambda-functions
auto task1 = Task::create([](std::shared_ptr<Task> t){
     /* do stuff */
     t->setMessage("task1");
});
// auto task2 = ...
// auto task3 = ...
// ...

// Activate the pool and add the tasks
pool->activate();
pool->addTask(task1);
// pool->addTask(task2);
// pool->addTask(task3);
// ....

// wait for all tasks and end the ThreadPool
pool->terminate();
```

# Usage

## getting the code 
```
git clone https://github.com/stefan-wolfsheimer/VisThreadPool.git
cd VisThreadPool
git submodule update --init --recursive
```

## compile (using make and gcc)
```
make
```

## run tests
```
./run_tests
```

## start the webserver
```
./server
```
# References

- [https://en.wikipedia.org/wiki/Eight_queens_puzzle](https://en.wikipedia.org/wiki/Eight_queens_puzzle)
- [https://en.wikipedia.org/wiki/Thread_pool](https://en.wikipedia.org/wiki/Thread_pool)
- [https://github.com/catchorg/Catch2](https://github.com/catchorg/Catch2)
- [https://github.com/cesanta/mongoose](https://github.com/cesanta/mongoose)



