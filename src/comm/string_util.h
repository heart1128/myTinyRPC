#ifndef SRC_COMM_STRING_UTIL_H
#define SRC_COMM_STRING_UTIL_H

#include <map>
#include <string>
#include <vector>

namespace tinyrpc {

class StringUtil{

public:
    // 分割请求参数为字典
    // a=1&tt=2&cc=3  -> {"a" : "1", "tt" : "2", "cc" : "3"}
    static void splitStrToMap(const std::string& str, const std::string& split_str,
        const std::string& joiner, std::map<std::string, std::string>& res);

    // for example:  str is a=1&tt=2&cc=3  split_str = '&' 
  // get res is {"a=1", "tt=2", "cc=3"}
    static void splitStrToVector(const std::string& str, const std::string& split_str,
        std::vector<std::string>& res);

};
    
} // namespace tinyrpc


#endif