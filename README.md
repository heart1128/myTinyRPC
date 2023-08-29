# myTinyRPC
学习tinyRPC项目创建的仓库，同时熟悉git一套使用
# 第一次提交修改测试



# 1、日志模块开发
```
1. 日志级别
2. 打印到文件，支持日期命名，日志的滚动(日志文件不能打印在一个文件里)。
3. c格式化风格打印（c++流式风格不好拼接字符串）
4. 支持线程安全
```

LogLevel:
```
Debug
Info
Error
```

LogEvent:
```
文件名__FILE__、行号__LINE__
MsgNo
pid
Thread id
日期,时间，精确到ms
自定义字符串
```

日志格式：
```
[%y-%m-%d %H:%M:%S.%ms]\t[pid:thread_id]\t[file_name:line][%msg]
```