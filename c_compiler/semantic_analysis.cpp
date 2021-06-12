#include "semantic_analysis.h"

//************************ ���ű� ****************************
Semantic_symbol_table::Semantic_symbol_table(const TableType table_type_, const string table_name_)
{
	this->table_type = table_type_;
	this->table_name = table_name_;
}

int Semantic_symbol_table::Find_symbol(const string id_name)
{
	for (int i = 0; i < table.size(); i++) {
		if (table[i].identifier_name == id_name)
			return i;
	}
	return -1;
}

int Semantic_symbol_table::Add_symbol(const IdentifierInfo id)
{
	if (Find_symbol(id.identifier_name) == -1) {
		table.push_back(id);
		return table.size() - 1;
	}
	//�Ѵ���
	return -1;
}
//*********************** ������� ***************************
//���캯��
Semantic_analysis::Semantic_analysis()
{
	//����ȫ�ֵķ��ű�
	tables.push_back(Semantic_symbol_table(Semantic_symbol_table::GlobalTable, "global table"));
	//��ǰ������Ϊȫ��������
	current_table_stack.push_back(0);
	//������ʱ������
	tables.push_back(Semantic_symbol_table(Semantic_symbol_table::TempTable, "temp variable table"));

	//�ӱ��1��ʼ������Ԫʽ��ţ�0������ (j, -, -, main_address)
	next_quaternary_index = 1;
	//main�����ı������Ϊ-1
	main_index = -1;
	//��ʼ������Ϊ0����ʾ����Ҫ����
	backpatching_level = 0;
	//��ʱ��������������
	tmp_var_count = 0;
}
//�����еķ�����Ϣ����symbol_list
void Semantic_analysis::Add_symbol_to_list(const Semantic_symbol symbol)
{
	symbol_list.push_back(symbol);
}
//��������
void Semantic_analysis::Analysis(const string production_left, const vector<string> production_right)
{
	//Program ::= ExtDefList 
	if (production_left == "Program")
		Analysis_production_Program(production_left, production_right);
	//ExtDef ::= VarSpecifier <ID> ; | FunSpecifier FunDec Block ExitFunTable_m
	else if (production_left == "ExtDef")
		Analysis_production_ExtDef(production_left, production_right);
	//VarSpecifier ::= int
	else if (production_left == "VarSpecifier")
		Analysis_production_VarSpecifier(production_left, production_right);
	else if (production_left == "FunSpecifier")
		Analysis_production_FunSpecifier(production_left, production_right);
	else if (production_left == "FunDec")
		Analysis_production_FunDec(production_left, production_right);
	else if (production_left == "CreateFunTable_m")
		Analysis_production_CreateFunTable_m(production_left, production_right);
	else if (production_left == "ParamDec")
		Analysis_production_ParamDec(production_left, production_right);
	else if (production_left == "Block")
		Analysis_production_Block(production_left, production_right);
	else if (production_left == "Def")
		Analysis_production_Def(production_left, production_right);
	else if (production_left == "AssignStmt")
		Analysis_production_AssignStmt(production_left, production_right);
	else if (production_left == "Exp")
		Analysis_production_Exp(production_left, production_right);
	else if (production_left == "AddSubExp")
		Analysis_production_AddSubExp(production_left, production_right);
	else if (production_left == "Item")
		Analysis_production_Item(production_left, production_right);
	else if (production_left == "Factor")
		Analysis_production_Factor(production_left, production_right);
	else if (production_left == "CallStmt")
		Analysis_production_CallStmt(production_left, production_right);
	else if (production_left == "CallFunCheck")
		Analysis_production_CallFunCheck(production_left, production_right);
	else if (production_left == "Args")
		Analysis_production_Args(production_left, production_right);
	else if (production_left == "ReturnStmt")
		Analysis_production_ReturnStmt(production_left, production_right);
	else if (production_left == "Relop")
		Analysis_production_Relop(production_left, production_right);
	else if (production_left == "IfStmt")
		Analysis_production_IfStmt(production_left, production_right);
	else if (production_left == "IfStmt_m1")
		Analysis_production_IfStmt_m1(production_left, production_right);
	else if (production_left == "IfStmt_m2")
		Analysis_production_IfStmt_m2(production_left, production_right);
	else if (production_left == "IfNext")
		Analysis_production_IfNext(production_left, production_right);
	else if (production_left == "IfStmt_next")
		Analysis_production_IfStmt_next(production_left, production_right);
	else if (production_left == "WhileStmt")
		Analysis_production_WhileStmt(production_left, production_right);
	else if (production_left == "WhileStmt_m1")
		Analysis_production_WhileStmt_m1(production_left, production_right);
	else if (production_left == "WhileStmt_m2")
		Analysis_production_WhileStmt_m2(production_left, production_right);
	else {
		if (production_right[0] != "@") {
			int count = production_right.size();
			while (count--)
				symbol_list.pop_back();
		}
		symbol_list.push_back({ production_left,"",-1,-1,-1,-1 });
	}

}
//��ӡ��Ԫʽ��
void Semantic_analysis::Print_quaternary(const string file_path) {
	ofstream file_out;
	file_out.open(file_path, ios::out);

	for (auto& q : quaternary) {
		file_out << q.index << "(" << q.operator_type << "," << q.arg1 << "," << q.arg2 << "," << q.result <<")"<< endl;
	}
}
//Program ::= ExtDefList 
void Semantic_analysis::Analysis_production_Program(const string production_left, const vector<string> production_right)
{
	//���û�ж���main�������򱨴�
	if (main_index == -1) {
		cout << "��������з�������δ����main����" << endl;
		throw(SEMANTIC_ERROR_NO_MAIN);
	}
	int count = production_right.size();
	while (count--) {
		symbol_list.pop_back();
	}
	//����ǰ�������Ԫʽ
	quaternary.insert(quaternary.begin(), { 0, "j","-" , "-", to_string(main_index) });

	symbol_list.push_back({ production_left, "", -1, -1, -1,-1 });
}
//ExtDef ::= VarSpecifier <ID> ; | FunSpecifier FunDec Block
void Semantic_analysis::Analysis_production_ExtDef(const string production_left, const vector<string> production_right)
{
	//����Ƕ������
	if (production_right.size() == 3) {
		//Ŀǰsymbol_list��ĩβ��Specifier <ID> ;���ɴ��ҵ�Specifier��<ID>
		Semantic_symbol specifier = symbol_list[symbol_list.size() - 3];//int
		Semantic_symbol identifier = symbol_list[symbol_list.size() - 2];//������

		//�����жϸñ����Ƿ��ڵ�ǰ���Ѿ�����
		bool existed = false;
		Semantic_symbol_table current_table = tables[current_table_stack.back()];
		if (current_table.Find_symbol(identifier.value) != -1) {
			cout << "��������з������󣺣�" << identifier.row << "�У�" << identifier.col << "�У�����" << identifier.value << "�ض���" << endl;
			throw(SEMANTIC_ERROR_REDEFINED);
		}

		//����һ��������table
		IdentifierInfo variable;
		variable.identifier_name = identifier.value;//������
		variable.identifier_type = IdentifierInfo::Variable;
		variable.specifier_type = specifier.value;//int

		//����table
		tables[current_table_stack.back()].Add_symbol(variable);

		//symbol_list����
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,current_table_stack.back(), int(tables[current_table_stack.back()].table.size() - 1) });
	}
	//����Ƕ��庯��
	else {
		Semantic_symbol identifier = symbol_list[symbol_list.size() - 2];

		//��Ҫ�˳�������
		current_table_stack.pop_back();
		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,identifier.table_index,identifier.index });
	}
}
//VarSpecifier ::= int
void Semantic_analysis::Analysis_production_VarSpecifier(const string production_left, const vector<string> production_right)
{
	//symbol_list�����һ����int
	Semantic_symbol specifier = symbol_list.back();
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,specifier.value,specifier.row,specifier.col,-1,-1 });
}
//FunSpecifier ::= void | int 
void Semantic_analysis::Analysis_production_FunSpecifier(const string production_left, const vector<string> production_right) 
{
	//symbol_list�����һ����int��void
	Semantic_symbol specifier = symbol_list.back();
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,specifier.value,specifier.row,specifier.col,-1,-1 });
}
//FunDec ::= <ID> CreateFunTable_m ( VarList )
void Semantic_analysis::Analysis_production_FunDec(const string production_left, const vector<string> production_right)
{
	//symbol_list��CreateFunTable_m��¼��table��Ϣ
	Semantic_symbol specifier = symbol_list[symbol_list.size()-4];
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,specifier.value,specifier.row,specifier.col,specifier.table_index,specifier.index });
}
//CreateFunTable_m ::= @
void Semantic_analysis::Analysis_production_CreateFunTable_m(const string production_left, const vector<string> production_right)
{
	//����������
	//��ʱsymbol_list���һ������Ϊ�������������ڶ���Ϊ��������ֵ
	Semantic_symbol identifier = symbol_list.back();
	Semantic_symbol specifier = symbol_list[symbol_list.size() - 2];

	//������ȫ�ֵ�table�жϺ������Ƿ��ض���
	if (tables[0].Find_symbol(identifier.value) != -1) {
		cout << "��������з������󣺣�" << identifier.row << "�У�" << identifier.col << "�У�����" << identifier.value << "�ض���" << endl;
		throw(SEMANTIC_ERROR_REDEFINED);
	}

	//����������
	tables.push_back(Semantic_symbol_table(Semantic_symbol_table::FunctionTable, identifier.value));
	//��ȫ�ַ��ű�����ǰ�����ķ�������������������ڵ�ַ����л��
	tables[0].Add_symbol({ IdentifierInfo::Function,specifier.value,identifier.value,0,0,int(tables.size() - 1) });

	//������ѹ��ջ
	current_table_stack.push_back(tables.size() - 1);
	//����ֵ
	IdentifierInfo return_value;
	return_value.identifier_type = IdentifierInfo::ReturnVar;
	return_value.identifier_name = tables.back().table_name + "_return_value";
	return_value.specifier_type = specifier.value;
	//���Ϊmain����������м�¼
	if (identifier.value == "main")
		main_index = next_quaternary_index;
	//������Ԫʽ
	quaternary.push_back({ next_quaternary_index++ , identifier.value,"-","-" ,"-" });
	//�������м��뷵�ر���
	tables[current_table_stack.back()].Add_symbol(return_value);
	//�մ�����Ҫpop
	//����pushback
	symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,0,int(tables[0].table.size()-1) });
}
//ParamDec ::= VarSpecifier <ID>
void Semantic_analysis::Analysis_production_ParamDec(const string production_left, const vector<string> production_right)
{
	//symbol_list���һ��Ϊ�������������ڶ���Ϊ����
	Semantic_symbol identifier = symbol_list.back();
	Semantic_symbol specifier = symbol_list[symbol_list.size() - 2];
	//��ǰ������
	Semantic_symbol_table& function_table = tables[current_table_stack.back()];


	//����Ѿ����й�����
	if (function_table.Find_symbol(identifier.value) != -1) {
		cout << "��������з������󣺣�" << identifier.row << "�У�" << identifier.col << "�У���������" << identifier.value << "�ض���" << endl;
		throw(SEMANTIC_ERROR_REDEFINED);
	}
	//����������βα���
	int new_position = function_table.Add_symbol({ IdentifierInfo::Variable,specifier.value,identifier.value,-1,-1,-1 });
	//��ǰ������ȫ�ַ����е�����
	int table_position = tables[0].Find_symbol(function_table.table_name);
	//�βθ���++
	tables[0].table[table_position].function_parameter_num++;

	//������Ԫʽ
	quaternary.push_back({ next_quaternary_index++, "defpar","-" , "-", identifier.value });

	//symbol_list����
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,current_table_stack.back(),new_position });
}
//Block ::= { DefList StmtList }
void Semantic_analysis::Analysis_production_Block(const string production_left, const vector<string> production_right)
{
	//����symbol_list
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
//Def ::= VarSpecifier <ID> ;
void Semantic_analysis::Analysis_production_Def(const string production_left, const vector<string> production_right)
{
	//symbol_list�ĵ����ڶ����������������Ǳ�����������
	Semantic_symbol identifier = symbol_list[symbol_list.size() - 2];
	Semantic_symbol specifier = symbol_list[symbol_list.size() - 3];
	Semantic_symbol_table& current_table = tables[current_table_stack.back()];

	//�ض����򱨴�
	if (current_table.Find_symbol(identifier.value) != -1)
	{
		cout << "��������з������󣺣�" << identifier.row << "�У�" << identifier.col << "�У�����" << identifier.value << "�ض���" << endl;
		throw(SEMANTIC_ERROR_REDEFINED);
	}

	current_table.Add_symbol({ IdentifierInfo::Variable,specifier.value,identifier.value ,-1,-1,-1 });

	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left, identifier.value ,identifier.row,identifier.col,current_table_stack.back(),int(tables[current_table_stack.back()].table.size()-1) });
}
//AssignStmt ::= <ID> = Exp
void Semantic_analysis::Analysis_production_AssignStmt(const string production_left, const vector<string> production_right)
{
	//symbol_list�ĵ�����һ����������������Exp�ͱ�����
	Semantic_symbol identifier = symbol_list[symbol_list.size() - 3];
	Semantic_symbol exp = symbol_list.back();

	//���id�Ƿ���ڣ��������򱨴�
	bool existed = false;
	int table_index = -1, index = -1;
	//�ӵ�ǰ�㿪ʼ���ϱ���
	for (int scope_layer = current_table_stack.size() - 1; scope_layer >= 0; scope_layer--) {
		auto current_table = tables[current_table_stack[scope_layer]];
		if ((index=current_table.Find_symbol(identifier.value)) != -1) {
			existed = true;
			table_index = current_table_stack[scope_layer];
			break;
		}
	}
	if (existed == false) {
		cout << "��������з������󣺣�" << identifier.row << "�У�" << identifier.col << "�У�����" << identifier.value << "δ����" << endl;
		throw(SEMANTIC_ERROR_UNDEFINED);
	}

	quaternary.push_back({ next_quaternary_index++, "=", exp.value, "-", identifier.value });

	//����symbol_list
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left, identifier.value ,identifier.row,identifier.col,table_index,index });
}
//Exp ::= AddSubExp | Exp Relop AddSubExp
void Semantic_analysis::Analysis_production_Exp(const string production_left, const vector<string> production_right)
{
	if (production_right.size() == 1) {
		Semantic_symbol exp = symbol_list.back();
		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, exp.value ,exp.row,exp.col,exp.table_index,exp.index });
	}
	else {
		Semantic_symbol sub_exp1 = symbol_list[symbol_list.size() - 3];
		Semantic_symbol op = symbol_list[symbol_list.size() - 2];
		Semantic_symbol sub_exp2 = symbol_list[symbol_list.size() - 1];
		int next_label_num = next_quaternary_index++;
		string new_tmp_var = "T" + to_string(tmp_var_count++);
		quaternary.push_back({ next_label_num, "j" + op.value, sub_exp1.value, sub_exp2.value, to_string(next_label_num + 3) });
		quaternary.push_back({ next_quaternary_index++, "=", "0", "-", new_tmp_var });
		quaternary.push_back({ next_quaternary_index++, "j", "-", "-", to_string(next_label_num + 4) });
		quaternary.push_back({ next_quaternary_index++, "=", "1", "-", new_tmp_var });

		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, new_tmp_var ,-1,-1,-1,-1 });
	}
}
//AddSubExp ::= Item | Item + Item | Item - Item
void Semantic_analysis::Analysis_production_AddSubExp(const string production_left, const vector<string> production_right)
{
	if (production_right.size() == 1) {
		Semantic_symbol exp = symbol_list.back();
		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, exp.value ,exp.row,exp.col,exp.table_index,exp.index });
	}
	else {
		Semantic_symbol sub_exp1 = symbol_list[symbol_list.size() - 3];
		Semantic_symbol op = symbol_list[symbol_list.size() - 2];
		Semantic_symbol sub_exp2 = symbol_list[symbol_list.size() - 1];
		string new_tmp_var = "T" + to_string(tmp_var_count++);
		quaternary.push_back({ next_quaternary_index++, op.value, sub_exp1.value, sub_exp2.value, new_tmp_var });

		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, new_tmp_var ,-1,-1,-1,-1 });
	}
}
//Item ::= Factor | Factor * Factor | Factor / Factor
void Semantic_analysis::Analysis_production_Item(const string production_left, const vector<string> production_right)
{
	if (production_right.size() == 1) {
		Semantic_symbol exp = symbol_list.back();
		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, exp.value ,exp.row,exp.col,exp.table_index,exp.index });
	}
	else {
		Semantic_symbol sub_exp1 = symbol_list[symbol_list.size() - 3];
		Semantic_symbol op = symbol_list[symbol_list.size() - 2];
		Semantic_symbol sub_exp2 = symbol_list[symbol_list.size() - 1];
		std::string new_tmp_var = "T" + to_string(tmp_var_count++);
		quaternary.push_back({ next_quaternary_index++, op.value, sub_exp1.value, sub_exp2.value, new_tmp_var });

		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, new_tmp_var ,-1,-1,-1,-1 });
	}
}
//Factor ::= <INT> | ( Exp ) | <ID> | CallStmt
void Semantic_analysis::Analysis_production_Factor(const string production_left, const vector<string> production_right)
{
	if (production_right.size() == 1) {
		Semantic_symbol exp = symbol_list.back();
		//�����ID������Ƿ���й�����
		if (production_right[0] == "<ID>") {
			bool existed = false;
			for (int scope_layer = current_table_stack.size() - 1; scope_layer >= 0; scope_layer--) {
				auto current_table = tables[current_table_stack[scope_layer]];
				if (current_table.Find_symbol(exp.value) != -1) {
					existed = true;
					break;
				}
			}
			if (existed == false) {
				cout << "��������з������󣺣�" << exp.row << "�У�" << exp.col << "�У�����" << exp.value << "δ����" << endl;
				throw(SEMANTIC_ERROR_UNDEFINED);
			}
		}

		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, exp.value ,exp.row,exp.col,exp.table_index,exp.index });
	}
	else {
		Semantic_symbol exp = symbol_list[symbol_list.size()-2];

		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, exp.value ,exp.row,exp.col,exp.table_index,exp.index });
	}
}
//CallStmt ::= <ID> ( CallFunCheck Args )
void Semantic_analysis::Analysis_production_CallStmt(const string production_left, const vector<string> production_right)
{
	Semantic_symbol identifier = symbol_list[symbol_list.size() - 5];
	Semantic_symbol check = symbol_list[symbol_list.size() - 3];
	Semantic_symbol args = symbol_list[symbol_list.size() - 2];

	//��麯���Ƿ��壨��CallFunCheckʱ�Ѿ���飩

	//����������
	int para_num = tables[check.table_index].table[check.index].function_parameter_num;
	if (para_num > stoi(args.value)) {
		cout << "��������з������󣺣�" << identifier.row << "�У�" << identifier.col << "�У�����" << identifier.value << "����ʱ��������" << endl;
		throw(SEMANTIC_ERROR_PARAMETER_NUM);
	}
	else if (para_num < stoi(args.value)) {
		cout << "��������з������󣺣�" << identifier.row << "�У�" << identifier.col << "�У�����" << identifier.value << "����ʱ��������" << endl;
		throw(SEMANTIC_ERROR_PARAMETER_NUM);
	}

	// ���ɺ���������Ԫʽ 
	string new_tmp_var = "T" + to_string(tmp_var_count++);
	quaternary.push_back({ next_quaternary_index++, "call", identifier.value, "-", new_tmp_var });

	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	// �µ�exp��valueΪ��ʱ������
	symbol_list.push_back({ production_left, new_tmp_var ,-1,-1,-1,-1 });
}
//CallFunCheck ::= @
void Semantic_analysis::Analysis_production_CallFunCheck(const string production_left, const vector<string> production_right)
{
	Semantic_symbol fun_id = symbol_list[symbol_list.size() - 2];

	int fun_id_pos = tables[0].Find_symbol(fun_id.value);

	if (-1 == fun_id_pos) {
		cout << "��������з������󣺣�" << fun_id.row << "�У�" << fun_id.col << "�У�����" << fun_id.value << "����δ����" << endl;
		throw(SEMANTIC_ERROR_UNDEFINED);
	}
	if (tables[0].table[fun_id_pos].identifier_type != IdentifierInfo::Function) {
		cout << "��������з������󣺣�" << fun_id.row << "�У�" << fun_id.col << "�У�����" << fun_id.value << "����δ����" << endl;
		throw(SEMANTIC_ERROR_UNDEFINED);
	}
	symbol_list.push_back({ production_left, fun_id.value,fun_id.row,fun_id.col, 0, fun_id_pos });
}
//Args ::= Exp , Args | Exp | @
void Semantic_analysis::Analysis_production_Args(const string production_left, const vector<string> production_right)
{
	if (production_right.size() == 3) {
		Semantic_symbol exp = symbol_list[symbol_list.size() - 3];
		quaternary.push_back({ next_quaternary_index++, "param", exp.value, "-", "-" });
		int aru_num = stoi(symbol_list.back().value) + 1;
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, to_string(aru_num),-1,-1,-1,-1 });
	}
	else if(production_right[0]=="Exp"){
		Semantic_symbol exp = symbol_list.back();
		quaternary.push_back({ next_quaternary_index++, "param", exp.value, "-", "-" });
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, "1",-1,-1,-1,-1 });
	}
	else if (production_right[0] == "@") {
		symbol_list.push_back({ production_left, "0",-1,-1,-1,-1 });
	}
}
//ReturnStmt ::= return Exp | return
void Semantic_analysis::Analysis_production_ReturnStmt(const string production_left, const vector<string> production_right)
{
	if (production_right.size() == 2){
		//����ֵ
		Semantic_symbol return_exp = symbol_list.back();
		//������
		Semantic_symbol_table function_table = tables[current_table_stack.back()];

		//�����Ԫʽ
		quaternary.push_back({ next_quaternary_index++,"=",return_exp.value,"-",function_table.table[0].identifier_name });

		//�����Ԫʽ
		quaternary.push_back({ next_quaternary_index++ ,"return",function_table.table[0].identifier_name,"-",function_table.table_name });

		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, return_exp.value,-1,-1,-1,-1 });
	}
	else {
		//������
		Semantic_symbol_table function_table = tables[current_table_stack.back()];

		//��麯���ķ���ֵ�Ƿ�Ϊvoid
		if (tables[0].table[tables[0].Find_symbol(function_table.table_name)].specifier_type != "void") {
			cout << "��������з������󣺣�" << symbol_list.back().row << "�У�" << symbol_list.back().col+sizeof("return") << "�У�����"<< function_table.table_name <<"�����з���ֵ" << endl;
			throw(SEMANTIC_ERROR_NO_RETURN);
		}

		//�����Ԫʽ
		quaternary.push_back({ next_quaternary_index++ ,"return","-","-",function_table.table_name });

		//����symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, "",-1,-1,-1,-1 });
	}
}
//Relop ::= > | < | >= | <= | == | !=
void Semantic_analysis::Analysis_production_Relop(const string production_left, const vector<string> production_right)
{
	Semantic_symbol op = symbol_list.back();

	int count = production_right.size();
	while (count--) {
		symbol_list.pop_back();
	}
	symbol_list.push_back({ production_left, op.value ,-1,-1,-1,-1 });
}
//IfStmt ::= if IfStmt_m1 ( Exp ) IfStmt_m2 Block IfNext
void Semantic_analysis::Analysis_production_IfStmt(const string production_left, const vector<string> production_right)
{
	Semantic_symbol ifstmt_m2 = symbol_list[symbol_list.size() - 3];
	Semantic_symbol ifnext = symbol_list[symbol_list.size() - 1];

	if (ifnext.value.empty()) {
		//ֻ��ifû��else
		//�����
		quaternary[backpatching_list.back()].result = ifstmt_m2.value;
		backpatching_list.pop_back();

		//�ٳ���
		quaternary[backpatching_list.back()].result = to_string(next_quaternary_index);
		backpatching_list.pop_back();
	}
	else {
		//if�����
		quaternary[backpatching_list.back()].result = to_string(next_quaternary_index);
		backpatching_list.pop_back();
		//if�����
		quaternary[backpatching_list.back()].result = ifstmt_m2.value;
		backpatching_list.pop_back();
		//if�ٳ���
		quaternary[backpatching_list.back()].result = ifnext.value;
		backpatching_list.pop_back();
	}
	backpatching_level--;

	//popback
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,"",-1,-1,-1,-1 });
}
//IfStmt_m1 ::= @
void Semantic_analysis::Analysis_production_IfStmt_m1(const string production_left, const vector<string> production_right)
{
	backpatching_level++;
	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
//IfStmt_m2 ::= @
void Semantic_analysis::Analysis_production_IfStmt_m2(const string production_left, const vector<string> production_right)
{
	Semantic_symbol if_exp = symbol_list[symbol_list.size() - 2];

	//��������Ԫʽ : �ٳ���
	quaternary.push_back({ next_quaternary_index++,"j=",if_exp.value,"0","" });
	backpatching_list.push_back(quaternary.size() - 1);

	//��������Ԫʽ : �����
	quaternary.push_back({ next_quaternary_index++,"j=","-","-","" });
	backpatching_list.push_back(quaternary.size() - 1);

	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
//IfNext ::= IfStmt_next else Block
void Semantic_analysis::Analysis_production_IfNext(const string production_left, const vector<string> production_right)
{
	Semantic_symbol if_stmt_next = symbol_list[symbol_list.size() - 3];

	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();

	symbol_list.push_back({ production_left,if_stmt_next.value,-1,-1,-1,-1 });
}
//IfStmt_next ::= @
void Semantic_analysis::Analysis_production_IfStmt_next(const string production_left, const vector<string> production_right)
{
	//if ���������(else ֮ǰ)(������)
	quaternary.push_back({ next_quaternary_index++,"j","-","-","" });
	backpatching_list.push_back(quaternary.size() - 1);
	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
//WhileStmt ::= while WhileStmt_m1 ( Exp ) WhileStmt_m2 Block
void Semantic_analysis::Analysis_production_WhileStmt(const string production_left, const vector<string> production_right)
{
	Semantic_symbol whilestmt_m1 = symbol_list[symbol_list.size() - 6];
	Semantic_symbol whilestmt_m2 = symbol_list[symbol_list.size() - 2];

	// ��������ת�� while �������ж���䴦
	quaternary.push_back({ next_quaternary_index++,"j","-","-" ,whilestmt_m1.value });

	//���������
	quaternary[backpatching_list.back()].result = whilestmt_m2.value;
	backpatching_list.pop_back();

	//����ٳ���
	quaternary[backpatching_list.back()].result = to_string(next_quaternary_index);
	backpatching_list.pop_back();

	backpatching_level--;

	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();

	symbol_list.push_back({ production_left,"",-1,-1,-1,-1 });

}
//WhileStmt_m1 ::= @
void Semantic_analysis::Analysis_production_WhileStmt_m1(const string production_left, const vector<string> production_right)
{
	backpatching_level++;
	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
//WhileStmt_m2 ::= @
void Semantic_analysis::Analysis_production_WhileStmt_m2(const string production_left, const vector<string> production_right)
{
	Semantic_symbol while_exp = symbol_list[symbol_list.size() - 2];

	//�ٳ���
	quaternary.push_back({ next_quaternary_index++,"j=",while_exp.value,"0","" });
	backpatching_list.push_back(quaternary.size() - 1);
	//�����
	quaternary.push_back({ next_quaternary_index++ ,"j","-","-" ,"" });
	backpatching_list.push_back(quaternary.size() - 1);

	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
