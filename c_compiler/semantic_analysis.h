#pragma once
#include "head.h"
#include "Generate_asm.h"


//��������еķ���
struct Semantic_symbol
{
	string token; //token����
	string value; //ֵ
	int row;//��
	int col;//��
	int table_index;//��������table��index
	int index;//������table�ڲ���index
};

//��ʶ����Ϣ������������������ʱ�����������ľ�����Ϣ
struct IdentifierInfo 
{
	//���ֱ�ʶ�����ͣ��ֱ�Ϊ��������������ʱ����������������ֵ
	enum IdentifierType { Function, Variable, TempVar, ConstVar, ReturnVar };

	IdentifierType identifier_type;//��ʶ��������
	string specifier_type;//��(��)������/������������
	string identifier_name;//��ʶ������/����ֵ
	int function_parameter_num;//������������
	int function_entry;//������ڵ�ַ(��Ԫʽ�ı��)
	int function_table_index;//�����ĺ������ű�����������ķ��ű��б��е�����

};

//��������еķ��ű�
struct Semantic_symbol_table 
{
	//���ֱ�����ͣ��ֱ�Ϊȫ�ֱ��������鼶����ʱ��
	enum TableType {GlobalTable , FunctionTable , BlockTable , TempTable};

	TableType table_type;//�������
	vector<IdentifierInfo> table;//���ű�
	string table_name;//����

	//���캯��
	Semantic_symbol_table(const TableType table_type_,const string table_name_);
	//Ѱ��һ������
	int Find_symbol(const string id_name);
	//����һ�����������ؼ����λ��
	int Add_symbol(const IdentifierInfo id);
};

//�������
class Semantic_analysis
{
public:
	vector<Quaternary> quaternary;//��Ԫʽ
	int main_index;//main������Ӧ����Ԫʽ���
	int backpatching_level;//������
	vector<int> backpatching_list;//�����б�
	int next_quaternary_index;//��һ����Ԫʽ���
	int tmp_var_count;//��ʱ��������
	vector<Semantic_symbol> symbol_list;//����������̵ķ�����
	vector<Semantic_symbol_table> tables;//�������з��ű�
	vector<int> current_table_stack;//��ǰ�������Ӧ�ķ��ű�����ջ

	//���캯��
	Semantic_analysis();
	//�����еķ�����Ϣ����symbol_list
	void Add_symbol_to_list(const Semantic_symbol symbol);
	//��������
	void Analysis(const string production_left, const vector<string> production_right);
	//��ӡ��Ԫʽ��
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

