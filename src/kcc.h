#ifndef __kcc_h__

#define __kcc_h__

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stack>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>

using namespace std;

// 标识 kcc 的define
#define __kcc__ 1

#define VERSION "kcc 0.7.1"

// 选择系统
#ifdef __linux__
	#include <sys/wait.h>
	#define __linux_system__ 1
	#ifdef __x86_64__
		#define __linux_64_system__ 1
	#else
		#define __linux_32_system__ 1
	#endif
#else
	#include <windows.h>
	#include <io.h>
	#define __windows_system__ 1
#endif

extern int MAKE_ACTION_INIT;

// =============================

#define MAX_CODE_LEN 100000

#define null "null"
#define MAX_GRAMMAR_NUMBER		230
#define MAX_GRAMMAR_STATEMENT_LENGTH	10
#define MAX_ALL_SYMBOL_NUMBER 		170
#define MAX_ITEMS_NUMBER 		3000

#ifdef __linux_system__
	#define MAX_DEPTH		100000
#else
	#define MAX_DEPTH		10000
#endif

// 输入源代码，在preprocessor处理后转至lexical
struct Source {
	string code;
	string file;
	int line;
	Source(){}
	Source(string _code, string _file, int _line) {
		code = _code;
		file = _file;
		line = _line;
	}
};
extern vector<Source> source;

struct DefDetail {
	string key, val;
	vector<string> para;
	DefDetail(){}
	DefDetail(string _key, string _val) {
		key = _key;
		val = _val;
	}
	DefDetail(string _key, string _val, vector<string> _para) {
		key = _key;
		val = _val;
		para = _para;
	}
};

// 不输出中间文件
extern int OUT_OTHER_FILE;

// 输入源代码的文件名
extern string filename;

// 输出的汇编文件
extern string assemblyname;

// 存词法分析的输出，即语法分析的输入
struct WORD
{
   string type,val;
   int line;
   WORD(){}
   WORD(string _type, string _val, int _line) {
	   type = _type; val = _val; line = _line;
   }
};
extern vector<WORD> word;

//
struct TYPE {
	string name;
	string type;
	int array;
	int pointer;
	vector<int> index;
	TYPE(){}
	TYPE(string _name, string _type, int _array, int _pointer, vector<int> _index) {
		name = _name;
		type = _type;
		array = _array;
		pointer = _pointer;
		index = _index;
	}
};

// 语法树
struct SyntaxTree {
	int statement;
	TYPE type;
	string val;
	vector<SyntaxTree*> tree;
	int line;
};

// 语法树的树根
extern SyntaxTree *headSubtr;

// 存语义分析的输出，即汇编的输入
struct Expression {
	string label, op, arg1, arg2, result;
	int out_struct_function;
	vector<string> parameter;
	Expression(){}
	Expression(string _op, string _arg1, string _arg2, string _result) {
		op = _op; arg1 = _arg1; arg2 = _arg2; result = _result;
	}
	Expression(string _label,string _op, string _arg1, string _arg2, string _result) {
		label = _label; op = _op; arg1 = _arg1; arg2 = _arg2; result = _result;
	}
};
extern vector<Expression> semantic_exp;			// +  i  j  k  ==>  k = i+j;

// 语义分析中识别出来的函数
struct Function {
	TYPE type;
	string name;
	vector<string> list;
	string start, mid, end; // like: start = .L1
	Function(){}
	Function(TYPE _type, string _name, vector<string> _list) {
		type = _type; name = _name; list = _list;
	}
	Function(TYPE _type, string _name, vector<string> _list, string _start, string _mid, string _end) {
		type = _type; name = _name; list = _list; start = _start; mid = _mid; end = _end;
	}
};
extern vector<Function> function_block;			// int lowbit(int x) { return x&(-x); }


inline string iTs(int i) // int转string
{
	stringstream ss;
	ss<<i;
	string num=ss.str();
	return num;
}

inline int sTi(string str) // string转int
{
	 istringstream iss(str);
	 int num;
	 iss >> num;
	 return num;
}

inline double sTd(string str) // string转double
{
	 istringstream iss(str);
	 double num;
	 iss >> num;
	 return num;
}

inline int letter_(char ch)
{
	if (ch >= 'a' && ch <= 'z') return 1;
	if (ch >= 'A' && ch <= 'Z') return 1;
	if (ch == '_') return 1;
	return 0;
}

inline int digit(int ch)
{
	if (ch >= '0' && ch <= '9') return 1;
	return 0;
}

inline int xdigit(int ch)
{
	if (ch >= '0' && ch <= '9') return 1;
	if (ch >= 'a' && ch <= 'f') return 1;
	if (ch >= 'A' && ch <= 'F') return 1;
	return 0;
}

inline int digit_val(int ch)
{
	if (ch >= '0' && ch <= '9') return ch-'0';
	if (ch >= 'a' && ch <= 'f') return ch-'a'+10;
	if (ch >= 'A' && ch <= 'F') return ch-'A'+10;
	return 0;
}

extern int preprocessor_main();
extern int lexical_main();
extern int syntax_main();
extern int semantic_main();
extern int assembly_main();

#endif
