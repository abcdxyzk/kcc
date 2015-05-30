#include "kcc.h"

vector<Expression> semantic_exp;			// +  i  j  k  ==>  k = i+j;
vector<Function> function_block;			// int lowbit(int x) { return x&(-x); }

// 结构体内的变量定义
struct Struct_declare {
	vector<string> stru_name;
	vector<TYPE> stru_type;
};

// break，continue语句
struct Break {
	int expid;
	string type;
	Break(){}
	Break(string _type, int _expid) {
		type = _type; expid = _expid;
	}
};

// switch
struct Switch {
	string exp, from;
	Switch() {}
	Switch(string _exp, string _from) {
		exp = _exp; from = _from;
	}
};

struct EType_name {
	string name;
	vector<string> node_name;
	vector<TYPE> node_type;
	EType_name(){};
	EType_name(string _name, vector<string> node_name_, vector<TYPE> node_type_) {
		name = _name; node_name = node_name_; node_type = node_type_;
	}
};

// typedef long long LL;
struct Typedef_name {
	string name, repname;
	Typedef_name(){}
	Typedef_name(string _name, string _repname) {
		name = _name; repname = _repname;
	}
};

int depth, break_depth;


static vector<EType_name> type_name;				// type_name[i].name = "FILE"; // struct Node { list } ; ==> type_name[j].name = "Node"; type_name[j].list = list;
static vector<Typedef_name> typedef_name;			// typedef long long LL;
static map<string, int> typedef_name_apprear;
static vector<Switch> switch_case;					// case exp;  or  default exp;

static vector<Break> break_exp[MAX_DEPTH];			// break; continue; ==> break_exp[ break_exp.size()-1 ]++;
static map<string, TYPE> appear[MAX_DEPTH];		// int i; { int i; }  ==> appear[0]["i"] = "int"; appear[1]["i"] = "int";

static Struct_declare struct_declaration_list;
static map<string, int> mark;						// mark["char"] = 2; mark["int"] = 4; ...

static string tmp_name, label;						// tmp_name = "_T_"; label = ".L";

static int tmp_ident;								// tmp_ident = 0;

// init in main
static string semantic_out, function_out;

static string declare;
static vector<string> dec_list;
static vector<string> spe_list;
static vector<string> arg_list;
static vector<string> ini_list;
static int f_start, f_mid, f_end;

static string gettype(string val)
{
	for (int i=0;i<val.length();i++) if (val[i] == '.' || val[i] == 'e' || val[i] == 'E')
		return "double";
	return "int";
}

static inline int typeCmp(TYPE &t1, TYPE &t2)
{
	if (mark[t1.type] > mark[t2.type]) return 1;
	return 0;
}

static int typeCheck(TYPE &t1, TYPE &t2, int judge)
{
	int type1, type2;
	type1 = mark[t1.type];
	if (t1.type == "char") type1 = mark["int"];
	type2 = mark[t2.type];
	if (t2.type == "char") type2 = mark["int"];
	if (judge < 0) {
		if (type1 < type2) {
			return 1;
		}
		return 0;
	} else if (judge > 0) {
		if (type1 > type2) {
			return 1;
		}
		return 0;
	} else if (judge == 0) {
		if (type1 == type2) {
			return 1;
		}
		return 0;
	}
	return 0;
}

static inline int typeEqu(TYPE &t1, string type) // TODO (TYPE &t1, TYPE &type)
{
	if (t1.type == type) {
		return 1;
	}
	return 0;
}

// 获得 tr 语法树对应的 val
//          tr
//          /\
//  (long)tr1  tr2(long)
// return tr->val = long long
static TYPE dfsVal(SyntaxTree* &tr)
{
	if (tr->statement == -1) {
		tr->type.name = tr->val;
		return tr->type;
	}
	tr->type.name = "";
	for (int i=0;i<tr->tree.size();i++) {
		TYPE tmp = dfsVal(tr->tree[i]);
		switch (tr->statement) {
			case 14:
			case 16:
			case 18:
			case 19:
			case 21:
			case 24:
				// all like <declare_...> ::= <declare_...> <some type>
				// 这种多类型的都取最后一个，比如unsigned char就取char
				tr->type.type = tmp.type;
				tr->type.array = tmp.array;
				tr->type.pointer = tmp.pointer;
				if (i == 0) {
					tr->type.name = tmp.name;
					continue;
				} else {
					tr->type.name += " ";
					tr->type.name += tmp.name;
					return tr->type;
				}
				break;
		}
		if (i == 0) { // 类似于：abc(int c)，类型都按第一个取
			tr->type.type = tmp.type;
			tr->type.array = tmp.array;
			tr->type.pointer = tmp.pointer;
		}
		if (tr->statement == 209) {
			if (gettype(tmp.name) == "int") {
				tr->type.name += "<int_num>";
				tr->type.type = "int";
			} else if (gettype(tmp.name) == "double") {
				tr->type.name += "<double_num>";
				tr->type.type = "double";
			}
		}
		if (tr->statement == 208) {
			int j;
			string name;
			name = tmp.name;
			for (j=depth;j>=0;j--) {
				if (appear[j].find(name) != appear[j].end()) {
					tr->type.type = appear[j][name].type;
					tr->type.array = appear[j][name].array;
					tr->type.pointer = appear[j][name].pointer;
					break;
				}
			}
			if (j < 0) {
				printf("typecheck error: %s: %d: %s is not declared\n", source[tr->line].file.c_str(), source[tr->line].line, name.c_str());
				exit(4);
			}
		}
		tr->type.name += tmp.name;
		if (tr->statement >= 25 && tr->statement <= 43) { // 是单一定义的类型 如 int, char
			dec_list.push_back(tmp.name);
		}
		switch (tr->statement) {
			case 77: // int a
			case 78: // int *b
			case 79: // int
				if (i == 0) { // 是定义的类型 如 int，unsigned char
					spe_list.push_back(tmp.name);
				}
				break;
			case 198: // 函数调用如 abc(123)
				arg_list.push_back(tmp.name); // 存调用参数 123
				break;
			case 199: // 函数调用如 abc(123, 456)
				if (i == 2) {
					arg_list.push_back(tmp.name);
				}
			case 70: // 初始化的值 如 int i = 123;
				ini_list.push_back(tmp.name); // 存 123
				break;
		}
	}
	return tr->type;
}

static void exp_name(TYPE &valt, string &name, int &array)
{
	name = "";
	array = 0;
	for (int i=0;i<valt.name.length();i++) {
		if (valt.name[i] == '[') {
			array++;
		}
	}
	for (int i=0;i<valt.name.length();i++) {
		if (valt.name[i] == '(' || valt.name[i] == '[') {
			break;
		}
		if (valt.name[i] != ' ' && valt.name[i] != '\t' && valt.name[i] != '\n') {
			name += valt.name[i];
		}
	}
}

// 定义变量
static int declare_fun(SyntaxTree *tr)
{
	TYPE valt;
	TYPE tmp;
	string name;
	int array;
	switch (tr->statement) {
		case 97:
			spe_list.clear();
			valt = dfsVal(tr);
			exp_name(valt, name, array);

			tmp.name = name;
			tmp.type = declare;
			tmp.array = array;
			tmp.pointer = 0;
			tmp.index.clear();

			if (appear[depth].find(name) != appear[depth].end()) {
				printf("typecheck error: %s: %d: %s is be declared\n", source[tr->line].file.c_str(), source[tr->line].line, name.c_str());
				return -1;
			}
			if (spe_list.size() == 0) {
				semantic_exp.push_back( Expression("declare", declare, valt.name, "") );
			} else {
				function_block.push_back( Function(tmp, name, spe_list, "-1", "-1", "-1") );
			}
			appear[depth][name] = tmp;

			tr->statement = -1;
			tr->val = valt.name;
			tr->type = tmp;
			break;
		case 96:
			spe_list.clear();
			valt = dfsVal(tr->tree[1]);
			exp_name(valt, name, array);

			tmp.name = name;
			tmp.type = declare;
			tmp.array = array;
			tmp.pointer = 1;
			tmp.index.clear();

			if (appear[depth].find(name) != appear[depth].end()) {
				printf("typecheck error: %s: %d: %s is be declared\n", source[tr->line].file.c_str(), source[tr->line].line, name.c_str());
				return -1;
			}
			if (spe_list.size() == 0) {
				semantic_exp.push_back( Expression("declare", declare, valt.name, "pointer") );
			} else {
				function_block.push_back( Function(tmp, name, spe_list, "-1", "-1", "-1") );
			}
			appear[depth][name] = tmp;

			tr->statement = -1;
			tr->val = valt.name;
			tr->type = tmp;
			break;
	}
	return 0;
}

// 定义函数
static int function_fun(SyntaxTree *tr)
{
	TYPE valt;
	TYPE tmp;
	string name;
	int array;
	switch (tr->statement) {
		case 97:
			spe_list.clear();
			valt = dfsVal(tr);
			exp_name(valt, name, array);

			if (appear[depth].find(name) != appear[depth].end()) {
				printf("typecheck error: %s: %d: %s is be declared\n", source[tr->line].file.c_str(), source[tr->line].line, name.c_str());
				return -1;
			}

			tmp.name = name;
			tmp.type = declare;
			tmp.array = array;
			tmp.pointer = 0;
			tmp.index.clear();

			function_block.push_back( Function(tmp, name, spe_list) );
			semantic_exp.push_back( Expression("function", declare, name, "") );
			appear[0][name] = tmp;
			break;
		case 96:
			spe_list.clear();
			valt = dfsVal(tr->tree[1]);
			exp_name(valt, name, array);

			if (appear[depth].find(name) != appear[depth].end()) {
				printf("typecheck error: %s: %d: %s is be declared\n", source[tr->line].file.c_str(), source[tr->line].line, name.c_str());
				return -1;
			}

			tmp.name = name;
			tmp.type = declare;
			tmp.array = array;
			tmp.pointer = 1;
			tmp.index.clear();

			function_block.push_back( Function(tmp, name, spe_list) );
			semantic_exp.push_back( Expression("function", declare, name, "pointer") );
			appear[0][name] = tmp;
			break;
	}
	return 0;
}

static int typedef_fun(SyntaxTree* &tr)
{
	TYPE valt;
	switch (tr->statement) {
		case 97:
			valt = dfsVal(tr);
			typedef_name.push_back( Typedef_name(valt.name, declare) );
			typedef_name_apprear[valt.name] = 1;
			break;
	}
	return 0;
}

static int assignment_expression(SyntaxTree* &tr)
{
	TYPE val0, val1, val2, valt;
	switch (tr->statement) {
		case 207: // i--
			val0 = dfsVal(tr->tree[0]);
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = val0;
			tr->statement = -1;
			semantic_exp.push_back( Expression("=", val0.name, null, tr->val) );
			semantic_exp.push_back( Expression("-", val0.name, "<int_num>1", val0.name) );
			break;
		case 206: // i++
			val0 = dfsVal(tr->tree[0]);
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = val0;
			tr->statement = -1;
			semantic_exp.push_back( Expression("=", val0.name, null, tr->val) );
			semantic_exp.push_back( Expression("+", val0.name, "<int_num>1", val0.name) );
			break;
		case 188: // --i
			val1 = dfsVal(tr->tree[1]);
			tr->val = val1.name;
			tr->type = val1;
			tr->statement = -1;
			semantic_exp.push_back( Expression("-", tr->val, "<int_num>1", tr->val) );
			break;
		case 187: // ++i
			val1 = dfsVal(tr->tree[1]);
			tr->val = val1.name;
			tr->type = val1;
			tr->statement = -1;
			semantic_exp.push_back( Expression("+", tr->val, "<int_num>1", tr->val) );
			break;
		case 178: // i+j
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = typeCmp(val0, val2) ? val0 : val2;
			tr->statement = -1;
			semantic_exp.push_back( Expression("+", val0.name, val2.name, tr->val) );
			break;
		case 179: // i-j
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = typeCmp(val0, val2) ? val0 : val2;
			tr->statement = -1;
			semantic_exp.push_back( Expression("-", val0.name, val2.name, tr->val) );
			break;
		case 181: // i*j
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = typeCmp(val0, val2) ? val0 : val2;
			tr->statement = -1;
			semantic_exp.push_back( Expression("*", val0.name, val2.name, tr->val) );
			break;
		case 182: // i/j
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = typeCmp(val0, val2) ? val0 : val2;
			tr->statement = -1;
			semantic_exp.push_back( Expression("/", val0.name, val2.name, tr->val) );
			break;
		case 183: // i%j
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			if (!(typeEqu(val0, "int") || typeEqu(val0, "char")) || !(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
				printf("typecheck error: a %% b: a, b must int\n");
				return -1;
			}
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = typeCmp(val0, val2) ? val0 : val2;
			tr->statement = -1;
			semantic_exp.push_back( Expression("%", val0.name, val2.name, tr->val) );
			break;
		case 189: // +i ie.
			val0 = dfsVal(tr->tree[0]);
			val1 = dfsVal(tr->tree[1]);
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = val1;
			tr->statement = -1;
			semantic_exp.push_back( Expression(val0.name, val1.name, null, tr->val) );
			break;
		case 211: // (i)
			val1 = dfsVal(tr->tree[1]);
			tr->val = val1.name;
			tr->type = val1;
			tr->statement = -1;
			break;
		case 176: // a >> b
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			if (!(typeEqu(val0, "int") || typeEqu(val0, "char")) || !(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
				printf("typecheck error: a >> b: a, b must int\n");
				return -1;
			}
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = typeCmp(val0, val2) ? val0 : val2;
			tr->statement = -1;
			semantic_exp.push_back( Expression(">>", val0.name, val2.name, tr->val) );
			break;
		case 175: // a << b
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			if (!(typeEqu(val0, "int") || typeEqu(val0, "char")) || !(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
				printf("typecheck error: a << b: a, b must int\n");
				return -1;
			}
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = typeCmp(val0, val2) ? val0 : val2;
			tr->statement = -1;
			semantic_exp.push_back( Expression("<<", val0.name, val2.name, tr->val) );
			break;
		case 139: // a , b
			val1 = dfsVal(tr->tree[1]);
			tr->val = val1.name;
			tr->type = val1;
			tr->statement = -1;
			break;
		case 141: // a *= b ie.
			valt = dfsVal(tr->tree[1]);
			val0 = dfsVal(tr->tree[0]);
			val2 = dfsVal(tr->tree[2]);
			if (typeCheck(val0, val2, -1) && !(valt.name == "=" && val0.pointer != 0 && val2.type == "int")) {
				printf("typecheck error: %s: %d: %s = %s\n", source[tr->line].file.c_str(), source[tr->line].line, val0.type.c_str(), val2.type.c_str());
				return -1;
			}
			tr->val = val0.name;
			tr->type = val0;
			tr->statement = -1;
			if (valt.name == "=") {
				semantic_exp.push_back( Expression(valt.name, val2.name, null, val0.name) );
			} else {
				semantic_exp.push_back( Expression(valt.name.substr(0, valt.name.size()-1), val0.name, val2.name, tr->val) );
			}
			break;
	}
	return 0;
}

static int conditional_expression(SyntaxTree* &tr)
{
	TYPE val0,val1,val2;
	tr->val = tmp_name + iTs(tmp_ident);
	tmp_ident++;
	val0 = dfsVal(tr->tree[0]);
	val2 = dfsVal(tr->tree[2]);
	switch (tr->statement)
	{
	case 173: // a >= b
		semantic_exp.push_back( Expression(">=", val0.name, val2.name, tr->val) );
		break;
	case 172: // a <= b
		semantic_exp.push_back( Expression("<=", val0.name, val2.name, tr->val) );
		break;
	case 171: // a > b
		semantic_exp.push_back( Expression(">", val0.name, val2.name, tr->val) );
		break;
	case 170: // a < b
		semantic_exp.push_back( Expression("<", val0.name, val2.name, tr->val) );
		break;
	case 168: // a != b
		semantic_exp.push_back( Expression("!=", val0.name, val2.name, tr->val) );
		break;
	case 167: // a == b
		semantic_exp.push_back( Expression("==", val0.name, val2.name, tr->val) );
		break;
	case 165: // a & b
		if (!(typeEqu(val0, "int") || typeEqu(val0, "char")) || !(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
			printf("typecheck error: %s: %d: a & b, a,b must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
			return -1;
		}
		semantic_exp.push_back( Expression("&", val0.name, val2.name, tr->val) );
		break;
	case 163: // a ^ b
		if (!(typeEqu(val0, "int") || typeEqu(val0, "char")) || !(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
			printf("typecheck error: %s: %d: a ^ b, a,b must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
			return -1;
		}
		semantic_exp.push_back( Expression("^", val0.name, val2.name, tr->val) );
		break;
	case 161: // a | b
		if (!(typeEqu(val0, "int") || typeEqu(val0, "char")) || !(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
			printf("typecheck error: %s: %d: a | b, a,b must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
			return -1;
		}
		semantic_exp.push_back( Expression("|", val0.name, val2.name, tr->val) );
		break;
	}
	tr->type.type = "int";
	tr->statement = -1;
	return 0;
}

static int statement_expression(SyntaxTree* &tr)
{
	TYPE val0;
	switch (tr->statement)
	{
	case 124: // ;
		tr->val = "";
		tr->statement = -1;
		semantic_exp.push_back( Expression("", "", "", "") );
		break;
	case 125: // exp_s ::= exp ;
		val0 = dfsVal(tr->tree[0]);
		tr->val = val0.name;
		tr->type = val0;
		tr->statement = -1;
		break;
	case 134: // continue ;
		semantic_exp.push_back( Expression("goto", "", "", ""));
		break_exp[break_depth-1].push_back( Break("continue", semantic_exp.size()-1));
		break;
	case 135: // break ;
		semantic_exp.push_back(Expression("goto", "", "", ""));
		break_exp[break_depth-1].push_back( Break("break", semantic_exp.size()-1));
		break;
	}
	return 0;
}

static int type_expression(SyntaxTree* &tr)
{
	TYPE valt, val2, val3;
	TYPE tmp;
	int pointer;
	switch (tr->statement)
	{
	case 185: // ( type ) s
		tr->statement = -1;
		valt = dfsVal(tr->tree[1]);
		val3 = dfsVal(tr->tree[3]);
		tr->type = valt;
		pointer = 0;
		if (valt.name[valt.name.size()-1] == '*') {
			pointer = 1;
			#ifdef __linux_64_system__
				valt.name = "qword";
			#else
				valt.name = "int";
			#endif
		}
		tr->val = "change_type_"+iTs(tmp_ident);
		tmp_ident++;
		semantic_exp.push_back( Expression("declare", valt.name, tr->val, (pointer == 0 ? "" : "pointer")) );
		tmp.type = valt.type;
		tmp.name = valt.name;
		tmp.array = 0;
		tmp.pointer = pointer;
		tmp.index.clear();
		appear[depth][tr->val] = tmp;

		semantic_exp.push_back( Expression("change_type", val3.name, "null", tr->val) );
		break;
	case 191: // sizeof ( s )
		tr->val = tmp_name + iTs(tmp_ident);
		tmp_ident++;
		tr->type.type = "int";
		tr->statement = -1;
		val2 = dfsVal(tr->tree[2]);
		semantic_exp.push_back( Expression("sizeof", val2.name, "null", tr->val) );
		break;
	}
	return 0;
}

static int struct_specifier(SyntaxTree* &tr)
{
	vector<string> list_tmp;
	int fsz, i, j, k=-1;
	TYPE val0, val1, val2;
	string name;

	switch (tr->statement)
	{
	case 52:  // {int} i
		k = 0;
	case 53: // {int} i , j
		if (k == -1) {
			k = 2;
		}
		val0 = dfsVal(tr->tree[k]);
		name = "";
		for (i=0;i<val0.name.size();i++) {
			if (val0.name[i] == '*') {
				continue;
			}
			if (val0.name[i] == '[') {
				break;
			}
			name += val0.name[i];
		}
		struct_declaration_list.stru_name.push_back(name);
		struct_declaration_list.stru_type.push_back(val0);
		break;
	case 44:  // struct Node { list }
	case 45:  // struct { list }
		val1 = dfsVal(tr->tree[1]);
		if (tr->statement == 45) {
			val1.name = "STRUCT_NAME_"+iTs(tmp_ident++);
		}
		type_name.push_back( EType_name(val1.name, struct_declaration_list.stru_name, struct_declaration_list.stru_type) );
		semantic_exp.push_back( Expression("STRUCT_END", "", "", "") );
		f_end = semantic_exp.size();
		fsz = function_block.size();
		function_block[fsz-1].name = val1.name;
		function_block[fsz-1].start = label + iTs(f_start);
		function_block[fsz-1].end = label + iTs(f_end);
		tr->val = val1.name;
		tr->statement = -1;
		break;
	case 46:  // struct Node
		val1 = dfsVal(tr->tree[1]);
		tr->val = val1.name;
		tr->statement = -1;
		break;
	case 204: // a . x
		val0 = dfsVal(tr->tree[0]);
		val2 = dfsVal(tr->tree[2]);
		for (i=0;i<type_name.size();i++) if (type_name[i].name == val0.type) break;
		if (i>=type_name.size()) {
			printf("typecheck error: %s: %d: a.x: a must be a struct\n", source[tr->line].file.c_str(), source[tr->line].line);
			return -1;
		}

		for (j=0;j<type_name[i].node_name.size();j++) {
			if (type_name[i].node_name[j] == val2.name) break;
		}
		if (j>=type_name[i].node_name.size()) {
			printf("typecheck error: %s: %d: a.x: x not be declare\n", source[tr->line].file.c_str(), source[tr->line].line);
			return -1;
		}
		tr->type = type_name[i].node_type[j];

		tr->val = "<struct>"+val0.name+"."+val2.name;
		tr->statement = -1;
		break;
	case 205: // a -> x
		val0 = dfsVal(tr->tree[0]);
		val2 = dfsVal(tr->tree[2]);
		for (i=0;i<type_name.size();i++) if (type_name[i].name == val0.type) break;
		if (i>=type_name.size()) {
			printf("typecheck error: %s: %d: a.x: a must be a struct\n", source[tr->line].file.c_str(), source[tr->line].line);
			return -1;
		}

		for (j=0;j<type_name[i].node_name.size();j++) {
			if (type_name[i].node_name[j] == val2.name) break;
		}
		if (j>=type_name[i].node_name.size()) {
			printf("typecheck error: %s: %d: a.x: x not be declare\n", source[tr->line].file.c_str(), source[tr->line].line);
			return -1;
		}
		tr->type = type_name[i].node_type[j];

		semantic_exp.push_back( Expression("*", val0.name, "null", tmp_name+iTs(tmp_ident)) );
		tr->val = "<struct>"+tmp_name+iTs(tmp_ident)+"."+val2.name;
		tmp_ident++;
		tr->statement = -1;
		break;
	}
	return 0;
}

static int depth_flag;
static int beforeDo(SyntaxTree* &tr, string &todo)
{
	TYPE val1;
	vector<int> index;
	switch (tr->statement) {
		case 97:
			if (todo == "declare") {
				if (declare_fun(tr) < 0) {
					return -1;
				}
				return 1;
			} else if (todo == "function") {
				if (function_fun(tr) < 0) {
					return -1;
				}
				todo = "declare";
			} else if (todo == "typedef") {
				if (typedef_fun(tr) < 0) {
					return -1;
				}
				return 1;
			}
			break;

		case 96:
			if (todo == "declare") {
				if (declare_fun(tr) < 0) {
					return -1;
				}
				return 1;
			} else if (todo == "function") {
				if (function_fun(tr) < 0) {
					return -1;
				}
				todo = "declare";
			} else if (todo == "typedef") {
				printf("typecheck error: %s: %d: typedef int *a;\n", source[tr->line].file.c_str(), source[tr->line].line);
				return -1;
			}
			break;
		case 67:
			if (todo == "typedef") {
				printf("typecheck error: %s: %d: typedef int a=10;\n", source[tr->line].file.c_str(), source[tr->line].line);
				return -1;
			}
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			f_start = semantic_exp.size();
			semantic_exp.push_back( Expression("FUNCTION_BEGIN", "", "", "") );
			depth++;
			appear[depth].clear();
			depth_flag = 1;
			break;
		case 118:
		case 119:
		case 120:
		case 121:
			if (depth_flag == 1) {
				depth_flag = 0;
				break;
			}
			depth++;
			appear[depth].clear();
			break;
		case 44:
		case 45:
			depth++;
			appear[depth].clear();
			f_start = semantic_exp.size();
			semantic_exp.push_back( Expression("STRUCT_BEGIN", "", "", "") );
			struct_declaration_list.stru_name.clear();
			struct_declaration_list.stru_type.clear();

			index.clear();
			function_block.push_back( Function(TYPE("", "struct", 0, 0, index), "", spe_list) );
			semantic_exp.push_back( Expression("struct", "", "", "") );
		case 46: // struct Node
			break;
	}
	return 0;
}


static int afterDo(SyntaxTree* &tr, string &todo)
{
	TYPE valt, val0, val1, val2;
	int fsz,i;
	switch (tr->statement) {
		case 5:
		case 6:
		case 7:
		case 8:
			semantic_exp.push_back( Expression("FUNCTION_END", "", "", "") );
			f_end = semantic_exp.size();
			fsz = function_block.size();
			function_block[fsz-1].start = label + iTs(f_start);
			function_block[fsz-1].mid = label + iTs(f_mid);
			function_block[fsz-1].end = label + iTs(f_end);
			break;
		case 118:
		case 119:
		case 120:
		case 121:
			for (map<string, TYPE> :: iterator it=appear[depth].begin();it!=appear[depth].end();it++) {
				semantic_exp.push_back( Expression("undeclare", it->first, "", "") );
			}
			depth--;
			break;
		case 67:
			val0 = dfsVal(tr->tree[0]);
			ini_list.clear();
			val2 = dfsVal(tr->tree[2]);
			if (val0.array == 0 && ini_list.size() > 1) {
				printf("typecheck error: %s: %d: int a={1,2}\n", source[tr->line].file.c_str(), source[tr->line].line);
				return -1;
			}
			if (val0.array == 0) {
				semantic_exp.push_back( Expression("=", ini_list[0], null, val0.name) );
			} else {
				string name = "";
				for (i=0;i<val0.name.length();i++) {
					if (val0.name[i] == '[') {
						break;
					}
					name += val0.name[i];
				}
				for (i=0;i<ini_list.size();i++) {
					semantic_exp.push_back( Expression("=", ini_list[i], null, name+"[<int_num>"+iTs(i)+"]") );
				}
			}
			break;

		case 136:
			semantic_exp.push_back( Expression("return", "", "", "") );
			break;
		case 137:
			val1 = dfsVal(tr->tree[1]);
			semantic_exp.push_back( Expression("=", val1.name, "null", "returnval") );
			semantic_exp.push_back( Expression("return", "", "", "") );
			break;
		case 202: // abc ( )
			val0 = dfsVal(tr->tree[0]);
			semantic_exp.push_back( Expression("call_function", val0.name, "", "") );
			tr->statement = -1;

			//tr->val = "returnval";
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = val0;
			semantic_exp.push_back( Expression("=", "returnval", null, tr->val) );
			break;

		case 203: // abc ( list )
			val0 = dfsVal(tr->tree[0]);
			arg_list.clear();
			val2 = dfsVal(tr->tree[2]);
			semantic_exp.push_back( Expression("call_function", val0.name, val2.name, "") );
			semantic_exp[semantic_exp.size()-1].parameter = arg_list;
			tr->statement = -1;

			//tr->val = "returnval";
			tr->val = tmp_name + iTs(tmp_ident);
			tmp_ident++;
			tr->type = val0;
			semantic_exp.push_back( Expression("=", "returnval", null, tr->val) );
			break;

		case 207: // i--
		case 206: // i++
		case 188: // --i
		case 187: // ++i
		case 178: // i+j
		case 179: // i-j
		case 181: // i*j
		case 182: // i/j
		case 183: // i%j
		case 189: // +i ie.
		case 211: // (i)
		case 176: // a >> b
		case 175: // a << b
		case 139: // a , b
		case 141: // a *= b ie.
			if (assignment_expression(tr) < 0) {
				return -1;
			}
			break;

		case 173: // a >= b
		case 172: // a <= b
		case 171: // a > b
		case 170: // a < b
		case 168: // a != b
		case 167: // a == b
		case 165: // a & b
		case 163: // a ^ b
		case 161: // a | b
			if (conditional_expression(tr) < 0) {
				return -1;
			}
			break;

		case 124: // ;
		case 125: // exp ;
		case 134: // continue ;
		case 135: // break ;
			if (statement_expression(tr) < 0) {
				return -1;
			}
			break;

		case 185: // ( type ) s
		case 191: // sizeof ( type )
			if (type_expression(tr) < 0) {
				return -1;
			}
			break;
		// struct_specifier
		case 44:  // struct Node { list }
		case 45:  // struct { list }
			for (map<string, TYPE> :: iterator it=appear[depth].begin();it!=appear[depth].end();it++) {
				semantic_exp.push_back( Expression("undeclare", it->first, "", "") );
			}
			depth--;
		case 46:  // struct ident
		case 52:  // {int} i
		case 53:
		case 204: // a.x
		case 205: // a -> x
			if (struct_specifier(tr) < 0) {
				return -1;
			}
			break;
	}
	return 0;
}

static std::stack<int> back1, back2;

static int needBackDo(SyntaxTree* &tr, int i)
{
	string op = "", va1, va2;
	TYPE val0, val1, val2;
	switch (tr->statement)
	{
		case 5:
			if (i == 2) {
				semantic_exp.push_back( Expression("PARAMETER_SPLIT", "", "", "") );
				f_mid = semantic_exp.size();
			}
			break;
		case 6:
			if (i == 1) {
				semantic_exp.push_back( Expression("PARAMETER_SPLIT", "", "", "") );
				f_mid = semantic_exp.size();
			}
			break;
		case 7:
			if (i == 1) {
				semantic_exp.push_back( Expression("PARAMETER_SPLIT", "", "", "") );
				f_mid = semantic_exp.size();
			}
			break;
		case 8:
			if (i == 0) {
				semantic_exp.push_back( Expression("PARAMETER_SPLIT", "", "", "") );
				f_mid = semantic_exp.size();
			}
			break;
		case 157: // a || b
			op = "if";
			va1 = "<int_num>0";
			va2 = "<int_num>1";
		case 159: // a && b
			if (op == "") {
				op = "ifn";
				va1 = "<int_num>1";
				va2 = "<int_num>0";
			}
			if (i == 0) {
				back1.push(semantic_exp.size());
				val0 = dfsVal(tr->tree[0]);
				if (!(typeEqu(val0, "int") || typeEqu(val0, "char"))) {
					printf("typecheck error: %s: %d: a &&(||) b, a must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression(op, val0.name, "goto", "back1") );
			}
			if (i == 2) {
				tr->val = tmp_name + iTs(tmp_ident);
				tr->type.type = "int";
				tmp_ident++;
				tr->statement = -1;
				semantic_exp[back1.top()].result = label+iTs(semantic_exp.size()+3);
				back1.pop();
				val2 = dfsVal(tr->tree[2]);
				if (!(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
					printf("typecheck error: %s: %d: a &&(||) b, b must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression(op, val2.name, "goto", label+iTs(semantic_exp.size()+3)) );
				semantic_exp.push_back( Expression("=", va1, null, tr->val) );
				semantic_exp.push_back( Expression("goto", label+iTs(semantic_exp.size()+2), "", "") );
				semantic_exp.push_back( Expression("=", va2, null, tr->val) );
				semantic_exp.push_back( Expression("", "", "", "") );
			}
			break;
		case 155:
			if (i == 0) {
				back1.push(semantic_exp.size());
				val0 = dfsVal(tr->tree[0]);
				if (!(typeEqu(val0, "int") || typeEqu(val0, "char"))) {
					printf("typecheck error: %s: %d: a ? b : c, a must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression("ifn", val0.name, "goto", "back1") );
			}
			if (i == 2) {
				val2 = dfsVal(tr->tree[2]);
				semantic_exp.push_back( Expression("=", val2.name, null, tmp_name + iTs(tmp_ident)) ); // i = 2 AND i = 4 共用一个t_X
				back2.push(semantic_exp.size());
				semantic_exp.push_back( Expression("goto", "back2", "", "") );
				semantic_exp[back1.top()].result = label+iTs(semantic_exp.size());
			}
			if (i == 4) {
				tr->val = tmp_name + iTs(tmp_ident);
				tmp_ident++;
				tr->statement = -1;
				val2 = dfsVal(tr->tree[4]);
				tr->type = val2;
				semantic_exp.push_back( Expression("=", val2.name, null, tr->val) );
				semantic_exp[back2.top()].arg1 = label+iTs(semantic_exp.size());
				back1.pop();
				back2.pop();
			}
			break;
		case 126: // if ( e ) s
			if (i == 2) {
				back1.push(semantic_exp.size());
				val2 = dfsVal(tr->tree[2]);
				if (!(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
					printf("typecheck error: %s: %d: if ( e ) s , e must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression("ifn", val2.name, "goto", "back1") );
			}
			if (i == 4) {
				semantic_exp[back1.top()].result = label+iTs(semantic_exp.size());
				semantic_exp.push_back( Expression("","","","") );
				back1.pop();
			}
			break;
		case 127: // if ( e ) s else t
			if (i == 2) {
				back1.push(semantic_exp.size());
				val2 = dfsVal(tr->tree[2]);
				if (!(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
					printf("typecheck error: %s: %d: if ( e ) s , e must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression("ifn", val2.name, "goto", "back1") );
			}
			if (i == 4) {
				back2.push(semantic_exp.size());
				semantic_exp.push_back( Expression("goto", "back2", "", "") );
				semantic_exp[back1.top()].result = label+iTs(semantic_exp.size());
			}
			if (i == 6) {
				semantic_exp[back2.top()].arg1 = label+iTs(semantic_exp.size());
				semantic_exp.push_back( Expression("","","","") );
				back1.pop();
				back2.pop();
			}
			break;
		case 116: // case e : s
			if (i == 2) {
				val0 = dfsVal(tr->tree[1]);
				if (!(typeEqu(val0, "int") || typeEqu(val0, "char"))) {
					printf("typecheck error: %s: %d: case e: s , e must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				switch_case.push_back( Switch(val0.name, iTs(semantic_exp.size())) );
			}
			break;
		case 117: // defalut : s
			if (i == 1) {
				switch_case.push_back( Switch("default", iTs(semantic_exp.size())) );
			}
			break;
		case 128: // switch ( e ) s
			if (i == 0) {
				break_depth++;
				break_exp[break_depth-1].clear();
				semantic_exp.push_back( Expression("","","","") );
			}
			if (i == 3) {
				back1.push(semantic_exp.size());
				semantic_exp.push_back( Expression("goto", "back1", "", "") );
			}
			if (i == 4) {
				back2.push(semantic_exp.size());
				semantic_exp.push_back( Expression("goto", "back2", "", "") );
				semantic_exp[back1.top()].arg1 = label+iTs(semantic_exp.size());

				val0 = dfsVal(tr->tree[2]);
				if (!(typeEqu(val0, "int") || typeEqu(val0, "char"))) {
					printf("typecheck error: %s: %d: switch ( e ) s , e must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				for (i=0;i<switch_case.size();i++)
				{
					if (switch_case[i].exp != "default") {
						semantic_exp.push_back( Expression("==", val0.name, switch_case[i].exp, tmp_name+iTs(tmp_ident)) );
						semantic_exp.push_back( Expression("if", tmp_name+iTs(tmp_ident), "goto", label+switch_case[i].from) );
						tmp_ident++;
					} else {
						semantic_exp.push_back( Expression("goto", label+switch_case[i].from, "", "") );
					}
				}
				switch_case.clear();
				semantic_exp[back2.top()].arg1 = label+iTs(semantic_exp.size());
				semantic_exp.push_back( Expression("","","","") );

				for (i=0;i<break_exp[break_depth-1].size();i++) {
					if (break_exp[break_depth-1][i].type == "continue") {
						printf("typecheck error: %s: %d: switch statement can't appear \"continue\"\n", source[tr->line].file.c_str(), source[tr->line].line);
						return -1;
					} else if (break_exp[break_depth-1][i].type == "break") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(semantic_exp.size()-1);
					}
				}
				back1.pop();
				back2.pop();
				break_depth--;
			}
			break;
		case 129: // while ( e ) s
			if (i == 0) {
				break_depth++;
				break_exp[break_depth-1].clear();
				back1.push(semantic_exp.size());
				semantic_exp.push_back( Expression("", "", "", "") );
			}
			if (i == 2) {
				back2.push(semantic_exp.size());
				val2 = dfsVal(tr->tree[2]);
				if (!(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
					printf("typecheck error: %s: %d: while ( e ) s , e must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression("ifn", val2.name, "goto", "back2") );
			}
			if (i == 4) {
				semantic_exp.push_back( Expression("goto", label+iTs(back1.top()), "", "") );
				semantic_exp[back2.top()].result = label + iTs(semantic_exp.size());
				semantic_exp.push_back( Expression("", "", "", "") );

				for (i=0;i<break_exp[break_depth-1].size();i++) {
					if (break_exp[break_depth-1][i].type == "continue") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(back1.top());
					} else if (break_exp[break_depth-1][i].type == "break") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(semantic_exp.size()-1);
					}
				}
				back1.pop();
				back2.pop();
				break_depth--;
			}
			break;
		case 130: // do s while ( e ) ;
			if (i == 0) {
				break_depth++;
				break_exp[break_depth-1].clear();
				back1.push(semantic_exp.size());
			}
			if (i == 3) {
				back2.push(semantic_exp.size());
			}
			if (i == 4) {
				val2 = dfsVal(tr->tree[4]);
				if (!(typeEqu(val2, "int") || typeEqu(val2, "char"))) {
					printf("typecheck error: %s: %d: do s while ( s ) ; , e must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression("if", val2.name, "goto", label+iTs(back1.top())) );
				semantic_exp.push_back( Expression("","","","") );

				for (i=0;i<break_exp[break_depth-1].size();i++) {
					if (break_exp[break_depth-1][i].type == "continue") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(back2.top());
					} else if (break_exp[break_depth-1][i].type == "break") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(semantic_exp.size()-1);
					}
				}
				back1.pop();
				back2.pop();
				break_depth--;
			}
			break;
		case 131: // for ( e; e; ) s
			if (i == 2) {
				break_depth++;
				break_exp[break_depth-1].clear();
				back1.push(semantic_exp.size());
			}
			if (i == 3) {
				val1 = dfsVal(tr->tree[3]);
				if (val1.name == "") {
					val1.name = "1";
					val1.type = "int";
				}
				if (!(typeEqu(val1, "int") || typeEqu(val1, "char"))) {
					printf("typecheck error: %s: %d: for ( e0; e1; ) s , e1 must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				back2.push(semantic_exp.size());
				semantic_exp.push_back( Expression("ifn", val1.name, "goto", "back1") );
			}
			if (i == 5) {
				semantic_exp.push_back( Expression("goto", label+iTs(back1.top()), "", "") );
				semantic_exp[back2.top()].result = label+iTs(semantic_exp.size());
				semantic_exp.push_back( Expression("", "", "", "") );

				for (i=0;i<break_exp[break_depth-1].size();i++) {
					if (break_exp[break_depth-1][i].type == "continue") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(back1.top());
					} else if (break_exp[break_depth-1][i].type == "break") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(semantic_exp.size()-1);
					}
				}
				back1.pop();
				back2.pop();
				break_depth--;
			}
			break;
		case 132: // for ( e; e; e ) s
			if (i == 2) {
				break_depth++;
				break_exp[break_depth-1].clear();
				back1.push(semantic_exp.size());
			}
			if (i == 3) {
				val1 = dfsVal(tr->tree[3]);
				if (val1.name == "") {
					val1.name = "1";
					val1.type = "int";
				}
				if (!(typeEqu(val1, "int") || typeEqu(val1, "char"))) {
					printf("typecheck error: %s: %d: for ( e0; e1; e2 ) s , e1 must be int\n", source[tr->line].file.c_str(), source[tr->line].line);
					return -1;
				}
				semantic_exp.push_back( Expression("ifn", val1.name, "goto", "back1") );
				back2.push(semantic_exp.size());
				semantic_exp.push_back( Expression("goto", "back2", "", "") );
			}
			if (i == 4) {
				semantic_exp.push_back( Expression("goto", label+iTs(back1.top()), "", "") );
				semantic_exp[back2.top()].arg1 = label + iTs(semantic_exp.size());
			}
			if (i == 6) {
				semantic_exp.push_back( Expression("goto", label+iTs(back2.top()+1), "", "") );
				semantic_exp[back2.top()-1].result = label+iTs(semantic_exp.size());
				semantic_exp.push_back( Expression("", "", "", "") );

				for (i=0;i<break_exp[break_depth-1].size();i++) {
					if (break_exp[break_depth-1][i].type == "continue") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(back1.top());
					} else if (break_exp[break_depth-1][i].type == "break") {
						semantic_exp[break_exp[break_depth-1][i].expid].arg1 = label+iTs(semantic_exp.size()-1);
					}
				}
				back1.pop();
				back2.pop();
				break_depth--;
			}
			break;
	}
	return 0;
}

static void typedef_replace()
{
	int i;
	for (i=0;i<typedef_name.size();i++) {
		if (declare == typedef_name[i].name) {
			declare = typedef_name[i].repname;
			break;
		}
	}
}

static inline int declare_spe(string declare)
{
	// TODO
	// 判断一个类型是否正确，像long long正确，int int就不正确... ，还需要修改lexical.cpp，因为前面把signed等都换了
	return 1;
}

static int dfs(SyntaxTree* &tr, string pre, string todo)
{
	TYPE val0;
	int ret = beforeDo(tr, todo); // 需要在dfs一进来的时候做些处理，比如输出函数的开头
	if (ret > 0) {
		return 1;
	}
	if (ret < 0) {
		return -1;
	}
	for (int i=0;i<tr->tree.size();i++) {
		if (tr->statement == 6 && i == 0) {
			todo = "function";
			dec_list.clear();
			val0 = dfsVal(tr->tree[i]);
			declare = dec_list[dec_list.size()-1];
			typedef_replace();
			if (!declare_spe(declare)) {
				printf("typecheck error: %s: %d: \"%s\" is not invalid\n", source[tr->line].file.c_str(), source[tr->line].line, declare.c_str());
				return -1;
			}
			continue;
		}
		if (tr->statement == 51 && i == 0) { // in struct: int i ;
			todo = "declare";
			dec_list.clear();
			val0 = dfsVal(tr->tree[i]);
			declare = dec_list[dec_list.size()-1];
			typedef_replace();
			continue;
		}
		if (tr->statement == 116 && i == 1) { // case e : s
			continue;
		}
		if ((tr->statement == 77 || tr->statement == 78 || tr->statement == 79) && i == 0) {
			dec_list.clear();
			val0 = dfsVal(tr->tree[i]);
			declare = dec_list[dec_list.size()-1];
			typedef_replace();
			continue;
		}

		if (dfs(tr->tree[i], pre+"\t", todo) < 0) {
			return -1;
		}

		if (tr->statement == 12 && i == 0) { // declare_spe ::= struct Node { ... } 所以要先dfs(declare_spe);
			dec_list.clear();
			val0 = dfsVal(tr->tree[i]);
			if (dec_list[0] == "typedef") {
				todo = "typedef";
			} else {
				todo = "declare";
			}
			declare = dec_list[dec_list.size()-1];
			typedef_replace();
			if (!declare_spe(declare)) {
				printf("typecheck error: %s: %d: \"%s\" is not invalid\n", source[tr->line].file.c_str(), source[tr->line].line, declare.c_str());
				return -1;
			}
			continue;
		}

		if (needBackDo(tr, i) < 0) { // 像for循环，开始并不知道要跳转到哪里，只能先标记，后面再来修改
			return -1;
		}
	}

	// AFTER
	if (afterDo(tr, todo) < 0) {
		return -1;
	}
	return 0;
}


static int SOLVE()
{
	int i,j,len,sz,flag=0;
	string str_name;

	map<string, string> :: iterator it;
	function_block.clear();

	type_name.clear();
	switch_case.clear();

	depth = 0; appear[depth].clear();
	tmp_ident = 0;
	break_depth = 1; break_exp[0].clear();
	depth_flag = 0;

	TYPE tmp;
	tmp.name = "stdin";
	tmp.type = "FILE";
	tmp.array = 0;
	tmp.pointer = 1;

	appear[0]["stdin"] = tmp;
	tmp.name = "stdout";
	appear[0]["stdout"] = tmp;
	tmp.name = "stderr";
	appear[0]["stderr"] = tmp;

	// dfs 语法树
	int typecheck = dfs(headSubtr, "", "");
	if (break_depth > 1 || break_exp[0].size() > 0) {
		printf("typecheck error: break or continue error\n");
		typecheck = -1;
	}
	return typecheck;
}

int semantic_main()
{
	semantic_out	= "./data/semantic.out";
	function_out	= "./data/function.out";

	tmp_name = "_T_";
	label = ".L";

	mark.clear();
	mark["void"]	= 2;
	mark["char"]	= 3;
	mark["int"]		= 4;
	mark["double"]	= 5;
	mark["string"]	= 6;

	int i,j,jum;

	if (SOLVE() < 0) {
		return 1;
	}
	FILE *fp;
	if (OUT_OTHER_FILE == 1) {
		fp = fopen(semantic_out.c_str(), "w");
	}
	semantic_exp.push_back( Expression("", "", "", "") );
	for (i=0;i<semantic_exp.size();i++)
	{
		semantic_exp[i].label = label+iTs(i);
		if (OUT_OTHER_FILE == 1) {
			fprintf(fp, "%s\t%s\t%s\t%s\t%s\n", semantic_exp[i].label.c_str(), semantic_exp[i].op.c_str(), semantic_exp[i].arg1.c_str(), semantic_exp[i].arg2.c_str(), semantic_exp[i].result.c_str());
		}
	}
	if (OUT_OTHER_FILE == 1) {
		fp = fopen(function_out.c_str(), "w");
		for (i=0;i<function_block.size();i++) {
			fprintf(fp, "%s\t%s\t%s\t%s\t%s\n", function_block[i].type.type.c_str(), function_block[i].name.c_str(), function_block[i].start.c_str(), function_block[i].mid.c_str(), function_block[i].end.c_str());
		}
		fclose(fp);
	}
	return 0;
}

