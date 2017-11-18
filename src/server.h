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
  HttpServer(const std::string & _port);
  void run();
  void setThreadPool(std::shared_ptr<ThreadPool> _pool);
  std::pair<std::string, std::string> handleRequest(const std::string & uri);
  void handleWebsocketFrame(const std::string & data);
  void sendWebsocketFrame(const std::string & msg);
  std::string getTasksJson() const;
private:
  std::string index;
  std::shared_ptr<ThreadPool> pool;
  struct mg_connection * nc;
  std::string port;
};
