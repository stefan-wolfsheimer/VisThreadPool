#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <map>

class ThreadPool;
class Task;
struct mg_connection;

class HttpServer
{
public:
  static std::shared_ptr<HttpServer> instance();
  void run();
  void setThreadPool(std::shared_ptr<ThreadPool> _pool);
  std::pair<std::string, std::string> handleRequest(const std::string & uri);
  void handleWebsocketFrame(const std::string & data);
  std::string getTasksJson() const;
private:
  HttpServer();
  std::string index;
  std::shared_ptr<ThreadPool> pool;
  struct mg_connection * nc;
};
