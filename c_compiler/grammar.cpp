#include "grammar.h"
//*********************** ���ߺ��� ************************
//ɾ��string��β�Ŀհ��ַ���ע��
string& trim(string& str) 
{
	if (str.empty())
		return str;
	str.erase(0, str.find_first_not_of("\f\v\r\t\n "));
	str.erase(str.find_last_not_of("\f\v\r\t\n ") + 1);
	return str;
}
//���ַ����ָ�Ϊ�Ӵ�����separatorΪ��(��boost���������������������Ҫ��װ����Ŀ�)
vector<string> split(const string str, const string separator)
{
	vector<string> strs; //���ص��ַ���
	int start = 0;//��ʼλ��
	int end = str.find(separator, start); //����λ��

	// str�в����ָ��end=string::npos��ֱ�Ӳ���ԭstr������
	if (end == string::npos) {
		strs.push_back(str);
		return strs;
	}

	// ѭ������splitStr
	while (end != string::npos) {
		// ȡ����ǰ�ָ�����Ӵ�
		string sub = str.substr(start, end - start);
		// ɾ����β���ַ�
		trim(sub);
		// ���ɾ�����Ӵ���Ϊ���ټ���
		if (!sub.empty())
			strs.push_back(sub);
		// ���²��ҵ���ʼλ��
		start = end + separator.length();
		end = str.find(separator, start);
	}

	// �������ʣ����Ӵ�����
	string sub = str.substr(start);
	// ɾ����β���ַ�
	trim(sub);
	if (!sub.empty())
		strs.push_back(sub);

	return strs;
}
//���ǿ�src������des������first����follow��������
bool mergeSetExceptEmpty(set<int>& des, set<int>& src,int epsilon_index)
{
	//��
	set<int> tmp;
	set_difference(src.begin(), src.end(), des.begin(), des.end(),inserter(tmp, tmp.begin()));
	//�жϿմ��Ƿ����
	bool desExisted = des.find(epsilon_index) != des.end();
	//des��insertǰ�Ĵ�С
	int beforeInsert = des.size();
	//����
	des.insert(tmp.begin(), tmp.end());
	//���des�в����ڿմ� ��ɾ��src�п��ܴ��ڵĿմ�	
	if(!desExisted)
	{
		des.erase(epsilon_index);
	}
	//����ֵ�����ж��Ƿ�����˲���
	return beforeInsert < des.size();
}

//******************* grammar_symbol ********************
//grammar_symbol���캯��
grammar_symbol::grammar_symbol(grammar_symbol::Symbol_type type_,const string token_)
{
	this->type = type_;
	this->token = token_;
}

//********************** grammar_item ****************************
//grammar_item���캯��
grammar_item::grammar_item(const int left_symbol_, const vector<int>& right_symbol_)
{
	this->left_symbol = left_symbol_;
	this->right_symbol = right_symbol_;
}

//************************* grammar *******************************
//grammar���캯��
grammar::grammar(const string file_path)
{
	//���ļ�
	read_grammar(file_path);
	//��ʼ�����е�Terminal��first��
	for (auto ter = terminals.begin(); ter != terminals.end(); ter++)
		symbols[*ter].first_set.insert(*ter);
	//��ʼ�����е�NonTerminal��first��
	getFirstOfNonterminal();
}

//read_grammar����
void grammar::read_grammar(const string file_path)
{
	//���ļ�
	ifstream file_in;
	file_in.open(file_path, ios::in);
	//δ���ļ�
	if (!file_in.is_open()) {
		cout << "�޷����ķ��ļ�" << endl;
		throw FILE_OPEN_ERROE;
	}

	//��symbols�м���EndToken #��������Ϊ�ս������terminals�в���symbols.size() - 1
	symbols.push_back(grammar_symbol(grammar_symbol::End, EndToken));
	terminals.insert(int(symbols.size()) - 1);
	//����EpsilonToken @
	symbols.push_back(grammar_symbol(grammar_symbol::Epsilon, EpsilonToken));

	//��¼�ǵڼ�������ʽ
	int grammar_row_num=0;
	
	//��ʼ��ȡ�ķ�
	string line;
	while (getline(file_in, line, '\n')) {
		//����++
		grammar_row_num++;
		if (line.empty())
			continue;
		//�����ʼ��ĩβ�Ŀո��ע��
		trim(line);
		if(line.find("$")!=line.npos)
			line.erase(line.find_first_of("$"));
		if (line.empty())
			continue;

		//������ʽ��Ϊ������������
		vector<string> production_left_and_right = split(line, ProToken);
		if (production_left_and_right.size() != 2) {
			cout << "��" << grammar_row_num << "�еĲ���ʽ��ʽ����ÿ��Ӧ����ֻ��һ��\""<< ProToken <<"\"���ţ�"<<endl;
			throw(GRAMMAR_ERROR);
		}

		string production_left = production_left_and_right[0];
		string production_right = production_left_and_right[1];
		
		//��߲��ֵ�index
		int left_symbol=-1;
		//��������������з��ս��
		if (production_left != AllTerminalToken) {
			left_symbol = find_symbol_index_by_token(production_left);
			
			if (left_symbol == -1) {
				symbols.push_back(grammar_symbol(grammar_symbol::NonTerminal, production_left));
				left_symbol = symbols.size() - 1;
				non_terminals.insert(left_symbol);
			}
		}
		//��ʱ������������з��ս����grammar����left_symbol=-1

		//�ұ߲�����" | "Ϊ���޷ֽ�
		vector<string> production_right_parts = split(production_right, SplitToken);
		if (production_right_parts.size() == 0) {
			cout << "��" << grammar_row_num << "�еĲ���ʽ��ʽ���󣨲���ʽ�Ҷ�û���ķ����ţ�" << endl;
			throw(GRAMMAR_ERROR);
		}

		for (auto production_right_it = production_right_parts.begin(); production_right_it != production_right_parts.end(); production_right_it++)
		{
			//������ս������
			if (left_symbol == -1) {
				symbols.push_back(grammar_symbol(grammar_symbol::Terminal, *production_right_it));
				terminals.insert(symbols.size() - 1);
			}
			else {
				//��һ������ʽ�е�ÿ�����ŷֽ�
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
				//����production��
				productions.push_back(grammar_item(left_symbol, right_symbol));
				//�������ʼ����ʽ
				if (symbols[left_symbol].token == ExtendStartToken)
					start_production = productions.size() - 1;
			}
		}
	}
	file_in.close();
}
//�����ַ����ҵ�����symbols�е�index������ҵ�����index�����û���ҵ�����-1
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
//��ʼ�����е�NonTerminal��first��
void grammar::getFirstOfNonterminal()
{
	// ���Ͻ��б�ǣ�ֱ�����м��ϲ������仯
	bool changed;
	while (true) {
		changed = false;
		// �������з��ս��
		for (auto nonTerminal = non_terminals.begin(); nonTerminal != non_terminals.end(); nonTerminal++)
		{   //�������в���ʽ
			for (auto production = productions.begin(); production != productions.end(); production++)
			{
				//�����߲�ΪnonTerminal��continue
				if (production->left_symbol != *nonTerminal)
					continue;

				// �ҵ���Ӧ����ʽ����������ʽ�Ҳ�
				auto it = production->right_symbol.begin();

				// ���ս����մ�ֱ�Ӽ���first���ϲ��˳�
				if (terminals.find(*it)!=terminals.end() || symbols[*it].type == grammar_symbol::Epsilon) {
					//insert��second��־�ǳɹ����
					changed = symbols[*nonTerminal].first_set.insert(*it).second || changed;
					continue;
				}
				// �Ҳ��Է��ս����ʼ
				bool flag = true; // ���Ƶ����մ��ı��
				for (; it != production->right_symbol.end(); ++it) {
					// ������ս����ֹͣ����
					if (terminals.find(*it) != terminals.end()) {
						changed = mergeSetExceptEmpty(symbols[*nonTerminal].first_set, symbols[*it].first_set,find_symbol_index_by_token(EpsilonToken)) || changed;
						flag = false;
						break;
					}

					changed = mergeSetExceptEmpty(symbols[*nonTerminal].first_set, symbols[*it].first_set, find_symbol_index_by_token(EpsilonToken)) || changed;
					// ���÷��ս�����Ƶ����մ������������
					flag = flag && symbols[*it].first_set.count(find_symbol_index_by_token(EpsilonToken));

					// ����ֱ�ӽ�����ǰ����ʽ�Ĵ���
					if (!flag)
						break;
				}
				// ����ò���ʽ�������Ҳ���Ϊ���ս���Ҿ����Ƶ����մ����򽫿մ�����First����
				if (flag && it == production->right_symbol.end())
					changed = symbols[*nonTerminal].first_set.insert(find_symbol_index_by_token(EpsilonToken)).second || changed;
			}
		}
		//���û���κθı䣬���˳�
		if (!changed)
			break;
	}
}
//����һ�����Ŵ���first��
set<int> grammar::getFirstOfString(const vector<int>& str)
{
	//First��
	set<int> FirstSet;
	//strΪ��ֱ�ӷ���
	if (str.empty())
		return FirstSet;
	//epsilonIn�����жϿմ��Ƿ���Ҫ����
	bool epsilonIn = true;

	for (auto it = str.begin(); it != str.end(); it++) {
		//������ս��
		if (symbols[*it].type == grammar_symbol::Terminal)
		{
			mergeSetExceptEmpty(FirstSet, symbols[*it].first_set,find_symbol_index_by_token(EpsilonToken));
			epsilonIn = false;
			break;
		}
		//�ǿմ�
		if (symbols[*it].type == grammar_symbol::Epsilon)
		{
			FirstSet.insert(*it);
			epsilonIn = false;
			break;
		}
		//���ս�����ϲ�first����
		mergeSetExceptEmpty(FirstSet, symbols[*it].first_set, find_symbol_index_by_token(EpsilonToken));
		//�����ǰ���ս�������Ƶ����մ��������ѭ��
		epsilonIn = epsilonIn && symbols[*it].first_set.count(find_symbol_index_by_token(EpsilonToken));
		if (!epsilonIn)
			break;
	}
	//������еĶ����Բ����մ���first������մ�
	if (epsilonIn) 
		FirstSet.insert(find_symbol_index_by_token(EpsilonToken));
	return FirstSet;

}