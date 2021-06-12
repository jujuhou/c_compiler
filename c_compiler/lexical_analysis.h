#pragma once
#include "head.h"

//��ʶ��
struct TOKEN 
{
	string token; //token����
	string value; //ֵ
	int row;//��
	int col;//��
};

//�ʷ���������
class lexical_analysis
{
private:
	//��ʶ������ʹ��vector��ԭ����ֻ����ĩβ��ӣ����ҿ죩
	vector<TOKEN> token_stream;

public:
	//ɾ��Ĭ�Ϲ��캯��
	lexical_analysis() = delete;
	//���캯���������ļ�·��
	lexical_analysis(const string file_path);
	//��������
	~lexical_analysis() {}
	//�����������ļ�
	void print_token_stream(const string file_path);
	//�õ�token_stream
	vector<TOKEN> get_token_stream();
};
