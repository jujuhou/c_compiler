#include "grammar.h"
//*********************** 工具函数 ************************
//删除string首尾的空白字符和注释
string& trim(string& str) 
{
	if (str.empty())
		return str;
	str.erase(0, str.find_first_not_of("\f\v\r\t\n "));
	str.erase(str.find_last_not_of("\f\v\r\t\n ") + 1);
	return str;
}
//将字符串分割为子串，以separator为界(在boost库中有这个函数，但是需要安装额外的库)
vector<string> split(const string str, const string separator)
{
	vector<string> strs; //返回的字符串
	int start = 0;//开始位置
	int end = str.find(separator, start); //结束位置

	// str中不含分割串，end=string::npos，直接插入原str并返回
	if (end == string::npos) {
		strs.push_back(str);
		return strs;
	}

	// 循环查找splitStr
	while (end != string::npos) {
		// 取出当前分割出的子串
		string sub = str.substr(start, end - start);
		// 删除首尾空字符
		trim(sub);
		// 如果删除后子串不为空再加入
		if (!sub.empty())
			strs.push_back(sub);
		// 更新查找的起始位置
		start = end + separator.length();
		end = str.find(separator, start);
	}

	// 加入最后剩余的子串部分
	string sub = str.substr(start);
	// 删除首尾空字符
	trim(sub);
	if (!sub.empty())
		strs.push_back(sub);

	return strs;
}
//将非空src集插入des（用于first集和follow集的扩大）
bool mergeSetExceptEmpty(set<int>& des, set<int>& src,int epsilon_index)
{
	//求差集
	set<int> tmp;
	set_difference(src.begin(), src.end(), des.begin(), des.end(),inserter(tmp, tmp.begin()));
	//判断空串是否存在
	bool desExisted = des.find(epsilon_index) != des.end();
	//des在insert前的大小
	int beforeInsert = des.size();
	//插入
	des.insert(tmp.begin(), tmp.end());
	//如果des中不存在空串 则删除src中可能存在的空串	
	if(!desExisted)
	{
		des.erase(epsilon_index);
	}
	//返回值用于判断是否进行了插入
	return beforeInsert < des.size();
}

//******************* grammar_symbol ********************
//grammar_symbol构造函数
grammar_symbol::grammar_symbol(grammar_symbol::Symbol_type type_,const string token_)
{
	this->type = type_;
	this->token = token_;
}

//********************** grammar_item ****************************
//grammar_item构造函数
grammar_item::grammar_item(const int left_symbol_, const vector<int>& right_symbol_)
{
	this->left_symbol = left_symbol_;
	this->right_symbol = right_symbol_;
}

//************************* grammar *******************************
//grammar构造函数
grammar::grammar(const string file_path)
{
	//读文件
	read_grammar(file_path);
	//初始化所有的Terminal的first集
	for (auto ter = terminals.begin(); ter != terminals.end(); ter++)
		symbols[*ter].first_set.insert(*ter);
	//初始化所有的NonTerminal的first集
	getFirstOfNonterminal();
}

//read_grammar函数
void grammar::read_grammar(const string file_path)
{
	//打开文件
	ifstream file_in;
	file_in.open(file_path, ios::in);
	//未打开文件
	if (!file_in.is_open()) {
		cout << "无法打开文法文件" << endl;
		throw FILE_OPEN_ERROE;
	}

	//在symbols中加入EndToken #，由于其为终结符，在terminals中插入symbols.size() - 1
	symbols.push_back(grammar_symbol(grammar_symbol::End, EndToken));
	terminals.insert(int(symbols.size()) - 1);
	//加入EpsilonToken @
	symbols.push_back(grammar_symbol(grammar_symbol::Epsilon, EpsilonToken));

	//记录是第几个产生式
	int grammar_row_num=0;
	
	//开始读取文法
	string line;
	while (getline(file_in, line, '\n')) {
		//行数++
		grammar_row_num++;
		if (line.empty())
			continue;
		//处理最开始和末尾的空格和注释
		trim(line);
		if(line.find("$")!=line.npos)
			line.erase(line.find_first_of("$"));
		if (line.empty())
			continue;

		//将产生式分为左右两个部分
		vector<string> production_left_and_right = split(line, ProToken);
		if (production_left_and_right.size() != 2) {
			cout << "第" << grammar_row_num << "行的产生式格式有误（每行应有且只有一个\""<< ProToken <<"\"符号）"<<endl;
			throw(GRAMMAR_ERROR);
		}

		string production_left = production_left_and_right[0];
		string production_right = production_left_and_right[1];
		
		//左边部分的index
		int left_symbol=-1;
		//如果不是声明所有非终结符
		if (production_left != AllTerminalToken) {
			left_symbol = find_symbol_index_by_token(production_left);
			
			if (left_symbol == -1) {
				symbols.push_back(grammar_symbol(grammar_symbol::NonTerminal, production_left));
				left_symbol = symbols.size() - 1;
				non_terminals.insert(left_symbol);
			}
		}
		//此时如果是声明所有非终结符的grammar，则left_symbol=-1

		//右边部分以" | "为界限分解
		vector<string> production_right_parts = split(production_right, SplitToken);
		if (production_right_parts.size() == 0) {
			cout << "第" << grammar_row_num << "行的产生式格式有误（产生式右端没有文法符号）" << endl;
			throw(GRAMMAR_ERROR);
		}

		for (auto production_right_it = production_right_parts.begin(); production_right_it != production_right_parts.end(); production_right_it++)
		{
			//如果是终结符声明
			if (left_symbol == -1) {
				symbols.push_back(grammar_symbol(grammar_symbol::Terminal, *production_right_it));
				terminals.insert(symbols.size() - 1);
			}
			else {
				//将一个产生式中的每个符号分解
				vector<int> right_symbol;
				vector<string> right_symbol_str = split(*production_right_it, " ");
				for (auto right_symbol_str_it = right_symbol_str.begin(); right_symbol_str_it != right_symbol_str.end(); right_symbol_str_it++) {
					int right_symbol_present = find_symbol_index_by_token(*right_symbol_str_it);
					if (right_symbol_present == -1) {
						symbols.push_back(grammar_symbol(grammar_symbol::NonTerminal, *right_symbol_str_it));
						right_symbol_present = symbols.size() - 1;
						non_terminals.insert(right_symbol_present);
					}
					right_symbol.push_back(right_symbol_present);
				}
				//加入production中
				productions.push_back(grammar_item(left_symbol, right_symbol));
				//如果是起始产生式
				if (symbols[left_symbol].token == ExtendStartToken)
					start_production = productions.size() - 1;
			}
		}
	}
	file_in.close();
}
//根据字符串找到其在symbols中的index，如果找到返回index，如果没有找到返回-1
int grammar::find_symbol_index_by_token(const string token)
{
	auto symbol_it = find_if(symbols.begin(), symbols.end(), [token](grammar_symbol it)->bool {
		return it.token == token;
		});
	if (symbol_it == symbols.end()) {
		return -1;
	}
	else {
		return int(symbol_it - symbols.begin());
	}
}
//初始化所有的NonTerminal的first集
void grammar::getFirstOfNonterminal()
{
	// 不断进行标记，直到所有集合不发生变化
	bool changed;
	while (true) {
		changed = false;
		// 遍历所有非终结符
		for (auto nonTerminal = non_terminals.begin(); nonTerminal != non_terminals.end(); nonTerminal++)
		{   //遍历所有产生式
			for (auto production = productions.begin(); production != productions.end(); production++)
			{
				//如果左边不为nonTerminal则continue
				if (production->left_symbol != *nonTerminal)
					continue;

				// 找到对应产生式，遍历产生式右部
				auto it = production->right_symbol.begin();

				// 是终结符或空串直接加入first集合并退出
				if (terminals.find(*it)!=terminals.end() || symbols[*it].type == grammar_symbol::Epsilon) {
					//insert的second标志是成功与否
					changed = symbols[*nonTerminal].first_set.insert(*it).second || changed;
					continue;
				}
				// 右部以非终结符开始
				bool flag = true; // 可推导出空串的标记
				for (; it != production->right_symbol.end(); ++it) {
					// 如果是终结符，停止迭代
					if (terminals.find(*it) != terminals.end()) {
						changed = mergeSetExceptEmpty(symbols[*nonTerminal].first_set, symbols[*it].first_set,find_symbol_index_by_token(EpsilonToken)) || changed;
						flag = false;
						break;
					}

					changed = mergeSetExceptEmpty(symbols[*nonTerminal].first_set, symbols[*it].first_set, find_symbol_index_by_token(EpsilonToken)) || changed;
					// 若该非终结符可推导出空串，则继续迭代
					flag = flag && symbols[*it].first_set.count(find_symbol_index_by_token(EpsilonToken));

					// 否则直接结束当前产生式的处理
					if (!flag)
						break;
				}
				// 如果该产生式的所有右部均为非终结符且均可推导出空串，则将空串加入First集合
				if (flag && it == production->right_symbol.end())
					changed = symbols[*nonTerminal].first_set.insert(find_symbol_index_by_token(EpsilonToken)).second || changed;
			}
		}
		//如果没有任何改变，则退出
		if (!changed)
			break;
	}
}
//返回一个符号串的first集
set<int> grammar::getFirstOfString(const vector<int>& str)
{
	//First集
	set<int> FirstSet;
	//str为空直接返回
	if (str.empty())
		return FirstSet;
	//epsilonIn用于判断空串是否需要加入
	bool epsilonIn = true;

	for (auto it = str.begin(); it != str.end(); it++) {
		//如果是终结符
		if (symbols[*it].type == grammar_symbol::Terminal)
		{
			mergeSetExceptEmpty(FirstSet, symbols[*it].first_set,find_symbol_index_by_token(EpsilonToken));
			epsilonIn = false;
			break;
		}
		//是空串
		if (symbols[*it].type == grammar_symbol::Epsilon)
		{
			FirstSet.insert(*it);
			epsilonIn = false;
			break;
		}
		//非终结符，合并first集合
		mergeSetExceptEmpty(FirstSet, symbols[*it].first_set, find_symbol_index_by_token(EpsilonToken));
		//如果当前非终结符可以推导出空串，则继续循环
		epsilonIn = epsilonIn && symbols[*it].first_set.count(find_symbol_index_by_token(EpsilonToken));
		if (!epsilonIn)
			break;
	}
	//如果所有的都可以产生空串，first集加入空串
	if (epsilonIn) 
		FirstSet.insert(find_symbol_index_by_token(EpsilonToken));
	return FirstSet;

}