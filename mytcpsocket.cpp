#include "mytcpsocket.h"
#include<QDebug>
#include"mytcpserver.h"
#include<QDir>//包含所有对文件夹的操作
#include<QFileInfoList>

MyTcpSocket::MyTcpSocket():m_bUpload(false)
{
    connect(this,&MyTcpSocket::readyRead,this,&MyTcpSocket::recvMsg);//那个socket来的数据(发出readyRead信号)，这个socket本身来进行接收数据
    //    connect(this,SIGNAL(readyRead()),this,SLOT(recvMsg()));
    connect(this,&MyTcpSocket::disconnected,this,&MyTcpSocket::clientOffline);//客户端挂掉了会发出disconnected信号
    m_pTimer=new QTimer;
    connect(m_pTimer,&QTimer::timeout,this,&MyTcpSocket::sendFileToClient);
}

QString MyTcpSocket::getName()
{
    return m_strName;
}

//                                 源文件夹            目的文件夹
void MyTcpSocket::copyDir(QString strSrcDir, QString strDestDir)
{
    QDir dir;//默认为当前工作目录
    dir.mkdir(strDestDir);//当前工作目录创建目的文件夹
    dir.setPath(strSrcDir);//设置dir对象所代表的目录路径为源文件夹的路径
    QFileInfoList fileInfoList=dir.entryInfoList();//entryInfoList()把目录下的文件以列表形式取出来

    QString srcTmp;
    QString destTmp;
    for(int i=0;i<fileInfoList.size();i++){
        qDebug()<<"filename:"<<fileInfoList[i].fileName();
        //文件夹里面是文件
        if(fileInfoList[i].isFile()){
            srcTmp=strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp=strDestDir+'/'+fileInfoList[i].fileName();
            QFile::copy(srcTmp,destTmp);
        }
        //文件夹里面还是文件夹(则递归调用这个函数)
        else if(fileInfoList[i].isDir()){
            if(QString(".")==fileInfoList[i].fileName()||QString("..")==fileInfoList[i].fileName()){
                continue;
            }
            srcTmp=strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp=strDestDir+'/'+fileInfoList[i].fileName();
            copyDir(srcTmp,destTmp);
        }
    }
}

void MyTcpSocket::recvMsg()
{
    if(!m_bUpload){
        qDebug()<<this->bytesAvailable();//打印可读数据的大小(客户端发送来的数据大小--有时候会混乱)
        uint uiPDULen=0;
        this->read((char*)&uiPDULen,sizeof(uint));//先读4个字节的数据(也就是先获取总的大小)
        uint uiMsgLen=uiPDULen-sizeof(PDU);//计算实际消息长度
        PDU *pdu=mkPDU(uiMsgLen);//通过实际消息长度产生一个pdu  来接受剩余的数据
        this->read((char*)pdu+sizeof(uint),uiPDULen-sizeof(uint));//(偏移)接受剩余的数据
        //    qDebug()<<pdu->uiMsgType<<(char *)(pdu->caMsg);  //测试

        switch (pdu->uiMsgType) {
        case ENUM_MSG_TYPE_REGIST_REQUEST://注册请求
        {
            char caName[32]={'\0'};
            char caPwd[32]={'\0'};
            strncpy(caName,pdu->caData,32);
            strncpy(caPwd,pdu->caData+32,32);
            //        qDebug()<<caName<<caPwd<<pdu->uiMsgType;
            bool ret=OpeDB::getinstance().handleRegist(caName,caPwd);

            PDU *respdu=mkPDU(0);//不需要申请额外的空间(实际消息传0)
            respdu->uiMsgType=ENUM_MSG_TYPE_REGIST_RESPOND;//注册回复
            if(ret){
                strcpy(respdu->caData,REGIST_OK);
                //注册成功创建文件夹(前面注册的需要手动加用户名同名文件夹)
                QDir dir;
                //bool mkdir(const QString &路径)
                dir.mkdir(QString("./%1").arg(caName));//D:\QtProject\build-TcpServer-Desktop_Qt_5_12_9_MinGW_32_bit-Debug下创建名为caName的目录
            }else{
                strcpy(respdu->caData,REGIST_FAILED);
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_REQUEST://登录请求
        {
            char caName[32]={'\0'};
            char caPwd[32]={'\0'};
            strncpy(caName,pdu->caData,32);
            strncpy(caPwd,pdu->caData+32,32);
            bool ret=OpeDB::getinstance().handleLogin(caName,caPwd);

            PDU *respdu=mkPDU(0);//不需要申请额外的空间(实际消息传0)
            respdu->uiMsgType=ENUM_MSG_TYPE_LOGIN_RESPOND;//登录回复
            if(ret){
                strcpy(respdu->caData,LOGIN_OK);
                m_strName=caName;
            }else{
                strcpy(respdu->caData,LOGIN_FAILED);
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST://在线用户请求
        {
            QStringList ret=OpeDB::getinstance().handleAllOnline();
            uint uiMsgLen=ret.size()*32;//一个名字占32字节
            PDU *respdu=mkPDU(uiMsgLen);//(传uiMsgLen实际消息长)
            respdu->uiMsgType=ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;//在线用户回复
            //将用户名循环拷贝进respdu(偏移)
            for (int i=0;i<ret.size();i++){
                memcpy((char*)(respdu->caMsg)+i*32,ret.at(i).toStdString().c_str(),ret.at(i).size());//memcpy(目的地址,源地址,大小)
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST://查找用户请求
        {
            int ret=OpeDB::getinstance().handleSearchUsr(pdu->caData);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_SEARCH_USR_RESPOND;//查找用户回复
            if(-1==ret){
                strcpy(respdu->caData,SEARCH_USR_NO);
            }else if(1==ret){
                strcpy(respdu->caData,SEARCH_USR_ONLINE);
            }else if(0==ret){
                strcpy(respdu->caData,SEARCH_USR_OFFLINE);
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST://添加用户请求
        {
            char caPerName[32]={'\0'};//对方名字
            char caName[32]={'\0'};//自己名字
            strncpy(caPerName,pdu->caData,32);
            strncpy(caName,pdu->caData+32,32);
            int ret=OpeDB::getinstance().handleAddFriend(caPerName,caName);
            PDU *respdu=nullptr;

            //为什么重复的要写好多遍，为什么不写在外面?
            /*
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
                free(respdu);
                respdu=NULL;
            */

            if(-1==ret){//未知错误
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,UNKNOW_ERROR);
                write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
                free(respdu);
                respdu=NULL;
            }else if(0==ret){//已经是好友了
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,EXISTED_FRIEND);
                write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
                free(respdu);
                respdu=NULL;
            }else if(1==ret){//不是好友且在线
                //获得对方的socket让他转发(resend)，由对方客户端做出回应
                MyTcpServer::getinstance().resend(caPerName,pdu);//这个pdu的消息类型还是ENUM_MSG_TYPE_ADD_FRIEND_REQUEST

            }else if(2==ret){//不是好友但不在线
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,ADD_FRIEND_OFFLINE);
                write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
                free(respdu);
                respdu=NULL;
            }else if(3==ret){//点击的人数据库没有(不存在,是测试数据)
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,ADD_FRIEND_NOEXIST);
                write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
                free(respdu);
                respdu=NULL;
            }
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_AGGREE://同意添加
        {
            // 获取客户端传输过来的好友名称
            char caFriendName[32] = {'\0'};
            char caLoginName[32] = {'\0'};
            // 前32位是好友名称，后32位是登录者名称
            strncpy(caFriendName, pdu->caData, 32);
            strncpy(caLoginName, pdu->caData + 32, 32);
            // 把他们两个变为好友
            OpeDB::getinstance().handleAggreeAddFriend(caFriendName, caLoginName);
            // 转发消息，添加好友成功(又给申请人的客户端回馈)
            MyTcpServer::getinstance().resend(caLoginName, pdu);//pdu的类型还是ENUM_MSG_TYPE_ADD_FRIEND_AGGREE
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE://拒绝添加
        {
            // 获取客户端传输过来的好友名称
            char caLoginName[32] = {'\0'};
            // 前32位是好友名称，后32位是登录者名称
            strncpy(caLoginName, pdu->caData + 32, 32);
            // 转发消息，申请加好友失败
            MyTcpServer::getinstance().resend(caLoginName, pdu);
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST://刷新好友请求
        {
            char caName[32]={'\0'};//自己名字
            strncpy(caName,pdu->caData,32);
            QStringList ret=OpeDB::getinstance().handleFlushFriend(caName);
            uint uiMsgLen=ret.size()*32;
            PDU *respdu=mkPDU(uiMsgLen);
            respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
            for(int i=0;i<ret.size();i++){//char类型+1偏移一个字节  而int+1偏移4个字节
                memcpy((char*)(respdu->caMsg)+i*32,ret.at(i).toStdString().c_str(),ret.at(i).size());//名字拷贝到pdu返回给客户端
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_DEL_FRIEND_REQUEST://删除好友请求
        {
            char caFriendName[32] = {'\0'};
            char caLoginName[32] = {'\0'};
            //存放顺序与客户端发来的要一致
            strncpy(caLoginName, pdu->caData, 32);
            strncpy(caFriendName, pdu->caData + 32, 32);
            bool ret=OpeDB::getinstance().handleDelFriend(caLoginName,caFriendName);//数据库删掉好友关系，后面给消息提示
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_DEL_FRIEND_RESPOND;
            strcpy(respdu->caData,DEL_FRIEND_OK);//提示信息放caData里面(提示删除人删除成功)
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;

            //转发给被删除人caFriendName
            MyTcpServer::getinstance().resend(caFriendName,pdu);//这个pdu的消息类型还是ENUM_MSG_TYPE_DEL_FRIEND_REQUEST
            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST://私聊请求
        {
            char caPerName[32]={'\0'};
            memcpy(caPerName,pdu->caData+32,32);//提取对方名字
            //        qDebug()<<caPerName;
            MyTcpServer::getinstance().resend(caPerName,pdu);//给对方做转发
            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST://群聊请求
        {
            char caName[32]={'\0'};//自己名字
            strncpy(caName,pdu->caData,32);
            //查询出在线好友
            QStringList onlineFriend=OpeDB::getinstance().handleFlushFriend(caName);
            onlineFriend.append(caName);//自己也添加进去，自己的群聊消息框也显示
            QString tmp;
            //给每个在线的好友转发消息
            for(int i=0;i<onlineFriend.size();i++){
                tmp=onlineFriend.at(i);
                MyTcpServer::getinstance().resend(tmp.toStdString().c_str(),pdu);//这个pdu的消息类型还是ENUM_MSG_TYPE_GROUP_CHAT_REQUEST
            }
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST://创建路径请求
        {
            QDir dir;
            QString strCurPath=QString("%1").arg((char*)(pdu->caMsg));//QString多余？
            bool ret=dir.exists(strCurPath);
            PDU *respdu;
            if(ret){
                char caNewDir[32]={'\0'};
                memcpy(caNewDir,pdu->caData+32,32);
                QString strNewPath=strCurPath+ "/" +caNewDir;
                qDebug()<<strNewPath;
                ret=dir.exists(strNewPath);
                if(ret)//创建的文件名已经存在，重名
                {
                    respdu=mkPDU(0);
                    respdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData,FILE_NAME_EXISTED);
                }else{//不存在则创建目录
                    dir.mkdir(strNewPath);
                    respdu=mkPDU(0);
                    respdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData,CREATE_DIR_SUCCESS);
                }
            }
            else//没有用户名同名文件
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strcpy(respdu->caData,DIR_NO_EXIST);
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST://刷新文件请求
        {
            char *pCurPath=new char[pdu->uiMsgLen];
            memcpy(pCurPath,pdu->caMsg,pdu->uiMsgLen);
            QDir dir(pCurPath);//pCurPath一开始为./登录名  而.表示D:\QtProject\build-TcpServer-Desktop_Qt_5_12_9_MinGW_32_bit-Debug
            //即若登录为jack,pCurPath为 D:\QtProject\build-TcpServer-Desktop_Qt_5_12_9_MinGW_32_bit-Debug/jack(Qt 的 QDir、QFile 等类会自动将 / 转换为适合目标平台的分隔符)
            QFileInfoList fileInfoList=dir.entryInfoList();//entryInfoList()把目录下的文件以列表形式取出来
            int iFileCount=fileInfoList.size();
            PDU *respdu=mkPDU(sizeof(FileInfo)*iFileCount);
            respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
            FileInfo *pFileInfo=NULL;//FileInfo为定义的结构体
            QString strFileName;
            for(int i=0;i<iFileCount;i++){
                pFileInfo=(FileInfo*)(respdu->caMsg)+i;//每循环一次偏移一个FileInfo结构体大小
                strFileName=fileInfoList[i].fileName();
                memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());//拷贝文件名
                //拷贝文件类型
                if(fileInfoList[i].isDir()){
                    pFileInfo->iFileType=0;//表示目录
                }else if(fileInfoList[i].isFile()){
                    pFileInfo->iFileType=1;//表示常规文件
                }
                qDebug()<<fileInfoList[i].fileName()
                       <<fileInfoList[i].size()
                      <<"是否为文件夹:"<<fileInfoList[i].isDir()
                     <<"常规文件"<<fileInfoList[i].isFile();
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_DEL_DIR_REQUEST://删除目录请求
        {
            char caName[32]={'\0'};
            strcpy(caName,pdu->caData);//拷贝要删除的文件名
            char *pPath=new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);//拷贝要当前路径
            QString strPath=QString("%1/%2").arg(pPath).arg(caName);
            //判断选中的是不是一个路径
            QFileInfo fileInfo(strPath);
            bool ret=false;
            if(fileInfo.isDir()){
                QDir dir;
                dir.setPath(strPath);//这两行等价于QDir dir(strPath);   都是设置路径为strPath,若不指定则默认为当前工作目录
                ret=dir.removeRecursively();//bool removeRecursively()删除这个路径并且包含它里面的所有内容(递归删除)
            }
            else if(fileInfo.isFile())//常规文件
            {
                ret=false;
            }
            PDU *respdu=NULL;
            if(ret)//删除成功
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData,DEL_DIR_OK,strlen(DEL_DIR_OK));
            }else{
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData,DEL_DIR_FAIL,strlen(DEL_DIR_FAIL));
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_REQUEST://重命名文件请求
        {
            char caOldName[32]={'\0'};
            char caNewName[32]={'\0'};
            memcpy(caOldName,pdu->caData,32);
            memcpy(caNewName,pdu->caData+32,32);
            char *pPath=new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);//拷贝要当前路径
            QString strOldPath=QString("%1/%2").arg(pPath).arg(caOldName);
            QString strNewPath=QString("%1/%2").arg(pPath).arg(caNewName);

            QDir dir;
            bool ret=dir.rename(strOldPath,strNewPath);//bool rename(const QString &oldName, const QString &newName);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
            if(ret){
                strcpy(respdu->caData,RENAME_FILE_OK);
            }else{
                strcpy(respdu->caData,RENAME_FILE_FAIL);

            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST://进入目录请求
        {
            char caEnterName[32]={'\0'};
            memcpy(caEnterName,pdu->caData,32);
            char *pPath=new char[pdu->uiMsgLen];
            memcpy(pPath,(char*)pdu->caMsg,pdu->uiMsgLen);//拷贝要当前路径

            QString strPath=QString("%1/%2").arg(pPath).arg(caEnterName);//拼接成要进入的路径
            qDebug()<<strPath;
            QFileInfo fileInfo(strPath);
            PDU *respdu=NULL;
            if(fileInfo.isDir())
            {
                //若为目录则还需要获得这个目录下其他文件的信息
                QDir dir(strPath);
                QFileInfoList fileInfoList=dir.entryInfoList();//entryInfoList()把目录下的文件以列表形式取出来
                int iFileCount=fileInfoList.size();
                respdu=mkPDU(sizeof(FileInfo)*iFileCount);
                respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;//消息类型就用更新的
                FileInfo *pFileInfo=NULL;
                QString strFileName;
                for(int i=0;i<iFileCount;i++){
                    pFileInfo=(FileInfo*)(respdu->caMsg)+i;//每循环一次偏移一个FileInfo结构体大小(转发FileInfo结构体信息)
                    strFileName=fileInfoList[i].fileName();
                    memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());//拷贝文件名
                    //拷贝文件类型
                    if(fileInfoList[i].isDir()){
                        pFileInfo->iFileType=0;//表示目录
                    }else if(fileInfoList[i].isFile()){
                        pFileInfo->iFileType=1;//表示常规文件
                    }
                }
                write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
                free(respdu);
                respdu=NULL;
            }else if(fileInfo.isFile())
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                memcpy(respdu->caData,ENTER_DIR_FAIL,strlen(ENTER_DIR_FAIL));
                write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
                free(respdu);
                respdu=NULL;
            }
            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST://上传文件请求
        {
            char caFileName[32]={'\0'};
            qint64 fileSize=0;//文件大小
            //sprintf放进去的  用sscanf拿出来  sscanf从字符串str中解析数据int sscanf(const char *str, const char *format, ...);
            sscanf(pdu->caData,"%s %lld",caFileName,&fileSize);
            char *pPath=new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);//拷贝要当前路径
            QString strPath=QString("%1/%2").arg(pPath).arg(caFileName);//拼接(给服务器的路径)
            qDebug()<<strPath;
            delete []pPath;
            pPath=nullptr;
            m_file.setFileName(strPath);//使QFile对象关联strPath文件    <==>QFile m_file(strPath)
            //以只写的方式打开文件，若文件不存在，则会自动创建文件
            //可改为多线程
            if(m_file.open(QIODevice::WriteOnly))
            {
                m_bUpload=true;
                m_iTotal=fileSize;
                m_iRecved=0;
            }
            break;
        }
        case ENUM_MSG_TYPE_DEL_FILE_REQUEST://删除文件请求
        {
            char caName[32]={'\0'};
            strcpy(caName,pdu->caData);//拷贝要删除的文件名
            char *pPath=new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);//拷贝要当前路径
            QString strPath=QString("%1/%2").arg(pPath).arg(caName);
            //判断选中的是不是一个文件
            QFileInfo fileInfo(strPath);
            bool ret=false;
            if(fileInfo.isDir()){
                ret=false;
            }
            else if(fileInfo.isFile())//常规文件
            {
                QDir dir;//dir默认为当前工作目录
                ret=dir.remove(strPath);//bool remove(const QString &fileName);

            }
            PDU *respdu=NULL;
            if(ret)//删除成功
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData,DEL_FILE_OK,strlen(DEL_FILE_OK));
            }else{
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData,DEL_FILE_FAIL,strlen(DEL_FILE_FAIL));
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST://下载文件请求
        {
            char caFileName[32]={'\0'};
            strcpy(caFileName,pdu->caData);//拷贝要下载的文件
            char *pPath=new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);//拷贝要当前路径
            QString strPath=QString("%1/%2").arg(pPath).arg(caFileName);//拼接
            delete []pPath;
            pPath=nullptr;

            //获得文件大小
            QFileInfo fileInfo(strPath);
            qint64 fileSize=fileInfo.size();
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType =ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
            sprintf(respdu->caData,"%s %lld",caFileName,fileSize);//文件名和文件的字节大小放caData里
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            m_file.setFileName(strPath);
            m_file.open(QIODevice::ReadOnly);//只读方式打开文件(然后写字节流数据)
            m_pTimer->start(1000);//给客户端回复文件信息(文件名和大小)1s之后给客户端发送内容
            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST://共享文件请求
        {
            char caSendName[32]={'\0'};
            int num=0;
            sscanf(pdu->caData,"%s %d",caSendName,&num);
            int size=num*32;
            PDU *respdu=mkPDU(pdu->uiMsgLen-size);
            respdu->uiMsgType=ENUM_MSG_TYPE_SHARE_FILE_NOTE;
            //(拷贝 发送人和 分享文件的路径,这里不需要其他被分享的人了)
            strcpy(respdu->caData,caSendName);//发送人拷贝到respdu的caData
            memcpy(respdu->caMsg,(char*)(pdu->caMsg)+size,pdu->uiMsgLen-size);//分享文件路径拷贝到respdu的caMsg
            char caRecvName[32]={'\0'};
            //得到接受者的名字进行转发(to接收者)
            for(int i=0;i<num ;i++){
                memcpy(caRecvName,(char*)(pdu->caMsg)+i*32,32);
                MyTcpServer::getinstance().resend(caRecvName,respdu);
            }
            free(respdu);
            respdu=nullptr;

            //给发送者回复
            respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            strcpy(respdu->caData,"share file ok");//发送人拷贝到respdu的caData
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND://分享文件通知回复(服务器做拷贝分享文件工作)
        {
            QString strRecvPath=QString("./%1").arg(pdu->caData);
            QString strShareFilePath=QString("%1").arg((char*)pdu->caMsg);
            int index=strShareFilePath.lastIndexOf('/');
            QString strFileName=strShareFilePath.right(strShareFilePath.size()-index-1);
            strRecvPath=strRecvPath+'/'+strFileName;
            QFileInfo fileInfo(strShareFilePath);
            if(fileInfo.isFile()){
                QFile::copy(strShareFilePath,strRecvPath);//static bool copy(const QString &fileName, const QString &newName);
            }else if(fileInfo.isDir()){
                copyDir(strShareFilePath,strRecvPath);
            }

            break;
        }
        case ENUM_MSG_TYPE_MOVE_FILE_REQUEST://移动文件请求
        {
            char caFileName[32]={'\0'};
            int srcLen=0;
            int destLen=0;
            sscanf(pdu->caData,"%d %d %s",&srcLen,&destLen,caFileName);//取出原路径(带文件名)大小和目的路径大小 和 文件名

            char *pSrcPath=new char[srcLen+1];
            char *pDestPath=new char[destLen+1+32];//加32是因为后面还要拼接文件名
            //初始化这两块空间 void *memset(void *s, int c, size_t n);
            memset(pSrcPath,'\0',srcLen+1);//给pSrcPath在srcLen+1大小下设置值为\0(清空)
            memset(pDestPath,'\0',destLen+1);
            //拷贝
            memcpy(pSrcPath,pdu->caMsg,srcLen);
            memcpy(pDestPath,(char*)(pdu->caMsg)+(srcLen+1),destLen);
            PDU *respdu=mkPDU(0);//反馈消息放在caData即可 无需caMsg
            respdu->uiMsgType=ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
            //判断要移动的位置是否为一个文件夹
            QFileInfo fileInfo(pDestPath);
            if(fileInfo.isDir()){
                strcat(pDestPath,"/");//c语言的拼接函数 char *strcat(char * Dest,const char * Source);
                strcat(pDestPath,caFileName);
                bool ret=QFile::rename(pSrcPath,pDestPath);//bool QFile::rename(const QString &oldName, const QString &newName);重命名或移动函数
                //              oldName：原始文件名或路径，即你想要重命名或移动的文件当前的名称和路径。
                //              newName：新文件名或路径，即你希望将文件重命名或移动到的目标名称和路径
                if(ret)//移动成功
                {
                    strcpy(respdu->caData,MOVE_FILE_OK);
                }
                else//移动失败(提示系统繁忙，真实开发需要解决这些问题)
                {
                    strcpy(respdu->caData,COMMON_ERR);
                }
            }
            else if(fileInfo.isFile())//是常规文件
            {
                strcpy(respdu->caData,MOVE_FILE_FAIL);
            }
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
            break;
        }

        default:
            break;
        }
        free(pdu);
        pdu=NULL;
    }
    else//是上传文件的状态(则接收数据)
    {
        PDU *respdu=mkPDU(0);
        respdu->uiMsgType=ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;


        //读写操作
        QByteArray buff=readAll();//接受读到的数据
        m_file.write(buff);//数据写到m_file文件里面


        m_iRecved+=buff.size();
        if(m_iTotal==m_iRecved)
        {
            m_file.close();
            m_bUpload=false;
            memcpy(respdu->caData,UPLOAD_FILE_OK,strlen(UPLOAD_FILE_OK));
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
        }else if(m_iTotal<m_iRecved){
            m_file.close();
            m_bUpload=false;
            memcpy(respdu->caData,UPLOAD_FILE_FAIL,strlen(UPLOAD_FILE_FAIL));
            write((char*)respdu,respdu->uiPDULen);//respdu发送给客户端  然后释放
            free(respdu);
            respdu=NULL;
        }
        //收完数据or接受出错才给回复，不能没收完就给回复

    }
}

//客户端下线
void MyTcpSocket::clientOffline()
{
    OpeDB::getinstance().handleOffline(m_strName.toStdString().c_str());//QString转char*类型(先转换成c++的string再取首地址)
    emit offline(this);//发送删除这个socket的信号
}

//把内容发给请求的客户端
void MyTcpSocket::sendFileToClient()
{
    char *pData=new char[4096];
    qint64 ret=0;
    //循环读取文件内容发送给客户端
    while(true)
    {
        ret=m_file.read(pData,4096);//qint64 read(char *data, qint64 maxlen);读到pData  若没有4096，有多少读多少，返回实际读到的字节数
        if(ret>0&&ret<=4096){
            write(pData,ret);//qint64 write(const char *data, qint64 len);
        }
        else if(0==ret)//读完了
        {
            m_file.close();
            break;
        }
        else if(ret<0){
            qDebug()<<"发送文件内容给客户端过程失败";
            m_file.close();
            break;
        }
    }
    delete []pData;
    pData=nullptr;
}






