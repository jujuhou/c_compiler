#pragma once
#include "head.h"

//标识符
struct TOKEN 
{
	string token; //token类型
	string value; //值
	int row;//行
	int col;//列
};

//词法分析的类
class lexical_analysis
{
private:
	//标识符流（使用vector的原因是只需在末尾添加，查找快）
	vector<TOKEN> token_stream;

public:
	//删除默认构造函数
	lexical_analysis() = delete;
	//构造函数，输入文件路径
	lexical_analysis(const string file_path);
	//析构函数
	~lexical_analysis() {}
	//将结果输出到文件
	void print_token_stream(const string file_path);
	//得到token_stream
	vector<TOKEN> get_token_stream();
};
