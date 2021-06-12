#pragma once
//用于处理文法的读入、生成
# include "head.h"
# include "lexical_analysis.h"

//文法中所有的符号
class grammar_symbol {
public:
	//内部成员（文法符号种类有空串、终结符、非终结符、终止符号四种）
	enum Symbol_type {Epsilon=0, Terminal, NonTerminal,End};
	Symbol_type type;//文法符号的种类
	set<int> first_set;//first集（使用set的原因是因为可以自动排序）
	set<int> follow_set;//follow集（使用set的原因是因为可以自动排序）
	string token;//符号字符串

	//构造函数
	grammar_symbol(Symbol_type type_,const string token_);
};

//文法中所有的项
class grammar_item {
public:
	int left_symbol;//产生式左边的symbol
	vector<int> right_symbol;//产生式右边的symbol
	
	//构造函数
	grammar_item(const int left_symbol_, const vector<int>& right_symbol_);
};

//整个文法
class grammar
{
public:
	const string EpsilonToken = "@";// Epsilon
	const string SplitToken = " | ";// 产生式右部分隔符
	const string ProToken= "::=";// 产生式左右部分隔符
	const string EndToken = "#";// 尾token 终止符号
	const string StartToken = "Program";// 文法起始符号
	const string ExtendStartToken = "S";// 扩展文法起始符号
	const string AllTerminalToken = "%token";//所有的终结符

	vector<grammar_symbol> symbols; //文法的所有符号集合
	set<int> terminals;//终结符在symbols中的下标（希望排序所以用set）
	set<int> non_terminals;//非终结符在symbols中的下标（希望排序所以用set）
	vector<grammar_item> productions;//所有的产生式
	int start_production;//起始产生式在productions中的位置

	//构造函数（调用read_grammar和getFirstOfNonterminal）
	grammar(const string file_path);

	//读入所有的grammar
	void read_grammar(const string file_path);
	//根据字符串找到其在symbols中的index，如果找到返回index，如果没有找到返回-1
	int find_symbol_index_by_token(const string token);
	//初始化所有的NonTerminal的first集
	void getFirstOfNonterminal();
	//返回一个符号串的first集
	set<int> getFirstOfString(const vector<int>& str);

};

