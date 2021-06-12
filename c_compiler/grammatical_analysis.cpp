#include "grammatical_analysis.h"

//************************** ���ߺ��� ******************************



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
//���캯��
LR1::LR1(const string file_path) :grammar(file_path) {
	//����LR1����洢��lr1_cluster��
	generateCluster();
	generateTable();
}

//����LR1�
void LR1::generateCluster()
{
	//��ʼ��lr1_cluster({S �� ��Program, $]})
	LR1_item initial_item(find_symbol_index_by_token(ExtendStartToken), { find_symbol_index_by_token(StartToken) }, start_production, 0, find_symbol_index_by_token(EndToken));
	LR1_closure initial_closure;
	initial_closure.lr1_closure.push_back(initial_item);

	//�Ž�cluster��
	lr1_cluster.push_back(generateClosure(initial_closure));

	//����lr1_cluster�е�ÿһ��
	for (int i = 0; i < lr1_cluster.size(); i++) {
		//���������ķ�����
		for (int s = 0; s < symbols.size(); s++) {
			//ֻ��Ϊ�ս������ս���Ž�����һ��
			if (symbols[s].type != grammar_symbol::NonTerminal && symbols[s].type != grammar_symbol::Terminal)
				continue;
			//�õ��������s�ᵽ���closure
			LR1_closure to_closure = generateGOTO(lr1_cluster[i], s);
			//���Ϊ����continue
			if (to_closure.lr1_closure.empty())
				continue;
			//����Ѿ����ڣ����¼���
			int exist_index=-1;
			for (int j = 0; j < lr1_cluster.size(); j++) {
				if (lr1_cluster[j] == to_closure) {
					exist_index = j;
					break;
				}
			}
			if (exist_index != -1)
				goto_info[{i, s}] = exist_index;
			else {//�����ڣ�������lr1_cluster
				lr1_cluster.push_back(to_closure);
				//��¼ת�ƹ�ϵ
				goto_info[{i, s}] = lr1_cluster.size() - 1;
			}
		}
	}

}
//���ɱհ�
LR1_closure& LR1::generateClosure(LR1_closure& initial_closure)
{
	//����ÿһ��initial_closure�е���
	for (int i = 0; i < initial_closure.lr1_closure.size(); i++) {
		LR1_item present_lr1_item = initial_closure.lr1_closure[i];
		//�������һ��λ�ã�������û�з��ս��
		if (present_lr1_item.dot_position >= static_cast<int>(present_lr1_item.right_symbol.size()))
			continue;
		//����ķ���
		int next_symbol_index = present_lr1_item.right_symbol[present_lr1_item.dot_position];
		grammar_symbol next_symbol = symbols[next_symbol_index];
		//�������ķ���Ϊ�ս��
		if (next_symbol.type == grammar_symbol::Terminal)
			continue;
		//�������ķ���Ϊ��,��->����ֱ�ӱ�Ϊ->�š�
		if (next_symbol.type == grammar_symbol::Epsilon) {
			initial_closure.lr1_closure[i].dot_position++;
			continue;
		}
		//�������������ķ���Ϊ���ս��
		//�õ�first��(A->����B��,a  �����a��first��)
		vector<int> BetaA(present_lr1_item.right_symbol.begin() + present_lr1_item.dot_position + 1, present_lr1_item.right_symbol.end());
		BetaA.push_back(present_lr1_item.lookahead_symbol);
		set<int> BetaAFirstSet = getFirstOfString(BetaA);
		//������next_symbol_index��ʼ��production
		for (int j = 0; j < productions.size(); j++) {
			grammar_item present_production = productions[j];
			if (present_production.left_symbol != next_symbol_index)
				continue;
			//���ҵ���next_symbol_index��ʼ��production����ʼ����initial_closure
			for (auto it = BetaAFirstSet.begin(); it != BetaAFirstSet.end(); it++) {
				//����ǦŲ���ʽ����ֱ�Ӽ���->�š���Ӷ���������ת�Ʊ�
				if (symbols[present_production.right_symbol.front()].type == grammar_symbol::Epsilon) {
					//ȷ����ǰ������һ���ټ���
					vector<LR1_item>::iterator tmp;
					for (tmp = initial_closure.lr1_closure.begin(); tmp != initial_closure.lr1_closure.end();tmp++) {
						if (*tmp == LR1_item({ present_production.left_symbol,present_production.right_symbol,j,1, *it }))
							break;
					}
					if(tmp== initial_closure.lr1_closure.end())
						initial_closure.lr1_closure.push_back({ present_production.left_symbol,present_production.right_symbol,j,1, *it });
				}
				else {
					//ȷ����ǰ������һ���ټ���
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
//����GOTO��closure
LR1_closure LR1::generateGOTO(const LR1_closure& from_closure, int present_symbol)
{
	LR1_closure to_closure;
	//�ж�һ��present_symbol�ǲ��Ƿ��ս�����ս������Ȼ������˵Ӧ���Ѿ��жϹ��ˣ����������Ҫ�󷵻ؿ�
	if (symbols[present_symbol].type != grammar_symbol::NonTerminal && symbols[present_symbol].type != grammar_symbol::Terminal)
		return to_closure;
	for (auto lr1_item_it = from_closure.lr1_closure.begin(); lr1_item_it != from_closure.lr1_closure.end(); lr1_item_it++) {
		//���dot�����
		if (lr1_item_it->dot_position >= lr1_item_it->right_symbol.size())
			continue;
		//�������һ���ַ�����present_symbol
		if (lr1_item_it->right_symbol[lr1_item_it->dot_position] != present_symbol)
			continue;
		//����һ���ַ�����present_symbol
		to_closure.lr1_closure.push_back({ lr1_item_it->left_symbol,lr1_item_it->right_symbol,lr1_item_it->index,lr1_item_it->dot_position + 1,lr1_item_it->lookahead_symbol });
	}
	return generateClosure(to_closure);
}
//����Table
void LR1::generateTable()
{
	for (int closure_index = 0; closure_index < lr1_cluster.size(); closure_index++) {
		for (int lr1_item_index = 0; lr1_item_index < lr1_cluster[closure_index].lr1_closure.size(); lr1_item_index++) {
			LR1_item present_lr1_item = lr1_cluster[closure_index].lr1_closure[lr1_item_index];
			
			//���������ĩβ
			if (present_lr1_item.dot_position >= present_lr1_item.right_symbol.size()) {
				//�����Ϊ��չ��ʼ����,����й�Լ
				if (symbols[present_lr1_item.left_symbol].token != ExtendStartToken)
					action_table[{closure_index, present_lr1_item.lookahead_symbol}] = { Action::Reduce,present_lr1_item.index };
				else//����Ϊ����
					action_table[{closure_index, present_lr1_item.lookahead_symbol}]= { Action::Accept, -1 };
			}
			else {//���������ĩβ
				int next_symbol = present_lr1_item.right_symbol[present_lr1_item.dot_position];
				//�����ս������goto���б��
				if (symbols[next_symbol].type == grammar_symbol::NonTerminal) {
					//��goto����Ѱ��
					auto it = goto_info.find({ closure_index,next_symbol });
					//����ҵ��������ƽ�
					if (it != goto_info.end())
						goto_table[{closure_index, next_symbol}] = { Action::ShiftIn,it->second };
				}
				//������action���б��
				else if (symbols[next_symbol].type == grammar_symbol::Terminal) {
					//��goto����Ѱ��
					auto it = goto_info.find({ closure_index,next_symbol });
					//����ҵ��������ƽ�
					if (it != goto_info.end())
						action_table[{closure_index, next_symbol}] = { Action::ShiftIn,it->second };
				}
			}
		}
	}
}
//��ӡTable
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

//�����﷨�������������
void LR1::parser(vector<TOKEN> token_stream, const string file_path)
{
	symantic_analysis = Semantic_analysis();
	//��д���ļ�
	ofstream file_out;
	file_out.open(file_path, ios::out);
	if (!file_out.is_open()) {
		cout << "�޷����﷨����������ļ�";
		throw(FILE_OPEN_ERROE);
	}

	//��token_stream��ĩβ���EndToken
	token_stream.push_back({ EndToken,EndToken,-1,-1 });
	//����״̬ջ
	vector<int> symbol_stack;
	//�������ջ
	vector<int> status_stack;
	//��¼����
	int step = 1;

	//��������ĸ�ʽ��
	struct {
		int step_len = 5, status_len = 200, symbol_len = 300, input_len = 200, production_len = 60;
	}print_format;
	//���һ�еĺ�����iΪ���봮���е��ڼ�����
	auto print_line = [&](int i,int production_index) {
		//����ڼ���
		file_out << setw(print_format.step_len) << step++;
		//״̬ջ��string
		string status_stack_str = "";
		for (auto& status : status_stack) {
			status_stack_str += " " + to_string(status);
		}
		file_out << setw(print_format.status_len) << status_stack_str;
		//����ջ��string
		string symbol_stack_str = "";
		for (auto& symbol : symbol_stack) {
			symbol_stack_str += "(" + to_string(symbol) + "," + symbols[symbol].token + ")";
		}
		file_out << setw(print_format.symbol_len) << symbol_stack_str;
		//���봮��string
		string input_str = "";
		for (auto token = token_stream.begin() + i; token != token_stream.end();token++) {
			input_str += token->token;
		}
		file_out << setw(print_format.input_len) << input_str;
		//����ʽ��string
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
	
	//���
	file_out << setw(print_format.step_len) << "����"<<setw(print_format.status_len)<<"״̬"<< setw(print_format.symbol_len) << "����" << setw(print_format.input_len) << "���봮"<<setw(print_format.production_len)<<"����ʽ"<<endl;

	symantic_analysis.Add_symbol_to_list({ StartToken,"",-1,-1,-1,-1 });

	//��ʼ��ջ
	symbol_stack.push_back(find_symbol_index_by_token(EndToken));
	status_stack.push_back(0);

	//���
	print_line(0, -1);

	//��token_stream�е�ÿһ�����Ž��б���
	for (int i = 0; i < token_stream.size(); i++) {
		int current_state = status_stack.back();
		int current_token_index = find_symbol_index_by_token(token_stream[i].token);
		//����Ҳ�����һ�ַ�
		if (current_token_index == -1) {
			cout << "�������ַ����г�����δ���ķ��н��ж�����ս��" << endl;
			throw(GRAMMATICAL_ERROR_UNDEFINED_WORD);
		}
		auto current_action_iter = action_table.find({ current_state ,current_token_index });
		//���û���ҵ������б���
		if (current_action_iter == action_table.end()) {
			cout << "�﷨�����������ڣ���" << token_stream[i].row << "�У���" << token_stream[i].col << "�У����ִ���" << endl;
			throw(GRAMMATICAL_ERROR_CANNOT_ANALYSIS);
		}
		//��ǰ��ActionInfo
		ActionInfo current_actioninfo = current_action_iter->second;
		//����ActionInfo����������Ӧ�Ķ���
		switch (current_actioninfo.action) {
			//�ƽ�
		case Action::ShiftIn: {
			symbol_stack.push_back(current_token_index);
			status_stack.push_back(current_actioninfo.info);
			print_line(i + 1, -1);

			symantic_analysis.Add_symbol_to_list({ token_stream[i].token , token_stream[i].value , token_stream[i].row , token_stream[i].col ,-1,-1 });
			break;
		}
			//��Լ
		case Action::Reduce: {
			//��Լʹ�õ�production
			int production_index = current_actioninfo.info;
			grammar_item production = productions[production_index];
			//�ǿմ���Ҫ��ջ �մ������Ҳ�Ϊ�ղ���Ҫ��ջ(ֱ��push�մ���Ӧ����ʽ�󲿷��ս������)
			if (symbols[production.right_symbol.front()].type != grammar_symbol::Epsilon) {
				int count = production.right_symbol.size();
				while (count--) {
					symbol_stack.pop_back();
					status_stack.pop_back();
				}
			}
			//��goto����Ѱ��
			auto current_goto_iter = goto_table.find({ status_stack.back(),production.left_symbol });
			//�Ҳ����򱨴�
			if (current_goto_iter == goto_table.end()) {
				cout << "�﷨�����������ڣ���" << token_stream[i].row << "�У���" << token_stream[i].col << "�У����ִ���" << endl;
				throw(GRAMMATICAL_ERROR_CANNOT_ANALYSIS);
			}
			//�������ջ��״̬ջ
			symbol_stack.push_back(production.left_symbol);
			status_stack.push_back(current_goto_iter->second.info);
			//�������
			print_line(i,production_index );
			//��ʱi����1
			i--;

			//�����������
			vector<string> production_right;
			for (auto s : production.right_symbol)
				production_right.push_back(symbols[s].token);
			
			symantic_analysis.Analysis(symbols[production.left_symbol].token, production_right);
		}
			break;
			//����
		case Action::Accept:
			//���acc
			file_out << setw(print_format.step_len) << step++<< setw(print_format.status_len)<<"acc!"<<endl;
			file_out.close();
			return;
			break;
			//������ʵ���������������
		case Action::Error:
			return;
			break;
		}
	}
}
//�����﷨��������.dot�ļ���
void LR1::draw_grammatical_analysis_tree(vector<TOKEN> token_stream, const string file_path)
{
	int tmp = 0;
	//��Ҫ�ڱ�֤�ķ���ȷ��ǰ��������
	ofstream file_out;
	file_out.open(file_path, ios::out);
	if (!file_out.is_open()) {
		cout << "�޷��������﷨����dot�ļ�";
		throw(FILE_OPEN_ERROE);
	}

	//����״̬ջ
	vector<int> symbol_stack;
	//�������ջ
	vector<int> status_stack;
	//���ڼ�¼����ջ�е�tmpֵ
	vector<int> tmp_stack;

	//��token_stream��ĩβ���EndToken
	token_stream.push_back({ EndToken,EndToken,-1,-1 });
	file_out << "digraph mygraph {\n";

	//��ʼ��ջ
	symbol_stack.push_back(find_symbol_index_by_token(EndToken));
	status_stack.push_back(0);

	//��token_stream�е�ÿһ�����Ž��б���
	for (int i = 0; i < token_stream.size(); i++) {
		int current_state = status_stack.back();
		int current_token_index = find_symbol_index_by_token(token_stream[i].token);
		auto current_action_iter = action_table.find({ current_state ,current_token_index });
		//��ǰ��ActionInfo
		ActionInfo current_actioninfo = current_action_iter->second;
		//����ActionInfo����������Ӧ�Ķ���
		switch (current_actioninfo.action) {
		//�ƽ�
		case Action::ShiftIn: {
			symbol_stack.push_back(current_token_index);
			status_stack.push_back(current_actioninfo.info);
			tmp_stack.push_back(tmp);
			file_out << "n" << tmp++ << "[label=\"" << token_stream[i].value << "\",color=red];"<<endl;
			break;
		}
		//��Լ
		case Action::Reduce: {
			//��Լʹ�õ�production
			int production_index = current_actioninfo.info;
			grammar_item production = productions[production_index];
			//�ǿմ���Ҫ��ջ �մ������Ҳ�Ϊ�ղ���Ҫ��ջ(ֱ��push�մ���Ӧ����ʽ�󲿷��ս������)

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
			//��goto����Ѱ��
			auto current_goto_iter = goto_table.find({ status_stack.back(),production.left_symbol });
			//�������ջ��״̬ջ
			symbol_stack.push_back(production.left_symbol);
			status_stack.push_back(current_goto_iter->second.info);

			tmp_stack.push_back(tmp);
			file_out << "n" << tmp++ << "[label=\"" << symbols[production.left_symbol].token << "\"];\n";

			if (tmp_left.size() != 0) {
				for (auto t = tmp_left.begin(); t != tmp_left.end(); t++)
					file_out << "n" << tmp - 1 << " -> " << "n" << *t << ";\n";
			}

			else {
				//�մ�
				file_out << "e" << tmp << "[label=\"@\"];\n";
				file_out << "n" << tmp - 1 << " -> " << "e" << tmp << ";\n";
			}
			//��ʱi����1
			i--;
			break;
		}				   //����
		case Action::Accept:
			file_out << "}";
			file_out.close();
			return;
			break;
			//������ʵ���������������
		case Action::Error:
			return;
			break;
		}
	}
}
