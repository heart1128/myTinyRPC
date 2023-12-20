#include "tinyxml/tinyxml.h"
#include <string>
#include <iostream>


// 流程就是创建xml类，插入头部信息，根，子节点和其属性
void createXml(const std::string& filename)
{
    // 1. 创建XML类
    TiXmlDocument* tinyXmlDoc = new TiXmlDocument();

    // 2. 创建头部信息(声明)，插入到XML文档中
        // 版本， 编码 ， ”“  == <?xml version="1.0" encoding="utf-8" ?>
    TiXmlDeclaration* tinyXmlDeclare = new TiXmlDeclaration("1.0", "utf-8", "");
        // 插入
    tinyXmlDoc->LinkEndChild(tinyXmlDeclare);

    // 3. 创建根节点
    TiXmlElement* root = new TiXmlElement("root");
    tinyXmlDoc->LinkEndChild(root);

    // 4. 子节点，是插入到根节点中
    TiXmlElement* child = new TiXmlElement("child");
        // 添加文本是另外的
    TiXmlText* childText = new TiXmlText("childText");
    child->LinkEndChild(childText);
    root->LinkEndChild(child);

    // 5. 带属性的子节点 如ID, name, age
    TiXmlElement* attrChild = new TiXmlElement("attrChild");
    root->LinkEndChild(attrChild);
        // 设置属性
    attrChild->SetAttribute("ID", 1);
    attrChild->SetAttribute("name", "xmlAttr");
    attrChild->SetAttribute("age", 25);
        // 创建子节点的描述（就是子节点的子节点）
    TiXmlElement* attrChildDeclare = new TiXmlElement("attrChildDeclare");
    TiXmlText* attrChildDeclareText = new TiXmlText("attrChildDeclareText");
    attrChildDeclare->LinkEndChild(attrChildDeclareText);
    attrChild->LinkEndChild(attrChildDeclare);

    TiXmlElement* attrChildLast = new TiXmlElement("attrChildLast");
    TiXmlText* attrChildLastText = new TiXmlText("attrChildLastText");
    attrChildLast->LinkEndChild(attrChildLastText);
    attrChild->LinkEndChild(attrChildLast);


    // 6. 写入xml文件
    bool rt = tinyXmlDoc->SaveFile(filename.c_str());

}

void parseXml()
{
    
}

int main()
{
    std::string filename = "myxml.xml";
    createXml(filename);
    return 0;
}