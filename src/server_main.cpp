#include "server.h"
#include "thread_pool.h"

int main()
{
  std::size_t n_threads = 4;
  auto pool = ThreadPool::create(n_threads);
  auto server = HttpServer::instance();
  server->setThreadPool(pool);
  pool->activate();
  server->run();
  pool->terminate();
  return 0;
}

