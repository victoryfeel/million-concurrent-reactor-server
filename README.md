Original Repo: https://github.com/victoryfeel/webserver-reactor-original

最终版本（ 38 ）实现了一个非常经典且成熟的 “One Loop Per Thread” (一线程一循环) 的主从 Reactor 模型（架构上非常类似于著名的 C++ 网络库 muduo ）。

下面是该最终版本的核心结构和各个模块的职责分析：

### 1. 核心网络框架 (Reactor 模型抽象)

• EventLoop (事件循环)
每个线程对应一个 EventLoop 。它内部封装了 Epoll ，负责执行 epoll*wait 监听事件，并在有事件发生时分发给对应的 Channel
去处理。此外，它还维护了一个任务队列 ( taskqueue* )，可以通过 eventfd 唤醒线程来执行跨线程派发的异步任务。
• Channel (事件分发器)
它是文件描述符 ( fd ) 的保姆。封装了 fd 、它所感兴趣的事件（如 EPOLLIN , EPOLLOUT ）以及事件触发时的各种回调函数（读、写、关闭、错误等）。 Channel
不拥有 fd ，只是负责事件的分发。
• Epoll (多路复用器)
对 Linux epoll 机制（ epoll_create , epoll_ctl , epoll_wait ）的底层面向对象封装，供 EventLoop 调用。

### 2. TCP 连接与通信层

• TcpServer (TCP 网络服务端)
这是整个网络框架的核心对外接口。它拥有：
• 一个 主事件循环 (Main EventLoop)，专门用来监听新连接。
• 一个 线程池 ( ThreadPool )，用来管理多个 从事件循环 (Sub EventLoops)。
• 维护了一个保存所有客户端连接 ( Connection ) 的 std::map 。
• 负责设置各种核心的业务回调函数（新连接建立、消息到达、连接断开等）。
• Acceptor (连接接收器)
运行在 Main EventLoop 中，专门封装了处理新客户端请求的逻辑（监听 listenfd 的读事件并执行 accept ）。
• Connection (TCP 连接封装)
代表一个已建立的 TCP 连接。它包含了一个通信用的 Socket 、一个处理通信事件的 Channel ，以及负责收发数据的输入输出缓冲区 ( Buffer
)。它还记录了连接的时间戳 ( Timestamp ) 以用于超时心跳断开。
• Buffer (应用层缓冲区)
由于 TCP 是基于字节流的，为了解决粘包半包问题以及非阻塞 I/O 的读写问题，框架提供了用户态的输入和输出缓冲区。

### 3. 多线程与业务层

• ThreadPool (线程池)
在 TcpServer 启动时，分配了固定数量的 IO 线程。每个线程运行一个专属的 EventLoop （即 SubLoop）。当有新连接到来时， TcpServer 会将其分配给某个 SubLoop
去负责后续的所有网络 I/O，做到无锁化处理。
• 业务实现 ( EchoServer , BankServer )
这是基于以上封装好的高并发底层组件实现的具体应用层服务。用户只需实例化 TcpServer 并将具体的业务回调函数（例如收到一条消息该怎么处理，也就是
onmessagecb\_ ）注册进去，就可以实现比如网银服务 ( bankserver ) 或回显服务 ( echoserver )。

架构总结流程：

1. 主线程的 MainLoop 监控 Acceptor 。
2. 客户端发起连接， Acceptor 接受连接，生成 Socket 。
3. TcpServer 将这个 Socket 封装为 Connection ，并通过轮询 ( Round-Robin ) 的方式派发给线程池中的某个 SubLoop 。
4. 该 SubLoop 接管此 Connection 的 Channel ，后续这个客户端所有的读写事件和断开事件，都在这个专属的 IO 线程中独立异步完成，实现了真正的高并发。
