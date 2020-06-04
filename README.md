# 我的聊天室项目

## 功能

- 实现基本的群聊功能，显示发送人（昵称@ClientId）和内容
- 实现基本的私聊功能，与指定用户单独通信

## 流程示意

![聊天室流程示意图](https://image-1301378304.cos.ap-nanjing.myqcloud.com/%E8%81%8A%E5%A4%A9%E5%AE%A4%E6%B5%81%E7%A8%8B%E7%A4%BA%E6%84%8F%E5%9B%BE.JPG)

## 补充思路

### 客户端

- ```Connect()```中需输入昵称
- ```RecvInput()```中输入\a切换群聊模式，用变量```msg.isGroupChat```记录；\ClientID切换私聊对象，```msg.toID```记录当前私聊对象

### 服务端

- 用```list<pair<int, char[10]>>``` 记录在线客户端Id及对应昵称
- ```AddNewClient()```中接受客户端发来的昵称，将ClientID和昵称添加进客户端列表

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

BUF_SIZE设置为65535时，出现一次send多次触发epoll_wait的情况，（查询recv的缓冲区大小）

运行时发现，一次send发送了65535个字节，一次recv只接收了约两万多字节

最终BUF_SIZE改为1024