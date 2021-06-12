#pragma once
//���ڴ����ķ��Ķ��롢����
# include "head.h"
# include "lexical_analysis.h"

//�ķ������еķ���
class grammar_symbol {
public:
	//�ڲ���Ա���ķ����������пմ����ս�������ս������ֹ�������֣�
	enum Symbol_type {Epsilon=0, Terminal, NonTerminal,End};
	Symbol_type type;//�ķ����ŵ�����
	set<int> first_set;//first����ʹ��set��ԭ������Ϊ�����Զ�����
	set<int> follow_set;//follow����ʹ��set��ԭ������Ϊ�����Զ�����
	string token;//�����ַ���

	//���캯��
	grammar_symbol(Symbol_type type_,const string token_);
};

//�ķ������е���
class grammar_item {
public:
	int left_symbol;//����ʽ��ߵ�symbol
	vector<int> right_symbol;//����ʽ�ұߵ�symbol
	
	//���캯��
	grammar_item(const int left_symbol_, const vector<int>& right_symbol_);
};

//�����ķ�
class grammar
{
public:
	const string EpsilonToken = "@";// Epsilon
	const string SplitToken = " | ";// ����ʽ�Ҳ��ָ���
	const string ProToken= "::=";// ����ʽ���Ҳ��ָ���
	const string EndToken = "#";// βtoken ��ֹ����
	const string StartToken = "Program";// �ķ���ʼ����
	const string ExtendStartToken = "S";// ��չ�ķ���ʼ����
	const string AllTerminalToken = "%token";//���е��ս��

	vector<grammar_symbol> symbols; //�ķ������з��ż���
	set<int> terminals;//�ս����symbols�е��±꣨ϣ������������set��
	set<int> non_terminals;//���ս����symbols�е��±꣨ϣ������������set��
	vector<grammar_item> productions;//���еĲ���ʽ
	int start_production;//��ʼ����ʽ��productions�е�λ��

	//���캯��������read_grammar��getFirstOfNonterminal��
	grammar(const string file_path);

	//�������е�grammar
	void read_grammar(const string file_path);
	//�����ַ����ҵ�����symbols�е�index������ҵ�����index�����û���ҵ�����-1
	int find_symbol_index_by_token(const string token);
	//��ʼ�����е�NonTerminal��first��
	void getFirstOfNonterminal();
	//����һ�����Ŵ���first��
	set<int> getFirstOfString(const vector<int>& str);

};

