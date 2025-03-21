#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include<QTcpSocket>
#include"protocol.h"
#include"opedb.h"
#include<QFile>
#include <QTimer>

//封装一下QTcpSocket是为了区分是那个客户端发的数据(P6 35min)
class MyTcpSocket:public QTcpSocket
{
    Q_OBJECT
public:
    MyTcpSocket();
    QString getName();//定义接口获得私有成员变量m_strName
    void copyDir(QString strSrcDir,QString strDestDir);

private:
    QString m_strName;
    QFile m_file;
    qint64 m_iTotal;//总大小
    qint64 m_iRecved;//以及接收的大小
    bool m_bUpload;//判断是否为上传文件状态
    QTimer *m_pTimer;

public slots:
    void recvMsg();
    void clientOffline();
    void sendFileToClient();
signals:
    void offline(MyTcpSocket *mysocket);//发送信号可以同级目录下跨文件做connect链接
};

#endif // MYTCPSOCKET_H
