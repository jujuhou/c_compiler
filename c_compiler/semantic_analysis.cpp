#include "semantic_analysis.h"

//************************ 符号表 ****************************
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
	//已存在
	return -1;
}
//*********************** 语义分析 ***************************
//构造函数
Semantic_analysis::Semantic_analysis()
{
	//创建全局的符号表
	tables.push_back(Semantic_symbol_table(Semantic_symbol_table::GlobalTable, "global table"));
	//当前作用域为全局作用域
	current_table_stack.push_back(0);
	//创建临时变量表
	tables.push_back(Semantic_symbol_table(Semantic_symbol_table::TempTable, "temp variable table"));

	//从标号1开始生成四元式标号；0号用于 (j, -, -, main_address)
	next_quaternary_index = 1;
	//main函数的标号先置为-1
	main_index = -1;
	//初始回填层次为0，表示不需要回填
	backpatching_level = 0;
	//临时变量计数器归零
	tmp_var_count = 0;
}
//将所有的符号信息放入symbol_list
void Semantic_analysis::Add_symbol_to_list(const Semantic_symbol symbol)
{
	symbol_list.push_back(symbol);
}
//分析过程
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
//打印四元式表
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
	//如果没有定义main函数，则报错
	if (main_index == -1) {
		cout << "语义分析中发生错误：未定义main函数" << endl;
		throw(SEMANTIC_ERROR_NO_MAIN);
	}
	int count = production_right.size();
	while (count--) {
		symbol_list.pop_back();
	}
	//在最前面加入四元式
	quaternary.insert(quaternary.begin(), { 0, "j","-" , "-", to_string(main_index) });

	symbol_list.push_back({ production_left, "", -1, -1, -1,-1 });
}
//ExtDef ::= VarSpecifier <ID> ; | FunSpecifier FunDec Block
void Semantic_analysis::Analysis_production_ExtDef(const string production_left, const vector<string> production_right)
{
	//如果是定义变量
	if (production_right.size() == 3) {
		//目前symbol_list的末尾是Specifier <ID> ;，由此找到Specifier和<ID>
		Semantic_symbol specifier = symbol_list[symbol_list.size() - 3];//int
		Semantic_symbol identifier = symbol_list[symbol_list.size() - 2];//变量名

		//用于判断该变量是否在当前层已经定义
		bool existed = false;
		Semantic_symbol_table current_table = tables[current_table_stack.back()];
		if (current_table.Find_symbol(identifier.value) != -1) {
			cout << "语义分析中发生错误：（" << identifier.row << "行，" << identifier.col << "列）变量" << identifier.value << "重定义" << endl;
			throw(SEMANTIC_ERROR_REDEFINED);
		}

		//将这一变量加入table
		IdentifierInfo variable;
		variable.identifier_name = identifier.value;//变量名
		variable.identifier_type = IdentifierInfo::Variable;
		variable.specifier_type = specifier.value;//int

		//加入table
		tables[current_table_stack.back()].Add_symbol(variable);

		//symbol_list更新
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,current_table_stack.back(), int(tables[current_table_stack.back()].table.size() - 1) });
	}
	//如果是定义函数
	else {
		Semantic_symbol identifier = symbol_list[symbol_list.size() - 2];

		//需要退出作用域
		current_table_stack.pop_back();
		//更新symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,identifier.table_index,identifier.index });
	}
}
//VarSpecifier ::= int
void Semantic_analysis::Analysis_production_VarSpecifier(const string production_left, const vector<string> production_right)
{
	//symbol_list的最后一个是int
	Semantic_symbol specifier = symbol_list.back();
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,specifier.value,specifier.row,specifier.col,-1,-1 });
}
//FunSpecifier ::= void | int 
void Semantic_analysis::Analysis_production_FunSpecifier(const string production_left, const vector<string> production_right) 
{
	//symbol_list的最后一个是int或void
	Semantic_symbol specifier = symbol_list.back();
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,specifier.value,specifier.row,specifier.col,-1,-1 });
}
//FunDec ::= <ID> CreateFunTable_m ( VarList )
void Semantic_analysis::Analysis_production_FunDec(const string production_left, const vector<string> production_right)
{
	//symbol_list的CreateFunTable_m记录了table信息
	Semantic_symbol specifier = symbol_list[symbol_list.size()-4];
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,specifier.value,specifier.row,specifier.col,specifier.table_index,specifier.index });
}
//CreateFunTable_m ::= @
void Semantic_analysis::Analysis_production_CreateFunTable_m(const string production_left, const vector<string> production_right)
{
	//创建函数表
	//此时symbol_list最后一个符号为函数名，倒数第二个为函数返回值
	Semantic_symbol identifier = symbol_list.back();
	Semantic_symbol specifier = symbol_list[symbol_list.size() - 2];

	//首先在全局的table判断函数名是否重定义
	if (tables[0].Find_symbol(identifier.value) != -1) {
		cout << "语义分析中发生错误：（" << identifier.row << "行，" << identifier.col << "列）函数" << identifier.value << "重定义" << endl;
		throw(SEMANTIC_ERROR_REDEFINED);
	}

	//创建函数表
	tables.push_back(Semantic_symbol_table(Semantic_symbol_table::FunctionTable, identifier.value));
	//在全局符号表创建当前函数的符号项（这里参数个数和入口地址会进行回填）
	tables[0].Add_symbol({ IdentifierInfo::Function,specifier.value,identifier.value,0,0,int(tables.size() - 1) });

	//函数表压入栈
	current_table_stack.push_back(tables.size() - 1);
	//返回值
	IdentifierInfo return_value;
	return_value.identifier_type = IdentifierInfo::ReturnVar;
	return_value.identifier_name = tables.back().table_name + "_return_value";
	return_value.specifier_type = specifier.value;
	//如果为main函数，则进行记录
	if (identifier.value == "main")
		main_index = next_quaternary_index;
	//加入四元式
	quaternary.push_back({ next_quaternary_index++ , identifier.value,"-","-" ,"-" });
	//向函数表中加入返回变量
	tables[current_table_stack.back()].Add_symbol(return_value);
	//空串不需要pop
	//进行pushback
	symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,0,int(tables[0].table.size()-1) });
}
//ParamDec ::= VarSpecifier <ID>
void Semantic_analysis::Analysis_production_ParamDec(const string production_left, const vector<string> production_right)
{
	//symbol_list最后一个为变量名，倒数第二个为类型
	Semantic_symbol identifier = symbol_list.back();
	Semantic_symbol specifier = symbol_list[symbol_list.size() - 2];
	//当前函数表
	Semantic_symbol_table& function_table = tables[current_table_stack.back()];


	//如果已经进行过定义
	if (function_table.Find_symbol(identifier.value) != -1) {
		cout << "语义分析中发生错误：（" << identifier.row << "行，" << identifier.col << "列）函数参数" << identifier.value << "重定义" << endl;
		throw(SEMANTIC_ERROR_REDEFINED);
	}
	//函数表加入形参变量
	int new_position = function_table.Add_symbol({ IdentifierInfo::Variable,specifier.value,identifier.value,-1,-1,-1 });
	//当前函数在全局符号中的索引
	int table_position = tables[0].Find_symbol(function_table.table_name);
	//形参个数++
	tables[0].table[table_position].function_parameter_num++;

	//加入四元式
	quaternary.push_back({ next_quaternary_index++, "defpar","-" , "-", identifier.value });

	//symbol_list更新
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,identifier.value,identifier.row,identifier.col,current_table_stack.back(),new_position });
}
//Block ::= { DefList StmtList }
void Semantic_analysis::Analysis_production_Block(const string production_left, const vector<string> production_right)
{
	//更新symbol_list
	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
//Def ::= VarSpecifier <ID> ;
void Semantic_analysis::Analysis_production_Def(const string production_left, const vector<string> production_right)
{
	//symbol_list的倒数第二个、倒数第三个是变量名和类型
	Semantic_symbol identifier = symbol_list[symbol_list.size() - 2];
	Semantic_symbol specifier = symbol_list[symbol_list.size() - 3];
	Semantic_symbol_table& current_table = tables[current_table_stack.back()];

	//重定义则报错
	if (current_table.Find_symbol(identifier.value) != -1)
	{
		cout << "语义分析中发生错误：（" << identifier.row << "行，" << identifier.col << "列）变量" << identifier.value << "重定义" << endl;
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
	//symbol_list的倒数第一个、倒数第三个是Exp和变量名
	Semantic_symbol identifier = symbol_list[symbol_list.size() - 3];
	Semantic_symbol exp = symbol_list.back();

	//检查id是否存在，不存在则报错
	bool existed = false;
	int table_index = -1, index = -1;
	//从当前层开始向上遍历
	for (int scope_layer = current_table_stack.size() - 1; scope_layer >= 0; scope_layer--) {
		auto current_table = tables[current_table_stack[scope_layer]];
		if ((index=current_table.Find_symbol(identifier.value)) != -1) {
			existed = true;
			table_index = current_table_stack[scope_layer];
			break;
		}
	}
	if (existed == false) {
		cout << "语义分析中发生错误：（" << identifier.row << "行，" << identifier.col << "列）变量" << identifier.value << "未定义" << endl;
		throw(SEMANTIC_ERROR_UNDEFINED);
	}

	quaternary.push_back({ next_quaternary_index++, "=", exp.value, "-", identifier.value });

	//更新symbol_list
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
		//更新symbol_list
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
		//更新symbol_list
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
		//更新symbol_list
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
		//如果是ID检查其是否进行过定义
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
				cout << "语义分析中发生错误：（" << exp.row << "行，" << exp.col << "列）变量" << exp.value << "未定义" << endl;
				throw(SEMANTIC_ERROR_UNDEFINED);
			}
		}

		//更新symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, exp.value ,exp.row,exp.col,exp.table_index,exp.index });
	}
	else {
		Semantic_symbol exp = symbol_list[symbol_list.size()-2];

		//更新symbol_list
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

	//检查函数是否定义（在CallFunCheck时已经检查）

	//检查参数个数
	int para_num = tables[check.table_index].table[check.index].function_parameter_num;
	if (para_num > stoi(args.value)) {
		cout << "语义分析中发生错误：（" << identifier.row << "行，" << identifier.col << "列）函数" << identifier.value << "调用时参数过少" << endl;
		throw(SEMANTIC_ERROR_PARAMETER_NUM);
	}
	else if (para_num < stoi(args.value)) {
		cout << "语义分析中发生错误：（" << identifier.row << "行，" << identifier.col << "列）函数" << identifier.value << "调用时参数过多" << endl;
		throw(SEMANTIC_ERROR_PARAMETER_NUM);
	}

	// 生成函数调用四元式 
	string new_tmp_var = "T" + to_string(tmp_var_count++);
	quaternary.push_back({ next_quaternary_index++, "call", identifier.value, "-", new_tmp_var });

	int count = production_right.size();
	while (count--)
		symbol_list.pop_back();
	// 新的exp的value为临时变量名
	symbol_list.push_back({ production_left, new_tmp_var ,-1,-1,-1,-1 });
}
//CallFunCheck ::= @
void Semantic_analysis::Analysis_production_CallFunCheck(const string production_left, const vector<string> production_right)
{
	Semantic_symbol fun_id = symbol_list[symbol_list.size() - 2];

	int fun_id_pos = tables[0].Find_symbol(fun_id.value);

	if (-1 == fun_id_pos) {
		cout << "语义分析中发生错误：（" << fun_id.row << "行，" << fun_id.col << "列）函数" << fun_id.value << "调用未定义" << endl;
		throw(SEMANTIC_ERROR_UNDEFINED);
	}
	if (tables[0].table[fun_id_pos].identifier_type != IdentifierInfo::Function) {
		cout << "语义分析中发生错误：（" << fun_id.row << "行，" << fun_id.col << "列）函数" << fun_id.value << "调用未定义" << endl;
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
		//返回值
		Semantic_symbol return_exp = symbol_list.back();
		//函数表
		Semantic_symbol_table function_table = tables[current_table_stack.back()];

		//添加四元式
		quaternary.push_back({ next_quaternary_index++,"=",return_exp.value,"-",function_table.table[0].identifier_name });

		//添加四元式
		quaternary.push_back({ next_quaternary_index++ ,"return",function_table.table[0].identifier_name,"-",function_table.table_name });

		//更新symbol_list
		int count = production_right.size();
		while (count--)
			symbol_list.pop_back();
		symbol_list.push_back({ production_left, return_exp.value,-1,-1,-1,-1 });
	}
	else {
		//函数表
		Semantic_symbol_table function_table = tables[current_table_stack.back()];

		//检查函数的返回值是否为void
		if (tables[0].table[tables[0].Find_symbol(function_table.table_name)].specifier_type != "void") {
			cout << "语义分析中发生错误：（" << symbol_list.back().row << "行，" << symbol_list.back().col+sizeof("return") << "列）函数"<< function_table.table_name <<"必须有返回值" << endl;
			throw(SEMANTIC_ERROR_NO_RETURN);
		}

		//添加四元式
		quaternary.push_back({ next_quaternary_index++ ,"return","-","-",function_table.table_name });

		//更新symbol_list
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
		//只有if没有else
		//真出口
		quaternary[backpatching_list.back()].result = ifstmt_m2.value;
		backpatching_list.pop_back();

		//假出口
		quaternary[backpatching_list.back()].result = to_string(next_quaternary_index);
		backpatching_list.pop_back();
	}
	else {
		//if块出口
		quaternary[backpatching_list.back()].result = to_string(next_quaternary_index);
		backpatching_list.pop_back();
		//if真出口
		quaternary[backpatching_list.back()].result = ifstmt_m2.value;
		backpatching_list.pop_back();
		//if假出口
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

	//待回填四元式 : 假出口
	quaternary.push_back({ next_quaternary_index++,"j=",if_exp.value,"0","" });
	backpatching_list.push_back(quaternary.size() - 1);

	//待回填四元式 : 真出口
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
	//if 的跳出语句(else 之前)(待回填)
	quaternary.push_back({ next_quaternary_index++,"j","-","-","" });
	backpatching_list.push_back(quaternary.size() - 1);
	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
//WhileStmt ::= while WhileStmt_m1 ( Exp ) WhileStmt_m2 Block
void Semantic_analysis::Analysis_production_WhileStmt(const string production_left, const vector<string> production_right)
{
	Semantic_symbol whilestmt_m1 = symbol_list[symbol_list.size() - 6];
	Semantic_symbol whilestmt_m2 = symbol_list[symbol_list.size() - 2];

	// 无条件跳转到 while 的条件判断语句处
	quaternary.push_back({ next_quaternary_index++,"j","-","-" ,whilestmt_m1.value });

	//回填真出口
	quaternary[backpatching_list.back()].result = whilestmt_m2.value;
	backpatching_list.pop_back();

	//回填假出口
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

	//假出口
	quaternary.push_back({ next_quaternary_index++,"j=",while_exp.value,"0","" });
	backpatching_list.push_back(quaternary.size() - 1);
	//真出口
	quaternary.push_back({ next_quaternary_index++ ,"j","-","-" ,"" });
	backpatching_list.push_back(quaternary.size() - 1);

	symbol_list.push_back({ production_left,to_string(next_quaternary_index),-1,-1,-1,-1 });
}
