#include "server.h"
#include "thread_pool.h"

static const char *s_http_port_1 = "8000";

int main()
{
  std::size_t n_threads = 4;
  HttpServer server1(s_http_port_1);
  auto pool = ThreadPool::create(n_threads);
  server1.setThreadPool(pool);
  pool->activate();
  server1.run();
  pool->terminate();
  return 0;
}

