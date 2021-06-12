#include "lexical_analysis.h"
#include "grammar.h"
#include "grammatical_analysis.h"
#include "Generate_asm.h"

int main() {
	string t;
	cout << "====================================================" << endl;
	cout << endl;
	cout << "                  类C编译器设计与实现               " << endl;
	cout << "                     鞠璇   1851846               " << endl;
	cout << endl;
	cout << "====================================================" << endl;

	cout << "欢迎使用类C编译器" << endl;
	cout << endl;

	cout << "默认使用 ./test_files/grammar.txt 文件中的语法进行语法分析" << endl;
	cout << endl;
	cout << "正在读入 ./test_files/grammar.txt 文件中的语法..." << endl;
	LR1 LR1("./test_files/grammar.txt");
	cout << endl;
	cout << "已完成语法读入和GOTO表及ACTION表生成，正在将GOTO表及ACTION表已输出到文件./test_files/table.txt" << endl;
	LR1.printTable("./test_files/table.txt");
	cout << endl;
	cout << "已完成GOTO表及ACTION表到文件./test_files/table.txt的输出" << endl;
	cout << endl;

	while (1) {
		try {
			cout << "请输入待编译程序所在的文件名（您可以直接输入 ./test_files/test_code.cpp ）" << endl;

			string test_code_file_path;
			cin >> test_code_file_path;
			cout << endl;
			cout << "正在进行词法分析..." << endl;
			lexical_analysis lexicalAnalysis(test_code_file_path);
			lexicalAnalysis.print_token_stream("./test_files/lexical_analysis.txt");
			cout << endl;
			cout << "词法分析已完成....\n\n词法分析结果已输出到./test_files/lexical_analysis.txt中" << endl;
			cout << endl;
			cout << "您是否希望将词法分析结果输出到屏幕？（y/n）" << endl;

			cin >> t;
			if (t == "y") {
				fstream lexical_analysis_result("./test_files/lexical_analysis.txt", ios::in);
				string tmp;
				while (!lexical_analysis_result.eof()) {
					getline(lexical_analysis_result, tmp);
					cout << tmp << endl;;
				}
				lexical_analysis_result.close();
			}

			cout << endl;

			cout << "正在进行语法分析和语义分析..." << endl;
			cout << endl;
			LR1.parser(lexicalAnalysis.get_token_stream(), "./test_files/grammatical_analysis.txt");

			cout << "语法分析和语义分析已完成....\n\n语法分析结果已输出到 ./test_files/grammatical_analysis.txt 中" << endl;
			cout << endl;
			cout << "语义分析结果已输出到 ./test_files/quaternary.txt 中" << endl;
			cout << endl;
			cout << "您是否希望将语法分析结果输出到屏幕？（y/n）" << endl;

			cin >> t;
			if (t == "y") {
				fstream LR1_analysis_result("./test_files/grammatical_analysis.txt", ios::in);
				string tmp;
				while (!LR1_analysis_result.eof()) {
					getline(LR1_analysis_result, tmp);
					cout << tmp << endl;;
				}
				LR1_analysis_result.close();
			}

			cout << endl;
			cout << "您是否希望绘制语法分析树（系统需安装graphviz）（y/n）" << endl;
			cin >> t;
			if (t == "y") {
				LR1.draw_grammatical_analysis_tree(lexicalAnalysis.get_token_stream(), "./test_files/grammatical_analysis_tree.dot");
				system("dot -Tpng ./test_files/grammatical_analysis_tree.dot -o ./test_files/grammatical_analysis_tree.png");
				cout << "语法分析树已生成，请在 ./test_files/grammatical_analysis_tree.png 目录进行查看" << endl;
			}
			LR1.symantic_analysis.Print_quaternary("./test_files/quaternary.txt");
			cout << endl;

			cout << "您是否希望将语义分析结果输出到屏幕？（y/n）" << endl;

			cin >> t;
			if (t == "y") {
				fstream symantic_analysis_result("./test_files/quaternary.txt", ios::in);
				string tmp;
				while (!symantic_analysis_result.eof()) {
					getline(symantic_analysis_result, tmp);
					cout << tmp << endl;;
				}
				symantic_analysis_result.close();
			}

			cout << endl;

			cout << "正在进行目标代码生成..." << endl;
			Generate_asm generate_asm("./test_files/mips.asm", LR1.symantic_analysis.quaternary);
			generate_asm.parse();
			cout << endl;

			cout << "目标代码生成已完成....\n\n目标代码生成结果已输出到 ./test_files/mips.asm 中" << endl;
			cout << endl;
			cout << "您是否希望将目标代码结果输出到屏幕？（y/n）" << endl;

			cin >> t;
			if (t == "y") {
				fstream generate_asm_result("./test_files/mips.asm", ios::in);
				string tmp;
				while (!generate_asm_result.eof()) {
					getline(generate_asm_result, tmp);
					cout << tmp << endl;
				}
				generate_asm_result.close();
			}

			cout << endl;

		}
		catch (int e) {
			cout << "【错误码】" << e << endl;
		}
		cout << "是否还有其他文件需要编译(y/n)" << endl;
		cin >> t;
		if (t != "y") {
			break;

		}
	}
	return 0;
}

