#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H
#include<QTcpServer>
#include<QList>
#include"mytcpsocket.h"//接受客户端的数据就产生一个socket(自己写),socket会发送一个readyRead信号

class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    MyTcpServer();
    //单例模式
    static MyTcpServer &getinstance();
    //重写
    virtual void incomingConnection(qintptr socketDescriptor);//有客户端连接这个服务器  会自动调用incomingConnection这个虚函数
    //做转发的函数
    void resend(const char* pername,PDU *pdu);
private:
    QList<MyTcpSocket*> m_tcpSocketList;//把所有的socket保存到这个链表里

public slots:
    void deleteSocket(MyTcpSocket *mysocket);
};

#endif // MYTCPSERVER_H
