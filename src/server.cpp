#include <sstream>
#include <chrono>
#include <exception>

#include "server.h"
#include "thread_pool.h"
#include "n_queens.h"

extern "C" {
#define MG_ENABLE_CALLBACK_USERDATA 1
#include "mongoose/mongoose.h"
}

static sig_atomic_t s_signal_received = 0;
static std::size_t maxSolutions = 10000;
static struct mg_serve_http_opts s_http_server_opts;

static void signal_handler(int sig_num)
{
  signal(sig_num, signal_handler);  // Reinstantiate signal handler
  s_signal_received = sig_num;
}

HttpServer::HttpServer(const std::string & _port) : port(_port)
{
#include "index.inc"
}

static void ev_handler(struct mg_connection *nc,
                       int ev,
                       void * ev_data,
                       void * user_data)
{
  HttpServer * server = (HttpServer*)user_data;
  switch (ev) {
    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      server->handleWebsocketFrame(std::string((char *) wm->data,
                                               (char *) wm->data + wm->size));
      break;
    }
    case MG_EV_HTTP_REQUEST: {
      auto result = server->
        handleRequest(std::string(((struct http_message *) ev_data)->uri.p,
                                  ((struct http_message *) ev_data)->uri.p +
                                  ((struct http_message *) ev_data)->uri.len));

      mg_printf(nc,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n\r\n%s",
		result.first.c_str(),
		(int) result.second.size(),
		result.second.c_str());
      break;
    }
  }
}

void HttpServer::run()
{
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);
  struct mg_mgr mgr;
  mg_mgr_init(&mgr, NULL);

  nc = mg_bind(&mgr, port.c_str(), ev_handler, this);
  if(!nc)
  {
    throw std::runtime_error (std::string("could not bind port ") + port);
  };
  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = ".";  // Serve current directory
  s_http_server_opts.enable_directory_listing = "yes";

  printf("Started on port %s\n", port.c_str());
  while (s_signal_received == 0) {
    mg_mgr_poll(&mgr, 200);
  }
  mg_mgr_free(&mgr);
  nc = nullptr;
}

std::pair<std::string, std::string>
HttpServer::handleRequest(const std::string & uri)
{
  if(uri == "/")
  {

    return std::make_pair(std::string("text/html"),
                          index);
  }
  else if(uri == "/list")
  {
    return std::make_pair(std::string("text/json"),
                          getTasksJson());
  }
  return std::make_pair(std::string("text/plain"),
                        std::string(""));
}

void HttpServer::sendWebsocketFrame(const std::string & msg)
{
  struct mg_connection *c;
  for (c = mg_next(nc->mgr, NULL);
       c != NULL;
       c = mg_next(nc->mgr, c))
  {
    mg_send_websocket_frame(c,
                            WEBSOCKET_OP_TEXT,
                            msg.c_str(),
                            msg.size());
  }
}

void HttpServer::handleWebsocketFrame(const std::string & data)
{
  if(pool)
  {
    std::size_t n;
    
    std::stringstream tmp(data);
    tmp >> n;
    auto task = Task::create([n](std::shared_ptr<Task> task){
        ChessBoard board(n);
        auto sol = board.solveNQueens(n, maxSolutions);
        std::stringstream ss;
        ss << "{";
        ss << "\"numQueens\":" << n;
        ss << ",\"numSolutions\":" << sol.getNumSolutions();
        ss << ",\"fundamentalSolutions\":" << sol.getFundamentalSolutions();
        ss << "}";
        task->setMessage(ss.str());
      });
    task->setMessage("{\"numQueens\":" + std::to_string(n) + "}");
    task->onStateChange([this](Task::State s,
                               std::shared_ptr<Task> task,
                               std::shared_ptr<ThreadPool> pool){
                          std::stringstream ss;
                          ss << "{ \"state\": \""
                             << Task::stateToString(s) << "\"";
                          if(task->getTaskId() != Task::undefinedTaskId)
                          {
                            ss << ",\"taskId\":" << task->getTaskId();
                          }
                          if(task->getThreadId() != Task::undefinedTaskId)
                          {
                            ss << ",\"threadId\":" << task->getThreadId();
                          }
                          ss << ",\"numThreads\":" << pool->size();
                          ss << ",\"result\":" << task->getMessage();
                          ss << "}";
                          sendWebsocketFrame(ss.str());
                        });
    pool->addTask(task);
  }
}

void HttpServer::setThreadPool(std::shared_ptr<ThreadPool> _pool)
{
  pool = _pool;
  pool->onStateChange(ThreadPool::State::Terminated, [](std::shared_ptr<ThreadPool> p){
    });
}

static void streamVectorTasks(std::stringstream & ss,
                              const std::vector<std::shared_ptr<Task> > & tasks)
{
  bool first = true;
  ss << "[";
  for(auto task : tasks)
  {
    if(first) first = false;
    else ss << ",";
    ss << "{";
    if(task)
    {
      ss<< "\"state\":\"" << Task::stateToString(task->getState()) << "\"";
      if(task->getTaskId() !=  Task::undefinedTaskId)
      {
        ss << ",\"taskId\":" << task->getTaskId();
      }
    }
    ss << "}";
  }
  ss << "]";
}

std::string HttpServer::getTasksJson() const
{
  if(pool)
  {
    auto tasks = pool->getTasks();
    std::stringstream ss;
    ss << "{\"queue\":";
    streamVectorTasks(ss, tasks.first);
    ss << ",\"threads\":";
    streamVectorTasks(ss, tasks.second);
    ss << "}";
    return ss.str();
  }
  else
  {
    return std::string("{}");
  }
}

