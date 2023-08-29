#include <iostream>
#include "test_People.pb.h"
using namespace std;

int main()
{
    string People_str;
    // 1. 创建Message对象
    test_People::People people;
    // 2. 设置字段
    people.set_age(20);
    people.set_name("张三");
    // 3. 序列化，序列化后的是二进制，存入string中
    if(!people.SerializeToString(&People_str))
        cout << "序列化失败！" << endl;
    
    test_People::People people;
    // 4. 反序列化，读取序列化的二进制，反序列化出对象
    if(!people.ParseFromString(People_str))
        cout << "反序列化失败！" << endl;

}