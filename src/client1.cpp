// 网络通讯的客户端程序。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

ssize_t tcpsend(int fd,void *data,size_t size)
{
    char tmpbuf[1024];                    // 临时的buffer，报文头部+报文内容。
    memset(tmpbuf,0,sizeof(tmpbuf));
    memcpy(tmpbuf,&size,4);         // 拼接报文头部。
    memcpy(tmpbuf+4,data,size);  // 拼接报文内容。

    return send(fd,tmpbuf,size+4,0);        // 把请求报文发送给服务端。
}

ssize_t tcprecv(int fd,void *data)
{
    int len;
    recv(fd,&len,4,0);            // 先读取4字节的报文头部。
    return recv(fd,data,len,0);           // 读取报文内容。
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage:./client ip port\n"); 
        printf("example:./client1 192.168.150.128 5085\n\n"); 
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];
 
    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0) { printf("socket() failed.\n"); return -1; }
    
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr=inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)
    {
        printf("connect(%s:%s) failed.\n",argv[1],argv[2]); close(sockfd);  return -1;
    }

    printf("connect ok.\n");

    //////////////////////////////////////////
    // 登录业务。
    memset(buf,0,sizeof(buf));
    sprintf(buf,"<bizcode>0001</bizcode><username>admin</username><password>123456</password>");
    if (tcpsend(sockfd,buf,strlen((buf))) <=0) { printf("tcpsend() failed.\n"); return -1; }
    printf("发送登录：%s\n",buf);

    memset(buf,0,sizeof(buf));
    if (tcprecv(sockfd,buf) <=0) { printf("tcprecv() failed.\n"); return -1; }
    printf("接收登录：%s\n",buf);
    //////////////////////////////////////////

    //////////////////////////////////////////
    // 心跳测试 (循环5次)。
    for (int ii = 0; ii < 5; ii++)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf,"<bizcode>0000</bizcode>");
        if (tcpsend(sockfd,buf,strlen((buf))) <=0) { printf("tcpsend() failed.\n"); return -1; }
        printf("发送心跳：%s\n",buf);

        memset(buf,0,sizeof(buf));
        if (tcprecv(sockfd,buf) <=0) { printf("tcprecv() failed.\n"); return -1; }
        printf("接收心跳：%s\n",buf);

        sleep(1);
    }
    //////////////////////////////////////////

    /*
    printf("开始时间：%d\n",time(0));

    for (int ii=0;ii<10000;ii++)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf,"这是第%d个超级女生。",ii);

        char tmpbuf[1024];                 // 临时的buffer，报文头部+报文内容。
        memset(tmpbuf,0,sizeof(tmpbuf));
        int len=strlen(buf);                 // 计算报文的大小。
        memcpy(tmpbuf,&len,4);       // 拼接报文头部。
        memcpy(tmpbuf+4,buf,len);  // 拼接报文内容。

        send(sockfd,tmpbuf,len+4,0);  // 把请求报文发送给服务端。
        
        recv(sockfd,&len,4,0);            // 先读取4字节的报文头部。

        memset(buf,0,sizeof(buf));
        recv(sockfd,buf,len,0);           // 读取报文内容。

        // printf("recv:%s\n",buf);
    }
    printf("结束时间：%d\n",time(0));
    */

    /*
    for (int ii=0;ii<10;ii++)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf,"这是第%d个超级女生。",ii);

        send(sockfd,buf,strlen(buf),0);  // 把请求报文发送给服务端。
        
        memset(buf,0,sizeof(buf));
        recv(sockfd,buf,1024,0);           // 读取报文内容。

        printf("recv:%s\n",buf);
sleep(1);
    }
    */

} 
