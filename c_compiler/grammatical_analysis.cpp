#include "grammatical_analysis.h"

//************************** 工具函数 ******************************



//*************************** LR1_item *****************************
LR1_item::LR1_item(const int left_symbol_, const vector<int>& right_symbol_, const int index_, const int dot_position_, const int lookahead_symbol_)
{
	this->left_symbol = left_symbol_;
	this->right_symbol = right_symbol_;
	this->index = index_;
	this->dot_position = dot_position_;
	this->lookahead_symbol = lookahead_symbol_;
}

bool LR1_item::operator==(const LR1_item& item)
{
	return (this->left_symbol == item.left_symbol && this->right_symbol == item.right_symbol && this->dot_position == item.dot_position && this->index == item.index && this->lookahead_symbol == item.lookahead_symbol);
}

//************************** Closure *********************************
bool LR1_closure::operator==(const LR1_closure& closure)
{
	if (this->lr1_closure.size() != closure.lr1_closure.size()) {
		return false;
	}
	int count = 0;
	for (auto& tmp : this->lr1_closure) {
		for (auto& tmp_ : closure.lr1_closure) {
			if (tmp == tmp_) {
				++count;
				break;
			}
		}
	}
	return count == this->lr1_closure.size();
}




//****************************** LR1 **********************************
//构造函数
LR1::LR1(const string file_path) :grammar(file_path) {
	//生成LR1项集，存储在lr1_cluster中
	generateCluster();
	generateTable();
}

//生成LR1项集
void LR1::generateCluster()
{
	//初始化lr1_cluster({S → ·Program, $]})
	LR1_item initial_item(find_symbol_index_by_token(ExtendStartToken), { find_symbol_index_by_token(StartToken) }, start_production, 0, find_symbol_index_by_token(EndToken));
	LR1_closure initial_closure;
	initial_closure.lr1_closure.push_back(initial_item);

	//放进cluster中
	lr1_cluster.push_back(generateClosure(initial_closure));

	//遍历lr1_cluster中的每一项
	for (int i = 0; i < lr1_cluster.size(); i++) {
		//遍历所有文法符号
		for (int s = 0; s < symbols.size(); s++) {
			//只有为终结符或非终结符才进行下一步
			if (symbols[s].type != grammar_symbol::NonTerminal && symbols[s].type != grammar_symbol::Terminal)
				continue;
			//得到输入符号s会到达的closure
			LR1_closure to_closure = generateGOTO(lr1_cluster[i], s);
			//如果为空则continue
			if (to_closure.lr1_closure.empty())
				continue;
			//如果已经存在，则记录编号
			int exist_index=-1;
			for (int j = 0; j < lr1_cluster.size(); j++) {
				if (lr1_cluster[j] == to_closure) {
					exist_index = j;
					break;
				}
			}
			if (exist_index != -1)
				goto_info[{i, s}] = exist_index;
			else {//不存在，则加入进lr1_cluster
				lr1_cluster.push_back(to_closure);
				//记录转移关系
				goto_info[{i, s}] = lr1_cluster.size() - 1;
			}
		}
	}

}
//生成闭包
LR1_closure& LR1::generateClosure(LR1_closure& initial_closure)
{
	//遍历每一个initial_closure中的项
	for (int i = 0; i < initial_closure.lr1_closure.size(); i++) {
		LR1_item present_lr1_item = initial_closure.lr1_closure[i];
		//·在最后一个位置，则其后继没有非终结符
		if (present_lr1_item.dot_position >= static_cast<int>(present_lr1_item.right_symbol.size()))
			continue;
		//·后的符号
		int next_symbol_index = present_lr1_item.right_symbol[present_lr1_item.dot_position];
		grammar_symbol next_symbol = symbols[next_symbol_index];
		//如果·后的符号为终结符
		if (next_symbol.type == grammar_symbol::Terminal)
			continue;
		//如果·后的符号为ε,则->·ε直接变为->ε·
		if (next_symbol.type == grammar_symbol::Epsilon) {
			initial_closure.lr1_closure[i].dot_position++;
			continue;
		}
		//其他情况，·后的符号为非终结符
		//得到first集(A->α·Bβ,a  则求βa的first集)
		vector<int> BetaA(present_lr1_item.right_symbol.begin() + present_lr1_item.dot_position + 1, present_lr1_item.right_symbol.end());
		BetaA.push_back(present_lr1_item.lookahead_symbol);
		set<int> BetaAFirstSet = getFirstOfString(BetaA);
		//查找以next_symbol_index开始的production
		for (int j = 0; j < productions.size(); j++) {
			grammar_item present_production = productions[j];
			if (present_production.left_symbol != next_symbol_index)
				continue;
			//查找到以next_symbol_index开始的production，开始加入initial_closure
			for (auto it = BetaAFirstSet.begin(); it != BetaAFirstSet.end(); it++) {
				//如果是ε产生式，则直接加入->ε·项，从而不引出ε转移边
				if (symbols[present_production.right_symbol.front()].type == grammar_symbol::Epsilon) {
					//确保当前不含这一项再加入
					vector<LR1_item>::iterator tmp;
					for (tmp = initial_closure.lr1_closure.begin(); tmp != initial_closure.lr1_closure.end();tmp++) {
						if (*tmp == LR1_item({ present_production.left_symbol,present_production.right_symbol,j,1, *it }))
							break;
					}
					if(tmp== initial_closure.lr1_closure.end())
						initial_closure.lr1_closure.push_back({ present_production.left_symbol,present_production.right_symbol,j,1, *it });
				}
				else {
					//确保当前不含这一项再加入
					vector<LR1_item>::iterator tmp;
					for (tmp = initial_closure.lr1_closure.begin(); tmp != initial_closure.lr1_closure.end(); tmp++) {
						if (*tmp == LR1_item({ present_production.left_symbol,present_production.right_symbol,j,0, *it }))
							break;
					}
					if (tmp == initial_closure.lr1_closure.end())
						initial_closure.lr1_closure.push_back({ present_production.left_symbol,present_production.right_symbol,j,0, *it });
				}
			}
		}
	}
	return initial_closure;
}
//生成GOTO的closure
LR1_closure LR1::generateGOTO(const LR1_closure& from_closure, int present_symbol)
{
	LR1_closure to_closure;
	//判断一下present_symbol是不是非终结符或终结符（虽然按理来说应该已经判断过了），如果不合要求返回空
	if (symbols[present_symbol].type != grammar_symbol::NonTerminal && symbols[present_symbol].type != grammar_symbol::Terminal)
		return to_closure;
	for (auto lr1_item_it = from_closure.lr1_closure.begin(); lr1_item_it != from_closure.lr1_closure.end(); lr1_item_it++) {
		//如果dot在最后
		if (lr1_item_it->dot_position >= lr1_item_it->right_symbol.size())
			continue;
		//如果后面一个字符不是present_symbol
		if (lr1_item_it->right_symbol[lr1_item_it->dot_position] != present_symbol)
			continue;
		//后面一个字符就是present_symbol
		to_closure.lr1_closure.push_back({ lr1_item_it->left_symbol,lr1_item_it->right_symbol,lr1_item_it->index,lr1_item_it->dot_position + 1,lr1_item_it->lookahead_symbol });
	}
	return generateClosure(to_closure);
}
//生成Table
void LR1::generateTable()
{
	for (int closure_index = 0; closure_index < lr1_cluster.size(); closure_index++) {
		for (int lr1_item_index = 0; lr1_item_index < lr1_cluster[closure_index].lr1_closure.size(); lr1_item_index++) {
			LR1_item present_lr1_item = lr1_cluster[closure_index].lr1_closure[lr1_item_index];
			
			//如果·已在末尾
			if (present_lr1_item.dot_position >= present_lr1_item.right_symbol.size()) {
				//如果不为扩展开始符号,则进行规约
				if (symbols[present_lr1_item.left_symbol].token != ExtendStartToken)
					action_table[{closure_index, present_lr1_item.lookahead_symbol}] = { Action::Reduce,present_lr1_item.index };
				else//否则为接受
					action_table[{closure_index, present_lr1_item.lookahead_symbol}]= { Action::Accept, -1 };
			}
			else {//如果·不在末尾
				int next_symbol = present_lr1_item.right_symbol[present_lr1_item.dot_position];
				//不是终结符则在goto表中标出
				if (symbols[next_symbol].type == grammar_symbol::NonTerminal) {
					//在goto表中寻找
					auto it = goto_info.find({ closure_index,next_symbol });
					//如果找到，进行移进
					if (it != goto_info.end())
						goto_table[{closure_index, next_symbol}] = { Action::ShiftIn,it->second };
				}
				//否则在action表中标出
				else if (symbols[next_symbol].type == grammar_symbol::Terminal) {
					//在goto表中寻找
					auto it = goto_info.find({ closure_index,next_symbol });
					//如果找到，进行移进
					if (it != goto_info.end())
						action_table[{closure_index, next_symbol}] = { Action::ShiftIn,it->second };
				}
			}
		}
	}
}
//打印Table
void LR1::printTable(const string file_path)
{
	ofstream file_out;
	file_out.open(file_path, ios::out);

	const int   state_width = 10;
	const int   action_width = 8;
	const int   goto_width = 14;
	const char* err_msg = " ";
	file_out.setf(std::ios::left);
	file_out << setw(state_width) << "STATUS" << setw(terminals.size() * action_width) << "ACTION"
		<< setw((non_terminals.size() - 1) * goto_width) << "GOTO" << endl;

	file_out << setw(state_width) << " ";
	for (auto& ter : terminals) {
		file_out << setw(action_width) << symbols[ter].token;
	}
	for (auto& non_ter : non_terminals) {
		if (symbols[non_ter].token== ExtendStartToken) {
			continue;
		}
		file_out << setw(goto_width) << symbols[non_ter].token;
	}
	file_out << endl;

	for (int i = 0; i < static_cast<int>(lr1_cluster.size()); ++i) {
		file_out << setw(state_width) << i;
		for (auto& ter : terminals) {
			auto iter = action_table.find({ i, ter });
			if (iter == action_table.end()) {
				file_out << setw(action_width) << err_msg;
			}
			else {
				string out_msg;
				if (iter->second.action == Action::Accept) {
					out_msg += "acc";
				}
				else if (iter->second.action == Action::Reduce) {
					out_msg += "r" + to_string(iter->second.info);
				}
				else if (iter->second.action == Action::ShiftIn) {
					out_msg += "s" + to_string(iter->second.info);
				}
				file_out << setw(action_width) << out_msg;
			}
		}

		for (auto& non_ter : non_terminals) {
			if (symbols[non_ter].token == ExtendStartToken) {
				continue;
			}
			auto iter = goto_table.find({ i, non_ter });
			if (iter == goto_table.end()) {
				file_out << setw(goto_width) << err_msg;
			}
			else {
				file_out << setw(goto_width) << to_string(iter->second.info);
			}
		}
		file_out << endl;
	}
	file_out << endl;
	file_out.close();
}

//进行语法分析和语义分析
void LR1::parser(vector<TOKEN> token_stream, const string file_path)
{
	symantic_analysis = Semantic_analysis();
	//打开写的文件
	ofstream file_out;
	file_out.open(file_path, ios::out);
	if (!file_out.is_open()) {
		cout << "无法打开语法分析的输出文件";
		throw(FILE_OPEN_ERROE);
	}

	//在token_stream的末尾添加EndToken
	token_stream.push_back({ EndToken,EndToken,-1,-1 });
	//定义状态栈
	vector<int> symbol_stack;
	//定义符号栈
	vector<int> status_stack;
	//记录步骤
	int step = 1;

	//用于输出的格式化
	struct {
		int step_len = 5, status_len = 200, symbol_len = 300, input_len = 200, production_len = 60;
	}print_format;
	//输出一行的函数（i为输入串进行到第几个）
	auto print_line = [&](int i,int production_index) {
		//输出第几步
		file_out << setw(print_format.step_len) << step++;
		//状态栈的string
		string status_stack_str = "";
		for (auto& status : status_stack) {
			status_stack_str += " " + to_string(status);
		}
		file_out << setw(print_format.status_len) << status_stack_str;
		//符号栈的string
		string symbol_stack_str = "";
		for (auto& symbol : symbol_stack) {
			symbol_stack_str += "(" + to_string(symbol) + "," + symbols[symbol].token + ")";
		}
		file_out << setw(print_format.symbol_len) << symbol_stack_str;
		//输入串的string
		string input_str = "";
		for (auto token = token_stream.begin() + i; token != token_stream.end();token++) {
			input_str += token->token;
		}
		file_out << setw(print_format.input_len) << input_str;
		//产生式的string
		if (production_index != -1) {
			string production_str = "";
			production_str += symbols[productions[production_index].left_symbol].token;
			production_str += " ::= ";
			for (auto& production_right_symbol : productions[production_index].right_symbol) {
				production_str += symbols[production_right_symbol].token + " ";
			}
			file_out << setw(print_format.production_len) << production_str;
		}
		file_out << endl;

	};
	
	//输出
	file_out << setw(print_format.step_len) << "步骤"<<setw(print_format.status_len)<<"状态"<< setw(print_format.symbol_len) << "符号" << setw(print_format.input_len) << "输入串"<<setw(print_format.production_len)<<"产生式"<<endl;

	symantic_analysis.Add_symbol_to_list({ StartToken,"",-1,-1,-1,-1 });

	//初始化栈
	symbol_stack.push_back(find_symbol_index_by_token(EndToken));
	status_stack.push_back(0);

	//输出
	print_line(0, -1);

	//对token_stream中的每一个符号进行遍历
	for (int i = 0; i < token_stream.size(); i++) {
		int current_state = status_stack.back();
		int current_token_index = find_symbol_index_by_token(token_stream[i].token);
		//如果找不到这一字符
		if (current_token_index == -1) {
			cout << "待分析字符流中出现了未在文法中进行定义的终结符" << endl;
			throw(GRAMMATICAL_ERROR_UNDEFINED_WORD);
		}
		auto current_action_iter = action_table.find({ current_state ,current_token_index });
		//如果没有找到，进行报错
		if (current_action_iter == action_table.end()) {
			cout << "语法分析过程中在（第" << token_stream[i].row << "行，第" << token_stream[i].col << "列）发现错误" << endl;
			throw(GRAMMATICAL_ERROR_CANNOT_ANALYSIS);
		}
		//当前的ActionInfo
		ActionInfo current_actioninfo = current_action_iter->second;
		//根据ActionInfo的类别进行相应的动作
		switch (current_actioninfo.action) {
			//移进
		case Action::ShiftIn: {
			symbol_stack.push_back(current_token_index);
			status_stack.push_back(current_actioninfo.info);
			print_line(i + 1, -1);

			symantic_analysis.Add_symbol_to_list({ token_stream[i].token , token_stream[i].value , token_stream[i].row , token_stream[i].col ,-1,-1 });
			break;
		}
			//规约
		case Action::Reduce: {
			//规约使用的production
			int production_index = current_actioninfo.info;
			grammar_item production = productions[production_index];
			//非空串需要出栈 空串由于右部为空不需要出栈(直接push空串对应产生式左部非终结符即可)
			if (symbols[production.right_symbol.front()].type != grammar_symbol::Epsilon) {
				int count = production.right_symbol.size();
				while (count--) {
					symbol_stack.pop_back();
					status_stack.pop_back();
				}
			}
			//在goto表中寻找
			auto current_goto_iter = goto_table.find({ status_stack.back(),production.left_symbol });
			//找不到则报错
			if (current_goto_iter == goto_table.end()) {
				cout << "语法分析过程中在（第" << token_stream[i].row << "行，第" << token_stream[i].col << "列）发现错误" << endl;
				throw(GRAMMATICAL_ERROR_CANNOT_ANALYSIS);
			}
			//移入符号栈和状态栈
			symbol_stack.push_back(production.left_symbol);
			status_stack.push_back(current_goto_iter->second.info);
			//进行输出
			print_line(i,production_index );
			//此时i不加1
			i--;

			//进行语义分析
			vector<string> production_right;
			for (auto s : production.right_symbol)
				production_right.push_back(symbols[s].token);
			
			symantic_analysis.Analysis(symbols[production.left_symbol].token, production_right);
		}
			break;
			//接受
		case Action::Accept:
			//输出acc
			file_out << setw(print_format.step_len) << step++<< setw(print_format.status_len)<<"acc!"<<endl;
			file_out.close();
			return;
			break;
			//错误（其实不会进来。。。）
		case Action::Error:
			return;
			break;
		}
	}
}
//绘制语法树（生成.dot文件）
void LR1::draw_grammatical_analysis_tree(vector<TOKEN> token_stream, const string file_path)
{
	int tmp = 0;
	//需要在保证文法正确的前提下运行
	ofstream file_out;
	file_out.open(file_path, ios::out);
	if (!file_out.is_open()) {
		cout << "无法打开生成语法树的dot文件";
		throw(FILE_OPEN_ERROE);
	}

	//定义状态栈
	vector<int> symbol_stack;
	//定义符号栈
	vector<int> status_stack;
	//用于记录符号栈中的tmp值
	vector<int> tmp_stack;

	//在token_stream的末尾添加EndToken
	token_stream.push_back({ EndToken,EndToken,-1,-1 });
	file_out << "digraph mygraph {\n";

	//初始化栈
	symbol_stack.push_back(find_symbol_index_by_token(EndToken));
	status_stack.push_back(0);

	//对token_stream中的每一个符号进行遍历
	for (int i = 0; i < token_stream.size(); i++) {
		int current_state = status_stack.back();
		int current_token_index = find_symbol_index_by_token(token_stream[i].token);
		auto current_action_iter = action_table.find({ current_state ,current_token_index });
		//当前的ActionInfo
		ActionInfo current_actioninfo = current_action_iter->second;
		//根据ActionInfo的类别进行相应的动作
		switch (current_actioninfo.action) {
		//移进
		case Action::ShiftIn: {
			symbol_stack.push_back(current_token_index);
			status_stack.push_back(current_actioninfo.info);
			tmp_stack.push_back(tmp);
			file_out << "n" << tmp++ << "[label=\"" << token_stream[i].value << "\",color=red];"<<endl;
			break;
		}
		//规约
		case Action::Reduce: {
			//规约使用的production
			int production_index = current_actioninfo.info;
			grammar_item production = productions[production_index];
			//非空串需要出栈 空串由于右部为空不需要出栈(直接push空串对应产生式左部非终结符即可)

			vector<int> tmp_left;
			if (symbols[production.right_symbol.front()].type != grammar_symbol::Epsilon) {
				int count = production.right_symbol.size();
				while (count--) {
					symbol_stack.pop_back();
					status_stack.pop_back();
					tmp_left.push_back(tmp_stack.back());
					tmp_stack.pop_back();
				}
			}
			//在goto表中寻找
			auto current_goto_iter = goto_table.find({ status_stack.back(),production.left_symbol });
			//移入符号栈和状态栈
			symbol_stack.push_back(production.left_symbol);
			status_stack.push_back(current_goto_iter->second.info);

			tmp_stack.push_back(tmp);
			file_out << "n" << tmp++ << "[label=\"" << symbols[production.left_symbol].token << "\"];\n";

			if (tmp_left.size() != 0) {
				for (auto t = tmp_left.begin(); t != tmp_left.end(); t++)
					file_out << "n" << tmp - 1 << " -> " << "n" << *t << ";\n";
			}

			else {
				//空串
				file_out << "e" << tmp << "[label=\"@\"];\n";
				file_out << "n" << tmp - 1 << " -> " << "e" << tmp << ";\n";
			}
			//此时i不加1
			i--;
			break;
		}				   //接受
		case Action::Accept:
			file_out << "}";
			file_out.close();
			return;
			break;
			//错误（其实不会进来。。。）
		case Action::Error:
			return;
			break;
		}
	}
}
