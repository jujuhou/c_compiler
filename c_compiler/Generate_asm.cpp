#include "Generate_asm.h"

//��32���Ĵ���
vector<string> registers = { "$zero", //$0 ����0(constant value 0)
	"$at", //$1 �����������(Reserved for assembler)
	"$v0","$v1", //$2-$3 �������÷���ֵ(values for results and expression evaluation)
	"$a0","$a1","$a2","$a3", //$4-$7 �������ò���(arguments)
	"$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7", //$8-$15 ��ʱ��(������õ�)
	"$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7", //$16-$23 �����(������ã���ҪSAVE/RESTORE��)(saved)
	"$t8","$t9", //$24-$25 ��ʱ��(������õ�)
	"$k0","k1",//����ϵͳ���쳣������������ҪԤ��һ�� 
	"$gp", //$28 ȫ��ָ��(Global Pointer)
	"$sp", //$29 ��ջָ��(Stack Pointer)
	"$fp", //$30 ָ֡��(Frame Pointer)
	"$ra"//$31 ���ص�ַ(return address)
};

//�Ƚ�δʹ��ʱ�䳤��
bool Reg::operator < (const Reg& b) {
	return unuse_time < b.unuse_time;
}

//���캯��
Generate_asm::Generate_asm(string file_path, vector<Quaternary> quaternary_)
{
	//���ļ�
	asm_ostream.open(file_path, ios::out);
	if (!asm_ostream.is_open()) {
		cout << "�޷���MIPS������������ļ�" << endl;
		throw(FILE_OPEN_ERROE);
	}

	//������Ԫʽ
	quaternary = quaternary_;
	for (auto& it : quaternary) {
		if (it.operator_type == "j" || it.operator_type == "j=" || it.operator_type == "j<" || it.operator_type == "j>" || it.operator_type == "j<="|| it.operator_type == "j>=") {
			label_need_output.insert(it.result);
		}
	}

	//��ʼ���Ĵ�����Ϣ
	for (int i = 0; i < RegNum; i++)
		regs_info.push_back({ registers[i],i,0 });

	//��ʼ���Ĵ���������(��0��ȫ����ֵΪnull�����˳�һ�����̺�ֲ�������Ҫ�ͷ�)
	RVALUE[0] = "0";
	for (int i = 1; i < RegNum; i++)
		RVALUE[i] = "null";
}
//����Ԫʽ�����Ĺ���
void Generate_asm::parse()
{
	//��Ҫ��Ϊ$sp��ֵesp��������Ϊ$fp��ֵebp�����ص�ַ��
	asm_ostream << "addi $sp,$sp," << to_string(STACK_SEG_ADDR) << endl;
	asm_ostream << "addi $fp,$fp," << to_string(STACK_SEG_ADDR - 4) << endl;

	//����ÿһ����Ԫʽ
	for (auto present_quaternary = quaternary.begin(); present_quaternary != quaternary.end(); present_quaternary++) {
		//unuse_time++
		for (auto it = regs_info.begin(); it != regs_info.end(); it++)
			if (it->unuse_time < REG_MAX_UNUSETIME)
				it->unuse_time++;
		//�������ɻ��Ĺ���
		//ջ֡�ṹ
		parse_step(present_quaternary);
	}
}

void Generate_asm::parse_step(vector<Quaternary>::iterator present_quaternary)
{
	//�������Ҫ�����ǩ�ģ��������ǩ
	if (label_need_output.find(to_string(present_quaternary->index)) != label_need_output.end()) {
		asm_ostream << "Label_" << to_string(present_quaternary->index) << " :" << endl;
	}

	//����ǽ��к������ã���Ҫ����ջ֡����ת(���������ѹջ�����)
	if (present_quaternary->operator_type == "call") {
		//��תǰ���Ĵ����ֳ����棨�����ڴ棩�����Ǻ������Ҳ�֪������
		for (auto& it : local_var_offset_table) {
			if (AVALUE.at(it.first).empty())
				continue;
			//������õ���
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

		//��ת
		asm_ostream << "jal " << present_quaternary->arg1 << endl;
		//��¼�洢����ֵ�ı���
		//����ֵ�ı��������Ѿ����ڴ��У�Ҳ�����ǵ�һ�γ��֣�������мĴ����ķ���
		int reg_index = get_reg(present_quaternary->result, -1);

		//�洢���ر���ǰ�Ȼָ��Ĵ���
		for (auto& it : local_var_offset_table) {
			if (AVALUE.at(it.first).empty())
				continue;
			//ÿ���Ĵ�����д
			for (auto& r : AVALUE.at(it.first)) {
				asm_ostream << "lw " << r << "," << it.second << "($fp)" << endl;
			}
		}

		asm_ostream << "move " << registers[reg_index] << ",$v1" << endl;
		mark_reg_in_regs_info(reg_index);
	}
	//����ǽ��з��أ���Ҫ����ջ֡����ת
	else if (present_quaternary->operator_type == "return") {
		//������ֵ����v1�Ĵ�����
		if (present_quaternary->arg1 != "-")
			asm_ostream << "move $v1," << *(AVALUE.at(present_quaternary->arg1).begin()) << endl;

		//�ͷžֲ�����ռ�õļĴ���,�޸�AVALUE��RVALUE
		for (auto it = local_var_offset_table.begin(); it != local_var_offset_table.end(); it++) {
			if (AVALUE.find(it->first) != AVALUE.end()) {
				for (auto reg = AVALUE.at(it->first).begin(); reg != AVALUE.at(it->first).end(); reg++)
					for (int i = 0; i < RegNum; i++)
						if (*reg == registers[i])
							RVALUE[i] = "null";
				AVALUE.erase(it->first);
			}
		}

		//�޸�esp($sp)�����ص�ַ($fp)�ŵ�esp($sp)
		asm_ostream << "move $sp,$fp" << endl;
		asm_ostream << "addi $sp,$sp," << to_string(param_offset) << endl;
		//���ص�ַ����$ra
		asm_ostream << "lw $ra,4($fp)" << endl;
		//�޸�ebp��$fp��
		asm_ostream << "lw $fp,0($fp)" << endl;

		//�������main��������ת
		if (procedureName != "main") {
			asm_ostream << "jr $ra" << endl;
		}
		//��һϵ�е�ֵ���г�ʼ��
		param_offset = PARAM_OFFSET_INIT;
		localvar_offset = LOCALVAR_OFFSET_INIT;
		local_var_offset_table.clear();
		procedureName = "";
	}
	//�����j
	else if (present_quaternary->operator_type == "j") {
		asm_ostream << "j ";
		//�������ת������
		if (!isdigit(present_quaternary->result[0])) {
			for (auto it = quaternary.begin(); it != quaternary.end(); it++)
				if (present_quaternary->result == to_string(it->index))
					asm_ostream << it->operator_type << endl;
		}
		//�������ת����ǩ
		else {
			asm_ostream << "Label_" << present_quaternary->result << endl;
		}
	}
	//����Ǽ��㣨+/*/-��A:=B op C ��+/*/-��A:=B op ������
	else if (present_quaternary->operator_type == "+" || present_quaternary->operator_type == "*" || present_quaternary->operator_type == "-" || present_quaternary->operator_type == "/") {
		//A�����Ѿ����ڴ��У�Ҳ�����ǵ�һ�γ��֣�������мĴ����ķ���
		int reg_index_A = get_reg(present_quaternary->result, -1);
		int reg_index_B = -1;
		int reg_index_C = -1;

		//���B��ֵ���ڼĴ����У���������Ѿ���֤��B��ֵһ����AVALUE��
		if (AVALUE.at(present_quaternary->arg1).empty()) {
			//���B�Ǿֲ�����
			if (local_var_offset_table.find(present_quaternary->arg1) != local_var_offset_table.end()) {
				int offset = local_var_offset_table.at(present_quaternary->arg1);
				asm_ostream << "lw " << registers[reg_index_A] << "," << to_string(offset) << "($fp)" << endl;
			}
			//���B��ȫ�ֱ���
			else if (global_var_addr_table.find(present_quaternary->arg1) != global_var_addr_table.end()) {
				int addr = global_var_addr_table.at(present_quaternary->arg1);
				asm_ostream << "lw " << registers[reg_index_A] << "," << to_string(addr) << "($zero)" << endl;
			}
			reg_index_B = reg_index_A;
		}
		else {// B��ֵ�ڼĴ�����
			reg_index_B = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg1).begin()));
		}
		mark_reg_in_regs_info(reg_index_B);

		//���C����������
		if (!isdigit(present_quaternary->arg2[0])) {
			//���C��ֵ���ڼĴ����У���������Ѿ���֤��B��ֵһ����AVALUE��
			if (AVALUE.at(present_quaternary->arg2).empty()) {
				//��C����һ���Ĵ��������˸ղŷ����A��
				reg_index_C = get_reg(present_quaternary->arg2, reg_index_A);
			}
			else {// C��ֵ�ڼĴ�����
				reg_index_C = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg2).begin()));
			}
			mark_reg_in_regs_info(reg_index_C);

			if (present_quaternary->operator_type == "+")
				asm_ostream << "add " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
			else if (present_quaternary->operator_type == "*")//mult�Ǳ��浽{hi��lo}
				asm_ostream << "mul " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
			else if (present_quaternary->operator_type == "-")
				asm_ostream << "sub " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
			else if (present_quaternary->operator_type == "/") {
				asm_ostream << "div " << registers[reg_index_B] << "," << registers[reg_index_C] << endl;
				asm_ostream << "mov " << registers[reg_index_A] << ",$lo"  << endl;
			}
		}
		//���C��������
		else {
			if (present_quaternary->operator_type == "+")
				asm_ostream << "addi " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << present_quaternary->arg2 << endl;
			else if (present_quaternary->operator_type == "*") {//mult�Ǳ��浽{hi��lo}
				//MIPS��û���������˷��������Ȱ�present_quaternary->arg2��ֵ��һ����ʱ�Ĵ���
				asm_ostream << "addi $t8,$zero," << present_quaternary->arg2 << endl;

				asm_ostream << "mul " << registers[reg_index_A] << "," << registers[reg_index_B] << ",$t8" << endl;
			}
			else if (present_quaternary->operator_type == "-") {
				asm_ostream << "subi " << registers[reg_index_A] << "," << registers[reg_index_B] << "," << present_quaternary->arg2 << endl;
			}
			else if (present_quaternary->operator_type == "/") {
				//MIPS��û�������������������Ȱ�-present_quaternary->arg2��ֵ��һ����ʱ�Ĵ���
				asm_ostream << "addi $t8,$zero," << present_quaternary->arg2 << endl;
				asm_ostream << "div " << registers[reg_index_B] << ",$t8" << endl;
				asm_ostream << "mov " << registers[reg_index_A] << ",$lo" << endl;
			}
		}
	}
	//�����=
	else if (present_quaternary->operator_type == "=") {
		int reg_index = get_reg(present_quaternary->result, -1);

		//�������������ֵ
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "addi " << registers[reg_index] << ",$zero," << present_quaternary->arg1 << endl;
			mark_reg_in_regs_info(reg_index);
		}
		//����Ǳ�����ֵ
		else {
			//�����ֵB�ڼĴ�����(A=B)
			if (!AVALUE.at(present_quaternary->arg1).empty()) {
				//�����ֵ���ڼĴ���index
				int reg_index_B = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg1).begin()));
				asm_ostream << "move " << registers[reg_index] << "," << registers[reg_index_B] << endl;
				mark_reg_in_regs_info(reg_index);
				mark_reg_in_regs_info(reg_index_B);
			}
			//�����ֵ���ڼĴ�����
			else {
				//�����ֵ�������ڴ��ַ�������ݷ�����ֵ�����Ĵ�����
				//�����ֵΪ�ֲ�����
				if (local_var_offset_table.find(present_quaternary->arg1) != local_var_offset_table.end()) {
					int offset = local_var_offset_table.at(present_quaternary->arg1);
					asm_ostream << "lw " << registers[reg_index] << "," << to_string(offset) << "($fp)" << endl;
				}
				else if (global_var_addr_table.find(present_quaternary->arg1) != global_var_addr_table.end()) {//�����ֵΪȫ�ֱ���
					int addr = global_var_addr_table.at(present_quaternary->arg1);
					asm_ostream << "lw " << registers[reg_index] << "," << to_string(addr) << "($zero)" << endl;
				}
				mark_reg_in_regs_info(reg_index);
			}
		}
	}
	//�����ʵ������
	else if (present_quaternary->operator_type == "param") {
		int reg_index;
		//����������ѹջ
		//���жϲ����Ƿ��ڼĴ�����(���ǲ����ȿ����Ǿֲ�����Ҳ������ȫ�ֱ���)	
		//�������ڼĴ�����
		if (AVALUE.at(present_quaternary->arg1).empty()) {
			//�Ǿֲ�����
			if (local_var_offset_table.find(present_quaternary->arg1) != local_var_offset_table.end()) {
				int offset = local_var_offset_table.at(present_quaternary->arg1);
				reg_index = get_reg(present_quaternary->arg1, -1);
				asm_ostream << "lw " << registers[reg_index] << "," << to_string(offset) << "($fp)" << endl;
				asm_ostream << "subi $sp,$sp,4" << endl;
				asm_ostream << "sw " << registers[reg_index] << ",0($sp)" << endl;
			}
			//��ȫ�ֱ���
			else if (global_var_addr_table.find(present_quaternary->arg1) != global_var_addr_table.end()) {
				int addr = global_var_addr_table.at(present_quaternary->arg1);
				reg_index = get_reg(present_quaternary->arg1, -1);
				asm_ostream << "lw " << registers[reg_index] << "," << to_string(addr) << "($zero)" << endl;
				asm_ostream << "subi $sp,$sp,4" << endl;
				asm_ostream << "sw " << registers[reg_index] << ",0($sp)" << endl;
			}
		}
		//�����ڼĴ�����
		else {
			reg_index = get_reg_index_by_name(*(AVALUE.at(present_quaternary->arg1).begin()));
			asm_ostream << "subi $sp,$sp,4" << endl;
			asm_ostream << "sw " << registers[reg_index] << ",0($sp)" << endl;
		}
		mark_reg_in_regs_info(reg_index);
	}
	//������β�����
	else if (present_quaternary->operator_type == "defpar") {
		//Ϊ�βα��������ڴ�ռ䣨����ƫ�Ƶ�ַ��
		local_var_offset_table.insert({ present_quaternary->result, {param_offset} });
		param_offset += 4;
		AVALUE.insert(pair<string, set<string>>(present_quaternary->result, {}));
	}
	//����ǹ��̶��壨����������-��
	else if (present_quaternary->arg1 == "-" && present_quaternary->arg2 == "-" && present_quaternary->result == "-") {
		//����һ�����̿飬���þֲ�ƫ�Ʊ���β�ƫ�Ʊ�
		procedureName = present_quaternary->operator_type;
		local_var_offset_table.clear();
		param_offset = PARAM_OFFSET_INIT;
		localvar_offset = LOCALVAR_OFFSET_INIT;

		//���̱��
		asm_ostream << present_quaternary->operator_type << " :" << endl;
		//ѹջ���ص�ַ
		asm_ostream << "subi $sp,$sp,4" << endl;
		//mips��ra����jʱ�Զ��洢֮ǰ��pc
		asm_ostream << "sw $ra,0($sp)" << endl;
		asm_ostream << "subi $sp,$sp,4" << endl;
		//ѹջ�ɵ�$fp
		asm_ostream << "sw $fp,0($sp)" << endl;
		//��ֵ��$fp
		asm_ostream << "move $fp,$sp" << endl;
	}
	//�����j>
	else if (present_quaternary->operator_type == "j>") {
		//C = A > B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//���A������������A����$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "subi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//�����ҵ�A���ڵļĴ���/ΪA��ռ�Ĵ���
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//���B������������B����$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "subi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//�����ҵ�B���ڵļĴ���/ΪB��ռ�Ĵ���
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_B] << "," << registers[reg_index_A] << endl;
		//A>B ��$t8Ϊ1��A<=B ��$t8Ϊ0
		//asm_ostream << "bne $t8,$zero,Label_" << present_quaternary->result << endl;
		asm_ostream << "bgt "<< registers[reg_index_A] << "," << registers[reg_index_B] <<",Label_" << present_quaternary->result << endl;
	}
	//�����j<
	else if (present_quaternary->operator_type == "j<") {
		//C = A < B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//���A������������A����$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "subi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//�����ҵ�A���ڵļĴ���/ΪA��ռ�Ĵ���
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//���B������������B����$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "subi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//�����ҵ�B���ڵļĴ���/ΪB��ռ�Ĵ���
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_A] << "," << registers[reg_index_B] << endl;
		//A<B ��$t8Ϊ1��A>=B ��$t8Ϊ0
		//asm_ostream << "bne $t8,$zero, Label_" << present_quaternary->result << endl;
		asm_ostream << "blt " << registers[reg_index_A] << "," << registers[reg_index_B] << ",Label_" << present_quaternary->result << endl;
	}
	//�����j>=
	else if (present_quaternary->operator_type == "j>=") {
		//C = A >= B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//���A������������A����$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "addi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//�����ҵ�A���ڵļĴ���/ΪA��ռ�Ĵ���
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//���B������������B����$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "addi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//�����ҵ�B���ڵļĴ���/ΪB��ռ�Ĵ���
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_A] << "," << registers[reg_index_B] << endl;
		//A>=B ��$t8Ϊ0��A<B ��$t8Ϊ1
		//asm_ostream << "beq $t8,$zero,Label_" << present_quaternary->result << endl;
		asm_ostream << "bge " << registers[reg_index_A] << "," << registers[reg_index_B] << ",Label_" << present_quaternary->result << endl;

	}
	//�����j<=
	else if (present_quaternary->operator_type == "j<=") {
		//C = A <= B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//���A������������A����$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "addi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//�����ҵ�A���ڵļĴ���/ΪA��ռ�Ĵ���
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//���B������������B����$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "addi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//�����ҵ�B���ڵļĴ���/ΪB��ռ�Ĵ���
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//asm_ostream << "slt $t8," << registers[reg_index_B] << "," << registers[reg_index_A] << endl;
		//A<=B ��$t8Ϊ0��A>B ��$t8Ϊ1
		//asm_ostream << "beq $t8,$zero,Label_" << present_quaternary->result << endl;
		asm_ostream << "ble " << registers[reg_index_A] << "," << registers[reg_index_B] << ",Label_" << present_quaternary->result << endl;

	}
	//�����j=
	else if (present_quaternary->operator_type == "j=") {
		//C = A == B
		int reg_index_A = -1;
		int reg_index_B = -1;
		//���A������������A����$t8
		if (isdigit(present_quaternary->arg1[0])) {
			asm_ostream << "subi $t8,$zero," << present_quaternary->arg1 << endl;
			reg_index_A = get_reg_index_by_name("$t8");
		}
		//�����ҵ�A���ڵļĴ���/ΪA��ռ�Ĵ���
		else {
			reg_index_A = get_reg(present_quaternary->arg1, -1);
		}
		//���B������������B����$t9
		if (isdigit(present_quaternary->arg2[0])) {
			asm_ostream << "subi $t9,$zero," << present_quaternary->arg2 << endl;
			reg_index_B = get_reg_index_by_name("$t9");
		}
		//�����ҵ�B���ڵļĴ���/ΪB��ռ�Ĵ���
		else {
			reg_index_B = get_reg(present_quaternary->arg2, -1);
		}

		//A==B ����ת
		asm_ostream << "beq "<< registers[reg_index_A] <<","<< registers[reg_index_B] <<",Label_" << present_quaternary->result << endl;
	}
}

//����һ���Ĵ��������ط���ļĴ�����ţ�
int Generate_asm::get_reg(string var, int except_index)
{
	//����ļĴ�����ź�����
	int reg_index = -1;

	//<1> ȷ��var���ڴ����з���ռ䣬��var�����ڴ��У�Ϊ֮�����ڴ�ռ�
	//���ȫ�ֺ;ֲ�������û����һ��������var��δ����ÿռ�ı���
	if (local_var_offset_table.find(var) == local_var_offset_table.end() && global_var_addr_table.find(var) == global_var_addr_table.end()) {
		//�ж��Ǿֲ���������ȫ�ֱ���
		//ȫ�ֱ���������ȫ�ֱ�����ַ���з����ַ
		if (procedureName == "") {
			global_var_addr_table.insert({ var, globalvar_addr });
			globalvar_addr += 4;//��������
		}
		//�ֲ��������ھֲ�����ƫ�Ʊ��з���
		else {
			local_var_offset_table.insert(pair<string, int>(var, localvar_offset));
			localvar_offset -= 4;//��������
			asm_ostream << "subi $sp,$sp,4" << endl;
		}
		//��AVALUE�в���
		AVALUE.insert({ var, {} });
	}

	//<2> ���мĴ����ķ���
	//������ڼĴ����У������Ĵ���
	if (AVALUE.at(var).empty()) {
		//�Ĵ����ķ����㷨���Ĵ���ֻ��ʹ��$txx��
		//�������ȣ�û�п������LRU
		//�������޿��е�reg
		for (int i = VAR_REG_START; i <= VAR_REG_END; i++) {
			if (RVALUE[i] == "null") {
				reg_index = i;//���еļĴ�����ֵ
				//�ԸüĴ���ռ�ã��޸�RVAULE��AVALUE
				AVALUE.at(var).insert(registers[reg_index]);
				RVALUE[reg_index] = var;
				break;
			}

		}
		//û���ҵ����е�reg��׼����ռһ��reg
		if (reg_index == -1) {
			//���Ҫ��ռ�ļĴ������
			int to_seize_reg_index = get_lru_reg_index(except_index);
			//��ñ���ռ�ı�������
			string to_seize_var_name = RVALUE[to_seize_reg_index];
			//�������ҪΪ�Ĵ����еı���V���ɴ���ָ��
			if (AVALUE.at(to_seize_var_name).size() >= 2) {
				//����Ҫ���ɴ���ָ�������������һ�����ɣ���
				//1.AVALUE�п���V�������ڱ�ļĴ�����
				//2.���V��A���Ҳ���B��C����δʵ�֣�
				//3.���V������֮��ʹ�ã���δʵ�֣�
			}
			//��Ҫ���ɴ���ָ��
			else {
				//�ж�V�Ǿֲ���������ȫ�ֱ������ò�ͬ��д��ʽ
				//�Ǿֲ�����
				if (local_var_offset_table.find(to_seize_var_name) != local_var_offset_table.end()) {
					int offset = local_var_offset_table.at(to_seize_var_name);
					asm_ostream << "sw " << registers[to_seize_reg_index] << "," << to_string(offset) << "($fp)" << endl;
				}
				//��ȫ�ֱ���
				else if (global_var_addr_table.find(to_seize_var_name) != global_var_addr_table.end()) {
					int addr = global_var_addr_table.at(to_seize_var_name);
					asm_ostream << "sw " << registers[to_seize_reg_index] << "," << to_string(addr) << "($zero)" << endl;
				}
			}
			//����AVALUE��RVALUE
			AVALUE.at(to_seize_var_name).erase(registers[to_seize_reg_index]);
			reg_index = to_seize_reg_index;
			RVALUE[reg_index] = var;
			AVALUE.at(var).insert(registers[to_seize_reg_index]);
		}
		//�Ǿֲ�����
		if (local_var_offset_table.find(var) != local_var_offset_table.end()) {
			int offset = local_var_offset_table.at(var);
			asm_ostream << "lw " << registers[reg_index] << "," << to_string(offset) << "($fp)" << endl;
		}
		//��ȫ�ֱ���
		else if (global_var_addr_table.find(var) != global_var_addr_table.end()) {
			int addr = global_var_addr_table.at(var);
			asm_ostream << "lw " << registers[reg_index] << "," << to_string(addr) << "($zero)" << endl;
		}
	}
	//���Ѿ��ڼĴ����У����ȡ����Ĵ����ı��
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

//����LRU�Ĵ���
int Generate_asm::get_lru_reg_index(int except_index) {
	int max_index = 0;
	int max_unuse = 0;
	for (int i = 0; i < regs_info.size(); i++) {
		//Ҫע����ռ��ֻ���Ƿ�Χ�ڼĴ�������������except_index
		if (regs_info[i].unuse_time > max_unuse && regs_info[i].reg_index >= VAR_REG_START && regs_info[i].reg_index <= VAR_REG_END && (except_index == -1 || except_index != regs_info[i].reg_index)) {
			max_index = i;
			max_unuse = regs_info[i].unuse_time;
		}
	}
	return regs_info[max_index].reg_index;
}
//��regs_info�н��и���
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