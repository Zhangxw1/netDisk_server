#include "opedb.h"
#include <QSqlQuery>//可以用来真正访问操作数据库
#include<QSqlError>
#include<QDebug>
#include<QMessageBox>

OpeDB::OpeDB(QObject *parent) : QObject(parent)
{
//init();
}

OpeDB &OpeDB::getinstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()
{
    m_db=QSqlDatabase::addDatabase("QSQLITE");

    //对于SQLite来说，主机名可能根本不重要，因为它使用的是文件。但对于MySQL或PostgreSQL这样的网络数据库，主机名是必须的,默认为localhost
    m_db.setHostName("localhost");//链接哪里的数据库写哪里的IP地址  本地则写localhost

    //只写一个cloud.db的话会在D:\QtProject\build-TcpServer-Desktop_Qt_5_12_9_MinGW_32_bit-Debug(即QDir::currentPath())里创建一个空的cloud.db文件
    m_db.setDatabaseName("D:\\QtProject\\TcpServer\\cloud.db");//设置数据库名(路径)    最好不在本地D盘,把db放到当前目录(不写死，使用QDir::currentPath())   todo..
    //想要进一步操作数据库 则要进行数据库打开操作
    if(!m_db.open())
    {
        QMessageBox::warning(nullptr,"警告","failed to open");
        qDebug()<<"Error failed to open"<<m_db.lastError();
    }else{
        qDebug()<<"open database success";
        QSqlQuery sql(m_db);
        QString strSql=QString("select * from userInfo;");
        if(!sql.exec(strSql)){
            qDebug()<<"select table error"<<sql.lastError();
        }else{
            while(sql.next()){
                QString data=QString("%1,%2,%3")
                        .arg(sql.value(0).toString())
                        .arg(sql.value(1).toString())
                        .arg(sql.value(2).toString());
                qDebug()<<data;
            }
        }
    }
}

OpeDB::~OpeDB()
{
    m_db.close();//析构函数关闭数据库
}

bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    if(NULL==name||NULL==pwd){
        return false;
    }
    QSqlQuery sql(m_db);
    QString strSql=QString("insert into userInfo(name,pwd) values('%1','%2');").arg(name).arg(pwd);
//    qDebug()<<strSql;
    return sql.exec(strSql);
}

bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    if(NULL==name||NULL==pwd){
        return false;
    }
    QSqlQuery sql(m_db);
    QString strSql=QString("select * from userInfo where name='%1' and pwd='%2' and online =0;").arg(name).arg(pwd);
    sql.exec(strSql);
    if(sql.next())//查询到未登录，即登陆成功(将这条记录的online改为1) 因为name是唯一的所以只有一条数据被查出来，不用while
    {
        strSql=QString("update userInfo set online =1 where name='%1' and pwd='%2';").arg(name).arg(pwd);
        return sql.exec(strSql);
    }else{
        return false;
    }
}

void OpeDB::handleOffline(const char *name)
{
    if(NULL==name){
        return;
    }
    QSqlQuery sql(m_db);
    QString strSql=QString("update userInfo set online =0 where name='%1';").arg(name);
    sql.exec(strSql);
}

QStringList OpeDB::handleAllOnline()
{
    QSqlQuery sql(m_db);
    QString strSql=QString("select name from userInfo where online =1;");
    QStringList l;
    l.clear();
    if(!sql.exec(strSql)){
        qDebug()<<"select table error"<<sql.lastError();
    }else{
        while(sql.next()){
            l.append(sql.value(0).toString());//QVariant value(int i) const;
        }
    }
    return l;
}


//查找返回可能不存在(-1)  存在但不在线(1)  存在且在线(0)
int OpeDB::handleSearchUsr(const char *name)
{
    if(NULL==name){
        return -1;
    }
    QSqlQuery sql(m_db);
    QString strSql=QString("select online from userInfo where name='%1';").arg(name);
    sql.exec(strSql);
    if(sql.next())//查到了--存在
    {
        int ret=sql.value(0).toInt();
        if(1==ret){
            return 1;
        }else{
            return 0;
        }
    }else//没查到--不存在
    {
        return -1;
    }
}
//                                     对方姓名             自身姓名
int OpeDB::handleAddFriend(const char *pername, const char *name)
{
    if(NULL==pername||NULL==name){
        return -1;
    }
    QSqlQuery sql(m_db);
    QString strSql=QString("select * from friendInfo where"
                           "(id=(select id from userInfo where name='%1')and friendId=(select id from userinfo where name='%2'))or"
                           "(friendId=(select id from userInfo where name='%3')and id=(select id from userinfo where name='%4'));")
                            .arg(pername).arg(name).arg(pername).arg(name);
    sql.exec(strSql);
    if(sql.next())//查到了表示他两已经是好友了
    {
        return 0;
    }
    else//不是好友
    {
        QString strSql=QString("select online from userInfo where name='%1';").arg(pername);
        sql.exec(strSql);
        if(sql.next())//查到了--存在
        {
            int ret=sql.value(0).toInt();
            if(1==ret){
                return 1;//在线
            }else{
                return 2;//不在线
            }
        }else
        {
            return 3;//没查到--不存在(点击的按道理都是存在的，只有测试的数据服务器是没有的)
        }
    }
}

void OpeDB::handleAggreeAddFriend(const char *pername, const char *name)
{
    if(NULL==pername||NULL==name){
        return;
    }
    QSqlQuery sql(m_db);
    QString strSql=QString("insert into friendInfo(id,friendId) values((select id from userInfo where name='%1'),(select id from userInfo where name='%2'));")
            .arg(pername).arg(name);
//    qDebug()<<strSql;
    sql.exec(strSql);
}

//根据用户名找到他的在线好友(根据用户名找其id 再通过id找好友id  再通过好友id找其名字)
QStringList OpeDB::handleFlushFriend(const char *name)
{
    QStringList strFriendList;
    strFriendList.clear();
    if(NULL==name){
        return strFriendList;
    }
    QSqlQuery sql(m_db);
    //1.name的id作为friendId(结果可能有多个,都存放到列表里 用in)
    QString strSql=QString("select name from userInfo where online=1 and "
                           "id in(select id from friendInfo where friendId=(select id from userInfo where name='%1'))").arg(name);
    sql.exec(strSql);
    while(sql.next()){
        strFriendList.append(sql.value(0).toString());
//        qDebug()<<sql.value(0).toString();
    }
    //2.name的id作为id(friendInfo里的)
    strSql=QString("select name from userInfo where online=1 and "
                            "id in(select friendId from friendInfo where id=(select id from userInfo where name='%1'))").arg(name);
    sql.exec(strSql);
    while(sql.next()){
        strFriendList.append(sql.value(0).toString());
//        qDebug()<<sql.value(0).toString();
    }
    return strFriendList;
    //有没有想过好友信息里 既有1 2  又有2 1，这样不就重复添加到strFriendList链表了吗？  实践显示确实
    //但是不手动添加这种是不会有双方面的情况的，因为数据库有了单方面的好友，再添加这回显示你们已经是好友
}

bool OpeDB::handleDelFriend(const char *name, const char *friendname)
{
    if(nullptr==name||nullptr==friendname){
        return false;
    }
    QSqlQuery sql(m_db);
    QString strSql=QString("delete from friendInfo where id=(select id from userInfo where name='%1') and friendId=(select id from userInfo where name='%2');").arg(name).arg(friendname);
    sql.exec(strSql);
    strSql=QString("delete from friendInfo where id=(select id from userInfo where name='%2') and friendId=(select id from userInfo where name='%1');").arg(name).arg(friendname);
    sql.exec(strSql);
    return true;
}






























