#ifndef ROOM_HPP_
#define ROOM_HPP_

#include <string>
#include <map>
#include <unistd.h>
#include <algorithm>
#include <sys/socket.h>

using namespace std;
class ChattingRoom;

struct User {
    bool is_admin_;
    string name_;
    string password_;
    ChattingRoom* room_;
};

class ChattingRoom {
public:
    ChattingRoom(const string& name) : name_(name) {}
    void broadcast(const string& msg) {
        for_each(users_.begin(),users_.end(),[&msg] (const pair<int,string>& item) {
            send(item.first,msg.c_str(),msg.size(),0);
        });
    }
    void addUser(int fd,const string& username) {
        users_.insert(make_pair(fd,username));
    }
    void removeUser(int fd) {
        if(users_.find(fd) == users_.end()) {
            return;
        }

        users_.erase(fd);
    }

    string getName() const { return name_; }
private:
    map<int,string> users_;
    string name_;
};

#endif // ROOM_HPP_