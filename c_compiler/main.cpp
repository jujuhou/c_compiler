#include "lexical_analysis.h"
#include "grammar.h"
#include "grammatical_analysis.h"
#include "Generate_asm.h"

int main() {
	string t;
	cout << "====================================================" << endl;
	cout << endl;
	cout << "                  ��C�����������ʵ��               " << endl;
	cout << "                     ���   1851846               " << endl;
	cout << endl;
	cout << "====================================================" << endl;

	cout << "��ӭʹ����C������" << endl;
	cout << endl;

	cout << "Ĭ��ʹ�� ./test_files/grammar.txt �ļ��е��﷨�����﷨����" << endl;
	cout << endl;
	cout << "���ڶ��� ./test_files/grammar.txt �ļ��е��﷨..." << endl;
	LR1 LR1("./test_files/grammar.txt");
	cout << endl;
	cout << "������﷨�����GOTO��ACTION�����ɣ����ڽ�GOTO��ACTION����������ļ�./test_files/table.txt" << endl;
	LR1.printTable("./test_files/table.txt");
	cout << endl;
	cout << "�����GOTO��ACTION���ļ�./test_files/table.txt�����" << endl;
	cout << endl;

	while (1) {
		try {
			cout << "�����������������ڵ��ļ�����������ֱ������ ./test_files/test_code.cpp ��" << endl;

			string test_code_file_path;
			cin >> test_code_file_path;
			cout << endl;
			cout << "���ڽ��дʷ�����..." << endl;
			lexical_analysis lexicalAnalysis(test_code_file_path);
			lexicalAnalysis.print_token_stream("./test_files/lexical_analysis.txt");
			cout << endl;
			cout << "�ʷ����������....\n\n�ʷ���������������./test_files/lexical_analysis.txt��" << endl;
			cout << endl;
			cout << "���Ƿ�ϣ�����ʷ���������������Ļ����y/n��" << endl;

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

			cout << "���ڽ����﷨�������������..." << endl;
			cout << endl;
			LR1.parser(lexicalAnalysis.get_token_stream(), "./test_files/grammatical_analysis.txt");

			cout << "�﷨������������������....\n\n�﷨�������������� ./test_files/grammatical_analysis.txt ��" << endl;
			cout << endl;
			cout << "����������������� ./test_files/quaternary.txt ��" << endl;
			cout << endl;
			cout << "���Ƿ�ϣ�����﷨��������������Ļ����y/n��" << endl;

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
			cout << "���Ƿ�ϣ�������﷨��������ϵͳ�谲װgraphviz����y/n��" << endl;
			cin >> t;
			if (t == "y") {
				LR1.draw_grammatical_analysis_tree(lexicalAnalysis.get_token_stream(), "./test_files/grammatical_analysis_tree.dot");
				system("dot -Tpng ./test_files/grammatical_analysis_tree.dot -o ./test_files/grammatical_analysis_tree.png");
				cout << "�﷨�����������ɣ����� ./test_files/grammatical_analysis_tree.png Ŀ¼���в鿴" << endl;
			}
			LR1.symantic_analysis.Print_quaternary("./test_files/quaternary.txt");
			cout << endl;

			cout << "���Ƿ�ϣ���������������������Ļ����y/n��" << endl;

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

			cout << "���ڽ���Ŀ���������..." << endl;
			Generate_asm generate_asm("./test_files/mips.asm", LR1.symantic_analysis.quaternary);
			generate_asm.parse();
			cout << endl;

			cout << "Ŀ��������������....\n\nĿ��������ɽ��������� ./test_files/mips.asm ��" << endl;
			cout << endl;
			cout << "���Ƿ�ϣ����Ŀ��������������Ļ����y/n��" << endl;

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
			cout << "�������롿" << e << endl;
		}
		cout << "�Ƿ��������ļ���Ҫ����(y/n)" << endl;
		cin >> t;
		if (t != "y") {
			break;

		}
	}
	return 0;
}

