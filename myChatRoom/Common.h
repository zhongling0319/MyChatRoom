#pragma once
#include <iostream>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

// 默认服务器端IP地址
#define SERVER_IP "49.232.36.240"
// 服务器端口号
#define SERVER_PORT 8888
// int epoll_create(int size)中的size
// 为epoll支持的最大句柄数
#define EPOLL_SIZE 5000
// 缓冲区大小65535
#define BUF_SIZE 1024
// 客户端昵称大小
#define NICKNAME_SIZE 10

// 新用户登录后的欢迎信息
#define SERVER_WELCOME "欢迎进入聊天室，你的ID为：%d"
// 新用户登录后广播给其他用户的信息
#define NEW_SERVER_NOTIFY "用户[%s@%d]进入聊天室"
// 用户退出后广播给其他用户的信息
#define SERVER_EXIT_NOTIFY "用户[%s@%d]退出聊天室"
// 其他用户收到消息的前缀 [昵称@ID] >> 
#define SERVER_MESSAGE "[用户%s@%d说] >> %s"	
#define SERVER_PRIVATE_MESSAGE "[用户%s@%d悄悄对你说] >> %s"
#define CLIENT_OFFLINE "对方不在线"
// 提醒你是聊天室中唯一的客户
#define CAUTION "当前聊天室仅有一人"

// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
static void addfd(int epollfd, int fd, bool enable_et)
{
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = EPOLLIN;
	if (enable_et)
		ev.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
	// 设置socket为非阻塞模式
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
	//printf("fd added to epoll!\n\n");
}

//定义信息结构，在服务端和客户端之间传送
struct Msg
{
	int isGroupChat;	//1表示群聊，0表示私聊
	int toID;
	char content[BUF_SIZE];
};