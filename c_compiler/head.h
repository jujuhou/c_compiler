#pragma once
//»ù±¾¿â
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <algorithm>//find_if
#include <string.h>//strtok

//STLÈÝÆ÷
#include <vector>
#include <set>
#include <map>

//´íÎóÂë
#define FILE_OPEN_ERROE 1
#define LEXICAL_ERROR_UNDEFINED_WORD 2
#define GRAMMAR_ERROR 3
#define GRAMMATICAL_ERROR_UNDEFINED_WORD 4
#define GRAMMATICAL_ERROR_CANNOT_ANALYSIS 5
#define SEMANTIC_ERROR_NO_MAIN 6
#define SEMANTIC_ERROR_REDEFINED 7
#define SEMANTIC_ERROR_UNDEFINED 8
#define SEMANTIC_ERROR_PARAMETER_NUM 9
#define SEMANTIC_ERROR_UNDEFINED 10
#define SEMANTIC_ERROR_NO_RETURN 11


using namespace std;