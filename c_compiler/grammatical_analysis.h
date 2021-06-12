#pragma once
#include"grammar.h"
#include "semantic_analysis.h"
//һ��LR1����
class LR1_item {
public:
	//ǰ����ͬgrammar_item��ֱ�Ӹ��ƹ���
	int left_symbol;//����ʽ��ߵ�symbol
	vector<int> right_symbol;//����ʽ�ұߵ�symbol

	int index;//��grammar_item��productions���е�index
	int dot_position;//���λ��
	int lookahead_symbol;//չ������symbols�е��±�

	//���캯��
	LR1_item(const int left_symbol_, const vector<int>& right_symbol_, const int index_, const int dot_position_, const int lookahead_symbol_);
	//�ж����
	bool operator==(const LR1_item& item);
};

//һ��LR1��closure
class LR1_closure {
public:
	vector<LR1_item> lr1_closure;

	bool operator==(const LR1_closure& closure);
};


//���������еĶ���
enum Action {
	ShiftIn, // ����
	Reduce,  // ��Լ
	Accept,  // ����
	Error	// ����
};
//����Ķ�����Ϣ
struct ActionInfo {
	Action action;	//����
	int info;	//��Լ����ʽ��ת��״̬
};

//LR1�ķ�
class LR1 :public grammar {
public:
	//�������
	vector<LR1_closure> lr1_cluster;
	//goto��Ϣ��¼������intΪ����ǰclosure��lr1_cluster�еı�ţ���ǰ������symbols�б�ţ�ת�Ƶ���closure��lr1_cluster�еı��
	map<pair<int, int>, int> goto_info;
	//GOTO��GOTO[i,A]=j��ֻ�õ�Action Error(��ʾδ����)��ShiftIn(��ʾת��)
	map<pair<int, int>, ActionInfo>goto_table;
	//ACTION��ACTION[i, A] = "����/��Լ/����"
	map<pair<int, int>, ActionInfo>action_table;
	//���������
	Semantic_analysis symantic_analysis;

	//���캯��
	LR1(const string file_path);
	//����LR1�
	void generateCluster();
	//���ɱհ�
	LR1_closure& generateClosure(LR1_closure& initial_closure);
	//����GOTO��closure
	LR1_closure generateGOTO(const LR1_closure& from_closure, int present_symbol);
	//����Table
	void generateTable();
	//��ӡTable
	void printTable(const string file_path);
	//�����﷨�������������
	void parser(vector<TOKEN> token_stream, const string file_path);
	//�����﷨��
	void draw_grammatical_analysis_tree(vector<TOKEN> token_stream, const string file_path);

};
