#include "lexical_analysis.h"


//��ʶ���Ķ���
//�ؼ��֣�ʹ��set��ԭ����Ϊ��ȷ��Ψһ�ԣ�
const set<string> Keyword = { "int" ,"void" ,"if" ,"else" ,"while" ,"return" };
//�ָ���
const set<string> Separator = { "," , ";" , "(" , ")" , "{" , "}" };
//�����
const set<string> Operator_1 = { "+",  "-",  "*",  "/",  "=",  ">",  "<" };
const set<string> Operator_2 = { "==",  ">=",  "<=",  "!=" };


const string Identifier = "<ID>";
const string ConstInt = "<INT>";

//�����з��Ŷ������һ��������
set<string> InsertTokens()
{
	set<string> temp;
	temp.insert(Keyword.begin(), Keyword.end());
	temp.insert(Separator.begin(), Separator.end());
	temp.insert(Operator_1.begin(), Operator_1.end());
	temp.insert(Operator_2.begin(), Operator_2.end());
	temp.insert(Identifier);
	temp.insert(ConstInt);
	return temp;
}
const set<string> AllTokens = move(InsertTokens());


//���캯���������ļ�·��
lexical_analysis::lexical_analysis(const string file_path)
{
	//���ļ�
	ifstream file_in;
	file_in.open(file_path,ios::in|ios::binary);

	//δ���ļ����׳��쳣
	if (!file_in.is_open()) {
		cout << "�޷���Դ�����ļ�" << endl;
		throw FILE_OPEN_ERROE;
	}

	//present_rowΪ��ǰ����
	int present_row = 1;
	//present_colΪ��ǰ����
	int present_col = 0;
	//��ǰ�ַ���
	string present_str;
	//��ǰ�ַ�
	char present_ch;

	//��ʼ����ʶ��
	while (file_in.peek()!=EOF) {
		present_ch = char(file_in.get());
		present_col++;
		//���Ϊ�ո�
		if (isspace(present_ch)) {
			if (present_ch == '\n') {
				present_row++;
				present_col = 0;
			}
			else if (present_ch == '\t') {
				present_col += 3;
			}
			continue;
		}
		//������ǿո�
		present_str = present_ch;

		//�������ĸ
		if (isalpha(present_ch)) {
			//�����һ���ַ������ֻ���ĸ
			while (isalnum(present_ch = char(file_in.get()))) {
				present_str += present_ch;
				present_col++;
			}
				
			//����ȥһ���ַ�
			file_in.seekg(-1, ios::cur);
			//����ǲ��Ǳ�����
			if (Keyword.find(present_str) != Keyword.cend())
				token_stream.push_back({present_str,present_str,present_row,present_col-int(present_str.length())+1});
			else//���Ǳ�����
				token_stream.push_back({ Identifier,present_str,present_row,present_col - int(present_str.length()) + 1 });
		}
		//���������
		else if (isdigit(present_ch)) {
			//�����һ���ַ�������
			while (isdigit(present_ch = char(file_in.get()))) {
				present_str += present_ch;
				present_col++;
			}
			//����ȥһ���ַ�
			file_in.seekg(-1, ios::cur);
			//����token_stream
			token_stream.push_back({ ConstInt,present_str,present_row,present_col - int(present_str.length()) + 1 });
		}
		//����Ƿָ���
		else if(Separator.find(present_str)!=Separator.cend())
			token_stream.push_back({ present_str,present_str,present_row,present_col });
		//����ǵ���ע��
		else if (present_ch == '/' && file_in.peek() == '/') {
			while (char(file_in.get()) != '\n')
				;
			present_row++;
			present_col = 0;
		}
		//����Ƕ���ע��
		else if (present_ch == '/' && file_in.peek() == '*') {
			while (!((present_ch = char(file_in.get())) == '*' && char(file_in.peek()) == '/')) {
				present_col++;
				if (present_ch == '\n') {
					present_row++;
					present_col = 0;
				}
				else if (present_ch == '\t')
					present_col += 3;
			}
			file_in.get();
			present_col += 2;
		}
		//������������ַ�����������ȼ���ǲ��������ټ��һ����
		else if (Operator_2.find(present_str + char(file_in.peek())) != Operator_2.cend()) {
			present_str += char(file_in.get());
			present_col++;
			token_stream.push_back({ present_str,present_str,present_row,present_col - 1 });
		}
		//�����һ���ַ��������
		else if (Operator_1.find(present_str) != Operator_1.cend())
			token_stream.push_back({ present_str,present_str,present_row,present_col });
		else {
			cout << "ʶ���޷�ʶ����ַ����ڵ�" <<present_row<<"�У���"<<present_col<<"��"<< endl;
			throw LEXICAL_ERROR_UNDEFINED_WORD;
		}

	}

}
//�����������ļ�
void lexical_analysis::print_token_stream(const string file_path)
{
	ofstream file_out;
	file_out.open(file_path, ios::out);
	file_out << "TOKEN STREAM"<<endl;
	file_out << "-------------------------------------------------------------------"<<endl;
	file_out << setw(20) << "Type";
	file_out << setw(20) << "Value";
	file_out << setw(8) << "Row";
	file_out << setw(8) << "Col";
	file_out << endl;

	for (auto it = token_stream.begin(); it != token_stream.end(); it++) {
		file_out << setw(20) << (*it).token;
		file_out << setw(20) << (*it).value;
		file_out << setw(8) << (*it).row;
		file_out << setw(8) << (*it).col;
		file_out << endl;
	}
}
//�õ�token_stream
vector<TOKEN> lexical_analysis::get_token_stream()
{
	return token_stream;
}