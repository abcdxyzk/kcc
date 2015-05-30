#include "kcc.h"

// include 时查询的目录顺序
static string includeFile[] = {
	"./",
	"./include/",
};
static int includeSize = 2;
vector<Source> source;

// 存 #define
static map<string, int> def;
static int def_line;
static struct DefDetail defdetail[100000];


static string preprocessor_out;
static string preprocessor_in;

static int m;
static int error_line;
static string error_file;

struct PStack {
	int status, preShow, nowShow;
	PStack(){}
	PStack(int _status, int _preShow, int _nowShow) {
		status = _status;
		preShow = _preShow;
		nowShow = _nowShow;
	}
};

/*
                                         -------------
                                         |           |
                                         | #elif     |
    #if/#ifdef/#ifndef         #elif     V           |
 -----------------------> 1 -----------> 2 ---------->
                          |              |
                          |              | #else
                          V    #else     V
                          -------------> 3
                          |              |
                          |              | #endif
                          V    #endif    V
                          -------------> 4
                                         |
                                         |
                                         V
                                        ...
*/

/*
	if match #if then
		sz++;
		stack[sz].preShow = stack[sz-1].nowShow; // preShow 表示外层的显示状况,初始化上一层nowShow
		stack[sz].nowShow = 0; // nowShow 表示当层的显示状况,初始化0
		if #if stack[sz].nowShow = 1;
	else match #elif, #else, ...
		sz++;
		stack[sz].preShow = stack[sz-1].preShow;
		stack[sz].nowShow = stack[sz-1].nowShow;
		if (stack[sz].nowShow > 0) stack[sz].nowShow++;
	else match #endif
		while (sz > 1) sz--;
		sz--;


	若满足条件显示则nowShow = 1; 即nowShow=1则该层显示
	if (stack[sz].nowShow > 0) stack[sz].nowShow++; 这样保证只有一支分支显示

*/

/*
   计算优先级：// 越小越先计算
	1: ! ~
	2: * / %
	3: + -
	4: << >>
	5: > >= < <=
	6: == !=
	7: &
	8: ^
	9: |
	10: &&
	11: ||

	处理表达式：
	(   : 压栈
	)   : 计算到(为止
	数字: 压栈
	符号: 计算到大于当前符号为止
*/

static int spe_char(char c)
{
	if (c == ' ' || c == '\0' || c == '\n' || c == '\r' || c == '\t') {
		return 1;
	}
	return 0;
}

static string but_spe_char(string str)
{
	int k,l;
	l = str.length()-1; while (l >= 0 && spe_char(str[l])) l--;
	k = 0; while (k <= l && spe_char(str[k])) k++;
	if (k > l) return "";
	return str.substr(k, l-k+1);
}

static int ifdef(string str)
{
	str = but_spe_char(str);
	if (def.find(str) == def.end()) return 0;
	return 1;
}

// =============================

// 去除defined S、defined (S),用0、1来表示
static string butDefined(string str)
{
	int i,j,k,l,s,t,len=str.length();
	for (i=0;i<len;i++)
	{
		if ((i == 0 || spe_char(str[i-1]) || str[i-1] == '(' || str[i-1] == '!') && str.substr(i, 7) == "defined") {
			s = i; i += 7; k = 0;
			while (i < len && (spe_char(str[i]) || str[i] == '(')) {
				i++;
				if (str[i] == '(') k++;
			}
			j = i;
			while (i < len && !spe_char(str[i])) i++;
			l = i-1;
			if (j == l) return "0";
			while (i < len && (spe_char(str[i]) || (k > 0 && str[i] == ')'))) {
				i++;
				if (str[i] == ')') k--;
			}

			if (ifdef(str.substr(j, l-j))) k = 1; else k = 0;
			for (t = s; t < i; t++) str[t] = ' ';
			str[s] = (char)(k+48);
		}
	}
	return str;
}

// 代码中出现名为tmp参数为list的函数进行替换，因为被define过。
static string strreplacenow(string tmp, vector<string> list)
{
	int i,j,k,l;
	string ret = "", val = defdetail[def[tmp]].val;
	vector<string> para = defdetail[def[tmp]].para;
	int len = val.length();
	for (i=0;i<len;i++)
	{
		if (letter_(val[i])) {
			j = i;
			while (i < len && (letter_(val[i]) || digit(val[i]))) i++;
			string str1 = val.substr(j, i-j);
			for (k=0;k<para.size();k++)
				if (para[k] == str1) {
					ret += list[k];
					break;
				}
			if (k >= para.size()) {
				ret += str1;
			}
			i--;
		} else {
			ret += val[i];
		}
	}
	return ret;
}

// 先看是不是函数，如果是则将参数分解出来，再调用strreplacenow进行替换
static string repDefineFunction(string str, int len, int &i, string tmp)
{
	while (i < len && spe_char(str[i])) i++;

	if (str[i] != '(') {
		return tmp;
	}
	string str1 = "(";
	int k = i;
	i++;
	int cou0 = 1;
	while (i+1 < len) {
		str1 += str[i];
		if (str[i] == '(') {
			cou0++;
		} else if (str[i] == ')') {
			cou0--;
			if (cou0 == 0) {
				i++;
				break;
			}
		}
		i++;
	}
	if (cou0 != 0) {
		return tmp;
	}

	vector<string> list;
	list.clear();
	int j;
	k = 1;
	for (j=1;j<str1.length();j++) {
		if (str1[j] == ',' || j == str1.length()-1) {
			if (k == j) {
				return tmp;
			}
			list.push_back(str1.substr(k, j-k));
			j++;
			k = j;
		}
	}

	if (list.size() != defdetail[def[tmp]].para.size()) {
		return tmp;
	} else {
		return strreplacenow(tmp, list);
	}
}

// 用define替换
static string repDefine(string str)
{
	int i,j,k,len=str.length();
	string ret = "";
	for (i=0;i<len;i++)
	{
		if (letter_(str[i])) {
			j = i;
			while (i < len && (letter_(str[i]) || digit(str[i]))) i++;
			string tmp = str.substr(j, i-j);
			if (def.find(tmp) == def.end()) { // 没有匹配的define
				ret += tmp;
			} else {
				if (defdetail[def[tmp]].para.size() == 0) { // 没有参数
					ret += defdetail[def[tmp]].val;
				} else { // 有参数
					ret += repDefineFunction(str, len, i, tmp);
				}
			}
			i--;
		} else {
			ret += str[i];
		}
	}
	if (ret != str) {
		return repDefine(ret); // 递归替换，如：#define ABC CD  #define CD 123
	} else {
		return ret;
	}
}

static int isrightchar(string str, int i, int len)
{
	if (i+2 <= len) {
		string tmp = str.substr(i, 2);
		if (tmp == "&&" || tmp == "||" || tmp == ">=" || tmp == "<=" || tmp == "==" || tmp == "!=" || tmp == "<<" || tmp == ">>") {
			return 2;
		}
	}
	if (i+1 <= len) {
		if (str[i] == '!' || str[i] == '>' || str[i] == '<' || str[i] == '+' || str[i] == '-' || str[i] == '*' || str[i] == '/' || str[i] == '%' || str[i] == '~' || str[i] == '&' || str[i] == '^' || str[i] == '|') {
			return 1;
		}
	}
	return 0;
}

// 将#if的表达式按照 (、)、运算符、数字等分开
static int explodeCalStr(string str, string v[100], int &sz)
{
	int i,j,k,l,len=str.length();
	sz = 0;

	for (i=0;i<len;i++)
	{
		if (str[i] == '(' || str[i] == ')') {
			v[sz++] = str.substr(i, 1);
		} else if ((l = isrightchar(str, i, len))) {
			v[sz++] = str.substr(i, l);
			i += l-1;
		} else if (digit(str[i])) {
			k = i;
			while (i < len && digit(str[i])) i++;
			v[sz++] = str.substr(k, i-k);
			i--;
		} else if (spe_char(str[i])) { // just do nothing
		} else {
			return 0;
		}
	}
	return 1;
}

static int getLevel(string op)
{
	if (op == "!" || op == "~") return 1;
	if (op == "*" || op == "/" || op == "%") return 2;
	if (op == "+" || op == "-") return 3;
	if (op == "<<" || op == ">>") return 4;
	if (op == ">" || op == ">=" || op == "<" || op == "<=") return 5;
	if (op == "==" || op == "!=") return 6;
	if (op == "&") return 7;
	if (op == "^") return 8;
	if (op == "|") return 9;
	if (op == "&&") return 10;
	if (op == "||") return 11;
	return 0;
}

static int arg1_op_arg2(string st[100], int &szs, int level)
{
	if (szs < 2) return 0;
	string arg1, op, arg2;
	int val1, val2, ret, getlevel;

	op = st[szs-2];
	getlevel = getLevel(op);
    if (getlevel == 0) return 0;
	if (getlevel > level) return 0;

	arg2 = st[szs-1];
	if (!(digit(arg2[0]) || (arg2[0] == '-' && digit(arg2[1])))) return 0;
	val2 = sTi(arg2);

	if (op == "-") {
		ret = -val2;
		st[szs-2] = iTs(ret);
		szs --;
		return 1;
	}
	if (op == "!") {
		ret = !val2;
		st[szs-2] = iTs(ret);
		szs --;
		return 1;
	}
	if (op == "~") {
		ret = ~val2;
		st[szs-2] = iTs(ret);
		szs --;
		return 1;
	}

	if (szs < 3) return 0;
	arg1 = st[szs-3];
	val1 = sTi(arg1);
	if (!(digit(arg1[0]) || (arg1[0] == '-' && digit(arg1[1])))) return 0;

	if (op == "+") {
		ret = sTi(arg1) + sTi(arg2);
	} else if (op == "-") {
		ret = sTi(arg1) - sTi(arg2);
	} else if (op == "*") {
		ret = sTi(arg1) * sTi(arg2);
	} else if (op == "/") {
		ret = sTi(arg1) / sTi(arg2);
	} else if (op == "%") {
		ret = sTi(arg1) % sTi(arg2);
	} else if (op == "<<") {
		ret = sTi(arg1) << sTi(arg2);
	} else if (op == ">>") {
		ret = sTi(arg1) >> sTi(arg2);
	} else if (op == "&") {
		ret = sTi(arg1) & sTi(arg2);
	} else if (op == "^") {
		ret = sTi(arg1) ^ sTi(arg2);
	} else if (op == "|") {
		ret = sTi(arg1) | sTi(arg2);
	} else if (op == ">") {
		ret = sTi(arg1) > sTi(arg2);
	} else if (op == ">=") {
		ret = sTi(arg1) >= sTi(arg2);
	} else if (op == "<") {
		ret = sTi(arg1) < sTi(arg2);
	} else if (op == "<=") {
		ret = sTi(arg1) <= sTi(arg2);
	} else if (op == "==") {
		ret = sTi(arg1) == sTi(arg2);
	} else if (op == "!=") {
		ret = sTi(arg1) != sTi(arg2);
	} else if (op == "&&") {
		ret = sTi(arg1) && sTi(arg2);
	} else if (op == "||") {
		ret = sTi(arg1) || sTi(arg2);
	}
	st[szs-3] = iTs(ret);
	szs -= 2;
	return 1;
}

// 前面优先级比当前低的都计算掉
static int caltop(string st[100], int &szs, int level)
{
	if (level > 1) {
		while (caltop(st, szs, level-1));
	}
	return arg1_op_arg2(st, szs, level);
}

// 计算栈
static int calstack(string v[100], int sz)
{
	int i,j,k,l, szs=0;
	string st[100];
	for (i=0;i<sz;i++)
	{
		if (v[i] == "(" || digit(v[i][0])) {
			st[szs++] = v[i];
		} else if (v[i] == ")") {
			caltop(st, szs, 11);
			if (szs >= 2 && st[szs-2] != "(") return 0;
			if (szs >= 2) {
				st[szs-2] = st[szs-1];
				szs--;
			}
		} else {
			l = getLevel(v[i]);
			caltop(st, szs, l);
			st[szs++] = v[i];
		}
	}
	// 最后会剩一个优先级将序的stack，再把它计算掉
	caltop(st, szs, 11);
	if (szs == 1 && sTi(st[szs-1]) != 0) return 1;
	return 0;
}

// 开始计算 #if 的表达式
static int califexp(string exp)
{
	string v[100];
	int i,ret,sz;
	exp = butDefined(exp); // 去除defined S, defined (S)
	exp = repDefine(exp);  // 替换define表达式
	ret = explodeCalStr(exp, v, sz); // 将表达式按照 (、)、运算符、数字等分开
	if (ret == 0) return ret;
	ret = calstack(v, sz); // 计算栈
	return ret;
}

// ===================================

// 将#define的exp按照数字、括号、字符串等等拆开
static int explodeDefStr(string str, string v[300], int end[300], int &sz)
{
	int i,j,k,l,len=str.length();
	sz = 0;

	for (i=0;i<len;i++)
	{
		if ((l = isrightchar(str, i, len))) {
			end[sz] = i+l;
			v[sz++] = str.substr(i, l);
			i += l-1;
		} else if (digit(str[i])) {
			k = i;
			while (i < len && digit(str[i])) i++;
			if (sz > 0 && v[sz-1] == "-") {
				end[sz-1] = i;
				v[sz-1] += str.substr(k, i-k);
			} else {
				end[sz] = i;
				v[sz++] = str.substr(k, i-k);
			}
			i--;
		} else if (spe_char(str[i])) { // just do nothing
		} else if (letter_(str[i])) {
			v[sz] = "";
			while (i < len && (letter_(str[i]) || digit(str[i]))) {
				v[sz] += str[i];
				i++;
			}
			end[sz] = i;
			sz++;
			i--;
		} else if (i+3 <= len && str.substr(i, 3) == "...") {
			end[sz] = i+3;
			v[sz++] = str.substr(i, 3);
			i += 2;
		} else if (str[i] == '(' || str[i] == ')' || str[i] == ',' || str[i] == '=' || str[i] == '.' || str[i] == '&' || str[i] == '|' || str[i] == '[' || str[i] == ']' || str[i] == '?' || str[i] == ':') {
			end[sz] = i+1;
			v[sz++] = str.substr(i, 1);
		} else if (str[i] == '{' || str[i] == '}' || str[i] == '#' || str[i] == '"' || str[i] == '\\' || str[i] == ';') {
			end[sz] = i+1;
			v[sz++] = str.substr(i, 1);
		} else {
			return 0;
		}
	}
	return 1;
}

/*
define func(para); para DFA

               <--------
               |       |
               |       | ident
 (      ident  V  ,    |
---> 1 ------> 2 ----> 3
               |
               | )
               V
               4
               |
               V
*/

// #define exp  检查exp是否合法，合法则存起来，否则返回错误
static string defineStr(string str)
{
	int sz,i,j,k,end[300];
	string v[300];
	k = explodeDefStr(str, v, end, sz);
	if (k == 0) {
		return "define str ERROR";
	}
	if (sz < 1 || !letter_(v[0][0])) {
		return "define A, A must be string";
	}
	vector<string> list;
	list.clear();
	i = 0;
	if (sz > 1 && v[1] == "(" && str[end[0]] == '(') {
		int cou = 1;
		int status = 1;
		i = 2;
		while (i < sz) {
			if (v[i] == ")") {
				cou--;
				status = 4;
				if (cou == 0) break;
			} else if (v[i] == "(") {
				if (status != 2 && status != 3) {
					return "define A(i( : second ( must after indet OR ,";
				}
				status = 1;
			} else if (letter_(v[i][0]) || v[i] == "..." || digit(v[i][0]) || v[i][0] == '-') {
				if (status != 1 && status != 3) {
					return "define A(i,i) : i must after ( OR ,";
				}
				status = 2;
				list.push_back(v[i]);
			} else if (v[i] == ",") {
				if (status !=2 && status != 4) {
					return "define A(i,f(),) : , must after i OR )";
				}
				status = 3;
			} else {
				return "define A(para), para must , ident";
			}
			i++;
		}
		if (cou > 0) {
			return "define A(para), ')' expect";
		}
	}
	def[v[0]] = def_line;
	defdetail[def_line] = DefDetail(v[0], str.substr(end[i]), list);
	def_line++;
	return "";
}

// =======================================

static void readCode(string codefile, vector<string> &code)
{
	code.clear();
	fstream fin(codefile.c_str());
	string tmp;
	while (getline(fin, tmp)) {
		code.push_back(tmp);
	}
}

// 去掉注释
static void solve(vector<string> &code)
{
	int i,j,k,l,ok,x,y;
	for (i=0;i<code.size();i++)
	{
		for (j=0;j+1<code[i].length();j++) if (code[i][j] == '/' && code[i][j+1] == '/')
		{
			code[i] = code[i].substr(0, j);
			break;
		}
	}
	for (i=0;i<code.size();i++)
	{
		for (j=0;j+1<code[i].length();j++) if (code[i][j] == '/' && code[i][j+1] == '*')
		{
			ok = 0;
			for (k=i;k<code.size();k++) {
				for (l=0;l+1<code[k].length();l++)
				{
					if (k == i && l <= j+1) continue;
					if (code[k][l] == '*' && code[k][l+1] == '/') {
						for (x=i;x<=k;x++) {
							for (y=0;y<code[x].length();y++) {
								if (x == i && y < j) continue;
								if (x == k && y > l+1) break;
								code[x][y] = '\0';
							}
						}
						ok = 1; x = k; y = l;
						break;
					}
				}
				if (ok == 1) break;
			}
			if (ok == 1) {
				i = x; j = y;
			} else if (ok == 0) {
				i = code.size();
			}
		}
	}
}

static void reset(vector<string> &code, int line, int pos, string str)
{
	int i;
	for (i=0;i<pos;i++) code[line][i] = ' ';
	for (i=0;i<str.length();i++) code[line][i] = str[i];
}

static void resetLine(vector<string> &code, int i)
{
	int l = code[i].length();
	int j = 0;
	while (j < l && spe_char(code[i][j])) j++;
		if (code[i][j] == '#') {
		j++;
		while (j < l && spe_char(code[i][j])) j++;
		if (code[i].substr(j, 5) == "ifdef") {
			if (spe_char(code[i][j+5])) {
				reset(code, i, j+5, "#ifdef");
			}
		} else if (code[i].substr(j, 6) == "ifndef") {
			if (spe_char(code[i][j+6])) {
				reset(code, i, j+6, "#ifndef");
			}
		} else if (code[i].substr(j, 2) == "if") {
			if (spe_char(code[i][j+2])) {
				reset(code, i, j+2, "#if");
			}
		} else if (code[i].substr(j, 4) == "elif") {
			if (spe_char(code[i][j+4])) {
				reset(code, i, j+4, "#elif");
			}
		} else if (code[i].substr(j, 4) == "else") {
			if (spe_char(code[i][j+4])) {
				reset(code, i, j+4, "#else");
			}
		} else if (code[i].substr(j, 5) == "endif") {
			if (spe_char(code[i][j+5])) {
				reset(code, i, j+5, "#endif");
			}
		} else if (code[i].substr(j, 6) == "define") {
			if (spe_char(code[i][j+6])) {
				reset(code, i, j+6, "#define");
			}
		} else if (code[i].substr(j, 5) == "undef") {
			if (spe_char(code[i][j+5])) {
				reset(code, i, j+5, "#undef");
			}
		} else if (code[i].substr(j, 6) == "pragma") {
			if (spe_char(code[i][j+6])) {
				reset(code, i, j+6, "#pragma");
			}
			code[i] = "";
		} else if (code[i].substr(j, 7) == "include") {
			if (spe_char(code[i][j+7])) {
				reset(code, i, j+7, "#include");
			}
		}
	}
}

static string readSource(string codefile)
{
	int i,j,k,l,jj,kk,ii;

	vector<string> code;

	readCode(codefile, code);
	// 去掉注释
	solve(code);

	int sz;
	vector<PStack> stack;

	stack.clear();
	stack.push_back( PStack(0, 1, 1) );

	int includeFrom;

	for (i=0;i<code.size();i++)
	{
		defdetail[def["__LINE__"]].val = iTs(i+1);
		l = code[i].length();
		ii = i+1;
		while (ii < code.size() && code[i].substr(max(0, l-1), 1) == "\\") { // 最后为 \\ 的接到下一行
			code[i] = code[i].substr(0, l-1) + code[ii];
			code[ii] = "";
			l = code[i].length();
			ii++;
		}

		resetLine(code, i); // 将#XX开头的都移到行首

		sz = stack.size();
		if (code[i].substr(0, 8) == "#include") {
			if (stack[sz-1].nowShow != 1) {
				code[i] = "";
				continue;
			}
			j = 8;
			while (j < code[i].length() && spe_char(code[i][j])) {
				j++;
			}
			k = code[i].length()-1;
			while (k > j && spe_char(code[i][k])) {
				k--;
			}
			if (j<k && code[i][j] == '"' && code[i][k] == '"') {
				includeFrom = 0;
			} else if (j<k && code[i][j] == '<' && code[i][k] == '>') {
				includeFrom = 1;
			} else {
				error_line = i;
				error_file = codefile;
				return "#include FILE: FILE error";
			}
			string includeTmp = code[i].substr(j+1, k-j-1);
			string includeNow;
			for (j=includeFrom;j<includeSize;j++) {
				includeNow = includeFile[j] + includeTmp;
				if (access(includeNow.c_str(), R_OK) == 0) {
					defdetail[def["__FILE__"]].val = "\""+includeNow+"\"";
					string tmps = readSource(includeNow); // 遇到#include就要递归输入
					defdetail[def["__FILE__"]].val = "\""+codefile+"\"";
					if (tmps.length() > 0) return tmps;
					break;
				}
			}
			if (j >= includeSize) {
				error_line = i;
				error_file = codefile;
				return includeTmp+" no such file";
			}
			code[i] = "";
		} else if (code[i].substr(0, 7) == "#define") {
			if (stack[sz-1].nowShow != 1) {
				code[i] = "";
				continue;
			}
			l = code[i].length();
			j = 8;
			string retstr = defineStr(code[i].substr(8));
			if (retstr.length() > 0) {
				return retstr;
			}
			code[i] = "";
		} else if (code[i].substr(0, 6) == "#undef") {
			if (stack[sz-1].nowShow != 1) {
				code[i] = "";
				continue;
			}
			l = code[i].length();
			j = 7;
			while (j < l && spe_char(code[i][j])) j++;
			k = j;
			while (k < l && !spe_char(code[i][k])) k++;
			if (k == j) return "no undef string";

			if (def.find(code[i].substr(j, k-j)) != def.end()) {
				def.erase(code[i].substr(j, k-j));
			}
			code[i] = "";
		} else if (code[i].substr(0, 6) == "#ifdef") {
			stack.push_back( PStack(1, stack[sz-1].nowShow, 0) );
			sz++;
			if (stack[sz-1].preShow == 1 && ifdef(code[i].substr(7)) == 1) {
				stack[sz-1].nowShow = 1;
			}
			code[i] = "";
		} else if (code[i].substr(0, 7) == "#ifndef") {
			stack.push_back( PStack(1, stack[sz-1].nowShow, 0) );
			sz++;
			if (stack[sz-1].preShow == 1 && ifdef(code[i].substr(8)) == 0) {
				stack[sz-1].nowShow = 1;
			}
			code[i] = "";
		} else if (code[i].substr(0, 3) == "#if") {
			stack.push_back( PStack(1, stack[sz-1].nowShow, 0) );
			sz++;
			if (stack[sz-1].preShow == 1 && code[i].length() > 4 && califexp(code[i].substr(4)) == 1) {
				stack[sz-1].nowShow = 1;
			}
			code[i] = "";
		} else if (code[i].substr(0, 5) == "#elif") {
			if (sz <= 0 || stack[sz-1].status > 2) {
				error_line = i;
				error_file = codefile;
				return "#elif must after #if OR #elif";
			}
			stack.push_back( PStack(2, stack[sz-1].preShow, stack[sz-1].nowShow) );
			if (stack[sz].nowShow > 0) stack[sz].nowShow ++;
			sz++;

			if (stack[sz-1].preShow == 1 && stack[sz-1].nowShow == 0 && code[i].length() > 6 && califexp(code[i].substr(6))) {
				stack[sz-1].nowShow = 1;
			}
			code[i] = "";
		} else if (code[i].substr(0, 5) == "#else") {
			if (sz <= 0 || (stack[sz-1].status >= 3)) {
				error_line = i;
				error_file = codefile;
				return "#else must after #if OR #elif OR #ifdef OR #ifndef";
			}
			stack.push_back( PStack(3, stack[sz-1].preShow, stack[sz-1].nowShow) );
			if (stack[sz].nowShow > 0) stack[sz].nowShow ++;
			sz++;

			if (stack[sz-1].preShow == 1 && stack[sz-1].nowShow == 0) {
				stack[sz-1].nowShow = 1;
			}
			code[i] = "";
		} else if (code[i].substr(0, 6) == "#endif") {
			if (sz > 0 && stack[sz-1].status < 4) {
				while (sz > 0) {
					j = stack[sz-1].status;
					sz--;
					stack.pop_back();
					if (j == 1) break;
				}
			} else {
				error_line = i;
				error_file = codefile;
				return "#endif must after #if";
			}
			code[i] = "";
			// nothing
		} else {
			if (stack[sz-1].nowShow != 1) {
				code[i] = "";
			} else {
				code[i] = repDefine(code[i]);
				source.push_back( Source(code[i], codefile, i+1) );
			}
		}
	}
	if (stack.size() != 1) {
		error_line = i;
		error_file = codefile;
		return "need #endif";
	}
	return "";
}

int preprocessor_main()
{
	preprocessor_out	= "./data/preprocessor.out";

	preprocessor_in = filename;

	int i, k, success = 0;
	string ss;

	def.clear();
	def_line = 0;
	def["__STDC__"] = def_line;
	defdetail[def_line] = DefDetail("__STDC__", "1");
	def_line++;

	def["__FILE__"] = def_line;
	defdetail[def_line] = DefDetail("__FILE__", "\""+preprocessor_in+"\"");
	def_line++;

	def["__LINE__"] = def_line;
	defdetail[def_line] = DefDetail("__LINE__", "");
	def_line++;

	def["__ASSERT_FUNCTION"] = def_line;
	defdetail[def_line] = DefDetail("__ASSERT_FUNCTION", "\"\"");
	def_line++;

	source.clear();

	// 输入
	string ret = readSource(preprocessor_in);

	if (ret.length() > 0) {
		printf("preprocessor error: %s: %d: %s\n", error_file.c_str(), error_line+1, ret.c_str());
		success = 1;
	} else {
		success = 0;
	}

	// 输出
	if (OUT_OTHER_FILE == 1) {
		FILE *fp = fopen(preprocessor_out.c_str(), "w");
		for (i=0;i<source.size();i++) {
			if (strlen(source[i].code.c_str()) > 0) {
				fprintf(fp, "%s\n", source[i].code.c_str());
			}
		}
		fclose(fp);
	}
	return success;
}

