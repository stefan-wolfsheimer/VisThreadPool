#include <sstream>
#include <chrono>
#include "server.h"
#include "thread_pool.h"

extern "C" {
#include "mongoose/mongoose.h"
}

static sig_atomic_t s_signal_received = 0;
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

static void signal_handler(int sig_num)
{
  signal(sig_num, signal_handler);  // Reinstantiate signal handler
  s_signal_received = sig_num;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
  switch (ev) {
    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      HttpServer::instance()
	->handleWebsocketFrame(std::string((char *) wm->data,
					   (char *) wm->data + wm->size));
      break;
    }
    case MG_EV_HTTP_REQUEST: {
      auto result = HttpServer::instance()
	->handleRequest(std::string(((struct http_message *) ev_data)->uri.p,
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

HttpServer::HttpServer()
{
  #include "index.inc"
}

std::shared_ptr<HttpServer> HttpServer::instance()
{
  static std::shared_ptr<HttpServer> server;
  if(!server)
  {
    server.reset(new HttpServer);
  }
  return server;
}

void HttpServer::run()
{
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);
  struct mg_mgr mgr;
  mg_mgr_init(&mgr, NULL);

  nc = mg_bind(&mgr, s_http_port, ev_handler);
  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = ".";  // Serve current directory
  s_http_server_opts.enable_directory_listing = "yes";

  printf("Started on port %s\n", s_http_port);
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
                        std::string());
}

void HttpServer::handleWebsocketFrame(const std::string & data)
{
  if(pool)
  {
    auto task = Task::create([](){
        std::this_thread::sleep_for(std::chrono::seconds(10));
      });
    task->onStateChange([this](Task::State s,
                               std::shared_ptr<Task> task,
                               std::shared_ptr<ThreadPool> pool){
                          struct mg_connection *c;
                          std::stringstream ss;
                          ss << "{ \"state\": \""
                             << Task::stateToString(s) << "\"";
                          if(task->getTaskId() != Task::undefinedTaskId)
                          {
                            ss << ",taskId=" << task->getTaskId();
                          }
                          if(task->getThreadId() != Task::undefinedTaskId)
                          {
                            ss << ",threadId=" << task->getThreadId();
                          }
                          ss << "}";
                          std::string msg = ss.str();
                          for (c = mg_next(nc->mgr, NULL);
                               c != NULL;
                               c = mg_next(nc->mgr, c))
                          {
                            mg_send_websocket_frame(c,
                                                    WEBSOCKET_OP_TEXT,
                                                    msg.c_str(),
                                                    msg.size());
                          }
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

