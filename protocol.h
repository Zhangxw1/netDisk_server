#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include<unistd.h>
#include<string.h>

typedef unsigned int uint;

#define REGIST_OK "regist ok"
#define REGIST_FAILED "regist failed :name existed"
#define LOGIN_OK "login ok"
#define LOGIN_FAILED "login failed :name error or pwd error or relogin"  //不超过64
#define SEARCH_USR_NO "No such person"
#define SEARCH_USR_ONLINE "online"
#define SEARCH_USR_OFFLINE "offline"
#define UNKNOW_ERROR "unknow error"
#define EXISTED_FRIEND "friend existed"
#define ADD_FRIEND_OFFLINE "user offline"
#define ADD_FRIEND_NOEXIST "user not exist"
#define DEL_FRIEND_OK "delete friend ok"
#define DIR_NO_EXIST "cur dir not exist"
#define FILE_NAME_EXISTED "file name existed"
#define CREATE_DIR_SUCCESS "create dir success"
#define DEL_DIR_OK "delete dir ok"
#define DEL_DIR_FAIL "delete dir fail:is regular file"
#define RENAME_FILE_OK "rename file ok"
#define RENAME_FILE_FAIL "rename file fail"
#define ENTER_DIR_OK "enter dir ok"
#define ENTER_DIR_FAIL "enter dir fail:is regular file"
#define UPLOAD_FILE_OK "upload file ok"
#define UPLOAD_FILE_FAIL "upload file fail"
#define DEL_FILE_OK "delete file ok"
#define DEL_FILE_FAIL "delete file fail:is a dir"
#define MOVE_FILE_OK "move file ok"
#define MOVE_FILE_FAIL "move file fail:is regular file"
#define COMMON_ERR "operate failed:system is busy"

enum ENUM_MSG_TYPE{
    ENUM_MSG_TYPE_MIN=0,
    ENUM_MSG_TYPE_REGIST_REQUEST,//注册请求 1(自动增长)
    ENUM_MSG_TYPE_REGIST_RESPOND,//注册回复 2

    ENUM_MSG_TYPE_LOGIN_REQUEST,//登录请求
    ENUM_MSG_TYPE_LOGIN_RESPOND,//登录回复

    ENUM_MSG_TYPE_ALL_ONLINE_REQUEST,//在线用户请求
    ENUM_MSG_TYPE_ALL_ONLINE_RESPOND,//在线用户回复

    ENUM_MSG_TYPE_SEARCH_USR_REQUEST,//搜索用户请求
    ENUM_MSG_TYPE_SEARCH_USR_RESPOND,//搜索用户回复

    ENUM_MSG_TYPE_ADD_FRIEND_REQUEST,//添加好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_RESPOND,//添加好友回复

    ENUM_MSG_TYPE_ADD_FRIEND_AGGREE,//同意添加好友
    ENUM_MSG_TYPE_ADD_FRIEND_REFUSE,//拒绝添加好友

    ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST,//刷新好友请求
    ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND,//刷新好友回复

    ENUM_MSG_TYPE_DEL_FRIEND_REQUEST,//删除好友请求
    ENUM_MSG_TYPE_DEL_FRIEND_RESPOND,//删除好友回复

    ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST,//私聊请求
    ENUM_MSG_TYPE_PRIVATE_CHAT_RESPOND,//私聊回复

    ENUM_MSG_TYPE_GROUP_CHAT_REQUEST,//群聊请求
    ENUM_MSG_TYPE_GROUP_CHAT_RESPOND,//群聊回复

    ENUM_MSG_TYPE_CREATE_DIR_REQUEST,//创建路径请求
    ENUM_MSG_TYPE_CREATE_DIR_RESPOND,//创建路径回复

    ENUM_MSG_TYPE_FLUSH_FILE_REQUEST,//刷新文件请求
    ENUM_MSG_TYPE_FLUSH_FILE_RESPOND,//刷新文件回复

    ENUM_MSG_TYPE_DEL_DIR_REQUEST,//删除目录请求
    ENUM_MSG_TYPE_DEL_DIR_RESPOND,//删除目录回复

    ENUM_MSG_TYPE_RENAME_FILE_REQUEST,//重命名文件请求
    ENUM_MSG_TYPE_RENAME_FILE_RESPOND,//重命名文件回复

    ENUM_MSG_TYPE_ENTER_DIR_REQUEST,//进入文件夹请求
    ENUM_MSG_TYPE_ENTER_DIR_RESPOND,//进入文件夹回复

    ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST,//上传文件夹请求
    ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND,//上传文件夹回复

    ENUM_MSG_TYPE_DEL_FILE_REQUEST,//删除常规文件请求
    ENUM_MSG_TYPE_DEL_FILE_RESPOND,//删除常规文件回复

    ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST,//下载文件请求
    ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND,//下载文件回复

    ENUM_MSG_TYPE_SHARE_FILE_REQUEST,//共享文件请求
    ENUM_MSG_TYPE_SHARE_FILE_RESPOND,//共享文件回复
    ENUM_MSG_TYPE_SHARE_FILE_NOTE,        //共享文件通知
    ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND,//共享文件通知回复

    ENUM_MSG_TYPE_MOVE_FILE_REQUEST,//移动文件请求
    ENUM_MSG_TYPE_MOVE_FILE_RESPOND,//移动文件回复

    ENUM_MSG_TYPE_MAX=0X00ffffff //空间大小设置为32位
};

struct FileInfo{
    char caFileName[32];//文件名字
    int iFileType; //文件类型
};


//pdu协议数据单元 protocol data unity
//弹性结构体(原理，最后一个成员为空的int数组)
struct PDU{
    uint uiPDULen;   //总的协议数据单元大小/总消息大小     (4字节大小)
    uint uiMsgType;  //消息类型(告诉对方数据是干嘛的)      (4字节大小)
    char caData[64]; //临时数组(用于存文件名)
    uint uiMsgLen;   //实际消息大小(弹性大小)             (4字节大小)

    int caMsg[];     //实际消息(弹性数组)---要打印得转char*
};

PDU *mkPDU(uint uiMsgLen);//给协议申请动态空间

#endif // PROTOCOL_H
