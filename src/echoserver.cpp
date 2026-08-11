/*
 * 程序名：echoserver.cpp，回显（EchoServer）服务器。
 * 作者：吴从周
 */
#include "EchoServer.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("usage: ./echoserver ip port\n");
    printf("example: ./echoserver 192.168.150.128 5085\n\n");
    return -1;
  }

  // TcpServer tcpserver(argv[1],atoi(argv[2]));
  // tcpserver.start();      // 运行事件循环。

  EchoServer echoserver(argv[1], atoi(argv[2]));
  echoserver.Start();

  return 0;
}
