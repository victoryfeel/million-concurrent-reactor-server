#pragma once
#include "Connection.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "ThreadPool.h"

/*
    BankServer类：网上银行服务器
*/

///////////////////////////////////// /////////////////////////////////////
// 解析xml格式字符串的函数族。
// xml格式的字符串的内容如下：
// <filename>/tmp/_public.h</filename><mtime>2020-01-01 12:20:35</mtime><size>18348</size>
// <filename>/tmp/_public.cpp</filename><mtime>2020-01-01 10:10:15</mtime><size>50945</size>
// xmlbuffer：待解析的xml格式字符串。
// fieldname：字段的标签名。
// value：传入变量的地址，用于存放字段内容，支持bool、int、insigned int、long、
//       unsigned long、double和char[]。
// 注意：当value参数的数据类型为char []时，必须保证value数组的内存足够，否则可能发生内存溢出的问题，
//           也可以用ilen参数限定获取字段内容的长度，ilen的缺省值为0，表示不限长度。
// 返回值：true-成功；如果fieldname参数指定的标签名不存在，返回失败。
bool getxmlbuffer(
  const std::string& xmlbuffer, const std::string& fieldname, std::string& value, const int ilen = 0
); // 视频中没有第三个参数，加上第三个参数更好。
// bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,char *value,const int
// ilen=0); bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,bool
// &value); bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,int &value);
// bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,unsigned int &value);
// bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,long &value);
// bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,unsigned long
// &value); bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,double
// &value); bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,float
// &value);
///////////////////////////////////// /////////////////////////////////////


class UserInfo // 用户信息（状态机）。
{
private:
  int fd_;             // 客户端的fd。
  std::string ip_;     // 客户端的ip地址。
  bool login_ = false; // 客户端登录的状态：true-已登录；false-未登录。
public:
  UserInfo(int fd, const std::string& ip) : fd_(fd), ip_(ip) {}
  void setLogin(bool login) {
    login_ = login;
  }
  bool Login() {
    return login_;
  }
};

class BankServer {
private:
  using spUserInfo = std::shared_ptr<UserInfo>;
  TcpServer tcpserver_;
  ThreadPool threadpool_; // 工作线程池。
  std::map<int, spUserInfo> usermap_;

public:
  BankServer(
    const std::string& ip, const uint16_t port, int subthreadnum = 3, int workthreadnum = 5
  );
  ~BankServer();

  void Start(); // 启动服务。
  void Stop();  // 停止服务。

  void HandleNewConnection(spConnection conn); // 处理新客户端连接请求，在TcpServer类中回调此函数。
  void HandleClose(spConnection conn);         // 关闭客户端的连接，在TcpServer类中回调此函数。
  void HandleError(spConnection conn);         // 客户端的连接错误，在TcpServer类中回调此函数。
  void HandleMessage(
    spConnection conn, std::string& message
  ); // 处理客户端的请求报文，在TcpServer类中回调此函数。

  void
  OnMessage(spConnection conn, std::string& message); // 处理客户端的请求报文，用于添加给线程池。
  void HandleRemove(int fd); // 客户端的连接超时，在TcpServer类中回调此函数。
};
