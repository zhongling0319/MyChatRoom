#include "Server.h"

Server::Server() {
	//初始化成员变量
	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);	//接受任意ip
	
	listener = 0;
	epfd = 0;
}

//服务器端初始化，建立socket，绑定，监听
void Server::Init() {
	cout << "服务器端初始化..." << endl;
	
	//创建监听socket
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		perror("socket");
		exit(-1);
	}
	//允许地址重用
	int on = 1;
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	//绑定地址
	if (bind(listener, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		perror("bind");
		exit(-1);
	}
	//设置监听
	if (listen(listener, 5) < 0) {
		perror("listen");
		exit(-1);
	}
	//创建epoll句柄
	epfd = epoll_create(EPOLL_SIZE);
	if (epfd < 0) {
		perror("epoll_create");
		exit(-1);
	}
	//将listener加入epoll事件表
	addfd(epfd, listener, true);
	
	cout << "开始监听" << endl;
}

//启动服务器端
void Server::Start() {
	//初始化服务端
	Init();
	//epoll就绪事件队列
	struct epoll_event events[EPOLL_SIZE];
	while (true) {
		//调用epoll_wait阻塞监听socket和客户端连接socket
		int epollEventsCount = epoll_wait(epfd, events, EPOLL_SIZE, -1);
		if (epollEventsCount < 0) {
			perror("epoll_wait");
			break;
		}
		cout << epollEventsCount << "个epoll事件就绪" << endl;

		//处理所有就绪事件
		for (int i = 0; i < epollEventsCount; i++) {
			int sockfd = events[i].data.fd;
			//新用户连接
			if (sockfd == listener) {
				int ret = AddNewClient();
				if (ret < 0) {
					perror("AddNewClient");
					Close();
					exit(-1);
				}
			}
			//处理用户发来的消息并广播
			else {
				int ret = SendBroadcastMessage(sockfd);
				if (ret < 0) {
					perror("SendBroadcastMessage");
					Close();
					exit(-1);
				}
			}
		}
	}
}

//关闭服务器端
void Server::Close() {
	//关闭socket和epoll句柄
	close(listener);
	close(epfd);
}


//广播消息给客户端
int Server::SendBroadcastMessage(int clientfd) {
	//接收客户端信息
	bzero(recvBuf, BUF_SIZE);
	int len = recv(clientfd, recvBuf, BUF_SIZE, 0);
	if (len < 0) {
		perror("recv");
		return -1;
	}
	char message[BUF_SIZE];
	//如果客户端关闭了连接
	if (len == 0) {
		//向所有用户发送通知信息
		bzero(message, BUF_SIZE);
		sprintf(message, SERVER_EXIT_NOTIFY, clients[clientfd].c_str(), clientfd);
		memset(&msg, 0, sizeof(msg));
		memcpy(msg.content, message, BUF_SIZE);
		bzero(sendBuf, BUF_SIZE);
		memcpy(sendBuf, &msg, sizeof(msg));
		for (auto it = clients.begin(); it != clients.end(); it++) {
			if (it->first != clientfd) {
				if (send(it->first, sendBuf, BUF_SIZE, 0) < 0) {
					perror("send notify");
					return -1;
				}
			}
		}
		//关闭对应socket
		close(clientfd);
		//从客户端列表中删除
		cout << "用户：" << clients[clientfd] << "@" << clientfd << "退出聊天室" << endl;
		clients.erase(clientfd);
		cout << "聊天室当前用户数：" << clients.size() << endl;
		return 0;
	}
	cout << "收到用户[" << clients[clientfd] << "@" << clientfd << "]的信息" << endl;
	//如果聊天室没有其他客户端，发送提醒
	if (clients.size() == 1) {
		bzero(message, BUF_SIZE);
		sprintf(message, CAUTION);
		memset(&msg, 0, sizeof(msg));
		memcpy(msg.content, message, BUF_SIZE);
		bzero(sendBuf, BUF_SIZE);
		memcpy(sendBuf, &msg, sizeof(msg));
		if (send(clientfd, sendBuf, BUF_SIZE, 0) < 0) {
			perror("send caution");
			return -1;
		}
	}
	//接收到的字符串转为信息结构体
	memset(&msg, 0, sizeof(msg));
	memcpy(&msg, recvBuf, sizeof(msg));
	//cout << msg.content << endl;
	//cout << msg.isGroupChat << endl;
	//群聊消息
	if (msg.isGroupChat) {
		//将群聊前缀添加到消息内容之前
		bzero(message, BUF_SIZE);
		sprintf(message, SERVER_MESSAGE, clients[clientfd].c_str(), clientfd, msg.content);
		memcpy(msg.content, message, BUF_SIZE);
		bzero(sendBuf, BUF_SIZE);
		memcpy(sendBuf, &msg, sizeof(msg));
		//遍历客户端列表依次发送消息，不给来源客户端发
		for (auto it = clients.begin(); it != clients.end(); it++) {
			if (it->first != clientfd) {
				if (send(it->first, sendBuf, BUF_SIZE, 0) < 0) {
					perror("send group chat");
					return -1;
				}
			}
		}
	}
	//私聊消息
	else {
		int toID = msg.toID;
		//私聊对象不在线
		if (clients.find(toID) == clients.end()) {
			bzero(message, BUF_SIZE);
			sprintf(message, CLIENT_OFFLINE);
			memset(&msg, 0, sizeof(msg));
			memcpy(msg.content, message, BUF_SIZE);
			bzero(sendBuf, BUF_SIZE);
			memcpy(sendBuf, &msg, sizeof(msg));
			if (send(clientfd, sendBuf, BUF_SIZE, 0) < 0) {
				perror("send client offline");
				return -1;
			}
			cout << "用户[" << clients[clientfd] << "@" << clientfd << "]私聊的对象不在线" << endl;
			return 0;
		}
		//私聊对象在线
		else {
			//将私聊前缀添加到消息内容之前
			bzero(message, BUF_SIZE);
			sprintf(message, SERVER_PRIVATE_MESSAGE, clients[clientfd].c_str(), clientfd, msg.content);
			memcpy(msg.content, message, BUF_SIZE);
			bzero(sendBuf, BUF_SIZE);
			memcpy(sendBuf, &msg, sizeof(msg));
			//给私聊对象单独发送消息
			if (send(toID, sendBuf, BUF_SIZE, 0) < 0) {
				perror("send private msg");
				return -1;
			}
		}	
	}
	cout << "用户[" << clients[clientfd] << "@" << clientfd << "]的信息已发出" << endl;
	return 0;
}

//添加新客户端
int Server::AddNewClient() {
	//连接客户端
	struct sockaddr_in clientAddr;
	bzero(&clientAddr, sizeof(clientAddr));
	socklen_t clientAddrLen = sizeof(clientAddr);
	int clientfd = accept(listener, (struct sockaddr*) & clientAddr, &clientAddrLen);
	if (clientfd < 0) {
		perror("accept");
		return -1;
	}
	//接受客户端昵称
	char nickname[NICKNAME_SIZE];
	bzero(nickname, NICKNAME_SIZE);
	int len = recv(clientfd, nickname, sizeof(nickname), 0);
	if (len < 0) {
		perror("recv");
		return -1;
	}
	//将新客户端添加进客户端列表
	clients[clientfd] = nickname;
	//clients.insert(make_pair<int, char[NICKNAME_SIZE]>(clientfd, nickname));
	//将新客户端添加进epoll
	addfd(epfd, clientfd, true);
	//向新客户端发送欢迎信息
	char message[BUF_SIZE];
	bzero(message, BUF_SIZE);
	sprintf(message, SERVER_WELCOME, clientfd);
	memset(&msg, 0, sizeof(msg));
	memcpy(msg.content, message, BUF_SIZE);
	bzero(sendBuf, BUF_SIZE);
	memcpy(sendBuf, &msg, BUF_SIZE);
	//cout << "发送欢迎信息" << endl;
	int len2 = send(clientfd, sendBuf, BUF_SIZE, 0);
	//cout << "发送了" << len2 << endl;
	if (len2 < 0) {
		perror("send");
		return -1;
	}
	//向所有用户发送通知信息
	bzero(message, BUF_SIZE);
	sprintf(message, NEW_SERVER_NOTIFY, nickname, clientfd);
	memset(&msg, 0, sizeof(msg));
	memcpy(msg.content, message, BUF_SIZE);
	bzero(sendBuf, BUF_SIZE);
	memcpy(sendBuf, &msg, BUF_SIZE);
	for (auto it = clients.begin(); it != clients.end(); it++) {
		if (it->first != clientfd) {
			if (send(it->first, sendBuf, BUF_SIZE, 0) < 0) {
				perror("send");
				return -1;
			}
		}
	}

	cout << "连接客户端：" << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << endl;
	cout << "用户：" << nickname << "@" << clientfd << "进入聊天室" << endl;
	cout << "聊天室当前用户数：" << clients.size() << endl;
	return 0;
}
