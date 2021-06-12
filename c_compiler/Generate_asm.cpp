#include "Generate_asm.h"

//共32个寄存器
vector<string> registers = { "$zero", //$0 常量0(constant value 0)
	"$at", //$1 保留给汇编器(Reserved for assembler)
	"$v0","$v1", //$2-$3 函数调用返回值(values for results and expression evaluation)
	"$a0","$a1","$a2","$a3", //$4-$7 函数调用参数(arguments)
	"$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7", //$8-$15 暂时的(或随便用的)
	"$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7", //$16-$23 保存的(或如果用，需要SAVE/RESTORE的)(saved)
	"$t8","$t9", //$24-$25 暂时的(或随便用的)
	"$k0","k1",//操作系统／异常处理保留，至少要预留一个 
	"$gp", //$28 全局指针(Global Pointer)
	"$sp", //$29 堆栈指针(Stack Pointer)
	"$fp", //$30 帧指针(Frame Pointer)
	"$ra"//$31 返回地址(return address)
};

//比较未使用时间长短
bool Reg::operator < (const Reg& b) {
	return unuse_time < b.unuse_time;
}

//构造函数
Generate_asm::Generate_asm(string file_path, vector<Quaternary> quaternary_)
{
	//打开文件
	asm_ostream.open(file_path, ios::out);
	if (!asm_ostream.is_open()) {
		cout << "无法打开MIPS汇编代码的生成文件" << endl;
		throw(FILE_OPEN_ERROE);
	}

	//存入四元式
	quaternary = quaternary_;
	for (auto& it : quaternary) {
		if (it.operator_type == "j" || it.operator_type == "j=" || it.operator_type == "j<" || it.operator_type == "j>" || it.operator_type == "j<="|| it.operator_type == "j>=") {
			label_need_output.insert(it.result);
		}
	}

	//初始化寄存器信息
	for (int i = 0; i < RegNum; i++)
		regs_info.push_back({ registers[i],i,0 });

	//初始化寄存器中内容(除0外全部赋值为null，在退出一个过程后局部变量需要释放)
	RVALUE[0] = "0";
	for (int i = 1; i < RegNum; i++)
		RVALUE[i] = "null";
}
//从四元式到汇编的过程
void Generate_asm::parse()
{
	//需要先为$sp赋值esp（顶），为$fp赋值ebp（返回地址）
	asm_ostream << "addi $sp,$sp," << to_string(STACK_SEG_ADDR) << endl;
	asm_ostream << "addi $fp,$fp," << to_string(STACK_SEG_ADDR - 4) << endl;

	//遍历每一个四元式
	for (auto present_quaternary = quaternary.begin(); present_quaternary != quaternary.end(); present_quaternary++) {
		//unuse_time++
		for (auto it = regs_info.begin(); it != regs_info.end(); it++)
			if (it->unuse_time < REG_MAX_UNUSETIME)
				it->unuse_time++;
		//真正生成汇编的过程
		//栈帧结构
		parse_step(present_quaternary);
	}
}

void Generate_asm::parse_step(vector<Quaternary>::iterator present_quaternary)
{
	//如果是需要输出标签的，则输出标签
	if (label_need_output.find(to_string(present_quaternary->index)) != label_need_output.end()) {
		asm_ostream << "Label_" << to_string(present_quaternary->index) << " :" << endl;
	}

	//如果是进行函数调用，需要创建栈帧并跳转(参数已完成压栈的情况)
	if (present_quaternary->operator_type == "call") {
		//跳转前将寄存器现场保存（存在内存）（这是合理的嘛？我不知道哎）
		for (auto& it : local_var_offset_table) {
			if (AVALUE.at(it.first).empty())
				continue;
			//存最近用到的
			int unuse_time = REG_MAX_UNUSETIME;
			string write_reg;
			for (auto& r : AVALUE.at(it.first)) {
				if (regs_info[get_reg_index_by_name(r)].unuse_time < unuse_time) {
					unuse_time = regs_info[get_reg_index_by_name(r)].unuse_time;
					write_reg = r;
				}
			}
			asm_ostream << "sw " << write_reg << "," << it.second << "($fp)" << endl;
		}

		//跳转
		asm_ostream << "jal " << present_quaternary->arg1 << endl;
		//记录存储返回值的变量
		//返回值的变量可能已经在内存中，也可能是第一次出现，对其进行寄存器的分配
		int reg_index = get_reg(present_quaternary->result, -1);

		//存储返回变量前先恢复寄存器
		for (auto& it : local_var_offset_table) {
			if (AVALUE.at(it.first).empty())
				continue;
			//每个寄存器都写
			for (auto& r : AVALUE.at(it.first)) {
				asm_ostream << "lw " << r << "," << it.second << "($fp)" << endl;
			}
		}

		asm_ostream << "move " << registers[reg_index] << ",$v1" << endl;
		mark_reg_in_regs_info(reg_index);
	}
	//如果是进行返回，需要撤销栈帧并跳转
	else if (present_quaternary->operator_type == "return") {
		//将返回值放在v1寄存器中
		if (present_quaternary->arg1 != "-")
			asm_ostream << "move $v1," << *(AVALUE.at(present_quaternary->arg1).begin()) << endl;

		//释放局部变量占用的寄存器,修改AVALUE与RVALUE
		for (auto it = local_var_offset_table.begin(); it != local_var_offset_table.end(); it++) {
			if (AVALUE.find(it->first) != AVALUE.end()) {
				for (auto reg = AVALUE.at(it->first).begin(); reg != AVALUE.at(it->first).end(); reg++)
					for (int i = 0; i < RegNum; i++)
						if (*reg == registers[i])
							RVALUE[i] = "null";
				AVALUE.erase(it->first);
			}
		}

		//修改esp($sp)，返回地址($fp)放到esp($sp)
		asm_ostream << "move $sp,$fp" << endl;
		asm_ostream << "addi $sp,$sp," << to_string(param_offset) << endl;
		//返回地址赋给$ra
		asm_ostream << "lw $ra,4($fp)" << endl;
		//修改ebp（$fp）
		asm_ostream << "lw $fp,0($fp)" << endl;

		//如果不是main函数则跳转
		if (procedureName != "main") {
			asm_ostream << "jr $ra" << endl;
		}
		//将一系列的值进行初始化
		param_offset = PARAM_OFFSET_INIT;
		localvar_offset = LOCALVAR_OFFSET_INIT;
		local_var_offset_table.clear();
		procedureName = "";
	}
	//如果是j
	else if (present_quaternary->operator_type == "j") {
		asm_ostream << "j ";
		//如果是跳转到函数
		if (!isdigit(present_quaternary->result[0])) {
			for (auto it = quaternary.begin(); it != quaternary.end(); it++)
				if (present_quaternary->result == to_string(it->index))
					asm_ostream << it->operator_type << endl;
		}
		//如果是跳转到标签
		else {
			asm_ostream << "Label_" << present_quaternary->result << endl;
		}
	}
	//如果是计算（+/*/-）A:=B op C 或（+/*/-）A:=B op 立即数
	else if (present_quaternary->operator_type == "+" || present_quaternary->operator_type == "*" || present_quaternary->operator_type == "-" || present_quaternary->operator_type == "/") {
		//A可能已经在内存中，也可能是第一次出现，对其进行寄存器的分配
		int reg_index_A = get_reg(present_quaternary->result, -1);
		int reg_index_B = -1;
		int reg_index_C = -1;

		//如果B的值不在寄存器中（语义分析已经保证了B的值一定在AVALUE）
		if (AVALUE.at(present_quaternary->arg1).empty()) {
			//如果B是局部变量
			if (local_var_offset_table.find(present_quaternary->arg1) != local_var_offset_table.end()) {
				int offset = local_var_offset_table.at(present_quaternary->arg1);
				asm_ostream << "lw " << registers[reg_index_A] << "," << to_string(offset) << "($fp)" << endl;
			}
			//如果B是全局变量
			else if (global_var_addr_table.find(present_quaternary->arg1) != global_var_addr_table.end()) {
				int addr = global_var_addr_table.at(present_quaternary->arg1);
				asm_ostream << "lw " << registers[reg_index_A] << "," << to_string(addr) << "($zero)" << endl;
			}
			reg_index_B = reg_index_A;
		}
		else {// B的值在寄存器中
			reg_index_B = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg1).begin()));
		}
		mark_reg_in_regs_info(reg_index_B);

		//如果C不是立即数
		if (!isdigit(present_quaternary->arg2[0])) {
			//如果C的值不在寄存器中（语义分析已经保证了B的值一定在AVALUE）
			if (AVALUE.at(present_quaternary->arg2).empty()) {
				//给C分配一个寄存器，除了刚才分配给A的
				reg_index_C = get_reg(present_quaternary->arg2, reg_index_A);
			}
			else {// C的值在寄存器中
				reg_index_C = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg2).begin()));
			}
			mark_reg_in_regs_info(reg_index_C);

			if (present_quaternary->operator_type == "+")
				asm_ostream << "add " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
			else if (present_quaternary->operator_type == "*")//mult是保存到{hi，lo}
				asm_ostream << "mul " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
			else if (present_quaternary->operator_type == "-")
				asm_ostream << "sub " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
			else if (present_quaternary->operator_type == "/") {
				asm_ostream << "div " << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
				asm_ostream << "mov " << registers[reg_index_A] << ",$lo"  << endl;
			}
		}
		//如果C是立即数
		else {
			if (present_quaternary->operator_type == "+")
				asm_ostream << "addi " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << present_quaternary->arg2 << endl;
			else if (present_quaternary->operator_type == "*") {//mult是保存到{hi，lo}
				//MIPS中没有立即数乘法，必须先把present_quaternary->arg2赋值到一个临时寄存器
				asm_ostream << "addi $t8,$zero," << present_quaternary->arg2 << endl;

				asm_ostream << "mul " << registers[reg_index_A] << "," << registers[reg_index_B] << ",$t8" << endl;
			}
			else if (present_quaternary->operator_type == "-") {
				asm_ostream << "subi " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << present_quaternary->arg2 << endl;
			}
			else if (present_quaternary->operator_type == "/") {
				//MIPS中没有立即数除法，必须先把-present_quaternary->arg2赋值到一个临时寄存器
				asm_ostream << "addi $t8,$zero," << present_quaternary->arg2 << endl;
				asm_ostream << "div " << registers[reg_index_B] << ",$t8" << endl;
				asm_ostream << "mov " << registers[reg_index_A] << ",$lo" << endl;
			}
		}
	}
	//如果是=
	else if (present_quaternary->operator_type == "=") {
		int reg_index = get_reg(present_quaternary->result, -1);

		//如果是立即数赋值
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "addi " << registers[reg_index] << ",$zero," << present_quaternary->arg1 << endl;
			mark_reg_in_regs_info(reg_index);
		}
		//如果是变量赋值
		else {
			//如果右值B在寄存器中(A=B)
			if (!AVALUE.at(present_quaternary->arg1).empty()) {
				//获得右值所在寄存器index
				int reg_index_B = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg1).begin()));
				asm_ostream << "move " << registers[reg_index] << "," << registers[reg_index_B] << endl;
				mark_reg_in_regs_info(reg_index);
				mark_reg_in_regs_info(reg_index_B);
			}
			//如果右值不在寄存器中
			else {
				//获得右值变量的内存地址，将内容放入左值变量寄存器中
				//如果右值为局部变量
				if (local_var_offset_table.find(present_quaternary->arg1) != local_var_offset_table.end()) {
					int offset = local_var_offset_table.at(present_quaternary->arg1);
					asm_ostream << "lw " << registers[reg_index] << "," << to_string(offset) << "($fp)" << endl;
				}
				else if (global_var_addr_table.find(present_quaternary->arg1) != global_var_addr_table.end()) {//如果右值为全局变量
					int addr = global_var_addr_table.at(present_quaternary->arg1);
					asm_ostream << "lw " << registers[reg_index] << "," << to_string(addr) << "($zero)" << endl;
				}
				mark_reg_in_regs_info(reg_index);
			}
		}
	}
	//如果是实参声明
	else if (present_quaternary->operator_type == "param") {
		int reg_index;
		//将参数变量压栈
		//先判断参数是否在寄存器中(考虑参数既可能是局部变量也可能是全局变量)	
		//参数不在寄存器中
		if (AVALUE.at(present_quaternary->arg1).empty()) {
			//是局部变量
			if (local_var_offset_table.find(present_quaternary->arg1) != local_var_offset_table.end()) {
				int offset = local_var_offset_table.at(present_quaternary->arg1);
				reg_index = get_reg(present_quaternary->arg1, -1);
				asm_ostream << "lw " << registers[reg_index] << "," << to_string(offset) << "($fp)" << endl;
				asm_ostream << "subi $sp,$sp,4" << endl;
				asm_ostream << "sw " << registers[reg_index] << ",0($sp)" << endl;
			}
			//是全局变量
			else if (global_var_addr_table.find(present_quaternary->arg1) != global_var_addr_table.end()) {
				int addr = global_var_addr_table.at(present_quaternary->arg1);
				reg_index = get_reg(present_quaternary->arg1, -1);
				asm_ostream << "lw " << registers[reg_index] << "," << to_string(addr) << "($zero)" << endl;
				asm_ostream << "subi $sp,$sp,4" << endl;
				asm_ostream << "sw " << registers[reg_index] << ",0($sp)" << endl;
			}
		}
		//参数在寄存器中
		else {
			reg_index = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg1).begin()));
			asm_ostream << "subi $sp,$sp,4" << endl;
			asm_ostream << "sw " << registers[reg_index] << ",0($sp)" << endl;
		}
		mark_reg_in_regs_info(reg_index);
	}
	//如果是形参声明
	else if (present_quaternary->operator_type == "defpar") {
		//为形参变量分配内存空间（给定偏移地址）
		local_var_offset_table.insert({ present_quaternary->result, {param_offset} });
		param_offset += 4;
		AVALUE.insert(pair<string, set<string>>(present_quaternary->result, {}));
	}
	//如果是过程定义（后三个都是-）
	else if (present_quaternary->arg1 == "-" && present_quaternary->arg2 == "-" && present_quaternary->result == "-") {
		//进入一个过程块，重置局部偏移表和形参偏移表
		procedureName = present_quaternary->operator_type;
		local_var_offset_table.clear();
		param_offset = PARAM_OFFSET_INIT;
		localvar_offset = LOCALVAR_OFFSET_INIT;

		//过程标号
		asm_ostream << present_quaternary->operator_type << " :" << endl;
		//压栈返回地址
		asm_ostream << "subi $sp,$sp,4" << endl;
		//mips中ra会在j时自动存储之前的pc
		asm_ostream << "sw $ra,0($sp)" << endl;
		asm_ostream << "subi $sp,$sp,4" << endl;
		//压栈旧的$fp
		asm_ostream << "sw $fp,0($sp)" << endl;
		//赋值新$fp
		asm_ostream << "move $fp,$sp" << endl;
	}
	//如果是j>
	else if (present_quaternary->operator_type == "j>") {
		//C = A > B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//如果A是立即数，把A放在$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "subi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//否则找到A所在的寄存器/为A抢占寄存器
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//如果B是立即数，把B放在$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "subi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//否则找到B所在的寄存器/为B抢占寄存器
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_B] << "," << registers[reg_index_A] << endl;
		//A>B 则$t8为1，A<=B 则$t8为0
		//asm_ostream << "bne $t8,$zero,Label_" << present_quaternary->result << endl;
		asm_ostream << "bgt "<< registers[reg_index_A] << "," << registers[reg_index_B] <<",Label_" << present_quaternary->result << endl;
	}
	//如果是j<
	else if (present_quaternary->operator_type == "j<") {
		//C = A < B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//如果A是立即数，把A放在$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "subi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//否则找到A所在的寄存器/为A抢占寄存器
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//如果B是立即数，把B放在$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "subi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//否则找到B所在的寄存器/为B抢占寄存器
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_A] << "," << registers[reg_index_B] << endl;
		//A<B 则$t8为1，A>=B 则$t8为0
		//asm_ostream << "bne $t8,$zero, Label_" << present_quaternary->result << endl;
		asm_ostream << "blt " << registers[reg_index_A] << "," << registers[reg_index_B] << ",Label_" << present_quaternary->result << endl;
	}
	//如果是j>=
	else if (present_quaternary->operator_type == "j>=") {
		//C = A >= B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//如果A是立即数，把A放在$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "addi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//否则找到A所在的寄存器/为A抢占寄存器
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//如果B是立即数，把B放在$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "addi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//否则找到B所在的寄存器/为B抢占寄存器
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_A] << "," << registers[reg_index_B] << endl;
		//A>=B 则$t8为0，A<B 则$t8为1
		//asm_ostream << "beq $t8,$zero,Label_" << present_quaternary->result << endl;
		asm_ostream << "bge " << registers[reg_index_A] << "," << registers[reg_index_B] << ",Label_" << present_quaternary->result << endl;

	}
	//如果是j<=
	else if (present_quaternary->operator_type == "j<=") {
		//C = A <= B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//如果A是立即数，把A放在$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "addi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//否则找到A所在的寄存器/为A抢占寄存器
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//如果B是立即数，把B放在$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "addi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//否则找到B所在的寄存器/为B抢占寄存器
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_B] << "," << registers[reg_index_A] << endl;
		//A<=B 则$t8为0，A>B 则$t8为1
		//asm_ostream << "beq $t8,$zero,Label_" << present_quaternary->result << endl;
		asm_ostream << "ble " << registers[reg_index_A] << "," << registers[reg_index_B] << ",Label_" << present_quaternary->result << endl;

	}
	//如果是j=
	else if (present_quaternary->operator_type == "j=") {
		//C = A == B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//如果A是立即数，把A放在$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "subi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//否则找到A所在的寄存器/为A抢占寄存器
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//如果B是立即数，把B放在$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "subi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//否则找到B所在的寄存器/为B抢占寄存器
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//A==B 则跳转
		asm_ostream << "beq "<< registers[reg_index_A] <<","<< registers[reg_index_B] <<",Label_" << present_quaternary->result << endl;
	}
}

//分配一个寄存器（返回分配的寄存器编号）
int Generate_asm::get_reg(string var, int except_index)
{
	//分配的寄存器编号和名称
	int reg_index = -1;

	//<1> 确保var在内存中有分配空间，若var不在内存中，为之分配内存空间
	//如果全局和局部变量表都没有这一变量，则var是未分配好空间的变量
	if (local_var_offset_table.find(var) == local_var_offset_table.end() && global_var_addr_table.find(var) == global_var_addr_table.end()) {
		//判断是局部变量还是全局变量
		//全局变量，则在全局变量地址表中分配地址
		if (procedureName == "") {
			global_var_addr_table.insert({ var, globalvar_addr });
			globalvar_addr += 4;//向下增长
		}
		//局部变量，在局部变量偏移表中分配
		else {
			local_var_offset_table.insert(pair<string, int>(var, localvar_offset));
			localvar_offset -= 4;//向上增长
			asm_ostream << "subi $sp,$sp,4" << endl;
		}
		//在AVALUE中插入
		AVALUE.insert({ var, {} });
	}

	//<2> 进行寄存器的分配
	//如果不在寄存器中，则分配寄存器
	if (AVALUE.at(var).empty()) {
		//寄存器的分配算法（寄存器只能使用$txx）
		//空余优先，没有空余的则LRU
		//查找有无空闲的reg
		for (int i = VAR_REG_START; i <= VAR_REG_END; i++) {
			if (RVALUE[i] == "null") {
				reg_index = i;//空闲的寄存器赋值
				//对该寄存器占用，修改RVAULE和AVALUE
				AVALUE.at(var).insert(registers[reg_index]);
				RVALUE[reg_index] = var;
				break;
			}

		}
		//没有找到空闲的reg，准备抢占一个reg
		if (reg_index == -1) {
			//获得要抢占的寄存器序号
			int to_seize_reg_index = get_lru_reg_index(except_index);
			//获得被抢占的变量名称
			string to_seize_var_name = RVALUE[to_seize_reg_index];
			//如果不需要为寄存器中的变量V生成存数指令
			if (AVALUE.at(to_seize_var_name).size() >= 2) {
				//不需要生成存数指令的条件（满足一个即可）：
				//1.AVALUE中看出V还保存在别的寄存器中
				//2.如果V是A，且不是B或C（暂未实现）
				//3.如果V不会在之后被使用（暂未实现）
			}
			//需要生成存数指令
			else {
				//判断V是局部变量还是全局变量采用不同的写方式
				//是局部变量
				if (local_var_offset_table.find(to_seize_var_name) != local_var_offset_table.end()) {
					int offset = local_var_offset_table.at(to_seize_var_name);
					asm_ostream << "sw " << registers[to_seize_reg_index] << "," << to_string(offset) << "($fp)" << endl;
				}
				//是全局变量
				else if (global_var_addr_table.find(to_seize_var_name) != global_var_addr_table.end()) {
					int addr = global_var_addr_table.at(to_seize_var_name);
					asm_ostream << "sw " << registers[to_seize_reg_index] << "," << to_string(addr) << "($zero)" << endl;
				}
			}
			//更新AVALUE和RVALUE
			AVALUE.at(to_seize_var_name).erase(registers[to_seize_reg_index]);
			reg_index = to_seize_reg_index;
			RVALUE[reg_index] = var;
			AVALUE.at(var).insert(registers[to_seize_reg_index]);
		}
		//是局部变量
		if (local_var_offset_table.find(var) != local_var_offset_table.end()) {
			int offset = local_var_offset_table.at(var);
			asm_ostream << "lw " << registers[reg_index] << "," << to_string(offset) << "($fp)" << endl;
		}
		//是全局变量
		else if (global_var_addr_table.find(var) != global_var_addr_table.end()) {
			int addr = global_var_addr_table.at(var);
			asm_ostream << "lw " << registers[reg_index] << "," << to_string(addr) << "($zero)" << endl;
		}
	}
	//若已经在寄存器中，则获取这个寄存器的编号
	else {
		string reg_name = *(AVALUE.at(var).begin());
		for (int i = 0; i < RegNum; i++) {
			if (registers[i] == reg_name) {
				reg_index = i;
				break;
			}
		}
	}
	mark_reg_in_regs_info(reg_index);
	return reg_index;
}

//查找LRU寄存器
int Generate_asm::get_lru_reg_index(int except_index) {
	int max_index = 0;
	int max_unuse = 0;
	for (int i = 0; i < regs_info.size(); i++) {
		//要注意抢占的只能是范围内寄存器，不可以是except_index
		if (regs_info[i].unuse_time > max_unuse && regs_info[i].reg_index >= VAR_REG_START && regs_info[i].reg_index <= VAR_REG_END && (except_index == -1 || except_index != regs_info[i].reg_index)) {
			max_index = i;
			max_unuse = regs_info[i].unuse_time;
		}
	}
	return regs_info[max_index].reg_index;
}
//在regs_info中进行更新
void Generate_asm::mark_reg_in_regs_info(int reg_index) {
	for (auto it = regs_info.begin(); it != regs_info.end(); it++) {
		if (it->reg_index == reg_index) {
			it->unuse_time = 0;
			return;
		}
	}
}

int Generate_asm::get_reg_index_by_name(string reg_name)
{
	for (auto it = registers.begin(); it != registers.end(); it++) {
		if (*it == reg_name)
			return it - registers.begin();
	}
	return -1;
}