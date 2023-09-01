##################################
# makefile
# heart
# 2022-08-21
##################################


# 1. 定义变量
#  = 赋值，最后一次赋值为变量的值
# := 赋值，本次赋值生效
# ?= 如果没有被赋值就被本次赋值
PATH_BIN = bin
PATH_LIB = lib
PATH_OBJ = obj

PATH_SRC = src
PATH_COMM = $(PATH_SRC)/comm
PATH_COROUTINE = $(PATH_SRC)/coroutine
PATH_NET = $(PATH_SRC)/net

PATH_TESTCASES = testcases

# 安装lib路径
PATH_INSTALL_LIB_ROOT = /usr/lib
# 安装头文件库路径
PATH_INSTALL_INC_ROOT = /usr/include

# 2. 设置编译
CXX := g++
    # 2.1 编译选项
        # -g:编译有调试信息  -O0:优化级别没有优化 -Wall:显示警告  Wno-unused-but-set-variable:不显示设置但未赋值的变量警告
CXXFLAGS += -g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable
        # 添加代码包含路径，告诉编译器去哪里找
CXXFLAGS += -I./ -I$(PATH_SRC) -I$(PATH_COMM) -I$(PATH_COROUTINE) -I$(PATH_NET)

    # 2.2 添加库文件路径
LIBS += /usr/lib/libprotobuf.a /usr/lib/libtinyxml.a


# 3.设置目标文件生成
    # wildcard ： 匹配目录下所有的.cc文件
    # patsubst ： 有三个参数 取出匹配目录下所有的.cc文件，第一个参数设置将.cc替换为第二参数的.o  最后赋值给COMM_OBJ
    # 最后的结果是指定目录下所有的.cc文件替换成.o文件给变量
COMM_OBJ := $(patsubst $(PATH_COMM)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_COMM)/*.cc))
COROUTINE_OBJ := $(patsubst $(PATH_COROUTINE)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_COROUTINE)/*.cc))
NET_OBJ := $(patsubst $(PATH_NET)/%.cc, $(PATH_OBJ)/%.o, $(wildcard $(PATH_NET)/*.cc))

COR_CTX_SWAP := coctx_swap.o

# 4. 测试文件
ALL_TESTS : $(PATH_BIN)/test_log $(PATH_BIN)/test_coroutine 

TEST_CASE_OUT := $(PATH_BIN)/test_log $(PATH_BIN)/test_coroutine

LIB_OUT := $(PATH_LIB)/libtinyrpc.a



# 5.设置编译命令
# 目标文件 ：依赖
#     	g++ FLAGS .cc -o $@表示目标文件 输出的lib 库文件 libl.o 多线程
$(PATH_BIN)/test_coroutine: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_coroutine.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(PATH_BIN)/test_log: $(LIB_OUT)
	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_log.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread

$(LIB_OUT): $(COMM_OBJ) $(COROUTINE_OBJ) $(PATH_OBJ)/coctx_swap.o $(NET_OBJ)
	cd $(PATH_OBJ) && ar rcv libtinyrpc.a *.o && cp libtinyrpc.a ../lib/

# $< : 第一个依赖文件$(PATH_COMM)/%.cc   $@目标文件(PATH_OBJ)/%.o
# 预编译 -E   编译 -S   汇编 -c  链接 -o
$(PATH_OBJ)/%.o : $(PATH_COMM)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/%.o : $(PATH_COROUTINE)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/coctx_swap.o : $(PATH_COROUTINE)/coctx_swap.S
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_OBJ)/%.o : $(PATH_NET)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@


# 6. 编译时打印的测试
# like this: make PRINT-PATH_BIN, and then will print variable PATH_BIN
PRINT-% : ; @echo $* = $($*)

# clean
clean :
	rm -f $(COMM_OBJ) $(COROUTINE_OBJ) $(NET_OBJ) $(TESTCASES) $(PATH_COROUTINE)/coctx_swap.o $(TEST_CASE_OUT)