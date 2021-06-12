#pragma once
#include"grammar.h"
#include "semantic_analysis.h"
//一个LR1的项
class LR1_item {
public:
	//前两项同grammar_item，直接复制过来
	int left_symbol;//产生式左边的symbol
	vector<int> right_symbol;//产生式右边的symbol

	int index;//在grammar_item（productions）中的index
	int dot_position;//点的位置
	int lookahead_symbol;//展望符在symbols中的下标

	//构造函数
	LR1_item(const int left_symbol_, const vector<int>& right_symbol_, const int index_, const int dot_position_, const int lookahead_symbol_);
	//判断相等
	bool operator==(const LR1_item& item);
};

//一个LR1的closure
class LR1_closure {
public:
	vector<LR1_item> lr1_closure;

	bool operator==(const LR1_closure& closure);
};


//分析过程中的动作
enum Action {
	ShiftIn, // 移入
	Reduce,  // 归约
	Accept,  // 接受
	Error	// 错误
};
//具体的动作信息
struct ActionInfo {
	Action action;	//动作
	int info;	//归约产生式或转移状态
};

//LR1文法
class LR1 :public grammar {
public:
	//整个项集族
	vector<LR1_closure> lr1_cluster;
	//goto信息记录表，三个int为：当前closure在lr1_cluster中的标号，当前符号在symbols中标号，转移到的closure在lr1_cluster中的标号
	map<pair<int, int>, int> goto_info;
	//GOTO表，GOTO[i,A]=j，只用到Action Error(表示未定义)和ShiftIn(表示转移)
	map<pair<int, int>, ActionInfo>goto_table;
	//ACTION表，ACTION[i, A] = "移入/规约/接受"
	map<pair<int, int>, ActionInfo>action_table;
	//语义分析器
	Semantic_analysis symantic_analysis;

	//构造函数
	LR1(const string file_path);
	//生成LR1项集
	void generateCluster();
	//生成闭包
	LR1_closure& generateClosure(LR1_closure& initial_closure);
	//生成GOTO的closure
	LR1_closure generateGOTO(const LR1_closure& from_closure, int present_symbol);
	//生成Table
	void generateTable();
	//打印Table
	void printTable(const string file_path);
	//进行语法分析和语义分析
	void parser(vector<TOKEN> token_stream, const string file_path);
	//绘制语法树
	void draw_grammatical_analysis_tree(vector<TOKEN> token_stream, const string file_path);

};
