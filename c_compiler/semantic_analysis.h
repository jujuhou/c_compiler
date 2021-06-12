#pragma once
#include "head.h"
#include "Generate_asm.h"


//语义分析中的符号
struct Semantic_symbol
{
	string token; //token类型
	string value; //值
	int row;//行
	int col;//列
	int table_index;//符号所在table的index
	int index;//符号在table内部的index
};

//标识符信息，即函数、变量、临时变量、常量的具体信息
struct IdentifierInfo 
{
	//几种标识符类型，分别为函数、变量、临时变量、常量、返回值
	enum IdentifierType { Function, Variable, TempVar, ConstVar, ReturnVar };

	IdentifierType identifier_type;//标识符的类型
	string specifier_type;//变(常)量类型/函数返回类型
	string identifier_name;//标识符名称/常量值
	int function_parameter_num;//函数参数个数
	int function_entry;//函数入口地址(四元式的标号)
	int function_table_index;//函数的函数符号表在整个程序的符号表列表中的索引

};

//语义分析中的符号表
struct Semantic_symbol_table 
{
	//几种表的类型，分别为全局表、函数表、块级表、临时表
	enum TableType {GlobalTable , FunctionTable , BlockTable , TempTable};

	TableType table_type;//表的类型
	vector<IdentifierInfo> table;//符号表
	string table_name;//表名

	//构造函数
	Semantic_symbol_table(const TableType table_type_,const string table_name_);
	//寻找一个变量
	int Find_symbol(const string id_name);
	//加入一个变量，返回加入的位置
	int Add_symbol(const IdentifierInfo id);
};

//语义分析
class Semantic_analysis
{
public:
	vector<Quaternary> quaternary;//四元式
	int main_index;//main函数对应的四元式标号
	int backpatching_level;//回填层次
	vector<int> backpatching_list;//回填列表
	int next_quaternary_index;//下一个四元式标号
	int tmp_var_count;//临时变量计数
	vector<Semantic_symbol> symbol_list;//语义分析过程的符号流
	vector<Semantic_symbol_table> tables;//程序所有符号表
	vector<int> current_table_stack;//当前作用域对应的符号表索引栈

	//构造函数
	Semantic_analysis();
	//将所有的符号信息放入symbol_list
	void Add_symbol_to_list(const Semantic_symbol symbol);
	//分析过程
	void Analysis(const string production_left, const vector<string> production_right);
	//打印四元式表
	void Print_quaternary(const string file_path); 

private:
	void Analysis_production_Program(const string production_left, const vector<string> production_right);
	void Analysis_production_ExtDef(const string production_left, const vector<string> production_right);
	void Analysis_production_VarSpecifier(const string production_left, const vector<string> production_right);
	void Analysis_production_FunSpecifier(const string production_left, const vector<string> production_right);
	void Analysis_production_FunDec(const string production_left, const vector<string> production_right);
	void Analysis_production_CreateFunTable_m(const string production_left, const vector<string> production_right);
	void Analysis_production_ParamDec(const string production_left, const vector<string> production_right);
	void Analysis_production_Block(const string production_left, const vector<string> production_right);
	void Analysis_production_Def(const string production_left, const vector<string> production_right);
	void Analysis_production_AssignStmt(const string production_left, const vector<string> production_right);
	void Analysis_production_Exp(const string production_left, const vector<string> production_right);
	void Analysis_production_AddSubExp(const string production_left, const vector<string> production_right);
	void Analysis_production_Item(const string production_left, const vector<string> production_right);
	void Analysis_production_Factor(const string production_left, const vector<string> production_right);
	void Analysis_production_CallStmt(const string production_left, const vector<string> production_right);
	void Analysis_production_CallFunCheck(const string production_left, const vector<string> production_right);
	void Analysis_production_Args(const string production_left, const vector<string> production_right);
	void Analysis_production_ReturnStmt(const string production_left, const vector<string> production_right);
	void Analysis_production_Relop(const string production_left, const vector<string> production_right);
	void Analysis_production_IfStmt(const string production_left, const vector<string> production_right);
	void Analysis_production_IfStmt_m1(const string production_left, const vector<string> production_right);
	void Analysis_production_IfStmt_m2(const string production_left, const vector<string> production_right);
	void Analysis_production_IfNext(const string production_left, const vector<string> production_right);
	void Analysis_production_IfStmt_next(const string production_left, const vector<string> production_right);
	void Analysis_production_WhileStmt(const string production_left, const vector<string> production_right);
	void Analysis_production_WhileStmt_m1(const string production_left, const vector<string> production_right);
	void Analysis_production_WhileStmt_m2(const string production_left, const vector<string> production_right);
	
};

