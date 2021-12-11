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
    ChattingRoom() = delete;
    explicit ChattingRoom(const string& name,size_t max_count = 3) : m_name(name),m_max_count(max_count) {}

    // send message to all users in the room
    void broadcast(const string& msg) {
        for_each(m_users.begin(),m_users.end(),[&msg] (const pair<int,string>& item) {
            send(item.first,msg.c_str(),msg.size(),0);
        });
    }

    bool addUser(int fd,const string& username) {
        if(m_count + 1 > m_max_count || m_users.find(fd) != m_users.end())
            return false;

        m_users.insert(make_pair(fd,username));
        ++m_count;
        return true;
    }

    bool removeUser(int fd) {
        if(m_users.find(fd) == m_users.end())
            return false;

        m_users.erase(fd);
        --m_count;
        return true;
    }

    string getName() const { return m_name; }

private:
    // socket,username
    map<int,string> m_users;
    string m_name;
    const size_t m_max_count;
    size_t m_count;
};

#endif // ROOM_HPP_