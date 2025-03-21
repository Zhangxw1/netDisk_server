#include "tcpserver.h"
#include "ui_tcpserver.h"

#include"mytcpserver.h"
#include<QByteArray>
#include<QDebug>
#include<QMessageBox>
#include<QHostAddress>
#include<QFile>


TcpServer::TcpServer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpServer)
{
    ui->setupUi(this);
    loadConfig();
    MyTcpServer::getinstance().listen(QHostAddress(m_strIP),m_usPort);//指定ip和端口进行监听
}

//加载配置文件
void TcpServer::loadConfig()
{
    QFile file(":/server.config");
    if(file.open(QIODevice::ReadOnly)){
    QByteArray baData=file.readAll();
    QString strData=baData.toStdString().c_str();
    file.close();
    strData.replace("\r\n"," ");
    QStringList strList=strData.split(" ");
    m_strIP=strList[0];
    m_usPort=strList[1].toUShort();
    qDebug()<<"ip:"<<m_strIP<<" port:"<<m_usPort;

    }else{
        QMessageBox::critical(this,"open config","open congif failed");
    }

}

TcpServer::~TcpServer()
{
    delete ui;
}

