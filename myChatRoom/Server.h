#pragma once
#include "Common.h"
using namespace std;

//服务器端类
class Server
{
public:
	Server();
	//服务器端初始化，建立socket，绑定，监听
	void Init();
	//启动服务器端
	void Start();
	//关闭服务器端
	void Close();
private:
	//服务器端ServerAddr信息
	struct sockaddr_in serverAddr;
	//监听socket
	int listener;
	//客户端列表
	unordered_map<int, string> clients;
	//epoll句柄
	int epfd;
	//消息结构体
	Msg msg;
	//通信用字符串
	char recvBuf[BUF_SIZE];
	char sendBuf[BUF_SIZE];
	//广播消息给客户端
	int SendBroadcastMessage(int clientfd);
	//添加新客户端
	int AddNewClient();
};

