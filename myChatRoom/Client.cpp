#include "Client.h"

Client::Client() {
	//初始化服务器地址
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	//初始化其他成员变量
	sock = 0;
	pid = 0;
	epfd = 0;
	isClientWork = true;
}

//连接服务器
void Client::Connect() {
	//创建socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(-1);
	}
	cout << "请输入昵称：" ;
	char message[NICKNAME_SIZE];
	//cin >> message;
	cin.getline(message, NICKNAME_SIZE);
	//连接服务器端
	if (connect(sock, (struct sockaddr*) & serverAddr, sizeof(serverAddr)) < 0) {
		perror("connect");
		exit(-1);
	}
	//向服务器端发送昵称信息
	if (send(sock, message, sizeof(message), 0) < 0) {
		perror("send nickname");
		exit(-1);
	}
	//创建管道，fd[0]用于父进程读，fd[1]用于子进程写
	if (pipe(pipefd) < 0) {
		perror("pipe");
		exit(-1);
	}
	//创建epoll句柄
	epfd = epoll_create(EPOLL_SIZE);
	if (epfd < 0) {
		perror("epoll create");
		exit(-1);
	}
	//将sock和fd[0]添加进epoll事件表
	addfd(epfd, sock, true);
	addfd(epfd, pipefd[0], true);
}

//启动客户端
void Client::Start() {
	//连接服务器
	Connect();
	//创建子进程
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(-1);
	}
	//进入子进程
	else if (pid == 0) {
		//关闭管道读端
		close(pipefd[0]);
		//子进程从键盘获取输入，通过管道传给父进程
		RecvInput();
	}
	//进入父进程
	else {
		//关闭管道写端
		close(pipefd[1]);
		//epoll就绪事件队列
		struct epoll_event events[2];
		//主循环，接收服务端消息并打印，接收子进程消息并发送给服务端
		while (isClientWork) {
			//调用epoll_waitcd
			int epollEventsCount = epoll_wait(epfd, events, 2, -1);
			//处理就绪事件
			//cout << epollEventsCount << "个就绪事件" << endl;
			for (int i = 0; i < epollEventsCount; i++) {
				bzero(recvBuf, BUF_SIZE);
				//服务器端发来消息
				if (events[i].data.fd == sock) {
					int len = recv(sock, recvBuf, BUF_SIZE, 0);
					//cout << "接收了" << len << endl;
					if (len < 0) {
						perror("recv from server");
						Close();
						exit(-1);
					}
					else if (len == 0) {	//服务器端关闭
						cout << "服务器端连接已断开，客户端将退出..." << endl;
						isClientWork = false;
						kill(pid, SIGKILL);
					}
					else {
						memset(&msg, 0, sizeof(msg));
						memcpy(&msg, recvBuf, BUF_SIZE);
						cout << msg.content << endl;
					}
				}
				//子进程发来消息
				else {
					int len = read(pipefd[0], recvBuf, BUF_SIZE);
					if (len < 0) {
						perror("read from child process");
						Close();
						exit(-1);
					}
					else if (len == 0) {
						isClientWork = false;
					}
					else {
						//将管道中读到的信息直接发送给服务器
						//cout << "子进程发来消息" << endl;
						if (send(sock, recvBuf, BUF_SIZE, 0) < 0) {
							perror("send to server");
							Close();
							exit(-1);
						}
					}
				}
			}
		}
		wait(nullptr);
	}
	Close();
}

//关闭客户端
void Client::Close() {
	if (pid == 0) {
		//关闭子进程的管道
		close(pipefd[1]);
	}
	else {
		//关闭父进程的管道和sock
		close(pipefd[0]);
		close(sock);
	}
}

//子进程从键盘获取输入，通过管道传给父进程
void Client::RecvInput() {
	cout << "输入\\a，切换群聊模式" << endl;
	cout << "输入\\+用户id，切换私聊对象" << endl;
	cout << "输入exit，退出客户端" << endl;
	//清空消息结构体，设置初始模式为群聊
	memset(&msg, 0, sizeof(msg));
	msg.isGroupChat = 1;
	//主循环，接收键盘输入
	while (isClientWork) {
		memset(msg.content, 0, sizeof(msg.content));
		//cin >> msg.content;
		cin.getline(msg.content, BUF_SIZE);
		//fgets(msg.content, BUF_SIZE, stdin);
		//cout << "接收到输入" << msg.content << endl;
		//输入exit，客户端退出
		if (strcmp(msg.content, "exit") == 0) {
			isClientWork = false;
		}
		//输入\a，切换群聊
		else if (strcmp(msg.content, "\\a") == 0) {
			msg.isGroupChat = 1;
			cout << "进入群聊模式" << endl;
		}
		//输入\+ID，切换私聊对象
		else if (msg.content[0] == '\\' && isdigit(msg.content[1])) {
			string temp = msg.content;
			temp = temp.substr(1);
			msg.toID = atoi(temp.data());
			msg.isGroupChat = 0;
			cout << "私聊" << msg.toID << endl;
		}
		else {
			bzero(recvBuf, BUF_SIZE);
			memcpy(recvBuf, &msg, BUF_SIZE);
			//cout << "子进程发送：" << msg.content << endl;
			if (write(pipefd[1], recvBuf, BUF_SIZE) < 0) {
				perror("write to father process");
				exit(-1);
			}
		}
	}
}