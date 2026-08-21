# 项目结构与逻辑解析

本项目是一个基于 C++11 实现的高并发网络服务器框架，采用经典的 **主从多 Reactor (One Loop Per Thread) + Worker 业务线程池** 架构。

---

## 1. 项目目录结构

```text
├── bin/                 # 编译输出的可执行程序目录 (echoserver, bankserver, client, client1)
├── src/                 # 源码目录
│   ├── Acceptor.*       # 连接接收器模块
│   ├── BankServer.*     # 网上银行业务服务器实现
│   ├── Buffer.*         # 应用层缓冲与协议拆包封装
│   ├── Channel.*        # 事件分发器模块
│   ├── Connection.*     # TCP 连接抽象与数据生命周期管理
│   ├── EchoServer.*     # 回显业务服务器实现
│   ├── Epoll.*          # Linux epoll 系统调用封装
│   ├── EventLoop.*      # 事件循环与定时/跨线程任务调度
│   ├── InetAddress.*    # IPv4 地址结构封装
│   ├── Socket.*         # 套接字操作及选项封装
│   ├── TcpServer.*      # TCP 基础服务器框架入口
│   ├── ThreadPool.*     # 基于条件变量的任务线程池
│   ├── Timestamp.*      # 时间戳与格式化工具
│   ├── bankserver.cpp   # BankServer main 入口
│   ├── echoserver.cpp   # EchoServer main 入口
│   ├── client.cpp       # 压测客户端实现
│   ├── client1.cpp      # 基础功能测试客户端
│   └── makefile         # 构建规则
├── README.md            # 项目架构与模块说明文档
├── manual.md            # 服务端与测试客户端使用手册
└── project_logic_and_structure.md # 本项目逻辑与架构设计文档
```

---

## 2. 核心模块与类职责

### 2.1 底层网络与 I/O 多路复用
- [`Socket`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Socket.h#L14)：封装底层 socket 文件描述符 (`fd`)，提供 `bind`、`listen`、`accept` 及 `SO_REUSEADDR`、`SO_REUSEPORT`、`TCP_NODELAY` 等选项配置。
- [`InetAddress`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/InetAddress.h)：封装 `sockaddr_in` 地址结构，提供 IP 与端口的解析与转换。
- [`Epoll`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Epoll.h#L15)：封装 Linux `epoll_create1`、`epoll_ctl`、`epoll_wait`，管理红黑树上的文件描述符及事件监听。
- [`Channel`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Channel.h#L11)：事件分发器，作为 `fd` 与多路复用器的纽带，绑定感兴趣的事件（`EPOLLIN`、`EPOLLOUT`）及对应的读、写、错误、关闭回调函数。

### 2.2 事件循环与调度
- [`EventLoop`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.h#L21)：事件循环核心，每个线程独立运行一个 `EventLoop`：
  - 调用 [`Epoll::loop`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Epoll.cpp#L40) 阻塞等待并分派就绪事件给 [`Channel::handleevent`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Channel.cpp#L69)。
  - 内置 `eventfd` 机制 ([`wakeupfd_`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.h#L31))：支持其他线程向本 I/O 线程派发异步任务 ([`queueinloop`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.cpp#L88)) 并即时唤醒。
  - 内置 `timerfd` 定时器 ([`timerfd_`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.h#L33))：周期性触发 [`EventLoop::handletimer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.cpp#L125)，检测并清除空闲超时的连接。

### 2.3 连接管理与缓冲区
- [`Acceptor`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Acceptor.h#L10)：运行在主事件循环 (`MainLoop`) 中，持有监听套接字 `servsock_` 与监听 Channel `acceptchannel_`，专门负责接受新客户端连接并将新生成的 socket 交由上层分配。
- [`Connection`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.h#L18)：继承 `std::enable_shared_from_this<Connection>`，管理单个已建立连接的完整生命周期：
  - 绑定通信用的 [`Socket`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Socket.h#L14) 与所属从事件循环 (`SubLoop`) 中的 [`Channel`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Channel.h#L11)。
  - 管理输入缓冲区 `inputbuffer_` 与输出缓冲区 `outputbuffer_`。
  - 维护最后活跃时间戳 [`Timestamp`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Timestamp.h) 以支持超时判断。
- [`Buffer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Buffer.h#L8)：应用层缓冲区，解决 TCP 字节流传输中的粘包、分包与半包问题。支持无分隔符、4字节包头长度模式 ([`pickmessage`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Buffer.cpp#L46)) 及 HTTP 分隔符。

### 2.4 线程模型与服务端封装
- [`ThreadPool`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/ThreadPool.h#L16)：基于 `std::thread`、互斥锁与条件变量实现的任务线程池。在系统中复用于两处：
  - **IO 线程池**：管理各个从事件循环 (`SubLoop`)。
  - **Worker 线程池**：用于执行具体的耗时业务计算，避免阻塞 I/O 线程。
- [`TcpServer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/TcpServer.h#L14)：核心服务器抽象：
  - 拥有 1 个 `MainLoop`（负责监听与接受连接）和 `N` 个 `SubLoop`（运行在 IO 线程池中）。
  - 维护连接映射表 `conns_` (`std::map<int, spConnection>`)。
  - 注册各项事件回调（新连接、消息接收、消息发送完成、连接关闭、错误处理）。

### 2.5 业务应用层
- [`EchoServer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EchoServer.h#L11)：回显服务器，收到客户端数据后添加前缀并原样返回，可配置独立 Worker 线程池测试并发吞吐。
- [`BankServer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/BankServer.h#L54)：模拟网上银行业务服务器，内置 XML 字符串解析函数 [`getxmlbuffer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/BankServer.cpp#L6) 与用户会话状态管理 [`UserInfo`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/BankServer.h#L38)，支持心跳包 (`<bizcode>0000</bizcode>`) 及身份验证登录 (`<bizcode>0001</bizcode>`)。

---

## 3. 系统核心运行逻辑

### 3.1 初始化与启动流程
1. 实例化 [`TcpServer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/TcpServer.cpp#L3)，创建 `MainLoop` 和 `Acceptor`，并创建指定数量的 `SubLoop`。
2. 将各个 `SubLoop` 的 [`EventLoop::run`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.cpp#L33) 注册为任务提交给 IO 线程池启动。
3. 业务层（如 [`EchoServer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EchoServer.cpp#L3) / [`BankServer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/BankServer.cpp#L29)）配置业务回调函数及 Worker 线程池，调用 `Start()` 启动 `MainLoop`。

### 3.2 新连接接入流程 (Connection Accept)
1. 客户端发起 TCP 握手连接。
2. 主线程 `MainLoop` 监听到 `listenfd` 的读事件，调用 [`Acceptor::newconnection`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Acceptor.cpp#L22)。
3. [`Acceptor`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Acceptor.cpp#L22) 通过 `accept` 得到客户端连接套接字并创建 [`Socket`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Socket.h#L14) 对象，触发回调进入 [`TcpServer::newconnection`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/TcpServer.cpp#L57)。
4. [`TcpServer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/TcpServer.cpp#L57) 使用轮询/散列算法（`clientfd % threadnum_`）从 `subloops_` 中选取一个 `SubLoop`。
5. 创建 [`Connection`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L3) 对象，在选定的 `SubLoop` 中将该连接的 `clientfd` 注册到其关联的 `Epoll` 红黑树上（采用边缘触发 ET 模式，监听 `EPOLLIN` 读事件）。

### 3.3 数据读取与业务分发流程 (Read & Dispatch)
1. 客户端发送数据，对应 `SubLoop` 所在 IO 线程被 `epoll_wait` 唤醒。
2. 触发 [`Connection::onmessage`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L71)，循环以非阻塞方式读取 socket 数据并存入 `inputbuffer_` 直至遇到 `EAGAIN`。
3. 调用 [`Buffer::pickmessage`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Buffer.cpp#L46) 根据 4 字节头部长度解析完整报文包，更新最近通信时间戳 `lastatime_`。
4. 触发业务回调 [`EchoServer::HandleMessage`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EchoServer.cpp#L65) / [`BankServer::HandleMessage`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/BankServer.cpp#L91)：
   - 若设置了 Worker 工作线程池，将业务计算任务打包通过 `ThreadPool::addtask` 派发给 Worker 线程执行，实现 I/O 与计算解耦。
   - 若无 Worker 线程池，则在当前 I/O 线程内直接同步处理。

### 3.4 跨线程数据发送流程 (Send & Cross-Thread I/O)
1. 业务处理完成后调用 [`Connection::send`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L111)。
2. [`Connection::send`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L111) 判断当前线程是否为该连接绑定的 `SubLoop` 线程：
   - **是 IO 线程**：直接执行 [`Connection::sendinloop`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L132)，写入 `outputbuffer_` 并为 Channel 注册 `EPOLLOUT` 写事件。
   - **是 Worker 线程**：调用 [`EventLoop::queueinloop`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.cpp#L88) 将 `sendinloop` 包装为任务推入 `SubLoop` 的 `taskqueue_`，并向 `wakeupfd_` 写入 8 字节计数唤醒目标 IO 线程执行数据发送。
3. IO 线程监听到 `EPOLLOUT` 事件后调用 [`Connection::writecallback`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L140)，调用 `send` 将 `outputbuffer_` 中的数据推送至 socket。发送完毕后注销 `EPOLLOUT` 写事件并触发发送完成回调。

### 3.5 心跳检测与连接销毁流程 (Heartbeat & Cleanup)
1. 各 `SubLoop` 上的 `timerfd_` 按照配置周期（如每 5 秒）到期触发读事件。
2. [`EventLoop::handletimer`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/EventLoop.cpp#L125) 遍历当前事件循环内所有连接，计算 `now - lastatime_`。若超时则将其从 `EventLoop` 及 `TcpServer` 的映射表中清除并释放连接。
3. 当客户端主动断开连接或发生错误时，`read` 返回 0 或触发 `EPOLLRDHUP`/`EPOLLERR`，调用 [`Connection::closecallback`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L36) 或 [`Connection::errorcallback`](file:///home/alex/alexmak/ongo/million-concurrent-reactor-server/src/Connection.cpp#L43)，注销 `Channel` 并从服务器管理列表中移除。
