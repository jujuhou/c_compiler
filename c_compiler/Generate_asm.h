#pragma once
#include "head.h"

#define RegNum 32
#define DATA_SEG_ADDR 0x10010000 //���ݶε���ʼ��ַ
#define STACK_SEG_ADDR 0x10040000  //��ջ�ε���ʼ��ַ
#define PARAM_OFFSET_INIT 8//����ջ֡��ʵ�����ĳ�ʼ��С
#define LOCALVAR_OFFSET_INIT -4//�ֲ��������ĳ�ʼ��С
#define REG_MAX_UNUSETIME INT_MAX
#define STR_SIZE 28
#define VAR_REG_START 8 //���ڱ��������REG
#define VAR_REG_END 15 //���ڱ��������REG

//��Ԫʽ
struct Quaternary
{
	int index;//��Ԫʽ���
	string operator_type;//��������
	string arg1;//������1
	string arg2;//������2
	string result;//���
};

//��¼Reg�����ơ���š�δʹ��ʱ��
struct Reg
{
	string reg_name;//����
	int reg_index;//���
	int unuse_time;//δʹ��ʱ��
	bool operator < (const Reg& b);
};

class Generate_asm
{
public:
	string RVALUE[RegNum] ;//ÿ���Ĵ����е�ǰ�ı���
	map<string, set<string>> AVALUE;//�������ڵļĴ�������
	vector<Reg>regs_info;//���мĴ�����ʹ����Ϣ
	vector<Quaternary> quaternary;//��Ԫʽ
	ofstream asm_ostream;//�����
	map<string, int> local_var_offset_table;  //�ֲ����������ebp��ƫ�ƣ�����Ǹ�ֵ���õ�ʱ��ebp+offset��
	map<string, int> global_var_addr_table; //ȫ�ֱ�����ַ
	int param_offset = PARAM_OFFSET_INIT; //��¼ʵ������С
	int localvar_offset = LOCALVAR_OFFSET_INIT;//��¼�ֲ���������С
	int globalvar_addr = DATA_SEG_ADDR;//��¼ȫ�ֱ�������С
	string procedureName=""; //��¼���̿������
	set<string> label_need_output;//��Ҫ���������label

	
	//���캯��
	Generate_asm(string file_path, vector<Quaternary> quaternary_);
	//����Ԫʽ�����Ĺ���
	void parse();
	void parse_step(vector<Quaternary>::iterator present_quaternary);
	//����һ���Ĵ��������ط���ļĴ�����ţ�
	int get_reg(string var, int except_index);
	//����LRU�Ĵ���
	int get_lru_reg_index(int except_index);
	//��regs_info�н��и���
	void mark_reg_in_regs_info(int reg_index);
	//ͨ��reg��name�õ�index
	int get_reg_index_by_name(string reg_name);



};

