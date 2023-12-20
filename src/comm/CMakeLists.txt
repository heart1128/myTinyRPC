# set是设置变量，变量名为HEADERS
set(HEADERS
    config.h
    error_code.h
    log.h
    msg_req.h
    run_time.h
    start.h
)

# 对应make install 命令，就是将一些编译的库放到指定地方，称为安装，要使用rpc的头文件就在这个目录下找
install(FILES ${HEADERS} DESTINATION include/myTinyRpc/comm)