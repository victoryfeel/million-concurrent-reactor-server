# 百万并发 Reactor 服务器项目 - 操作使用手册

这份手册将带您一步步启动并测试本项目的核心服务模块（包含 `BankServer` 和 `EchoServer`）。

## 1. 目录结构与可执行文件

本项目主要产出物存放在 `bin/` 目录下。编译（`make`）完成后，您应该可以在该目录下看到四个核心可执行文件：

- `echoserver`: 基础的回显服务器（将收到的内容原样退回，主要用于测算极高吞吐下的并发能力）。
- `bankserver`: 高级银行业务服务器（自带了 XML 报文解析逻辑，带有登录状态机）。
- `client` / `client1`: 提供的高并发/功能性测试客户端。

## 2. 启动服务

> [!TIP]
> 运行服务端时，您必须指定绑定的 IP 地址和监听的端口。

### 启动 Echo 回显服务器

在终端中进入 `bin/` 目录，执行以下命令：

```bash
cd ~/ongo/webserver-reactor/bin
./echoserver 0.0.0.0 5085
```

_(注：`0.0.0.0` 表示监听本机所有网卡 IP，端口 `5085` 可根据需要自行修改。)_

### 启动 Bank 银行服务器

同样在 `bin/` 目录下：

```bash
cd ~/ongo/webserver-reactor/bin
./bankserver 0.0.0.0 5085
```

当您看到如下类似的输出时，说明各个核心组件（MainLoop，IO-SubLoops，Worker 线程池）已成功拉起，正在等待客户端连入：

```text
create IO thread(2444).
create WORKS thread(2447).
create IO thread(2446).
...
```

## 3. 进行压力测试与功能验证

启动好服务端后，请**新开一个终端窗口**来运行客户端。

### 方法一：使用预设的批量高并发测试客户端 (`client`)

由于服务器底层做了自定义的封包拆包协议（`4字节消息头部表示长度 + 消息体`），建议直接使用配套的 `client` 工具进行测试。

```bash
cd ~/ongo/webserver-reactor/bin
./client 127.0.0.1 5085
```

`client` 将会尝试通过非阻塞模式往服务端高频发送数万条数据，测试服务端的极端承压能力。你会在服务端的终端看到类似：

```text
[当前时间] new connection(fd=x,ip=127.0.0.1,port=xxx) ok.
```

的日志打印。

### 方法二：自定义发送 BankServer 业务报文

由于我为您补全的 `BankServer` 内部集成了基于 XML 的简易业务流。您如果开发或修改配套客户端发送特定的报文：

- 发送 `<bizcode>0000</bizcode>`：将会收到 `<message>heartbeat ok</message>` 的回应。
- 发送 `<bizcode>0001</bizcode><username>admin</username><password>123456</password>`：服务端会为您标记登录态并响应 `login success`。

## 4. 常见问题排查 (Troubleshooting)

> [!WARNING]
> **百万压测注意事项**
>
> 如果您打算用数万个以上的并发打入这个服务器，普通的 Linux 默认设置一定会遇到 `Too many open files` 错误。
> 在发起真正的极限并发测试前，请确保在运行服务器的终端内调大了文件描述符限制：
>
> ```bash
> ulimit -n 1048576
> ```

> [!CAUTION]
> **退出服务器**
> 由于该程序屏蔽了默认的部分退出机制以防误触，要关闭它，推荐直接使用 `Ctrl+C`。代码在内部捕获了 `SIGINT` (信号2)，会自动通过析构线程池和平滑停机，随后您会看到 `bankserver已停止` 的日志并安全退出。
