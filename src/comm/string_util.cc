#include <map>
#include <string>
#include "src/comm/string_util.h"
#include "src/comm/log.h"
#include "string_util.h"

namespace tinyrpc {

void StringUtil::splitStrToMap(const std::string &str, const std::string &split_str, 
    const std::string &joiner, std::map<std::string, std::string> &res)
{
    if (str.empty() || split_str.empty() || joiner.empty()) 
    {
        DebugLog << "str or split_str or joiner_str is empty";
        return;
    }

    std::string tmp = str;

    std::vector<std::string> vec;
    splitStrToVector(tmp, split_str, vec); // 分割成 ("a=1", "tt=2", "cc=3")
    // 接下来分割每一项变成字典
    for(const auto& it : vec)
    {
        if(!it.empty())
        {
            int i = it.find(joiner);
            if(i != it.npos && i != 0)
            {
                std::string key = it.substr(0, i);
                std::string val = it.substr(i + joiner.length(), it.length() - i - joiner.length());
                DebugLog << "splitStrToMap::insert key = " << key << ", value = " << val;
                res[key.c_str()] = val;
            }
        }
    }
}

void StringUtil::splitStrToVector(const std::string &str, const std::string &split_str, std::vector<std::string> &res)
{
    if (str.empty() || split_str.empty()) 
    {
        DebugLog << "str or split_str is empty";
        return;
    }

    std::string tmp = str;

    // 不断循环查找split_str
    int index = 0;
    int pre = index;
    while((index = tmp.find(split_str.c_str(), index + 1) != tmp.npos))
    {
        res.push_back(std::move(tmp.substr(pre, index - pre)));
        pre = index + 1;
    }
}

} // namespace tinyrpc

