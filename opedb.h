#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase>
#include<QStringList>
class OpeDB:public QObject
{
    Q_OBJECT
public:
    OpeDB(QObject *parent=nullptr);
    static OpeDB &getinstance();
    void init();

    ~OpeDB();
    bool handleRegist(const char *name,const char *pwd);
    bool handleLogin(const char *name,const char *pwd);
    void handleOffline(const char *name);
    QStringList handleAllOnline();//返回所有在线用户的name列表
    int handleSearchUsr(const char *name);
    int handleAddFriend(const char *pername,const char *name);//(选取的用户名,自己的用户名)
    void handleAggreeAddFriend(const char *pername,const char *name);
    QStringList handleFlushFriend(const char *name);
    bool handleDelFriend(const char *name, const char *friendname);//(要删的名子,自己的用户名)


private:
    QSqlDatabase m_db;//用来连接数据库

};

#endif // OPEDB_H
