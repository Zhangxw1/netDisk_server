#include "mytcpserver.h"
#include<QDebug>



MyTcpServer::MyTcpServer()
{

}


//(后面凡是需要用到MyTcpServer的地方直接通过类名调用getinstance()获得静态局部对象)
MyTcpServer &MyTcpServer::getinstance()
{
    static MyTcpServer instance;//getinstance无论调用多少次都只有这一个对象
    return instance;
}


//有客户端连接这个服务器  会自动调用incomingConnection这个虚函数，这里重写这个函数
void MyTcpServer::incomingConnection(qintptr socketDescriptor)//socketDescriptor为描述符  把描述符保存在socket里面，socket才可以收发数据
{
    qDebug()<<"new client connected";
    MyTcpSocket *pTcpSocket=new MyTcpSocket;
    pTcpSocket->setSocketDescriptor(socketDescriptor);
    m_tcpSocketList.append(pTcpSocket);
    //注销时socket要删掉
    connect(pTcpSocket,&MyTcpSocket::offline,this,&MyTcpServer::deleteSocket);//offline和deleteSocket参数即对应的单个pTcpSocket
}

void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    QList<MyTcpSocket*>::iterator iter=m_tcpSocketList.begin();
    for(;iter!=m_tcpSocketList.end();iter++){
        if(mysocket==*iter){
//            delete *iter;//先删除指针指向的对象(new出来的mysocket)
            (*iter)->deleteLater();//延迟释放空间  否则会崩溃(删除new出来的mysocket)
            *iter=NULL;
            m_tcpSocketList.erase(iter);//再删除列表里放的指针
            break;
        }
        for(int i=0;i<m_tcpSocketList.size();i++){
            qDebug()<<m_tcpSocketList.at(i)->getName();//打印注销当前用户后列表里剩余的用户名
        }
    }
}

//转发给对方(让pername来发送这个pdu)---由这个指定的人的客户端来做回复
void MyTcpServer::resend(const char* pername,PDU *pdu)
{
    if(NULL==pername||NULL==pdu){
        return;
    }
    QString strName=pername;
    for(int i=0;i<m_tcpSocketList.size();i++)
    {
        if(strName==m_tcpSocketList.at(i)->getName())
        {
            m_tcpSocketList.at(i)->write((char*)pdu,pdu->uiPDULen);//发送者为第一个参数为名字的用户客户端
            break;
        }
    }
}
