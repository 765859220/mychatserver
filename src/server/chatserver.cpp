#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <iostream>
#include <functional>
#include <string>
#include <string.h>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开链接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string enc = buffer->retrieveAllAsString();
    cout << "密文" << enc << endl;
    int  decLen = 0;
    char enc1[1024];
    strcpy(enc1, enc.c_str());
    //cout << "解密前长度" << strlen(enc1) << endl;
    int encLen = enc.size();
    char* buf1 = (char*)_aes->Decrypt(enc1, encLen, decLen);
    //cout << "解密后长度" << strlen(buf1) << endl;
    //cout << "明文1" << buf1 << endl;
    string buf = buf1;
    // 测试，添加json打印代码
    cout << "明文" << buf << endl;
    auto index = buf.find('}');
    if(index != -1) {
        buf = buf.substr(0, index + 1);
    }
    cout << "裁剪后明文" << buf << endl;

    // 数据的反序列化
    json js = json::parse(buf);
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取=》业务handler=》conn  js  time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}