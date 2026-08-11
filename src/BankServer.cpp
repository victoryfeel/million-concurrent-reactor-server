#include "BankServer.h"

#include "Timestamp.h"

// 解析xml格式字符串的函数。
bool getxmlbuffer(
  const std::string& xmlbuffer, const std::string& fieldname, std::string& value, const int ilen
) {
  std::string start_tag = "<" + fieldname + ">";
  std::string end_tag = "</" + fieldname + ">";

  size_t start_pos = xmlbuffer.find(start_tag);
  if (start_pos == std::string::npos)
    return false;
  start_pos += start_tag.length();

  size_t end_pos = xmlbuffer.find(end_tag, start_pos);
  if (end_pos == std::string::npos)
    return false;

  value = xmlbuffer.substr(start_pos, end_pos - start_pos);

  if (ilen > 0 && value.length() > (size_t)ilen) {
    value = value.substr(0, ilen);
  }
  return true;
}

BankServer::BankServer(
  const std::string& ip, const uint16_t port, int subthreadnum, int workthreadnum
)
    : tcpserver_(ip, port, subthreadnum), threadpool_(workthreadnum, "WORKS") {
  // 注册回调函数
  tcpserver_.setnewconnectioncb(
    std::bind(&BankServer::HandleNewConnection, this, std::placeholders::_1)
  );
  tcpserver_.setcloseconnectioncb(std::bind(&BankServer::HandleClose, this, std::placeholders::_1));
  tcpserver_.seterrorconnectioncb(std::bind(&BankServer::HandleError, this, std::placeholders::_1));
  tcpserver_.setonmessagecb(
    std::bind(&BankServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2)
  );
  tcpserver_.setremoveconnectioncb(
    std::bind(&BankServer::HandleRemove, this, std::placeholders::_1)
  );
}

BankServer::~BankServer() {}

void BankServer::Start() {
  tcpserver_.start();
}

void BankServer::Stop() {
  // 停止工作线程。
  threadpool_.stop();
  printf("工作线程已停止。\n");

  // 停止IO线程（事件循环）。
  tcpserver_.stop();
}

void BankServer::HandleNewConnection(spConnection conn) {
  printf(
    "%s new connection(fd=%d,ip=%s,port=%d) ok.\n",
    Timestamp::now().tostring().c_str(),
    conn->fd(),
    conn->ip().c_str(),
    conn->port()
  );
  // 创建一个UserInfo对象，存入状态机 map 中
  usermap_[conn->fd()] = std::make_shared<UserInfo>(conn->fd(), conn->ip());
}

void BankServer::HandleClose(spConnection conn) {
  printf(
    "%s connection closed(fd=%d,ip=%s,port=%d) ok.\n",
    Timestamp::now().tostring().c_str(),
    conn->fd(),
    conn->ip().c_str(),
    conn->port()
  );
  // 移除对应的UserInfo
  usermap_.erase(conn->fd());
}

void BankServer::HandleError(spConnection conn) {
  printf("%s connection error(fd=%d).\n", Timestamp::now().tostring().c_str(), conn->fd());
  usermap_.erase(conn->fd());
}

void BankServer::HandleMessage(spConnection conn, std::string& message) {
  if (threadpool_.size() == 0) {
    // 如果没有工作线程，表示在IO线程中计算。
    OnMessage(conn, message);
  } else {
    // 把业务添加到线程池的任务队列中，交给工作线程去处理业务。
    threadpool_.addtask(std::bind(&BankServer::OnMessage, this, conn, message));
  }
}

void BankServer::OnMessage(spConnection conn, std::string& message) {
  // 这里演示一个基础的网上银行报文处理逻辑：
  // 例如心跳报文：<bizcode>0000</bizcode>
  // 例如登录报文：<bizcode>0001</bizcode><username>admin</username><password>123456</password>
  std::string bizcode;

  if (getxmlbuffer(message, "bizcode", bizcode)) {
    if (bizcode == "0000") // 心跳业务
    {
      std::string reply =
        "<bizcode>0000</bizcode><retcode>0</retcode><message>heartbeat ok</message>\n";
      conn->send(reply.data(), reply.size());
    } else if (bizcode == "0001") // 登录业务
    {
      std::string username, password;
      getxmlbuffer(message, "username", username);
      getxmlbuffer(message, "password", password);

      if (username == "admin" && password == "123456") {
        // 标记为已登录
        if (usermap_.find(conn->fd()) != usermap_.end()) {
          usermap_[conn->fd()]->setLogin(true);
        }
        std::string reply =
          "<bizcode>0001</bizcode><retcode>0</retcode><message>login success</message>\n";
        conn->send(reply.data(), reply.size());
      } else {
        std::string reply = "<bizcode>0001</bizcode><retcode>-1</retcode><message>login failed: "
                            "wrong user or password</message>\n";
        conn->send(reply.data(), reply.size());
      }
    } else // 其他未知业务
    {
      std::string reply =
        "<bizcode>" + bizcode +
        "</bizcode><retcode>-1</retcode><message>unknown business code</message>\n";
      conn->send(reply.data(), reply.size());
    }
  } else {
    // 无效的xml格式
    std::string reply = "<retcode>-1</retcode><message>invalid xml format</message>\n";
    conn->send(reply.data(), reply.size());
  }
}

void BankServer::HandleRemove(int fd) {
  // TcpServer中超时的Connection对象会被清理，此处需同步清理业务层的状态 map
  usermap_.erase(fd);
}
