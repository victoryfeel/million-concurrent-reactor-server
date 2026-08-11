all:client echoserver client1 bankserver
client:client.cpp
	g++ -g -o client client.cpp
	mv -f client ../bin/.
client1:client1.cpp
	g++ -g -o client1 client1.cpp
	mv -f client1 ../bin/.
echoserver:echoserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Timestamp.cpp
	g++ -g -o echoserver echoserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Timestamp.cpp -lpthread
	mv -f echoserver ../bin/.
bankserver:bankserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp BankServer.cpp ThreadPool.cpp Timestamp.cpp
	g++ -g -o bankserver bankserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp BankServer.cpp ThreadPool.cpp Timestamp.cpp -lpthread
	mv -f bankserver ../bin/.
clean:
	rm -rf ../bin/*


rune:
	../bin/echoserver 0.0.0.0 5085
runb:
	../bin/bankserver 0.0.0.0 5085
runc:
	../bin/client 127.0.0.1 5085
runsh:
	sh ./tmp1.sh
