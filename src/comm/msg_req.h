#ifndef SRC_COMM_MSG_REQ_H
#define SRC_COMM_MSG_REQ_H

#include <string>

namespace tinyrpc{

class MsgReqUtil{
public:
    static std::string genMsgNumber();

};
    
} // namespace tinyrpc


#endif