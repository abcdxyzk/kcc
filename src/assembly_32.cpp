#include "kcc.h"

struct Ident {
	string name, type, array, pointer; // name：变量名，type：变量类型，array：是否是数组
	vector<int> index; // 如果是数组则表示每一维的大小

	string init_val;   // 存定义时的初始化的值
	string array_address; // 如果是数组则存数组起始地址

	string global_variable;			// int k; int main() {} ==> k's global_variable = "global_variable";
	string addr_exp;

	int addr,step; // 全局变量时addr=占用内存，局部变量addr=内存起始位置
	Ident(){}
	Ident(string _name) {
		name = _name;
	}
};

// Ident.pointer 有三种形态 ""、"address"、"pointer"，其中address类型只会出现在临时变量中
// pointer = &i; address = *p;
//
// int i = 123, *p, *q;
//
// p = &i    =>   pointer = &i
// *p = *q   =>   address1 = *p; address2 = *q; address1 = address2; 当address在等号左边时则对地址赋值，其他则取地址里的值
// i = *p    =>   address = p;  i = address;
// p = *q    =>   address = *q; p = address;
// *p = q    =>   address = *p; address = q;

struct Asse {
	string label, op, arg1, arg2;
	Asse(){}
	Asse(string _label, string _op, string _arg1, string _arg2) {
		label = _label; op = _op; arg1 = _arg1; arg2 = _arg2;
	}
};

struct AType_name {
	string name;
	int size;
	vector<Ident> node;
	AType_name(){};
	AType_name(string _name) {
		name = _name;
	}
};

static vector<Ident> global_ident_section;			// int i=1; int main() { } ==> i is global_ident_section;
static vector<Ident> :: iterator it;

static vector<Asse> data_section;					// data_section code
static vector<Asse> text_section;					// text_section code
static vector<Asse> get_global_array_address;		// int a[10]; int main() {} ==> get_global_array_address.push_back( leal a array_address_0 )
static vector<Ident> ident_stack;					// int i; { int i; i = 1; } ==> declare i; { declare i ; i = 1; undelete i; } undeclare i;

static vector<AType_name> type_name;				// type_name[i].name = "FILE"; // struct Node { list } ; ==> type_name[j].name = "Node"; type_name[j].list = list;
static map<string, int> type_name_appear;			// type_name_appear[ type_name[i] ] = 1;
static map<string, int> tmp_ident_appear;			// when call function add_to_tmp_ident( ... ) , must check if the tmp_ident had been declared or not.
static map<string, int> mark;						// mark["char"] = 2; mark["int"] = 4; ...
static map<string, int> mm;						// .L0 declare int main ==> mm[ ".L0" ] = 0;
											// .L1 +   i   j   k    ==> mm[ ".L1" ] = 1;
static string tmp_name;							// tmp_name = _T_  ==> _T_0 _T_1 ... they are all tmp_ident

static string l_name;								// l_name = .L_ 注意和行标号 .L 区分

static string tmp_stack;							// tmp_stack = _tmp_, 函数内的变量的内存位置是未知的，因为可能栈的开始有一部分被用来做函数参数传递用。因此先用_tmp_表示。

static TYPE type_of_call_function;

static int stacktop, max_stacktop, max_function_stacktop, Label_id, array_number;

//										stacktop  max_stacktop  max_function_stacktop
//											0			0				0
//	int main()
//	{
//		int i;	----------------------->	1*4			1*4				0
//		for (i=0;i<10;i++)
//		{
//			int k;	------------------->	2*4			2*4				0
//			k = i*i;
//			printf("%d\n",k);	------->	3*4			3*4				4
//		}
//			--------------------------->	1*4			3*4				4
//		for (i=0;i<10;i++)
//		{
//			int k,l;	--------------->	3*4			3*4				1*4
//			k = i*i; l = i*i*i;
//			printf("%d %d\n",k,l);	--->	5*4			5*4				2*4
//		}
//			--------------------------->	1*4			5*4				2*4
//		return 0;
//	}


static void Input()
{
	int i,j;

	for (i=0;i<semantic_exp.size();i++) {
		mm[semantic_exp[i].label] = i;
	}
}

static void char_fld(string arg1, string arg2, Ident ar1, Ident ar2, string ar1_s, string ar2_s, int num)
{
	if (arg1 == "returnval") return;

	if (ar1.pointer == "pointer") {
if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
	}
	else
	if (ar1.pointer == "address")
	{
		if (ar1.type == "char") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movb", "(%eax)", "%al") );
		}
		if (ar1.type == "int") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", "(%eax)", "%eax") );
		}
	}
	else
	{
		if (ar1.type == "char")		text_section.push_back( Asse("", "	movb", ar1_s, "%al") );
		if (ar1.type == "int_num")
if (ar1_s != "%eax")					text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		if (ar1.type == "int")
if (ar1_s != "%eax")					text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
	}

	if (num == 1) return;

	if (ar2.pointer == "pointer") {
if (ar2_s != "%edx")
		text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
	} else if (ar2.pointer == "address") {
		if (ar2.type == "char") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movb", "(%edx)", "%dl") );
		}
		if (ar2.type == "int") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movl", "(%edx)", "%edx") );
		}
	} else {
		if (ar2.type == "char")		text_section.push_back( Asse("", "	movb", ar2_s, "%dl") );
		if (ar2.type == "int_num")
if (ar2_s != "%edx")					text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
		if (ar2.type == "int")
if (ar2_s != "%edx")					text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
	}
}

static void int_fld(string arg1, string arg2, Ident ar1, Ident ar2, string ar1_s, string ar2_s, int num)
{
	if (arg1 == "returnval") return;

	if (ar1.pointer == "pointer") {
if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
	} else if (ar1.pointer == "address") {
		if (ar1.type == "char") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movb", "(%eax)", "%al") );
			text_section.push_back( Asse("", "	movsbl", "%al", "%eax") );
		}
		if (ar1.type == "int") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", "(%eax)", "%eax") );
		}
	} else {
		if (ar1.type == "char") {
			text_section.push_back( Asse("", "	movb", ar1_s, "%al") );
			text_section.push_back( Asse("", "	movsbl", "%al", "%eax") );
		}
		if (ar1.type == "int_num")	text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		if (ar1.type == "int")		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		if (ar1.type == "string_num")            text_section.push_back( Asse("", "      movl", ar1_s, "%eax") );
		if (ar1.type == "string")                text_section.push_back( Asse("", "      movl", ar1_s, "%eax") );
	}

	if (num == 1) return;

	if (ar2.pointer == "pointer") {
if (ar2_s != "%edx")
		text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
	} else if (ar2.pointer == "address") {
		if (ar2.type == "char") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movb", "(%edx)", "%dl") );
			text_section.push_back( Asse("", "	movsbl", "%dl", "%edx") );
		}
		if (ar2.type == "int") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movl", "(%edx)", "%edx") );
		}
	} else {
		if (ar2.type == "char") {
			text_section.push_back( Asse("", "	movb", ar2_s, "%dl") );
			text_section.push_back( Asse("", "	movsbl", "%dl", "%edx") );
		}
		if (ar2.type == "int_num")
if (ar2_s != "%edx")					text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
		if (ar2.type == "int")
if (ar2_s != "%edx")					text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
		if (ar2.type == "string_num")            text_section.push_back( Asse("", "      movl", ar2_s, "%edx") );
		if (ar2.type == "string")                text_section.push_back( Asse("", "      movl", ar2_s, "%edx") );
	}
}

static void double_fld_12(string arg1, string arg2, Ident ar1, Ident ar2, string ar1_s, string ar2_s, int num)
{
	if (arg1 == "returnval") return;

	if (ar1.pointer == "pointer") {
if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
		text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
	} else if (ar1.pointer == "address") {
		if (ar1.type == "char") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movb", "(%eax)", "%al") );
			text_section.push_back( Asse("", "	movsbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar1.type == "int") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", "(%eax)", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar1.type == "double") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	fldl", "(%eax)", "") );
		}
	} else {
		if (ar1.type == "char") {
			text_section.push_back( Asse("", "	movb", ar1_s, "%al") );
			text_section.push_back( Asse("", "	movsbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar1.type == "int_num") {
			data_section.push_back( Asse("", l_name+iTs(Label_id), ".double", ar1_s.substr(1)) ); Label_id++;
			text_section.push_back( Asse("", "	fldl", l_name+iTs(Label_id-1), "") );
		}
		if (ar1.type == "int") {
			text_section.push_back( Asse("", "	fildl", ar1_s, "") );
		}
		if (ar1.type == "double" || ar1.type == "double_num") {
			text_section.push_back( Asse("", "	fldl", ar1_s, "") );
		}
	}

	if (num == 1) return;

	if (ar2.pointer == "pointer") {
if (ar2_s != "%edx")
		text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
		text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
		text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
	} else if (ar2.pointer == "address") {
		if (ar2.type == "char") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movb", "(%edx)", "%dl") );
			text_section.push_back( Asse("", "	movsbl", "%dl", "%edx") );
			text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar2.type == "int") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movl", "(%edx)", "%edx") );
			text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar2.type == "double") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	fldl", "(%edx)", "") );
		}
	} else {
		if (ar2.type == "char") {
			text_section.push_back( Asse("", "	movb", ar2_s, "%dl") );
			text_section.push_back( Asse("", "	movsbl", "%dl", "%edx") );
			text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar2.type == "int_num") {
			data_section.push_back( Asse("", l_name+iTs(Label_id), ".double", ar2_s.substr(1)) ); Label_id++;
			text_section.push_back( Asse("", "	fldl", l_name+iTs(Label_id-1), "") );
		}
		if (ar2.type == "int") {
			text_section.push_back( Asse("", "	fildl", ar2_s, "") );
		}
		if (ar2.type == "double" || ar2.type == "double_num") {
			text_section.push_back( Asse("", "	fldl", ar2_s, "") );
		}
	}
}

static void double_fld_21(string arg2, string arg1, Ident ar2, Ident ar1, string ar2_s, string ar1_s, int num)
{
	if (arg2 == "returnval") return;

	if (ar2.pointer == "pointer") {
if (ar2_s != "%edx")
		text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
		text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
		text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
	} else if (ar2.pointer == "address") {
		if (ar2.type == "char") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movb", "(%edx)", "%dl") );
			text_section.push_back( Asse("", "	movsbl", "%dl", "%edx") );
			text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar2.type == "int") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	movl", "(%edx)", "%edx") );
			text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar2.type == "double") {
if (ar2_s != "%edx")
			text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
			text_section.push_back( Asse("", "	fldl", "(%edx)", "") );
		}
	} else {
		if (ar2.type == "char") {
			text_section.push_back( Asse("", "	movb", ar2_s, "%dl") );
			text_section.push_back( Asse("", "	movsbl", "%dl", "%edx") );
			text_section.push_back( Asse("", "	movl", "%edx", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar2.type == "int_num") {
			data_section.push_back( Asse("", l_name+iTs(Label_id), ".double", ar2_s.substr(1)) ); Label_id++;
			text_section.push_back( Asse("", "	fldl", l_name+iTs(Label_id-1), "") );
		}
		if (ar2.type == "int") {
			text_section.push_back( Asse("", "	fildl", ar2_s, "") );
		}
		if (ar2.type == "double" || ar2.type == "double_num") {
			text_section.push_back( Asse("", "	fldl", ar2_s, "") );
		}
	}

	if (num == 1) return;

	if (ar1.pointer == "pointer") {
if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
		text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
	} else if (ar1.pointer == "address") {
		if (ar1.type == "char") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movb", "(%eax)", "%al") );
			text_section.push_back( Asse("", "	movsbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar1.type == "int") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", "(%eax)", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar1.type == "double") {
if (ar1_s != "%eax")
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	fldl", "(%eax)", "") );
		}
	} else {
		if (ar1.type == "char") {
			text_section.push_back( Asse("", "	movb", ar1_s, "%al") );
			text_section.push_back( Asse("", "	movsbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
			text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
		}
		if (ar1.type == "int_num") {
			data_section.push_back( Asse("", l_name+iTs(Label_id), ".double", ar1_s.substr(1)) ); Label_id++;
			text_section.push_back( Asse("", "	fldl", l_name+iTs(Label_id-1), "") );
		}
		if (ar1.type == "int") {
			text_section.push_back( Asse("", "	fildl", ar1_s, "") );
		}
		if (ar1.type == "double" || ar1.type == "double_num") {
			text_section.push_back( Asse("", "	fldl", ar1_s, "") );
		}
	}
}

static void add_exp(string op, string arg1, string arg2, Ident ar1, Ident ar2, Ident res, string ar1_s, string ar2_s, string res_s)
{
	if (res.type == "char")
	{
		if (op == "+") {
			if (arg2 != "null") {
				char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
				text_section.push_back( Asse("", "	addb", "%al", "%dl") );
				text_section.push_back( Asse("", "	movb", "%dl", res_s) );
			} else {
				char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
				text_section.push_back( Asse("", "	movb", "%al", res_s) );
			}
		}
		if (op == "-") {
			if (arg2 != "null") {
				char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
				text_section.push_back( Asse("", "	subb", "%dl", "%al") );
				text_section.push_back( Asse("", "	movb", "%al", res_s) );
			} else {
				char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
				text_section.push_back( Asse("", "	movb", "$0", "%dl") );
				text_section.push_back( Asse("", "	subb", "%dl", "%al") );
				text_section.push_back( Asse("", "	movb", "%al", res_s) );
			}
		}
		if (op == "*") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	imulb", "%dl", "%al") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "/") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	movb", "%al", "%ah") );
			text_section.push_back( Asse("", "	sarb", "$7", "%ah") );
			text_section.push_back( Asse("", "	idivb", "%dl", "") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "=") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "%") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	movb", "%al", "%ah") );
			text_section.push_back( Asse("", "	sarb", "$7", "%ah") );
			text_section.push_back( Asse("", "	idivb", "%dl", "") );
			text_section.push_back( Asse("", "	movb", "%ah", res_s) );
		}
		if (op == "&") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	andb", "%dl", "%al") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "|") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	orb", "%dl", "%al") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "^") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	xorb", "%dl", "%al") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "<<") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	movb", "%dl", "%cl") );
			text_section.push_back( Asse("", "	sal", "%cl", "%al") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == ">>") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	movb", "%dl", "%cl") );
			text_section.push_back( Asse("", "	sar", "%cl", "%al") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "~") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	notb", "%al", "") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}
		if (op == "!") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	cmpb", "$0", "%al") );
			text_section.push_back( Asse("", "	sete", "%al", "") );
			text_section.push_back( Asse("", "	movb", "%al", res_s) );
		}

		return;
	}
	if (res.type == "int")
	{
		if (op == "+") {
			if (arg2 != "null") {
				int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
				text_section.push_back( Asse("", "	addl", "%eax", "%edx") );
				text_section.push_back( Asse("", "	movl", "%edx", res_s) );
			} else {
				int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
				text_section.push_back( Asse("", "	movl", "%eax", res_s) );
			}
		}
		if (op == "-") {
			if (arg2 != "null") {
				int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
				text_section.push_back( Asse("", "	subl", "%edx", "%eax") );
				text_section.push_back( Asse("", "	movl", "%eax", res_s) );
			} else {
				int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
				text_section.push_back( Asse("", "	negl",  "%eax", "") );
				text_section.push_back( Asse("", "	movl", "%eax", res_s) );
			}
		}
		if (op == "*") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	imull", "%edx", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "/") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	movl", "%edx", "%ecx") );
			text_section.push_back( Asse("", "	movl", "%eax", "%edx") );
			text_section.push_back( Asse("", "	sarl", "$31", "%edx") );
			text_section.push_back( Asse("", "	idivl", "%ecx", "") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "=") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "%") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	movl", "%edx", "%ecx") );
			text_section.push_back( Asse("", "	movl", "%eax", "%edx") );
			text_section.push_back( Asse("", "	sarl", "$31", "%edx") );
			text_section.push_back( Asse("", "	idivl", "%ecx", "") );
			text_section.push_back( Asse("", "	movl", "%edx", res_s) );
		}
		if (op == "&") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	andl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "|") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	orl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "^") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	xorl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "<<") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
#ifdef __linux_system__
			text_section.push_back( Asse("", "	.cfi_escape 0x10,0x3,0x7,0x55,0x9,0xf0,0x1a,0x9,0xfc,0x22", "", "") );
#endif
			text_section.push_back( Asse("", "	movl", "%edx", "%ecx") );
			text_section.push_back( Asse("", "	sall", "%cl", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == ">>") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
#ifdef __linux_system__
			text_section.push_back( Asse("", "	.cfi_escape 0x10,0x3,0x7,0x55,0x9,0xf0,0x1a,0x9,0xfc,0x22", "", "") );
#endif
			text_section.push_back( Asse("", "	movl", "%edx", "%ecx") );
			text_section.push_back( Asse("", "	sarl", "%cl", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "~") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	notl", "%eax", "") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "!") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	cmpl", "$0", "%eax") );
			text_section.push_back( Asse("", "	sete", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}

		return;
	}
	if (res.type == "double")
	{
		if (op == "+" && arg2 == "null") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	fstpl", res_s, "") );
		}

		if (op == "+" && arg2 != "null") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	faddp", "%st(0)", "%st(1)") );
			text_section.push_back( Asse("", "	fstpl", res_s, "") );
		}
		if (op == "-" && arg2 == "null") {
			text_section.push_back( Asse("", "	fldz",  "", "") );
			text_section.push_back( Asse("", "	fsubl", ar1_s, "") );
			text_section.push_back( Asse("", "	fstpl", res_s, "") );
		}
		if (op == "-" && arg2 != "null") {
			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 2);
			text_section.push_back( Asse("", "	fsubp", "%st(0)", "%st(1)") );
			text_section.push_back( Asse("", "	fstpl", res_s, "") );
		}
		if (op == "*") {
			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 2);
			text_section.push_back( Asse("", "	fmulp", "%st(0)", "%st(1)") );
			text_section.push_back( Asse("", "	fstpl", res_s, "") );
		}
		if (op == "/") {
			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 2);
			text_section.push_back( Asse("", "	fdivp", "%st(0)", "%st(1)") );
			text_section.push_back( Asse("", "	fstpl", res_s, "") );
		}
		if (op == "=") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	fstpl", res_s, "") );
		}
	}
}

static void cmp_exp(string op, string arg1, string arg2, Ident ar1, Ident ar2, Ident res, string ar1_s, string ar2_s, string res_s)
{
	if (mark[ar1.type] <= mark["int"] && mark[ar2.type] <= mark["int"])
	{
		int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);

		if (op == ">=") {
			text_section.push_back( Asse("", "	cmpl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	setge", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == ">") {
			text_section.push_back( Asse("", "	cmpl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	setg", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "<=") {
			text_section.push_back( Asse("", "	cmpl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	setle", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "<") {
			text_section.push_back( Asse("", "	cmpl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	setl", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "==") {
			text_section.push_back( Asse("", "	cmpl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	sete", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "!=") {
			text_section.push_back( Asse("", "	cmpl", "%edx", "%eax") );
			text_section.push_back( Asse("", "	setne", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "&&") {
			text_section.push_back( Asse("", "	cmpl", "$0", "%eax") );
			text_section.push_back( Asse("", "	je", l_name+iTs(Label_id), "") );
			text_section.push_back( Asse("", "	cmpl", "$0", "%edx") );
			text_section.push_back( Asse("", "	je", l_name+iTs(Label_id), "") ); Label_id++;
			text_section.push_back( Asse("", "	movl", "$1", "%eax") );
			text_section.push_back( Asse("", "	jmp", l_name+iTs(Label_id), "") ); Label_id++;
			text_section.push_back( Asse(l_name+iTs(Label_id-2)+":", "", "", "") );
			text_section.push_back( Asse("", "	movl", "$0", "%eax") );
			text_section.push_back( Asse(l_name+iTs(Label_id-1)+":", "", "", "") );

			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "||") {
			text_section.push_back( Asse("", "	cmpl", "$0", "%eax") );
			text_section.push_back( Asse("", "	jne", l_name+iTs(Label_id), "") ); Label_id++;
			text_section.push_back( Asse("", "	cmpl", "$0", "%edx") );
			text_section.push_back( Asse("", "	je", l_name+iTs(Label_id), "") ); Label_id++;
			text_section.push_back( Asse(l_name+iTs(Label_id-2)+":", "", "", "") );
			text_section.push_back( Asse("", "	movl", "$1", "%eax") );
			text_section.push_back( Asse("", "	jmp", l_name+iTs(Label_id), "") ); Label_id++;
			text_section.push_back( Asse(l_name+iTs(Label_id-2)+":", "", "", "") );
			text_section.push_back( Asse("", "	movl", "$0", "%eax") );
			text_section.push_back( Asse(l_name+iTs(Label_id-1)+":", "", "", "") );

			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		return;
	}

	if (mark[ar1.type] <= mark["double"] && mark[ar2.type] <= mark["double"])
	{
		if (op == ">=") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	fxch", "%st(1)", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	setae", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == ">") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	fxch", "%st(1)", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	seta", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "<=") {
			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 2);
			text_section.push_back( Asse("", "	fxch", "%st(1)", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	setae", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "<") {
			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 2);
			text_section.push_back( Asse("", "	fxch", "%st(1)", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	seta", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "==") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	setnp", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%edx") );
			text_section.push_back( Asse("", "	movl", "$0", "%eax") );

			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	cmove", "%edx", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "!=") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	setp", "%al", "") );
			text_section.push_back( Asse("", "	movzbl", "%al", "%edx") );
			text_section.push_back( Asse("", "	movl", "$1", "%eax") );

			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 2);
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	cmove", "%edx", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "&&") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	jp", l_name+iTs(Label_id), "") ); Label_id++;
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	je", l_name+iTs(Label_id), "") ); Label_id++;

			text_section.push_back( Asse(l_name+iTs(Label_id-2)+":", "", "", "") );

			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	jp", l_name+iTs(Label_id), "") ); Label_id++;
			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	je", l_name+iTs(Label_id-2), "") );

			text_section.push_back( Asse(l_name+iTs(Label_id-1)+":", "", "", "") );

			text_section.push_back( Asse("", "	movl", "$1", "%eax") );
			text_section.push_back( Asse("", "	jmp", l_name+iTs(Label_id), "") ); Label_id++;
			text_section.push_back( Asse(l_name+iTs(Label_id-3)+":", "", "", "") );
			text_section.push_back( Asse("", "	movl", "$0", "%eax") );
			text_section.push_back( Asse(l_name+iTs(Label_id-1)+":", "", "", "") );

			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
		if (op == "||") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	jp", l_name+iTs(Label_id), "") ); Label_id++;
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	jne", l_name+iTs(Label_id-1), "") );

			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	jp", l_name+iTs(Label_id-1), "") );
			double_fld_21(arg2, arg1, ar2, ar1, ar2_s, ar1_s, 1);
			text_section.push_back( Asse("", "	fldz", "", "") );
			text_section.push_back( Asse("", "	fucomip", "%st(1)", "%st(0)") );
			text_section.push_back( Asse("", "	fstp", "%st(0)", "") );
			text_section.push_back( Asse("", "	je", l_name+iTs(Label_id-2), "") ); Label_id++;

			text_section.push_back( Asse(l_name+iTs(Label_id-2)+":", "", "", "") );

			text_section.push_back( Asse("", "	movl", "$1", "%eax") );
			text_section.push_back( Asse("", "	jmp", l_name+iTs(Label_id), "") ); Label_id++;
			text_section.push_back( Asse(l_name+iTs(Label_id-2)+":", "", "", "") );
			text_section.push_back( Asse("", "	movl", "$0", "%eax") );
			text_section.push_back( Asse(l_name+iTs(Label_id-1)+":", "", "", "") );

			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		}
	}
}

static void change_type(string op, string arg1, string arg2, Ident ar1, Ident ar2, Ident res, string ar1_s, string ar2_s, string res_s)
{
	if (ar1.pointer == "pointer" && res.pointer == "pointer") {
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
	} else if ((ar1.type == "int" || ar1.type == "int_num") && res.type == "char") {
if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movb", "%al", res_s) );
	} else if ((ar1.type == "double" || ar1.type == "double_num") && res.type == "char") {
		text_section.push_back( Asse("", "	fldl", ar1_s, "") );
		text_section.push_back( Asse("", "	fnstcw", "-14(%esp)", "") );
		text_section.push_back( Asse("", "	movzwl", "-14(%esp)", "%eax") );
		text_section.push_back( Asse("", "	movb", "$12", "%ah") );
		text_section.push_back( Asse("", "	movw", "%ax", "-16(%esp)") );
		text_section.push_back( Asse("", "	fldcw", "-16(%esp)", "") );
		text_section.push_back( Asse("", "	fistps", "-18(%esp)", "") );
		text_section.push_back( Asse("", "	fldcw", "-14(%esp)", "") );
		text_section.push_back( Asse("", "	movzwl", "-18(%esp)", "%eax") );
		text_section.push_back( Asse("", "	movb", "%al", res_s) );
	} else if ((ar1.type == "double" || ar1.type == "double_num") && res.type == "int") {
		text_section.push_back( Asse("", "	fldl", ar1_s, "") );
		text_section.push_back( Asse("", "	fnstcw", "-14(%esp)", "") );
		text_section.push_back( Asse("", "	movzwl", "-14(%esp)", "%eax") );
		text_section.push_back( Asse("", "	movb", "$12", "%ah") );
		text_section.push_back( Asse("", "	movw", "%ax", "-16(%esp)") );
		text_section.push_back( Asse("", "	fldcw", "-16(%esp)", "") );
		text_section.push_back( Asse("", "	fistpl", res_s, "") );
		text_section.push_back( Asse("", "	fldcw", "-14(%esp)", "") );
	} else {
		add_exp(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
	}
}

static Ident get_address_or_value_addrexp(string tmp, string arg, string get_address)
{
	int i,j,k;
	Ident ret;
	string array = "";
	ret.global_variable = "local_variable";

	if (tmp.substr(0, 8) == "<struct>")
	{
		int nn;
		string tmp1,tmp2,tmp3;
		tmp1 = "";
		while (tmp.substr(0, 8) == "<struct>") tmp = tmp.substr(8);
		for (nn=0;tmp[nn]!='.';nn++) tmp1 += tmp[nn];

		ret = get_address_or_value_addrexp(tmp1, arg, "get_address");

		int inde=0;
		do {
			tmp2 = tmp3 = "";
			for (nn++;nn<tmp.size() && tmp[nn]!='.';nn++) tmp2 += tmp[nn];
			for (i=0;i<tmp2.size() && tmp2[i] != '[';i++) tmp3 += tmp2[i];

			for (k=0;k<type_name.size();k++) if (type_name[k].name == ret.type) break;
			for (i=0;i<type_name[k].node.size();i++) {
				if (type_name[k].node[i].name == tmp3) break;
				inde += type_name[k].node[i].addr;
			}
			ret.type	= type_name[k].node[i].type;
			ret.pointer	= type_name[k].node[i].pointer;
			ret.step	= type_name[k].node[i].step;
			ret.array	= type_name[k].node[i].array;
		} while (type_name_appear.find(type_name[k].node[i].type) != type_name_appear.end() && nn < tmp.size());

		if (type_name[k].node[i].array == "array") {
			if (arg == "arg1" || arg == "arg2" || arg == "result") {
				text_section.push_back( Asse("", "	movl", "$0", "-33(%esp)") );
			}

			int index=0;
			string str, addr_s;
			Ident addr;
			for (j=0;j<tmp2.length();j++)
			{
				if (tmp2[j] == '[') {
					str = "";
				} else if (tmp2[j] == ']') {
					addr = get_address_or_value_addrexp(str, "not_arg", "get_value");
					addr_s = addr.addr_exp;
					if (arg == "arg1" || arg == "arg2" || arg == "result") {
						text_section.push_back( Asse("", "	movl", addr_s, "%ecx") );
if (type_name[k].node[i].index[index] != 1)
						text_section.push_back( Asse("", "	imull", "$"+iTs(type_name[k].node[i].index[index]), "%ecx") );
						text_section.push_back( Asse("", "	addl", "%ecx", "-33(%esp)") );
					}
					index++;
				} else {
					str += tmp2[j];
				}
			}

			if (type_name[k].node[i].index.size() > index) {
				get_address = "get_address";
				ret.pointer = "pointer";
			}

			if (arg == "arg1") {
if (ret.addr_exp != "%eax")
				text_section.push_back( Asse("", "	movl", ret.addr_exp, "%eax") );
				text_section.push_back( Asse("", "	addl", "-33(%esp)", "%eax") );
if (inde != 0) 	text_section.push_back( Asse("", "	addl", "$"+iTs(inde), "%eax") );
				ret.addr_exp = "(%eax)";
				if (get_address == "get_address" && arg != "not_arg")
					ret.addr_exp = "%eax";
			}
			if (arg == "arg2") {
if (ret.addr_exp != "%edx")
				text_section.push_back( Asse("", "	movl", ret.addr_exp, "%edx") );
				text_section.push_back( Asse("", "	addl", "-33(%esp)", "%edx") );
if (inde != 0)	text_section.push_back( Asse("", "	addl", "$"+iTs(inde), "%edx") );
				ret.addr_exp = "(%edx)";
				if (get_address == "get_address" && arg != "not_arg")
					ret.addr_exp = "%edx";
			}
			if (arg == "result") {
if (ret.addr_exp != "%ebx")
				text_section.push_back( Asse("", "	movl", ret.addr_exp, "%ebx") );
				text_section.push_back( Asse("", "	addl", "-33(%esp)", "%ebx") );
if (inde != 0)	text_section.push_back( Asse("", "	addl", "$"+iTs(inde), "%ebx") );
				ret.addr_exp = "(%ebx)";
				if (get_address == "get_address" && arg != "not_arg")
					ret.addr_exp = "%ebx";
			}
		} else {
			if (arg == "arg1") {
if (ret.addr_exp != "%eax")
				text_section.push_back( Asse("", "	movl", ret.addr_exp, "%eax") );
if (inde != 0)	text_section.push_back( Asse("", "	addl", "$"+iTs(inde), "%eax") );
				ret.addr_exp = "(%eax)";
				if (get_address == "get_address" && arg != "not_arg")
					ret.addr_exp = "%eax";
			}
			if (arg == "arg2") {
if (ret.addr_exp != "%edx")
				text_section.push_back( Asse("", "	movl", ret.addr_exp, "%edx") );
if (inde != 0)	text_section.push_back( Asse("", "	addl", "$"+iTs(inde), "%edx") );
				ret.addr_exp = "(%edx)";
				if (get_address == "get_address" && arg != "not_arg")
					ret.addr_exp = "%edx";
			}
			if (arg == "result") {
if (ret.addr_exp != "%ebx")
				text_section.push_back( Asse("", "	movl", ret.addr_exp, "%ebx") );
if (inde != 0)	text_section.push_back( Asse("", "	addl", "$"+iTs(inde), "%ebx") );
				ret.addr_exp = "(%ebx)";
				if (get_address == "get_address" && arg != "not_arg")
					ret.addr_exp = "%ebx";
			}
		}

		return ret;
	}

	for (j=ident_stack.size()-1;j>=0;j--) if (ident_stack[j].name == tmp)
	{
		if (ident_stack[j].array != "array")
		{
			ret = ident_stack[j];

			if (ret.global_variable == "global_variable") ret.addr_exp = ret.name;
			if (ret.global_variable == "local_variable")  ret.addr_exp = tmp_stack+iTs(ret.addr)+"_"+"(%esp)";
			if (get_address == "get_address" && arg != "not_arg")
			{
				if (ident_stack[j].pointer == "address") {
					text_section.push_back( Asse("", "	movl", ret.addr_exp, "%eax") );
				} else {
					text_section.push_back( Asse("", "	leal", ret.addr_exp, "%eax") );
				}
				ret.addr_exp = "%eax";
			}
			return ret;
		}
		else
			array = "array";
	}

	if (tmp == "returnval")  // only arg1 == returnval
	{
		ret.type	= type_of_call_function.type;
		ret.name	= "returnval";
		ret.array	= "ident";
		ret.pointer	= type_of_call_function.pointer == 0 ? "" : "pointer";

		ret.addr_exp = "%eax";
		return ret;
	}

	if (tmp.substr(0, 9) == "<int_num>")
	{
		ret.type = "int_num";
		ret.addr = sTi(tmp.substr(9));
		ret.array = "ident";
		ret.pointer = "";

		ret.addr_exp = "$"+iTs(ret.addr);
		return ret;
	}

	if (tmp.substr(0, 12) == "<double_num>")
	{
		ret.type = "double_num";
		ret.addr = 0;
		ret.name = l_name+iTs(Label_id);
		ret.array = "ident";
		ret.pointer = "";
		data_section.push_back( Asse("", l_name+iTs(Label_id), ".double", tmp.substr(12))); Label_id++;

		ret.addr_exp = ret.name;
		return ret;
	}

	if (tmp[0] == '"')
	{
		ret.type = "string_num";
		ret.array = "ident";
		ret.pointer = "";
		for (i=0;i<data_section.size();i++)
		if (data_section[i].arg2 == tmp)
		{
			ret.name = data_section[i].op;

			ret.addr_exp = "$" + ret.name;
			if (get_address == "get_address" && arg != "not_arg") {
				text_section.push_back( Asse("", "	movl", ret.addr_exp, "%eax") );
				ret.addr_exp = "%eax";
			}
			return ret;
		}
		for (i=0;i<tmp.length();i++) if (tmp[i] == 8) tmp[i] = ' ';
		#ifdef __windows_system__
			tmp[tmp.length()-1] = '\\'; tmp += "0\"";
		#endif
		data_section.push_back( Asse("", l_name+iTs(Label_id), ".string", tmp)); Label_id++;
		ret.name = data_section[data_section.size()-1].op;

		ret.addr_exp = "$" + ret.name;
		if (get_address == "get_address" && arg != "not_arg") {
			text_section.push_back( Asse("", "	movl", ret.addr_exp, "%eax") );
			ret.addr_exp = "%eax";
		}
		return ret;
	}

	if (tmp[tmp.size()-1] == ']' || array == "array")
	{
		string str;
		str = ""; for (i=0;i<tmp.length() && tmp[i] != '[';i++) str += tmp[i];
		for (j=ident_stack.size()-1;j>=0;j--) if (ident_stack[j].name == str) break;

		Ident addr;
		string addr_s;

		if (arg == "arg1" || arg == "arg2" || arg == "result") text_section.push_back( Asse("", "	movl", "$0", "%ebx") );

		int index=0;
		for (; i<tmp.length();i++)
		{
			if (tmp[i] == '[') {
				str = "";
			} else if (tmp[i] == ']') {
				addr = get_address_or_value_addrexp(str, "not_arg", "get_value");
				addr_s = addr.addr_exp;

				if (arg == "arg1" || arg == "arg2" || arg == "result") {
					text_section.push_back( Asse("", "	movl", addr_s, "%ecx") );
if (ident_stack[j].index[index] != 1)
					text_section.push_back( Asse("", "	imull", "$"+iTs(ident_stack[j].index[index]), "%ecx") );
					text_section.push_back( Asse("", "	addl", "%ecx", "%ebx") );
				}
				index++;
				//id += addr.ival*ident_stack[j].index[ii]; ii++;
			} else {
				str += tmp[i];
			}

			if (index >= ident_stack[j].index.size()) break;
		}

		ret = ident_stack[j];

		if (index < ident_stack[j].index.size()) {
			ret.pointer = "pointer";
			get_address = "get_address";
		} else if (index > ident_stack[j].index.size()) {
			cout<<"array error at :"<<tmp<<endl;
		}

		if (arg == "arg1")
		{
			text_section.push_back( Asse("", "	movl", "%ebx", "%eax") );
if (ret.step != 1)
			text_section.push_back( Asse("", "	imull", "$"+iTs(ret.step), "%eax") );
			text_section.push_back( Asse("", "	movl", ret.array_address, "%ecx") );
			text_section.push_back( Asse("", "	addl", "%ecx", "%eax") );

			ret.addr_exp = "(%eax)";
			if (get_address == "get_address" && arg != "not_arg")
				ret.addr_exp = "%eax";
		}
		if (arg == "arg2") {
			text_section.push_back( Asse("", "	movl", "%ebx", "%edx") );
if (ret.step != 1)
			text_section.push_back( Asse("", "	imull", "$"+iTs(ret.step), "%edx") );
			text_section.push_back( Asse("", "	movl", ret.array_address, "%ecx") );
			text_section.push_back( Asse("", "	addl", "%ecx", "%edx") );

			ret.addr_exp = "(%edx)";
			if (get_address == "get_address" && arg != "not_arg")
				ret.addr_exp = "%edx";
		}
		if (arg == "result") {
if (ret.step != 1)
			text_section.push_back( Asse("", "	imull", "$"+iTs(ret.step), "%ebx") );
			text_section.push_back( Asse("", "	movl", ret.array_address, "%ecx") );
			text_section.push_back( Asse("", "	addl", "%ecx", "%ebx") );

			ret.addr_exp = "(%ebx)";
			if (get_address == "get_address" && arg != "not_arg")
				ret.addr_exp = "%ebx";
		}
		return ret;
	}
	return ret;
}

static void struct_equal_struct(string op, string arg1, string arg2, Ident ar1, Ident ar2, Ident res, string ar1_s, string ar2_s, string res_s)
{
	int i,k;
	string a1_s,re_s;

	for (k=0;k<type_name.size();k++) if (type_name[k].name == res.type) break;

	if (ar1.name == "returnval")
	{
		for (i=0;i+4<=type_name[k].size;i+=4)
		{
			if (res.global_variable == "global_variable") {
				re_s = res.name+"+"+iTs(i);
			} else {
				re_s = tmp_stack+iTs(res.addr+i)+"_"+"(%esp)";
			}

			text_section.push_back( Asse("", "	movl", "%eax", "%edx") );
if (i != 0)	text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%edx") );
			text_section.push_back( Asse("", "	movl", "(%edx)", "%edx") );
			text_section.push_back( Asse("", "	movl", "%edx", re_s) );
		}
		for (;i<type_name[k].size;i++)
		{
			if (res.global_variable == "global_variable") {
				re_s = res.name+"+"+iTs(i);
			} else {
				re_s = tmp_stack+iTs(res.addr+i)+"_"+"(%esp)";
			}
			text_section.push_back( Asse("", "	movl", "%eax", "%edx") );
if (i != 0)	text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%edx") );
			text_section.push_back( Asse("", "	movb", "(%edx)", "%dl") );
			text_section.push_back( Asse("", "	movb", "%dl", re_s) );
		}
		return;
	}

	if (ar1.pointer == "address")
	{
		string a1_s,re_s;
		for (i=0;i+4<=type_name[k].size;i+=4)
		{
			if (ar1.global_variable == "global_variable") {
				a1_s = ar1.name;
			} else {
				a1_s = tmp_stack+iTs(ar1.addr)+"_"+"(%esp)";
			}

			if (res.global_variable == "global_variable") {
				re_s = res.name+"+"+iTs(i);
			} else {
				re_s = tmp_stack+iTs(res.addr+i)+"_"+"(%esp)";
			}

			text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
			text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%eax") );
			text_section.push_back( Asse("", "	movl", "(%eax)", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", re_s) );
		}
		for (;i<type_name[k].size;i++)
		{
			if (ar1.global_variable == "global_variable") {
				a1_s = ar1.name;
			} else {
				a1_s = tmp_stack+iTs(ar1.addr)+"_"+"(%esp)";
			}

			if (res.global_variable == "global_variable") {
				re_s = res.name+"+"+iTs(i);
			} else {
				re_s = tmp_stack+iTs(res.addr+i)+"_"+"(%esp)";
			}
			text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
			text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%eax") );
			text_section.push_back( Asse("", "	movb", "(%eax)", "%al") );
			text_section.push_back( Asse("", "	movb", "%al", re_s) );
		}

		return;
	}

	for (i=0;i+4<=type_name[k].size;i+=4)
	{
		if (ar1.global_variable == "global_variable") {
			a1_s = ar1.name+"+"+iTs(i);
		} else {
			a1_s = tmp_stack+iTs(ar1.addr+i)+"_"+"(%esp)";
		}

		if (res.global_variable == "global_variable") {
			re_s = res.name+"+"+iTs(i);
		} else {
			re_s = tmp_stack+iTs(res.addr+i)+"_"+"(%esp)";
		}
		text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", re_s) );
	}
	for (;i<type_name[k].size;i++)
	{
		if (ar1.global_variable == "global_variable") {
			a1_s = ar1.name+"+"+iTs(i);
		} else {
			a1_s = tmp_stack+iTs(ar1.addr+i)+"_"+"(%esp)";
		}

		if (res.global_variable == "global_variable") {
			re_s = res.name+"+"+iTs(i);
		} else {
			re_s = tmp_stack+iTs(res.addr+i)+"_"+"(%esp)";
		}
		text_section.push_back( Asse("", "	movb", a1_s, "%al") );
		text_section.push_back( Asse("", "	movb", "%al", re_s) );
	}
}

static void pointer_exp(string op, string arg1, string result)
{
	Ident ar1,res;
	string ar1_s,res_s;

	if (op == "&") {
		ar1 = get_address_or_value_addrexp(arg1, "arg1", "get_address");
		res = get_address_or_value_addrexp(result, "result", "get_value");

		ar1_s = ar1.addr_exp;
		res_s = res.addr_exp;

		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
	}
	if (op == "*") {
		ar1 = get_address_or_value_addrexp(arg1, "arg1", "get_value");
		res = get_address_or_value_addrexp(result, "result", "get_value");

		ar1_s = ar1.addr_exp;
		res_s = res.addr_exp;

if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
	}
}

static void pointer_equal_xxx(string op, string arg1, string arg2, Ident ar1, Ident ar2, Ident res, string ar1_s, string ar2_s, string res_s)
{
	if (ar1.pointer == "pointer") {
if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		return;
	}
	int k;
	if (type_name_appear.find(res.type) != type_name_appear.end()) {
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		return;
	}
	if (ar1.type == "char") {
		text_section.push_back( Asse("", "	movb", ar1_s, "%al") );
		text_section.push_back( Asse("", "	movsbl", "%al", "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		return;
	}
	if (ar1.type == "int" || ar1.type == "int_num" || ar1.type == "string_num") {
		int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		return;
	}
	if (ar1.type == "double" || ar1.type == "double_num") {
		text_section.push_back( Asse("", "	fldl", ar1_s, "") );
		text_section.push_back( Asse("", "	fnstcw", "-14(%esp)", "") );
		text_section.push_back( Asse("", "	movzwl", "-14(%esp)", "%eax") );
		text_section.push_back( Asse("", "	movb", "$12", "%ah") );
		text_section.push_back( Asse("", "	movw", "%ax", "-16(%esp)") );
		text_section.push_back( Asse("", "	fldcw", "-16(%esp)", "") );
		text_section.push_back( Asse("", "	fistpl", res_s, "") );
		text_section.push_back( Asse("", "	fldcw", "-14(%esp)", "") );
	}
}

static void address_equal_xxx(string op, string arg1, string arg2, Ident ar1, Ident ar2, Ident res, string ar1_s, string ar2_s, string res_s)
{
	int i,k;
	string a1_s,re_s;

	if (op == "=" && ar1.pointer == "pointer") {
if (ar1_s != "%eax")
		text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
		return;
	}
	if (op == "=" && ar1.pointer == "address") {
		if (res.type == "char") {
			char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	movl", res_s, "%edx") );
			text_section.push_back( Asse("", "	movb", "%al", "(%edx)") );
		}
		if (res.type == "int" || res.type == "int_num") {
			int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	movl", res_s, "%edx") );
			text_section.push_back( Asse("", "	movl", "%eax", "(%edx)") );
		}
		if (res.type == "double" || res.type == "double_num") {
			double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			text_section.push_back( Asse("", "	movl", res_s, "%edx") );
			text_section.push_back( Asse("", "	fstpl", "(%edx)", "") );
		}
		if (type_name_appear.find(res.type) != type_name_appear.end()) {
			for (k=0;k<type_name.size();k++) if (type_name[k].name == res.type) break;
			for (i=0;i+4<=type_name[k].size;i+=4)
			{
				if (ar1.global_variable == "global_variable") {
					a1_s = ar1.name;
				} else {
					a1_s = tmp_stack+iTs(ar1.addr)+"_"+"(%esp)";
				}

				if (res.global_variable == "global_variable") {
					re_s = res.name;
				} else {
					re_s = tmp_stack+iTs(res.addr)+"_"+"(%esp)";
				}
				text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
if (i != 0)  	text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%eax") );
				text_section.push_back( Asse("", "	movl", "(%eax)", "%eax") );
				text_section.push_back( Asse("", "	movl", res_s, "%edx") );
if (i != 0)  	text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%edx") );
				text_section.push_back( Asse("", "	movl", "%eax", "(%edx)") );
			}
			for (;i<type_name[k].size;i++)
			{
				if (ar1.global_variable == "global_variable") {
					a1_s = ar1.name;
				} else {
					a1_s = tmp_stack+iTs(ar1.addr)+"_"+"(%esp)";
				}

				if (res.global_variable == "global_variable") {
					re_s = res.name;
				} else {
					re_s = tmp_stack+iTs(res.addr)+"_"+"(%esp)";
				}
				text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
if (i != 0)		text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%eax") );
				text_section.push_back( Asse("", "	movb", "(%eax)", "%al") );
				text_section.push_back( Asse("", "	movl", res_s, "%edx") );
if (i != 0)		text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%edx") );
				text_section.push_back( Asse("", "	movb", "%al", "(%edx)") );
			}
		}

		return;
	}
	if (res.type == "char") {
		char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
		text_section.push_back( Asse("", "	movl", res_s, "%edx") );
		text_section.push_back( Asse("", "	movb", "%al", "(%edx)") );
		return;
	}
	if (ar1_s == "<int_num>0" || res.type == "int" || res.type == "int_num") {
		int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
		text_section.push_back( Asse("", "	movl", res_s, "%edx") );
		text_section.push_back( Asse("", "	movl", "%eax", "(%edx)") );
		return;
	}
	if (res.type == "double" || res.type == "double_num") {
		double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
		text_section.push_back( Asse("", "	movl", res_s, "%edx") );
		text_section.push_back( Asse("", "	fstpl", "(%edx)", "") );
		return;
	}
	if (type_name_appear.find(res.type) != type_name_appear.end()) {
		for (k=0;k<type_name.size();k++) if (type_name[k].name == res.type) break;
		for (i=0;i+4<=type_name[k].size;i+=4)
		{
			if (ar1.global_variable == "global_variable") {
				a1_s = ar1.name+"+"+iTs(i);
			} else {
				a1_s = tmp_stack+iTs(ar1.addr+i)+"_"+"(%esp)";
			}

			if (res.global_variable == "global_variable") {
				re_s = res.name;
			} else {
				re_s = tmp_stack+iTs(res.addr)+"_"+"(%esp)";
			}
			text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", res_s, "%edx") );
			text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%edx") );
			text_section.push_back( Asse("", "	movl", "%eax", "(%edx)") );
		}
		for (;i<type_name[k].size;i++)
		{
			if (ar1.global_variable == "global_variable") {
				a1_s = ar1.name+"+"+iTs(i);
			} else {
				a1_s = tmp_stack+iTs(ar1.addr+i)+"_"+"(%esp)";
			}

			if (res.global_variable == "global_variable") {
				re_s = res.name;
			} else {
				re_s = tmp_stack+iTs(res.addr)+"_"+"(%esp)";
			}
			text_section.push_back( Asse("", "	movb", a1_s, "%al") );
			text_section.push_back( Asse("", "	movl", res_s, "%edx") );
			text_section.push_back( Asse("", "	addl", "$"+iTs(i), "%edx") );
			text_section.push_back( Asse("", "	movb", "%al", "(%edx)") );
		}
	}
}

static void add_pointer_exp(string op, string arg1, string arg2, Ident ar1, Ident ar2, Ident res, string ar1_s, string ar2_s, string res_s)
{
	text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
	if (ar1.pointer == "") {
		if (res.type == "int")	text_section.push_back( Asse("", "	imull", "$4", "%eax") );
		if (res.type == "double")text_section.push_back( Asse("", "	imull", "$8", "%eax") );
		if (type_name_appear.find(res.type) != type_name_appear.end()) {
			int k;
			for (k=0;k<type_name.size();k++) if (type_name[k].name == res.type) break;
			text_section.push_back( Asse("", "	imull", "$"+iTs(type_name[k].size), "%eax") );
		}
	}

	text_section.push_back( Asse("", "	movl", ar2_s, "%edx") );
	if (ar2.pointer == "") {
		if (res.type == "int")	text_section.push_back( Asse("", "	imull", "$4", "%edx") );
		if (res.type == "double")text_section.push_back( Asse("", "	imull", "$8", "%edx") );
		if (type_name_appear.find(res.type) != type_name_appear.end()) {
			int k;
			for (k=0;k<type_name.size();k++) if (type_name[k].name == res.type) break;
			text_section.push_back( Asse("", "	imull", "$"+iTs(type_name[k].size), "%edx") );
		}
	}

	if (op == "+") {
		text_section.push_back( Asse("", "	addl", "%eax", "%edx") );
		text_section.push_back( Asse("", "	movl", "%edx", res_s) );
	}
	if (op == "-") {
		text_section.push_back( Asse("", "	subl", "%edx", "%eax") );
		text_section.push_back( Asse("", "	movl", "%eax", res_s) );
	}
}

static void declare(string arg1, string arg2, string result, string global_variable, string where)
{
	int j,len;
	string str;
	Ident addr;

	int arr_flag = 0;
	for (j=0;j<arg2.length();j++) {
		if (arg2[j] == '[') {
			arr_flag = 1;
			break;
		}
	}

	if (arr_flag == 1)
	{
		str = ""; for (j=0;arg2[j] != '[';j++) str += arg2[j];
		ident_stack.push_back( Ident(str) );
		ident_stack[ident_stack.size()-1].index.clear();
		ident_stack[ident_stack.size()-1].array	= "array";
		ident_stack[ident_stack.size()-1].type	= arg1;
		ident_stack[ident_stack.size()-1].global_variable = global_variable;
		if (result == "pointer") {
			ident_stack[ident_stack.size()-1].pointer = "pointer";
		} else {
			ident_stack[ident_stack.size()-1].pointer = "";
		}

		for (;j<arg2.length();j++)
		{
			if (arg2[j] == '[') {
				str = "";
			} else if (arg2[j] == ']') {
				addr = get_address_or_value_addrexp(str, "not_arg", "get_value");
				if (str == "") addr.addr = 100;
				ident_stack[ident_stack.size()-1].index.push_back(addr.addr);
			} else {
				str += arg2[j];
			}
		}
		len = 1;
		for (j=ident_stack[ident_stack.size()-1].index.size()-1;j>=0;j--) {
			len = len*ident_stack[ident_stack.size()-1].index[j];
			ident_stack[ident_stack.size()-1].index[j] = len/ident_stack[ident_stack.size()-1].index[j];
		}

		if (result == "pointer")
		{
			if (global_variable == "local_variable") {
				ident_stack[ident_stack.size()-1].addr = stacktop;
				ident_stack[ident_stack.size()-1].step = 4;
				stacktop += len*4;
			}
			if (global_variable == "global_variable") {
				ident_stack[ident_stack.size()-1].addr = len*4;
				ident_stack[ident_stack.size()-1].step = 4;
			}
		}
		else
		{
			if (global_variable == "local_variable")
			{
				ident_stack[ident_stack.size()-1].addr = stacktop;
				if (arg1 == "int") {
					ident_stack[ident_stack.size()-1].step = 4;
					stacktop += len*4;
				}
				if (arg1 == "double") {
					ident_stack[ident_stack.size()-1].step = 8;
					stacktop += len*8;
				}
				if (arg1 == "char") {
					ident_stack[ident_stack.size()-1].step = 1;
					stacktop += len*1;
				}
				if (type_name_appear.find(arg1) != type_name_appear.end()) {
					int k;
					for (k=0;k<type_name.size();k++) if (type_name[k].name == arg1) break;

					ident_stack[ident_stack.size()-1].step = type_name[k].size;
					stacktop += len*type_name[k].size;
				}
			}
			if (global_variable == "global_variable")
			{
				if (arg1 == "int") {
					ident_stack[ident_stack.size()-1].addr = len*4;
					ident_stack[ident_stack.size()-1].step = 4;
				}
				if (arg1 == "double") {
					ident_stack[ident_stack.size()-1].addr = len*8;
					ident_stack[ident_stack.size()-1].step = 8;
				}
				if (arg1 == "char") {
					ident_stack[ident_stack.size()-1].addr = len*1;
					ident_stack[ident_stack.size()-1].step = 1;
				}
				if (type_name_appear.find(arg1) != type_name_appear.end()) {
					int k;
					for (k=0;k<type_name.size();k++) if (type_name[k].name == arg1) break;

					ident_stack[ident_stack.size()-1].addr = len*type_name[k].size;
					ident_stack[ident_stack.size()-1].step = type_name[k].size;
				}
			}
		}
		if (where == "<function>")
		{
			if (global_variable == "global_variable") text_section.push_back( Asse("", "	leal", ident_stack[ident_stack.size()-1].name, "%ecx") );
			if (global_variable == "local_variable")	 text_section.push_back( Asse("", "	leal", tmp_stack+iTs(ident_stack[ident_stack.size()-1].addr)+"_"+"(%esp)", "%ecx") );

			ident_stack[ident_stack.size()-1].array_address = "array_address_"+iTs(array_number++);
			data_section.push_back( Asse("", ident_stack[ident_stack.size()-1].array_address, ".int", "0") );
			text_section.push_back( Asse("", "	movl", "%ecx", ident_stack[ident_stack.size()-1].array_address) );
		}
		if (where == "<function_par>")
		{
			ident_stack[ident_stack.size()-1].array_address = "array_address_"+iTs(array_number++);
			data_section.push_back( Asse("", ident_stack[ident_stack.size()-1].array_address, ".int", "0") );
		}
		if (where == "<out_struct_function>")
		{
			if (global_variable == "global_variable") get_global_array_address.push_back( Asse("", "	leal", ident_stack[ident_stack.size()-1].name, "%ecx") );
			if (global_variable == "local_variable")	 get_global_array_address.push_back( Asse("", "	leal", tmp_stack+iTs(ident_stack[ident_stack.size()-1].addr)+"_"+"(%esp)", "%ecx") );

			ident_stack[ident_stack.size()-1].array_address = "array_address_"+iTs(array_number++);
			data_section.push_back( Asse("", ident_stack[ident_stack.size()-1].array_address, ".int", "0") );
			get_global_array_address.push_back( Asse("", "	movl", "%ecx", ident_stack[ident_stack.size()-1].array_address) );
		}
		return;
	}

	if (arr_flag == 0)
	{
		ident_stack.push_back( Ident(arg2) );
		ident_stack[ident_stack.size()-1].type	= arg1;
		ident_stack[ident_stack.size()-1].step	= 1;
		ident_stack[ident_stack.size()-1].array	= "ident";
		ident_stack[ident_stack.size()-1].global_variable = global_variable;
		if (result == "pointer") {
			ident_stack[ident_stack.size()-1].pointer = "pointer";
		} else {
			ident_stack[ident_stack.size()-1].pointer = "";
		}

		if (result == "pointer")
		{
			if (global_variable == "local_variable") {
				ident_stack[ident_stack.size()-1].addr = stacktop;
				stacktop += 4;
			}
			if (global_variable == "global_variable") {
				ident_stack[ident_stack.size()-1].addr = 4;
			}
		}
		else
		{
			if (global_variable == "local_variable")
			{
				ident_stack[ident_stack.size()-1].addr = stacktop;
				if (arg1 == "int")	stacktop += 4;
				if (arg1 == "double")stacktop += 8;
				if (arg1 == "char")	stacktop += 1;
				if (type_name_appear.find(arg1) != type_name_appear.end()) {
					int k;
					for (k=0;k<type_name.size();k++)
						if (type_name[k].name == arg1) break;
					stacktop += type_name[k].size;
				}
			}
			if (global_variable == "global_variable")
			{
				if (arg1 == "int")	ident_stack[ident_stack.size()-1].addr = 4;
				if (arg1 == "double")ident_stack[ident_stack.size()-1].addr = 8;
				if (arg1 == "char")	ident_stack[ident_stack.size()-1].addr = 1;
				if (type_name_appear.find(arg1) != type_name_appear.end()) {
					int k;
					for (k=0;k<type_name.size();k++)
						if (type_name[k].name == arg1) break;
					ident_stack[ident_stack.size()-1].addr = type_name[k].size;
				}
			}
		}
	}
}

static void add_to_tmp_iden(string op, string arg1, string arg2, string tmp)
{
	if (tmp.substr(0, tmp_name.size()) == tmp_name)
	{
		if (tmp_ident_appear.find(tmp) != tmp_ident_appear.end()) return;
		tmp_ident_appear[tmp] = 1;

		int val;
		string type;
		Ident re1,re2;

		val = sTi(tmp.substr(tmp_name.length()));
		re1 = get_address_or_value_addrexp(arg1, "not_arg", "get_value");
		re2 = get_address_or_value_addrexp(arg2, "not_arg", "get_value");

		if (re1.type == "int_num") 	re1.type = "int";
		if (re1.type == "double_num")re1.type = "double";
		if (re1.type == "string_num")re1.type = "string";

		if (re2.type == "int_num") 	re2.type = "int";
		if (re2.type == "double_num")re2.type = "double";
		if (re2.type == "string_num")re2.type = "string";

		type = mark[re1.type] > mark[re2.type] ? re1.type : re2.type;

		if (re1.pointer == "pointer") type = re1.type;
		if (re2.pointer == "pointer") type = re2.type;

		if (op == ">=" || op == ">" || op == "<=" || op == "<" || op == "==" || op == "!=") type = "int";
		if (op == "&" && arg2 == "null") type = "int";
		if (op == "*" && arg2 == "null") type = re1.type;
		if (arg1 == "returnval") type = type_of_call_function.type;
		if (op == "sizeof") 		type = "int";

		ident_stack.push_back( Ident(tmp) );
		ident_stack[ident_stack.size()-1].type	= type;
		ident_stack[ident_stack.size()-1].addr	= stacktop;
		ident_stack[ident_stack.size()-1].global_variable	= "local_variable";

		if (op == "&" && arg2 == "null") {
			ident_stack[ident_stack.size()-1].pointer = "pointer";
		} else if (op == "*" && arg2 == "null") {
			ident_stack[ident_stack.size()-1].pointer = "address";
		} else if (re1.pointer == "pointer" || re2.pointer == "pointer") {
			ident_stack[ident_stack.size()-1].pointer = "pointer";
		} else if (op == "=" && re1.pointer == "address") {
			ident_stack[ident_stack.size()-1].pointer = "address";
		} else if (arg1 == "returnval" && type_of_call_function.pointer == 1) {
			ident_stack[ident_stack.size()-1].pointer = "pointer";
		} else {
			ident_stack[ident_stack.size()-1].pointer = "";
		}

		if (type == "int" || ident_stack[ident_stack.size()-1].pointer == "pointer" || ident_stack[ident_stack.size()-1].pointer == "address") {
			stacktop += 4;
		} else if (type == "double") {
			stacktop += 8;
		} else if (type == "char") {
			stacktop += 1;
		}
	}
}

static void call_function(int now)
{
	int i, top = 0;
	Ident ar1;
	string ar1_s, arg1;
	vector<string> list;

	for (i=0;i<function_block.size();i++)
		if (semantic_exp[now].arg1 == function_block[i].name) {
			type_of_call_function = function_block[i].type;
			list = function_block[i].list;
		}

	// TODO (int*)p;
	if (type_of_call_function.pointer == 1) {
		type_of_call_function.type = "int";
	}

	#ifdef __linux_system__
		if (semantic_exp[now].arg1 == "sin" || semantic_exp[now].arg1 == "cos" || semantic_exp[now].arg1 == "sqrt")
		{
			arg1  = semantic_exp[now].parameter[0];
			ar1	  = get_address_or_value_addrexp(arg1, "arg1", "get_value");
			ar1_s = ar1.addr_exp;

			double_fld_12(arg1, arg1, ar1, ar1, ar1_s, ar1_s, 1);

			#ifdef __linux_system__
				text_section.push_back( Asse("", "	f"+semantic_exp[now].arg1, "", "") );
			#endif
			#ifdef __windows_system__
				text_section.push_back( Asse("", "	call", "_f"+semantic_exp[now].arg1, "") );
			#endif

			return;
		}
	#endif

	for (i=0;i<semantic_exp[now].parameter.size();i++)
	{
		arg1	= semantic_exp[now].parameter[i];

		if (arg1 == "stdin" || arg1 == "stdout")
		{
			#ifdef __linux_system__
				text_section.push_back( Asse("", "	movl", arg1, "%eax") );
				text_section.push_back( Asse("", "	movl", "%eax", iTs(top)+"(%esp)") );
				top += 4;
			#endif
			#ifdef __windows_system__
				text_section.push_back( Asse("", "	movl", "__imp___iob", "%eax") );
				if (arg1 == "stdout")
					text_section.push_back( Asse("", "	addl", "$32", "%eax") );
				text_section.push_back( Asse("", "	movl", "%eax", iTs(top)+"(%esp)") );
				top += 4;
			#endif

			continue;
		}

		ar1 	= get_address_or_value_addrexp(arg1, "arg1", "get_value");
		ar1_s	= ar1.addr_exp;

		if (ar1.type == "FILE") {
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", iTs(top)+"(%esp)") );
			top += 4;
		}
		if (ar1.type == "string_num") {
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", iTs(top)+"(%esp)") );
			top += 4;
		}
		if (ar1.type == "char" || ar1.type == "int" || ar1.type == "int_num")
		{
			int_fld(arg1, arg1, ar1, ar1, ar1_s, ar1_s, 1);

			if (list.size() > i && list[i] == "double") {
				text_section.push_back( Asse("", "	movl", "%eax", "-24(%esp)") );
				text_section.push_back( Asse("", "	fildl", "-24(%esp)", "") );
				text_section.push_back( Asse("", "	fstpl", iTs(top)+"(%esp)", "") );
				top += 8;
			} else {
				text_section.push_back( Asse("", "	movl", "%eax", iTs(top)+"(%esp)") );
				top += 4;
			}
		}

		if (ar1.type == "double" || ar1.type == "double_num")
		{
			if (ar1.pointer == "pointer") {
if (ar1_s != "%eax")
				text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
				text_section.push_back( Asse("", "	movl", "%eax", iTs(top)+"(%esp)") );
				top += 4;
			} else if (ar1.pointer == "address") {
if (ar1_s != "%eax")
				text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
				text_section.push_back( Asse("", "	fldl", "(%eax)", "") );
				text_section.push_back( Asse("", "	fstpl", iTs(top)+"(%esp)", "") );
				top += 8;
			} else {
				text_section.push_back( Asse("", "	fldl",  ar1_s, "") );
				text_section.push_back( Asse("", "	fstpl", iTs(top)+"(%esp)", "") );
				top += 8;
			}
		}
		if (type_name_appear.find(ar1.type) != type_name_appear.end())
		{
			if (ar1.pointer == "pointer") {
if (ar1_s != "%eax")
				text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
				text_section.push_back( Asse("", "	movl", "%eax", iTs(top)+"(%esp)") );
				top += 4;
			} else if (ar1.pointer == "address") {
				int ii,kk;
				string a1_s,re_s;
				for (kk=0;kk<type_name.size();kk++) if (type_name[kk].name == ar1.type) break;
				for (ii=0;ii+4<=type_name[kk].size;ii+=4)
				{
					if (ar1.global_variable == "global_variable") {
						a1_s = ar1.name;
					} else {
						a1_s = tmp_stack+iTs(ar1.addr)+"_"+"(%esp)";
					}

					re_s = iTs(top+ii)+"(%esp)";

					text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
					text_section.push_back( Asse("", "	addl", "$"+iTs(ii), "%eax") );
					text_section.push_back( Asse("", "	movl", "(%eax)", "%eax") );
					text_section.push_back( Asse("", "	movl", "%eax", re_s) );
				}
				for (;ii<type_name[kk].size;ii++)
				{
					if (ar1.global_variable == "global_variable") {
						a1_s = ar1.name;
					} else {
						a1_s = tmp_stack+iTs(ar1.addr)+"_"+"(%esp)";
					}

					re_s = iTs(top+ii)+"(%esp)";

					text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
					text_section.push_back( Asse("", "	addl", "$"+iTs(ii), "%eax") );
					text_section.push_back( Asse("", "	movb", "(%eax)", "%al") );
					text_section.push_back( Asse("", "	movb", "%al", re_s) );
				}
				top += type_name[kk].size;
			}
			else
			{
				int ii,kk;
				string a1_s,re_s;
				for (kk=0;kk<type_name.size();kk++) if (type_name[kk].name == ar1.type) break;
				for (ii=0;ii+4<=type_name[kk].size;ii+=4)
				{
					if (ar1.global_variable == "global_variable") {
						a1_s = ar1.name+"+"+iTs(ii);
					} else {
						a1_s = tmp_stack+iTs(ar1.addr+ii)+"_"+"(%esp)";
					}

					re_s = iTs(top+ii)+"(%esp)";

					text_section.push_back( Asse("", "	movl", a1_s, "%eax") );
					text_section.push_back( Asse("", "	movl", "%eax", re_s) );
				}
				for (;ii<type_name[kk].size;ii++)
				{
					if (ar1.global_variable == "global_variable") {
						a1_s = ar1.name+"+"+iTs(ii);
					} else {
						a1_s = tmp_stack+iTs(ar1.addr+ii)+"_"+"(%esp)";
					}

					re_s = iTs(top+ii)+"(%esp)";

					text_section.push_back( Asse("", "	movb", a1_s, "%al") );
					text_section.push_back( Asse("", "	movb", "%al", re_s) );
				}
				top += type_name[kk].size;
			}
		}
	}
	if (top > max_function_stacktop) max_function_stacktop = top;

	#ifdef __linux_system__
		text_section.push_back( Asse("", "	call", semantic_exp[now].arg1, "") );
	#endif
	#ifdef __windows_system__
		text_section.push_back( Asse("", "	call", "_"+semantic_exp[now].arg1, "") );
	#endif
}

static void function(TYPE ftype, string name, int start, int mid, int end)
{
	int		i,now,stackbegin;
	string	label,op,arg1,arg2,result,ar1_s,ar2_s,res_s;
	Ident	ar1,ar2,res;
	int		ebp_top;// 函数传递参数时将变量从%ebp复制到%esp, ebp_top指向参数原始位置

	#ifdef __linux_system__
		text_section.push_back( Asse("", ".section .text", "", "") );
		text_section.push_back( Asse("", ".globl "+name, "", "") );
		text_section.push_back( Asse("", "	.type	"+name+", @function", "", "") );
		text_section.push_back( Asse("", name+":", "", "") );

		text_section.push_back( Asse("", "	.cfi_startproc", "", "") );
		text_section.push_back( Asse("", "	pushl %ebp", "", "") );
		text_section.push_back( Asse("", "	movl %esp, %ebp", "", "") );
		text_section.push_back( Asse("", "	subl", "max_top", "%esp") );
	#endif

	#ifdef __windows_system__
		name = "_"+name;

		text_section.push_back( Asse("", ".section .text", "", "") );
		text_section.push_back( Asse("", ".globl "+name, "", "") );
		text_section.push_back( Asse("", "	.def	"+name+"; .scl 2; .type 32; .endef", "", "") );
		text_section.push_back( Asse("", name+":", "", "") );
	
		text_section.push_back( Asse("", "	pushl %ebp", "", "") );
		text_section.push_back( Asse("", "	movl %esp, %ebp", "", "") );
	
		if (name != "_main") {
			text_section.push_back( Asse("", "	subl", "max_top", "%esp") );
		} else {
			text_section.push_back( Asse("", "	movl", "max_top", "%eax") );
			text_section.push_back( Asse("", "	call", "__alloca", "") );
			text_section.push_back( Asse("", "	andl", "$-16", "%esp") );
			text_section.push_back( Asse("", "	movl", "$0", "%eax") );
			text_section.push_back( Asse("", "	addl", "$15", "%eax") );
			text_section.push_back( Asse("", "	addl", "$15", "%eax") );
			text_section.push_back( Asse("", "	shrl", "$4", "%eax") );
			text_section.push_back( Asse("", "	shrl", "$4", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "max_top(%esp)") );
			text_section.push_back( Asse("", "	movl", "max_top(%esp)", "%eax") );
			text_section.push_back( Asse("", "	call", "__alloca", "") );
		}
	#endif

    if (name == "main" || name == "_main")
    {
		for (i=0;i<get_global_array_address.size();i++)
    		text_section.push_back( get_global_array_address[i] );
    	get_global_array_address.clear();
    }

	stackbegin	 = stacktop = 0;
	max_stacktop = stacktop;
	max_function_stacktop = 0;
	ebp_top = stacktop;

	for (now=start;now<end;now++)
	if (now != mid-1)
	{
		op = semantic_exp[now].op; arg1 = semantic_exp[now].arg1; arg2 = semantic_exp[now].arg2; result = semantic_exp[now].result;

		text_section.push_back( Asse(semantic_exp[now].label+":", "", "", "") );

		if (op == "return")
		{
			text_section.push_back( Asse("", "	leave", "", "") );
			text_section.push_back( Asse("", "	ret", "", "") );
			continue;
		}

		if (op == "call_function")
		{
			call_function(now);
			continue;
		}

		if (op == "declare")
		{
			int pre = stacktop;
			if (now < mid-1) {
				declare(arg1, arg2, result, "local_variable", "<function_par>");
			} else {
				declare(arg1, arg2, result, "local_variable", "<function>");
			}

			if (now < mid-1)
			{
				if (ident_stack[ident_stack.size()-1].array == "array") {
					text_section.push_back( Asse("", "	movl", iTs(ebp_top-stackbegin+8)+"(%ebp)", "%ebx") );
					stacktop = pre;
					text_section.push_back( Asse("", "	movl", "%ebx", ident_stack[ident_stack.size()-1].array_address) );
					ebp_top += 4;
				} else {
					if (stacktop-pre == 1) {
						text_section.push_back( Asse("", "	movl", iTs(ebp_top-stackbegin+8)+"(%ebp)", "%eax") );
						text_section.push_back( Asse("", "	movb", "%al", tmp_stack+iTs(pre)+"_"+"(%esp)") );
						stacktop += 3;
						ebp_top += 4;
					} else if (stacktop-pre == 2) {
						text_section.push_back( Asse("", "	movl", iTs(ebp_top-stackbegin+8)+"(%ebp)", "%eax") );
						text_section.push_back( Asse("", "	movw", "%ax", tmp_stack+iTs(pre)+"_"+"(%esp)") );
						stacktop += 2;
						ebp_top += 4;
					} else if (stacktop-pre == 4) {
						text_section.push_back( Asse("", "	movl", iTs(ebp_top-stackbegin+8)+"(%ebp)", "%eax") );
						text_section.push_back( Asse("", "	movl", "%eax", tmp_stack+iTs(pre)+"_"+"(%esp)") );
						ebp_top += 4;
					} else if (stacktop-pre == 8) {
						text_section.push_back( Asse("", "	fldl", iTs(ebp_top-stackbegin+8)+"(%ebp)", "") );
						text_section.push_back( Asse("", "	fstpl", tmp_stack+iTs(pre)+"_"+"(%esp)", "") );
						ebp_top += 8;
					} else {
						int ii;
						for (ii=0;ii+4<=stacktop-pre;ii+=4) {
							text_section.push_back( Asse("", "	movl", iTs(ebp_top-stackbegin+8+ii)+"(%ebp)", "%eax") );
							text_section.push_back( Asse("", "	movl", "%eax", tmp_stack+iTs(pre+ii)+"_"+"(%esp)") );
						}
						for (;ii<stacktop-pre;ii++) {
							text_section.push_back( Asse("", "	movb", iTs(ebp_top-stackbegin+8+ii)+"(%ebp)", "%al") );
							text_section.push_back( Asse("", "	movb", "%al", tmp_stack+iTs(pre+ii)+"_"+"(%esp)") );
						}
						ebp_top += stacktop-pre;
					}
				}
			}
			continue;
		}
		if (op == "undeclare")
		{
			if (ident_stack.size() == 0) continue;
			it=ident_stack.end();
			while (true)
			{
				it--;
				if (it->name == arg1) { ident_stack.erase(it); break; }
				if (it == ident_stack.begin()) break;
			}
			continue;
		}
		if (op == "goto")
		{
			text_section.push_back( Asse("", "	jmp",semantic_exp[now].arg1, "") );
			continue;
		}
		if (op == "if")
		{
			ar1   = get_address_or_value_addrexp(arg1, "arg1", "get_value");
			ar1_s = ar1.addr_exp;

			if (ar1.type == "int_num" || ar1.type == "int") {
				text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
				text_section.push_back( Asse("", "	cmpl", "$0", "%eax") );
				text_section.push_back( Asse("", "	jne", semantic_exp[now].result, "") );
			} else if (ar1.type == "char_num" || ar1.type == "char") {
				text_section.push_back( Asse("", "	movl", "$0", "%eax") );
				text_section.push_back( Asse("", "	movsbl", ar1_s, "%edx") );
				text_section.push_back( Asse("", "	cmpl", "%eax", "%edx") );
				text_section.push_back( Asse("", "	jne", semantic_exp[now].result, "") );
			}
			continue;
		}
		if (op == "ifn")
		{
			ar1   = get_address_or_value_addrexp(arg1, "arg1", "get_value");
			ar1_s = ar1.addr_exp;

			if (ar1.type == "int_num" || ar1.type == "int") {
				text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
				text_section.push_back( Asse("", "	cmpl", "$0", "%eax") );
				text_section.push_back( Asse("", "	je", semantic_exp[now].result, "") );
			} else if (ar1.type == "char_num" || ar1.type == "char") {
				text_section.push_back( Asse("", "	movl", "$0", "%eax") );
				text_section.push_back( Asse("", "	movsbl", ar1_s, "%edx") );
				text_section.push_back( Asse("", "	cmpl", "%eax", "%edx") );
				text_section.push_back( Asse("", "	je", semantic_exp[now].result, "") );
			}
			continue;
		}


		add_to_tmp_iden(op, arg1, arg2, result);

		if (op == "sizeof")
		{
			res   = get_address_or_value_addrexp(result, "result", "get_value");
			res_s = res.addr_exp;
			if (arg1[arg1.size()-1] == '*') {
				text_section.push_back( Asse("", "	movl", "$4", res_s) );
			} else {
				for (i=0;i<type_name.size();i++) if (type_name[i].name == arg1) break;
				if (i < type_name.size()) {
					text_section.push_back( Asse("", "	movl", "$"+iTs(type_name[i].size), res_s) );
				} else {
					if (arg1 == "char") text_section.push_back( Asse("", "	movl", "$1", res_s) );
					if (arg1 == "int") text_section.push_back( Asse("", "	movl", "$4", res_s) );
					if (arg1 == "double") text_section.push_back( Asse("", "	movl", "$8", res_s) );
				}
			}
		}

		if (result == "returnval")
		{
			ar1   = get_address_or_value_addrexp(arg1, "arg1", "get_value");
			ar1_s = ar1.addr_exp;

			type_of_call_function = ftype;

			if (type_of_call_function.type == "int" || type_of_call_function.type == "int_num") int_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			if (type_of_call_function.type == "char") char_fld(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			if (type_of_call_function.type == "double" || type_of_call_function.type == "double_num") double_fld_12(arg1, arg2, ar1, ar2, ar1_s, ar2_s, 1);
			if (type_name_appear.find(type_of_call_function.type) != type_name_appear.end()) {
				text_section.push_back( Asse("", "	leal", ar1_s, "%eax") );
			}
			continue;
		}

		if ((op == "&" || op == "*") && arg2 == "null")
		{
			pointer_exp(op, arg1, result);
			continue;
		}

		ar1 = get_address_or_value_addrexp(arg1,   "arg1", "get_value");
		ar2 = get_address_or_value_addrexp(arg2,   "arg2", "get_value");
		res = get_address_or_value_addrexp(result, "result", "get_value");

		ar1_s = ar1.addr_exp;
		ar2_s = ar2.addr_exp;
		res_s = res.addr_exp;

		if (op == "=" && arg2 == "null" && res.type == "FILE")
		{
			text_section.push_back( Asse("", "	movl", ar1_s, "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", res_s) );
			continue;
		}
		if (op == "=" && arg2 == "null" && res.pointer == "pointer")
		{
			pointer_equal_xxx(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
			continue;
		}
		if (op == "=" && arg2 == "null" && res.pointer == "address")
		{
			address_equal_xxx(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
			continue;
		}
		if ((op == "+" || op == "-") && res.pointer == "pointer")
		{
			add_pointer_exp(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
			continue;
		}
		if (op == "change_type")
		{
			op = "=";
			change_type(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
			continue;
		}

		if (op == "=" && type_name_appear.find(res.type) != type_name_appear.end() && res.type == ar1.type)
		{
			struct_equal_struct(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
			continue;
		}
		if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "=" || op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>" || op == "~" || op == "!")
		{
			add_exp(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
		}

		if (op == ">=" || op == ">" || op == "<=" || op == "<" || op == "==" || op == "!=" || op == "&&" || op == "||")
		{
			cmp_exp(op, arg1, arg2, ar1, ar2, res, ar1_s, ar2_s, res_s);
		}

		if (arg1 == "returnval")
		{
			text_section.push_back( Asse("", "	finit", "", "") );
		}
	}

	if (name == "_main")
	{
		#ifdef __windows_system__
			text_section.push_back( Asse("", "	subl", "$4", "%esp") );
			text_section.push_back( Asse("", "	movl", "__imp___iob", "%eax") );
			text_section.push_back( Asse("", "	addl", "$32", "%eax") );
			text_section.push_back( Asse("", "	movl", "%eax", "(%esp)") );
			text_section.push_back( Asse("", "	call", "_fflush", "") );

			text_section.push_back( Asse("", "	movl", "$0", "%eax") );
			text_section.push_back( Asse("", "	leave", "", "") );
			text_section.push_back( Asse("", "	ret", "","") );
			text_section.push_back( Asse("", "	.def	_printf;	.scl	2;	.type	32;	.endef", "","") );
			text_section.push_back( Asse("", "	.def	_scanf;	.scl	2;	.type	32;	.endef", "","") );
		#endif
	} else {
		text_section.push_back( Asse("", "	leave", "", "") );
		text_section.push_back( Asse("", "	ret", "", "") );
	}
	#ifdef __linux_system__
		text_section.push_back( Asse("", "	.cfi_endproc", "", "") );
	#endif

	if (max_stacktop < stacktop) max_stacktop = stacktop;
}

static void out_declare(int now, int num)
{
	declare(semantic_exp[now].arg1, semantic_exp[now].arg2, semantic_exp[now].result, "global_variable", "<out_struct_function>");
	global_ident_section.push_back( Ident(semantic_exp[now].arg2) );
	int  id  = global_ident_section.size()-1;
	global_ident_section[id] = ident_stack[ident_stack.size()-1];
	global_ident_section[id].init_val = "";

	if (num == 1) return;
    
	Ident  ar1   = get_address_or_value_addrexp(semantic_exp[now+1].arg1, "not_arg", "get_value");
	string ar1_s = ar1.addr_exp;

	global_ident_section[id].init_val = ar1_s.substr(1);
}

static void add_to_struct(string name, int start, int mid, int end)
{
	int now;
	string op,arg1,arg2,result;

	type_name.push_back( AType_name(name) );
	type_name[type_name.size()-1].size = 0;
	type_name_appear[name] = 1;

	for (now=start;now<end;now++)
	{
		op = semantic_exp[now].op; arg1 = semantic_exp[now].arg1; arg2 = semantic_exp[now].arg2; result = semantic_exp[now].result;

		if (op == "declare")
		{
			int pre = stacktop;
			declare(arg1, arg2, result, "global_variable", "<struct>");
			type_name[type_name.size()-1].node.push_back( ident_stack[ident_stack.size()-1] );
			type_name[type_name.size()-1].size += ident_stack[ident_stack.size()-1].addr;
		}
		if (op == "undeclare")
		{
			if (ident_stack.size() == 0) continue;
			it=ident_stack.end();
			while (true) {
				it--;
				if (it->name == arg1) { ident_stack.erase(it); break; }
				if (it == ident_stack.begin()) break;
			}
			continue;
		}
	}
}

static void solve()
{
	Input();

	int i,j,k,s_s;
	vector<Asse> :: iterator it;

	mark["char"]		= 2;
	mark["int_num"]		= 3;
	mark["int"]			= 4;
	mark["double_num"]	= 5;
	mark["double"]		= 6;
	mark["address"]		= 7;
	mark["pointer"]		= 8;

	global_ident_section.clear();
	data_section.clear();
	text_section.clear();
	tmp_ident_appear.clear();

	type_name.clear();
	type_name_appear.clear();

	get_global_array_address.clear();

	Label_id = 0;
	array_number = 0;

	data_section.push_back( Asse(".section .data", "", "", "") );

	for (i=0;i<function_block.size();i++)
	{
		if (function_block[i].type.type == "struct") {
			add_to_struct(function_block[i].name, mm[function_block[i].start], mm[function_block[i].mid], mm[function_block[i].end]);
		}
	}

	for (i=0;i<semantic_exp.size();i++)  semantic_exp[i].out_struct_function = 1;
	for (i=0;i<function_block.size();i++) for (j=mm[function_block[i].start];j<mm[function_block[i].end];j++) semantic_exp[j].out_struct_function = 0;
	for (i=0;i<semantic_exp.size();i++)  if (semantic_exp[i].out_struct_function == 1)
	{
		if (semantic_exp[i].op == "declare")
		{
			if (i+1<semantic_exp.size() && semantic_exp[i+1].out_struct_function == 1 && semantic_exp[i+1].op == "=") {
				out_declare(i, 2); // 表示有初始值，那么下一句的arg1就是初始值
			} else {
				out_declare(i, 1);
			}
		}
	}

	for (i=0;i<function_block.size();i++)
	{
		if (function_block[i].type.type == "struct") continue;
		if (function_block[i].start == "-1") continue; // int abc();

		s_s = text_section.size();
		function(function_block[i].type, function_block[i].name, mm[function_block[i].start], mm[function_block[i].mid], mm[function_block[i].end]);
		it = text_section.end()-1;
		while (true) {
			if (it->label == "") break;
			it--;
			text_section.erase(it+1);
		}

		for (j=s_s;j<text_section.size();j++)
		{
            if (text_section[j].arg1 == "max_top") {
				text_section[j].arg1 = "$"+iTs(max_function_stacktop + max_stacktop);
			}

			#ifdef __windows_system__
				if (text_section[j].arg1.substr(0, 7) == "max_top") { // max_top 表示一个函数需要的最多栈空间，在函数一开始的时候要用，但需要执行完“function“后才知道，所以在“function“里就用max_top表示。
					text_section[j].arg1 = iTs(max_function_stacktop + max_stacktop)+text_section[j].arg1.substr(7);
				}
				if (text_section[j].arg2.substr(0, 7) == "max_top") {
					text_section[j].arg2 = iTs(max_function_stacktop + max_stacktop)+text_section[j].arg2.substr(7);
				}
			#endif

			if (text_section[j].arg1.substr(0, 5) == tmp_stack)  // tmp_stack：函数内的变量的内存位置是未知的，因为可能栈的开始有一部分被用来做函数参数传递用。因此先用_tmp_表示。
			{
				string ss = "";
				for (k=5;text_section[j].arg1[k] != '_';k++)
					ss += text_section[j].arg1[k];
				text_section[j].arg1 = iTs(sTi(ss)+max_function_stacktop) + text_section[j].arg1.substr(k+1);
			}
			if (text_section[j].arg2.substr(0, 5) == tmp_stack)
			{
				string ss = "";
				for (k=5;text_section[j].arg2[k] != '_';k++)
					ss += text_section[j].arg2[k];
				text_section[j].arg2 = iTs(sTi(ss)+max_function_stacktop) + text_section[j].arg2.substr(k+1);
			}
		}
	}

	FILE *fp = fopen(assemblyname.c_str(), "w");

	for (i=0;i<global_ident_section.size();i++)
	{
		fprintf(fp, ".globl %s\n",global_ident_section[i].name.c_str());
		if (global_ident_section[i].init_val == "") fprintf(fp, "	.bss\n"); else 	fprintf(fp, "	.data\n");
		fprintf(fp, "	.align %d\n",global_ident_section[i].addr<32?4:32);
		#ifdef __linux_system__
			fprintf(fp, "	.type %s, @object\n",global_ident_section[i].name.c_str());
			fprintf(fp, "	.size %s, %d\n",global_ident_section[i].name.c_str(), global_ident_section[i].addr);
			fprintf(fp, "%s:\n",global_ident_section[i].name.c_str());
			if (global_ident_section[i].init_val == "") {
				fprintf(fp, "	.zero %d\n",global_ident_section[i].addr);
			} else {
				fprintf(fp, "	.%s %s\n",global_ident_section[i].type.c_str(), global_ident_section[i].init_val.c_str());
			}
		#endif
		#ifdef __windows_system__
			fprintf(fp, "%s:\n",global_ident_section[i].name.c_str());
			if (global_ident_section[i].init_val == "") {
				fprintf(fp, "	.space %d\n",global_ident_section[i].addr);
			} else {
				fprintf(fp, "	.%s %s\n",global_ident_section[i].type.c_str(), global_ident_section[i].init_val.c_str());
			}
		#endif
	}

	for (i=0;i<data_section.size();i++)
	{
		if (data_section[i].label != "") {
			fprintf(fp, "%s\n", data_section[i].label.c_str());
		} else {
			fprintf(fp, "	%s: %s %s\n", data_section[i].op.c_str(), data_section[i].arg1.c_str(), data_section[i].arg2.c_str());
		}
	}
	fprintf(fp, "\n");

	for (i=0;i<text_section.size();i++)
	{
		if (text_section[i].label != "") {
			fprintf(fp, "%s\n", text_section[i].label.c_str());
		} else {
			if (text_section[i].arg2 != "") {
				fprintf(fp, "%s %s, %s\n", text_section[i].op.c_str(), text_section[i].arg1.c_str(), text_section[i].arg2.c_str());
			} else {
				fprintf(fp, "%s %s\n", text_section[i].op.c_str(), text_section[i].arg1.c_str());
			}
		}
	}
	fclose(fp);
}

int assembly_32_main()
{
	tmp_name = "_T_";
	l_name = ".L_";
	tmp_stack = "_tmp_";

	solve();

	return 0;
}

