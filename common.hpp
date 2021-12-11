#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <string>
#include <cstring>

namespace Tool {
    using namespace std;
    auto startsWith = [] (const string& str,const string& head) {
        if(str.size() < head.size())
            return false;

        if(!strncmp(str.c_str(),head.c_str(),head.size()))
            return true;
        else
            return false;
    };
}

#endif // COMMON_HPP_