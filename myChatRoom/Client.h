#pragma once
#include "Common.h"
using namespace std;

class Client
{
public:
	Client();
	//连接服务器
	void Connect();
	//启动客户端
	void Start();
	//关闭客户端
	void Close();
private:
	//子进程从键盘获取输入，通过管道传给父进程
	void RecvInput();
	//管道标识符，父进程用fd[0]读，子进程用fd[1]写
	int pipefd[2];
	//epoll句柄
	int epfd;
	//当前进程id
	int pid;
	//连接服务器端的sock
	int sock;
	//发送的聊天信息
	Msg msg;
	//通信用字符串
	char recvBuf[BUF_SIZE];
	//服务器端ServerAddr信息
	struct sockaddr_in serverAddr;
	//标识客户端运行
	bool isClientWork;
};

