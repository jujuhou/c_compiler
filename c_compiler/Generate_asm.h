#pragma once
#include "head.h"

#define RegNum 32
#define DATA_SEG_ADDR 0x10010000 //数据段的起始地址
#define STACK_SEG_ADDR 0x10040000  //堆栈段的起始地址
#define PARAM_OFFSET_INIT 8//弹出栈帧的实参区的初始大小
#define LOCALVAR_OFFSET_INIT -4//局部变量区的初始大小
#define REG_MAX_UNUSETIME INT_MAX
#define STR_SIZE 28
#define VAR_REG_START 8 //用于保存变量的REG
#define VAR_REG_END 15 //用于保存变量的REG

//四元式
struct Quaternary
{
	int index;//四元式标号
	string operator_type;//操作类型
	string arg1;//操作数1
	string arg2;//操作数2
	string result;//结果
};

//记录Reg的名称、编号、未使用时间
struct Reg
{
	string reg_name;//名称
	int reg_index;//编号
	int unuse_time;//未使用时间
	bool operator < (const Reg& b);
};

class Generate_asm
{
public:
	string RVALUE[RegNum] ;//每个寄存器中当前的变量
	map<string, set<string>> AVALUE;//变量所在的寄存器数组
	vector<Reg>regs_info;//所有寄存器的使用信息
	vector<Quaternary> quaternary;//四元式
	ofstream asm_ostream;//输出流
	map<string, int> local_var_offset_table;  //局部变量相对于ebp的偏移（存的是负值，用的时候ebp+offset）
	map<string, int> global_var_addr_table; //全局变量地址
	int param_offset = PARAM_OFFSET_INIT; //记录实参区大小
	int localvar_offset = LOCALVAR_OFFSET_INIT;//记录局部变量区大小
	int globalvar_addr = DATA_SEG_ADDR;//记录全局变量区大小
	string procedureName=""; //记录过程块的名称
	set<string> label_need_output;//需要进行输出的label

	
	//构造函数
	Generate_asm(string file_path, vector<Quaternary> quaternary_);
	//从四元式到汇编的过程
	void parse();
	void parse_step(vector<Quaternary>::iterator present_quaternary);
	//分配一个寄存器（返回分配的寄存器编号）
	int get_reg(string var, int except_index);
	//查找LRU寄存器
	int get_lru_reg_index(int except_index);
	//在regs_info中进行更新
	void mark_reg_in_regs_info(int reg_index);
	//通过reg的name得到index
	int get_reg_index_by_name(string reg_name);



};

