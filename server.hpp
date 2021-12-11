#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <array>
#include <map>
#include <cstring>
#include <sstream>
#include <cassert>
#include "common.hpp"
#include "room.hpp"


class ChattingServer {
public:
    ChattingServer() = delete;
    explicit ChattingServer(const short port,const string& host = "127.0.0.1") : m_port(port),m_host(host) {}

    void start() {
        addTestData();
        init();
        listen(m_server,10);

        fd_set ready_fds;
        FD_SET(m_server,&m_read_fds);
        ready_fds = m_read_fds;
        m_fd_max = m_server;
        array<char,1024> buffer;
        while(select(m_fd_max + 1,&ready_fds,nullptr,nullptr,nullptr) > 0) {
            int fd_max_copy = m_fd_max;
            for(int i = 0;i <= fd_max_copy;++i) {
                if(!FD_ISSET(i,&ready_fds))
                    continue;

                if(i == m_server) {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int client_fd = accept(m_server,reinterpret_cast<struct sockaddr*>(&client_addr),&addr_len);
                    if(client_fd != -1) {
                        if(client_fd > m_fd_max)
                            m_fd_max = client_fd;
                        FD_SET(client_fd,&m_read_fds);
                        m_clients.push_back(client_fd);
                    } else {
                        assert(inet_ntop(AF_INET,&client_addr.sin_addr,buffer.data(),1024));
                        perror("accept failed");
                        cout << " ;client is from" << buffer.data() << ":" << ntohs(client_addr.sin_port)  << endl;
                    }
                } else {
                    int ret = read(i,buffer.data(),1024);
                    const string content(buffer.data(),ret);
                    cout << content << endl;
                    if(Tool::startsWith(content,"cr"))
                        parseCmd(i,content);
                    else
                        if(m_users.find(i) != m_users.end() && m_users.at(i).room_)
                            // non login user can not send content expect command
                            // client send msg
                            sendContentToRoom(i,content);
                }
            }

            ready_fds = m_read_fds;
        }

    }

    void close() {}

private:
    void init() {
        m_server = socket(AF_INET,SOCK_STREAM,0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_port);
        inet_pton(AF_INET,m_host.c_str(),&addr.sin_addr);
        bind(m_server,reinterpret_cast<const struct sockaddr*>(&addr),sizeof(sockaddr));
    }

    void addTestData() {
        User user = {true,"admin","admin",nullptr};
        m_accounts.insert(make_pair("admin",user));
        User user2 = {false,"jack","jack",nullptr};
        m_accounts.insert(make_pair("jack",user2));
        User user3 = {false,"mike","mike",nullptr};
        m_accounts.insert(make_pair("mike",user3));
    }

    void parseCmd(int fd,const string& content) {
        string parameter;
        stringstream ss(content);
        assert(ss >> parameter && parameter == string("cr"));
        ss >> parameter;
        if(parameter == "register") {
            string name,password;
            ss >> name >> password;
            if(m_accounts.find(name) != m_accounts.end()) {
                const string msg = "register failed! " + name + " is existed!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            User user = {false,name,password,nullptr};
            m_accounts.insert(make_pair(name,user));
            const string msg = "register success!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "login") {
            string name,password;
            ss >> name >> password;
            if(m_accounts.find(name) == m_accounts.end()) {
                const string msg = "login failed! " + name + " is not existed!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            if(m_accounts[name].password_ != password) {
                const string msg = "login failed! password is not correct!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            User user = m_accounts[name];
            m_users.insert(make_pair(fd,user));

            const string msg = "login success! welcome " + name + "!\r\n";
            send(fd,msg.c_str(),msg.size(),0);

        } else if(parameter == "logout") {
            m_users.erase(fd);
            const string msg = "logout success! see you next time!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
            FD_CLR(fd,&m_read_fds);
            int i;
            // get the maximium number of available fd in read_set
            for(i = m_fd_max;i >= 0;--i) {
                if(FD_ISSET(i,&m_read_fds))
                    break;
            }
            m_fd_max = i;
            ::close(fd);
        } else if(parameter == "list") {
            string msg = "available rooms:";
            for_each(m_rooms.begin(),m_rooms.end(),[&msg] (const pair<string,ChattingRoom>& item) {
                msg += " " + item.first;
            });
            msg += "\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "join") {
            string room_name;
            ss >> room_name;
            if(m_rooms.find(room_name) == m_rooms.end()) {
                return;
            }
            ChattingRoom& room = m_rooms.at(room_name);
            if(!room.addUser(fd,m_users[fd].name_)) {
                const string msg = "join room " + room_name + " failed!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            m_users[fd].room_ = &room;
            const string msg = "you join " + room_name + "!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "exit") {
            string room_name = m_users[fd].room_->getName();
            if(!m_users[fd].room_->removeUser(fd)) {
                const string msg = "exit room " + room_name + " failed!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            m_users[fd].room_ = nullptr;
            const string msg = "you exit " + room_name + "!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "add") {
            if(!m_users.at(fd).is_admin_) {
                const string msg = "you are not adminstrator!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            string room_name;
            ss >> room_name;
            if(m_rooms.find(room_name) != m_rooms.end()) {
                const string msg = "room \"" + room_name + "\" already exists!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            ChattingRoom room(room_name,2);
            m_rooms.insert(make_pair(room_name,room));
            const string msg = "room \"" + room_name + "\" create successful!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "delete") {
            if(!m_users.at(fd).is_admin_) {
                const string msg = "you are not adminstrator!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            string room_name;
            ss >> room_name;
            if(m_rooms.find(room_name) == m_rooms.end()) {
                const string msg = "room \"" + room_name + "\" not exists!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            m_rooms.erase(room_name);
            const string msg = "room \"" + room_name + "\" delete successful!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else {
            const string msg = "invalid command!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        }
    }

    void sendContentToRoom(int fd,const string& content) {
        ChattingRoom* room = m_users[fd].room_;
        string msg = m_users[fd].name_ + ": " + content + "\r\n";
        room->broadcast(msg);
    }

    const short m_port;
    const string m_host;
    vector<int> m_clients;
    map<int,User> m_users;
    map<string,User> m_accounts;
    map<string,ChattingRoom> m_rooms;
    int m_server;
    fd_set m_read_fds;
    int m_fd_max;
};
#endif // SERVER_HPP_