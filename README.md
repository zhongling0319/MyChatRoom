# 我的聊天室项目

Linux下基于TCP/IP协议的即时通讯

## 功能

- 实现基本的群聊功能，显示发送人（昵称@ClientId）和内容
- 实现基本的私聊功能，与指定用户单独通信

## 流程示意


![聊天室流程示意图](https://image-1301378304.cos.ap-nanjing.myqcloud.com/%E8%81%8A%E5%A4%A9%E5%AE%A4%E6%B5%81%E7%A8%8B%E7%A4%BA%E6%84%8F%E5%9B%BE.JPG)

## 技术应用

- 通过socket编程实现服务端与客户端的网络通信
- 通过epoll实现一对多的数据交互（服务端同时对多个客户端连接的监听，客户端同时对服务端连接和子进程管道消息的监听）
- 通过管道实现客户端父进程与子进程的通信

## 实现思路

### 客户端Client

- 与服务端建立链接
- 创建管道
- 将socket与管道描述符添加进epoll事件表

- 创建子进程
- 在子进程中接收用户输入并通过管道发送给父进程
- 在父进程中接收子进程的消息并发送给服务端，以及接收服务端的消息显示在屏幕上

### 服务端Server

- 建立监听socket并添加进epoll事件表，等待客户端连接
- 客户端连接后，将对应socket添加进epoll事件表，监听客户端消息
- 用```list<pair<int, char[10]>>``` 记录在线客户端Id及对应昵称
- 接收到客户端消息后，根据内容判断群聊与私聊，并进行相应的转发
- 客户端断开连接，关闭对应socket，并从客户端链表中删除

### 传输数据的结构

```C++
struct Msg
{
	int isGroupChat;	//1是群聊，0是私聊
	int toID;
	char content[BUF_SIZE];
};
```

传输数据时，结构体与字符串数组相互转化

```c++
memcpy(buf, &msg, BUF_SIZE)
```

### 缓冲区大小问题

BUF_SIZE设置为65535时，出现一次send多次触发epoll_wait的情况

运行时发现，一次send发送了65535个字节，一次recv只接收了约两万多字节

最终BUF_SIZE改为1024

## 效果截图

### 服务端界面

![服务端截图](https://image-1301378304.cos.ap-nanjing.myqcloud.com/%E6%9C%8D%E5%8A%A1%E7%AB%AF%E6%88%AA%E5%9B%BE.png)

![服务端截图2](https://image-1301378304.cos.ap-nanjing.myqcloud.com/%E6%9C%8D%E5%8A%A1%E7%AB%AF%E6%88%AA%E5%9B%BE2.png)

### 客户端界面

![客户端截图1](https://image-1301378304.cos.ap-nanjing.myqcloud.com/%E8%81%8A%E5%A4%A9%E6%88%AA%E5%9B%BE1.png)

![客户端截图2](https://image-1301378304.cos.ap-nanjing.myqcloud.com/%E8%81%8A%E5%A4%A9%E6%88%AA%E5%9B%BE2.png)

![客户端截图3](https://image-1301378304.cos.ap-nanjing.myqcloud.com/%E8%81%8A%E5%A4%A9%E6%88%AA%E5%9B%BE3.png)

