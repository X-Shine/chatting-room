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
    explicit ChattingServer(const short port,const string& host = "127.0.0.1") : port_(port),host_(host) {}
    void start() {
        init();
        listen(server_,10);

        fd_set ready_fds;
        FD_SET(server_,&read_fds_);
        ready_fds = read_fds_;
        fd_max_ = server_;
        int fd_count;
        array<char,1024> buffer;
        while((fd_count = select(fd_max_ + 1,&ready_fds,nullptr,nullptr,nullptr)) > 0) {
            int fd_max_copy = fd_max_;
            for(int i = 0;i <= fd_max_copy;++i) {
                if(!FD_ISSET(i,&ready_fds))
                    continue;

                if(i == server_) {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int client_fd = accept(server_,reinterpret_cast<struct sockaddr*>(&client_addr),&addr_len);
                    if(client_fd != -1) {
                        if(client_fd > fd_max_)
                            fd_max_ = client_fd;
                        FD_SET(client_fd,&read_fds_);
                        clients_.push_back(client_fd);
                    }
                } else {
                    int ret = read(i,buffer.data(),1024);
                    const string content(buffer.data(),ret);
                    cout << content << endl;
                    if(Tool::startsWith(content,"cr")) {
                        parseCmd(i,content);
                    } else {
                        // non login user can not send content expect command
                        if(users_.find(i) != users_.end() && users_.at(i).room_) {
                            // client send msg
                            sendContentToRoom(i,content);
                        }
                    }


                }
            }

            ready_fds = read_fds_;
        }

    }

    void close() {}

private:
    void init() {
        accounts_.insert(make_pair("admin","admin"));
        accounts_.insert(make_pair("jack","jack"));
        accounts_.insert(make_pair("mike","mike"));
        server_ = socket(AF_INET,SOCK_STREAM,0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        inet_pton(AF_INET,host_.c_str(),&addr.sin_addr);
        bind(server_,reinterpret_cast<const struct sockaddr*>(&addr),sizeof(sockaddr));
    }

    void parseCmd(int fd,const string& content) {
        string parameter;
        stringstream ss(content);
        assert(ss >> parameter && parameter == string("cr"));
        ss >> parameter;
        if(parameter == "login") {
            string name,password;
            ss >> name >> password;
            if(accounts_.find(name) == accounts_.end()) {
                const string msg = "login failed! " + name + " is not existed!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            if(accounts_[name] != password) {
                const string msg = "login failed! password is not correct!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            User user = {false,name,password,nullptr};
            users_.insert(make_pair(fd,user));

            const string msg = "login success! welcome " + name + "!\r\n";
            send(fd,msg.c_str(),msg.size(),0);

        } else if(parameter == "logout") {
            users_.erase(fd);
            const string msg = "logout success! see you next time!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
            FD_CLR(fd,&read_fds_);
            ::close(fd);
        } else if(parameter == "list") {
            string msg = "available rooms:";
            for_each(rooms_.begin(),rooms_.end(),[&msg] (const pair<string,ChattingRoom>& item) {
                msg += " " + item.first;
            });
            msg += "\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "join") {
            string room_name;
            ss >> room_name;
            if(rooms_.find(room_name) == rooms_.end()) {
                return;
            }
            ChattingRoom& room = rooms_.at(room_name);
            room.addUser(fd,users_[fd].name_);
            users_[fd].room_ = &room;
            const string msg = "you join " + room_name + "!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "exit") {
            string room_name = users_[fd].room_->getName();
            users_[fd].room_->removeUser(fd);
            users_[fd].room_ = nullptr;
            const string msg = "you exit " + room_name + "!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "add") {
            string room_name;
            ss >> room_name;
            if(rooms_.find(room_name) != rooms_.end()) {
                const string msg = "room \"" + room_name + "\" already exists!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            ChattingRoom room(room_name);
            rooms_.insert(make_pair(room_name,room));
            const string msg = "room \"" + room_name + "\" create successful!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else if(parameter == "delete") {
            string room_name;
            ss >> room_name;
            if(rooms_.find(room_name) == rooms_.end()) {
                const string msg = "room \"" + room_name + "\" not exists!\r\n";
                send(fd,msg.c_str(),msg.size(),0);
                return;
            }

            rooms_.erase(room_name);
            const string msg = "room \"" + room_name + "\" delete successful!\r\n";
            send(fd,msg.c_str(),msg.size(),0);
        } else {
            // something wrong
            // format error
            assert(false);
        }
    }

    void sendContentToRoom(int fd,const string& content) {
        ChattingRoom* room = users_[fd].room_;
        string msg = users_[fd].name_ + ": " + content + "\r\n";
        room->broadcast(msg);
    }
    const short port_;
    const string host_;
    vector<int> clients_;
    map<int,User> users_;
    map<string,string> accounts_;
    map<string,ChattingRoom> rooms_;
    int server_;
    fd_set read_fds_;
    int fd_max_;
};
#endif // SERVER_HPP_