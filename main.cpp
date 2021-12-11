/*
 * 注册模块
 * 登录模块
 * 管理（创建，删除，查询）聊天室
 * 加入，退出，查询聊天室
 * 有人数限制，能看到其它人的对话
 * */

#include "server.hpp"

int main(int argc,char* argv[]) {
    ChattingServer server(atoi(argv[1]));
    server.start();
    return 0;
}
