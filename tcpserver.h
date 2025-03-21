#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class TcpServer; }
QT_END_NAMESPACE

class TcpServer : public QWidget
{
    Q_OBJECT  

public:
    TcpServer(QWidget *parent = nullptr);
    void loadConfig();

    ~TcpServer();

private:
    Ui::TcpServer *ui;
    QString m_strIP;
    quint16 m_usPort;//无符号短整型端口号
};
#endif // TCPSERVER_H
