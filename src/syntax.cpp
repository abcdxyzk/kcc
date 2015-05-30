#include "kcc.h"

SyntaxTree *subtr;

// 语法树的树根
SyntaxTree *headSubtr;

struct Item {
	int grammar_id, pos, forward;
	Item(){}
	Item(int _grammar_id, int _pos, int _forward) {
		grammar_id = _grammar_id; pos = _pos; forward = _forward;
	}
	bool operator<(Item bb) const
	{
		if (grammar_id < bb.grammar_id || grammar_id == bb.grammar_id && pos < bb.pos
			|| grammar_id==bb.grammar_id && pos==bb.pos && forward < bb.forward)
			return 1;
		return 0;
	}
} ;

struct Action {
	int to,sta;
	Action(){}
	Action(int _to, int _sta) {
		to = _to; sta = _sta;
	}
} ;

struct Go {
	int from,symbol,to;
	Go(){}
	Go(int _from, int _symbol, int _to) {
		from = _from; symbol = _symbol; to = _to;
	}
} ;

struct SStack {
	int state;
	string val;
	vector<string> list;
	SyntaxTree *subtr;
	SStack() {};
	SStack(int _state, string _val) {
		state = _state; val = _val;
		list.clear();
	}
};

struct SType_name {
	string name;
	SType_name(){};
	SType_name(string _name) {
		name = _name;
	}
};


static int grammar_number,items_number;

// init in INPUT_GRAMMAR
static vector<string> grammar[MAX_GRAMMAR_NUMBER];				// grammar[i] = statement(<program> ::= <block> .)
static vector<string> end_symbol, exp_symbol;					// end_symbol = int < ; ...  exp_symbol = <program> <block> ...
static vector<string> all_symbol;								// all_symbol = end_symbol + exp_symbol
static map<string, int> mm;									// mm[ all_symbol[i] ] = i;

// init in MAKE_FIRST
static vector<int>  first[MAX_ALL_SYMBOL_NUMBER];				// if (all_symbol[j] in all_symbol[i]'s first) first[i][j] = 1

// init in MAKE_VINIT
static vector<int> vgrammar[MAX_GRAMMAR_NUMBER];				// vgrammar[i] = list(j) where mm[grammar[j][0]] = i;
static vector<int> vfirst[MAX_ALL_SYMBOL_NUMBER];				// vfirst[i] = list(j) where fitst[i][j] == 1
static vector<int> vexp_first[MAX_GRAMMAR_NUMBER][MAX_GRAMMAR_STATEMENT_LENGTH][MAX_ALL_SYMBOL_NUMBER];	// if (expand_item(Item)) vexp_first[Item] = exp_first;
static vector<int> vsymbol[MAX_GRAMMAR_NUMBER];				// vsymbol[i][j] = mm[grammar[i][j]]

// init in MAKE_ITEMS
static vector<Item> items[MAX_ITEMS_NUMBER];					// items[i] = a item
static vector<Go> go;											// Action => go[i] = (symbol==end_symbol?push:goto)
static vector<int> X[MAX_ALL_SYMBOL_NUMBER];					// i = all_symbol; X[i] = list(j) where mm[ grammar[ items[][j].grammar_id ][ items[][j].pos ] ] = i;
static int now, check[MAX_GRAMMAR_NUMBER][MAX_GRAMMAR_STATEMENT_LENGTH][MAX_ALL_SYMBOL_NUMBER];	// now = making items[id];  check[i][j][k] = (Item(i,j,k)==appear in items[id])?(=now):(!=now)
static int appear[MAX_GRAMMAR_NUMBER][MAX_GRAMMAR_STATEMENT_LENGTH][MAX_ALL_SYMBOL_NUMBER];		// boo[i][j][k] = (Item(i,j,k)==appear before)1:0

// init in MAKE_ACTION
static vector<Action> action[MAX_ITEMS_NUMBER];				// action[i][j].sta = 0 => push; action[i][j].sta = 1 && symbol==end_symbol => pop; action[i][j].sta = 1 && symbol==exp_symbol => goto;

// used in MAKE_SOLVE
static vector<SStack> sstack;									// sstack[top++] = push  sstack[top-len+3 -1] = pop; sstack[top-1] = goto
static vector<SType_name> type_name;

// init in main   used to choose grammar
static string action_name, number, ident, stringg, syntax_out;

static void INPUT_GRAMMAR()
{
	//fstream fin(grammar_name.c_str());
// 对于grammar.init，要求文法语句不为空，最后加一空行，该空行不会计入文法
#include "./grammar.init"
	istringstream is;

	string tmp, str;
	int i,j,k, ok;

	grammar_number = 1;

	//while (getline(fin, str))
	for (i=0;;i++)
	{
		str = grammar_init[i]; if (str.length() == 0) break;
		is.clear(); is.str(str); ok = 0;
		while (is>>tmp) {
			grammar[grammar_number].push_back(tmp);
			ok = 1;
		}
		if (ok==1) grammar_number++;
	}

	grammar[0].clear();
	grammar[0].push_back("<S>");
	grammar[0].push_back("::=");
	grammar[0].push_back(grammar[1][0]);

	mm.clear();
	end_symbol.clear(); exp_symbol.clear();
	all_symbol.clear();

	for (i=0;i<grammar_number;i++)
	{
		for (j=0;j<grammar[i].size();j++)
		{
			k = grammar[i][j].length();
			if ((grammar[i][j][0] == '<' && grammar[i][j][k-1] == '>' && k > 2) && grammar[i][j] != "<ident>" && grammar[i][j] != "<number>" && mm.find(grammar[i][j]) == mm.end()) {
				exp_symbol.push_back(grammar[i][j]);
				mm[grammar[i][j]] = 1;
			} else if (grammar[i][j] != "::=" && mm.find(grammar[i][j]) == mm.end()) {
				end_symbol.push_back(grammar[i][j]);
				mm[grammar[i][j]] = 1;
			}
		}
	}

	k = 1; for (i=0;i<end_symbol.size();i++) if (end_symbol[i] == null) k = 0;
	if (k == 1) end_symbol.push_back(null);
	end_symbol.push_back("$");

	mm.clear(); k = 0;
	for (i=0;i<end_symbol.size();i++) { mm[end_symbol[i]] = k++; all_symbol.push_back(end_symbol[i]); }
	for (i=0;i<exp_symbol.size();i++) { mm[exp_symbol[i]] = k++; all_symbol.push_back(exp_symbol[i]); }
}

// 计算每个符号的first集，即计算每个符号的头符号级
// 1. 终结符的first集等于本身
// 2. 非终结符的first集 FIRST(A) => {a|A=>*aw}

// 对于 <A> ::= ; 这样的文法，null就是<A>的一个头符号
// 对于 <A> ::= <B><C><D>...
//      若b是<B>的头符号，那么b也是<A>的头符号
//      若null是<B>的头符号，即<B> => null，<B>可以为空，那么<C>的头符号也是<A>的头符号
//      计算到<X> !=> null

static void MAKE_FIRST()
{
	int i,j,k,ok;
	for (i=0;i<all_symbol.size();i++) {
		first[i].clear();
		for (j=0;j<all_symbol.size();j++) first[i].push_back(0);
	}

	// 终结符的first集等于本身
	for (i=0;i<end_symbol.size();i++) first[i][i] = 1;

	ok = 1;
	while (ok == 1)
	{
		ok = 0;
		for (i=0;i<grammar_number;i++)
		{
			if (grammar[i].size() == 3 && grammar[i][2] == null)
			{
				if (first[mm[grammar[i][0]]][mm[null]] == 0) {
						ok = 1;
						first[mm[grammar[i][0]]][mm[null]] = 1; // 对于 <A> ::= ; 这样的文法，null就是<A>的一个头符号
				}
				continue;
			}

			k = 2;
			while (true)
			{
				for (j=0;j<all_symbol.size();j++) {
					if (first[mm[grammar[i][k]]][j] == 1) { // 若b是<B>的头符号，那么b也是<A>的头符号
						if (first[mm[grammar[i][0]]][j] == 0) {
							ok = 1;
							first[mm[grammar[i][0]]][j] = 1;
						}
					}
				}
				// 计算到<X> !=> null
				if (first[mm[grammar[i][k]]][mm[null]] == 0) break;
				k++;
				if (k >= grammar[i].size()) break;
			}
		}
	}

	for (i=0;i<end_symbol.size();i++) first[i][i] = 1;
}

// 优化用的，用够极大的提高速度
static void MAKE_VINIT()
{
	int i,j;
	for (i=0;i<all_symbol.size();i++) {
		vfirst[i].clear();
		for (j=0;j<all_symbol.size();j++) if (first[i][j] == 1) vfirst[i].push_back(j);
	}

	for (i=0;i<all_symbol.size();i++) {
		vgrammar[mm[all_symbol[i]]].clear();
		for (j=0;j<grammar_number;j++) {
			if (grammar[j][0] == all_symbol[i])
				vgrammar[mm[all_symbol[i]]].push_back(j);
		}
	}
	for (i=0;i<grammar_number;i++) {
		vsymbol[i].clear();
		for (j=0;j<grammar[i].size();j++) {
			if (mm.find(grammar[i][j]) == mm.end()) {
				vsymbol[i].push_back(-1);
			} else {
				vsymbol[i].push_back(mm[grammar[i][j]]);
			}
		}
	}
}

static int tmp_in_items(vector<Item> tmp)
{
	int i,j;
	for (i=0;i<items_number;i++)
	if (items[i].size() == tmp.size())
	{
		for (j=0;j<items[i].size();j++) {
			if (items[i][j].grammar_id != tmp[j].grammar_id || items[i][j].pos != tmp[j].pos || items[i][j].forward != tmp[j].forward)
				break;
		}
		if (j == items[i].size()) return i;
	}
	return -1;
}

// grammar_id，从pos开始的的first集
static void expand_item(Item tmp, vector<int> &exp_first)
{
	int pos = tmp.pos, i, gram;

	if (appear[tmp.grammar_id][tmp.pos][tmp.forward] == 1) {
		exp_first = vexp_first[tmp.grammar_id][tmp.pos][tmp.forward];
		return;
	}

	exp_first.clear();

	while (true)
	{
		if (pos+1 >= grammar[tmp.grammar_id].size()) {//printf("%d %d %d  %d   %d\n",tmp.grammar_id,tmp.pos,tmp.forward,pos,grammar[tmp.grammar_id].size());
			exp_first.push_back(tmp.forward);
			break;
		}

		gram = vsymbol[tmp.grammar_id][pos+1];

		if (gram < end_symbol.size()) {
			exp_first.push_back(gram);
			break;
		} else {
			for (i=vfirst[gram].size()-1;i>=0;i--) {
				exp_first.push_back(vfirst[gram][i]);
			}
			if (first[ vsymbol[tmp.grammar_id][pos] ][mm[null]] == 1) {
				pos++;
			} else {
				break;
			}
		}
	}
	appear[tmp.grammar_id][tmp.pos][tmp.forward] = 1;
	vexp_first[tmp.grammar_id][tmp.pos][tmp.forward] = exp_first;
}

static void CLOSURE(vector<Item> &item)
{
	int i,j,jj,k, gram;
	vector<int> exp_first;
	for (i=0;i<item.size();i++)
	if (item[i].pos+1 <= grammar[ item[i].grammar_id ].size())
	{
		gram = vsymbol[ item[i].grammar_id ][ item[i].pos ];

		for (jj=vgrammar[ gram ].size()-1; jj>=0; jj--)
		{
			j = vgrammar[ gram ][jj];
			expand_item(item[i], exp_first); // 计算item[i]中对应的grammar_id，从pos开始的的first集，存于exp_first。

			for (k=exp_first.size()-1;k>=0;k--) {
				if (check[j][2][exp_first[k]] != now) {
					item.push_back( Item(j, 2, exp_first[k]) );
					check[j][2][exp_first[k]] = now;
				}
			}
		}
	}
}

// check[grammar_id][pos][forward] = now; 表示Item(grammar_id, pos, forward)这个item已经加进了当前正要计算的item集(正在计算的不一定能够加到items里)。now不停地加，这样check数组就不用每次清零了
static void GOTO(int i, int j, vector<Item> &tmp)
{
	int k,kk;
	tmp.clear(); now++;
	for (kk=X[j].size()-1;kk>=0;kk--)
	{
		k = X[j][kk];

		tmp.push_back(items[i][k]);
		tmp[tmp.size()-1].pos++;
		check[items[i][k].grammar_id][items[i][k].pos+1][items[i][k].forward] = now;
	}
}

// grammar[0] => <S> ::= <translation_unit>$
// 对于每个项集，若遇到symbol[j]能产生新的项集，那么加一条go[i][j][status] = 1;
static void MAKE_ITEMS()
{
	int i,j,next_state;
	vector<Item> tmp;

	int len=0; for (i=0;i<grammar_number;i++) if (grammar[i].size() > len) len = grammar[i].size();
	for (i=0;i<grammar_number;i++) {
		for (j=0;j<=len;j++)
			for (int k=0;k<all_symbol.size();k++) {
				check[i][j][k] = -1; appear[i][j][k] = 0;
			}
	}
	now = -1;

	items_number = 1;
	items[0].clear(); now++;
	items[0].push_back( Item(0,2,mm["$"]) ); check[0][2][mm["$"]] = now; //check[2*1000+mm["$"]] = 1;
	CLOSURE(items[0]);

	sort(items[0].begin(), items[0].end());

	go.clear();
	for (i=0;i<items_number;i++)
	{

		for (j=all_symbol.size()-1;j>=0;j--) X[j].clear();
		for (j=items[i].size()-1;j>=0;j--) if (items[i][j].pos < grammar[items[i][j].grammar_id].size())
			X[vsymbol[items[i][j].grammar_id][items[i][j].pos]].push_back(j);

		for (j=all_symbol.size()-1;j>=0;j--) if (X[j].size() > 0)
		{
			GOTO(i, j, tmp); // 挑出i项集中每个项，pos下一个是symbol[j]的项集。 <A> ::=> a<B>c，
			if (tmp.size() == 0) continue;

			CLOSURE(tmp);
			sort(tmp.begin(), tmp.end());

			next_state = tmp_in_items(tmp);

			if (next_state == -1) {
				items[items_number] = tmp;
				items_number++;
				next_state = items_number-1;
			}

			go.push_back( Go(i, j, next_state) );
		}
	}
}

static void MAKE_ACTION()
{
	int i,j;

	for (i=0;i<items_number;i++) {
		action[i].clear();
		for (j=0;j<all_symbol.size();j++) action[i].push_back( Action(-1, -1) );
	}

	for (i=0;i<items_number;i++)
	{
		for (j=0;j<items[i].size();j++)
		{
			if (items[i][j].pos == grammar[items[i][j].grammar_id].size() ||
			   (items[i][j].pos+1 == grammar[items[i][j].grammar_id].size() && grammar[items[i][j].grammar_id][items[i][j].pos] == null)) // 是规约
			{
				action[i][items[i][j].forward].sta = 1;
				action[i][items[i][j].forward].to = items[i][j].grammar_id;
			}
		}
	}
	for (i=0;i<go.size();i++) // 是移入
	{
		action[go[i].from][go[i].symbol].to = go[i].to;
		action[go[i].from][go[i].symbol].sta = 0;
	}

	FILE *fp = fopen(action_name.c_str(), "w");

	fprintf(fp, "int action_row = %d, action_col = %d;\r\n", items_number, (int)(all_symbol.size()));
	fprintf(fp, "int action_init[%d][%d]={\r\n", items_number, 2*(int)(all_symbol.size()));
	for (i=0;i<items_number;i++)
	{
		fprintf(fp, "{");
		for (j=0;j<all_symbol.size();j++) {
			if (j > 0) {
				fprintf(fp, ", ");
			}
			fprintf(fp, "%d,%d", action[i][j].sta, action[i][j].to);
		}
		fprintf(fp, "},\r\n");
	}
	fprintf(fp, "};\r\n");
	fclose(fp);
}

static void INPUT_ACTION()
{
#include "./action.init"
	int i,j,k,l,row,col;
	row = action_row; col = action_col;

	for (i=0;i<row;i++) {
		action[i].clear();
		for (j=0;j<col;j++) {
			k = action_init[i][j+j]; l = action_init[i][j+j+1];
			action[i].push_back( Action(l, k) );
		}
	}
}

static void INPUT_CODE()
{
	word.push_back( WORD("<keyword>", "$", source.size()) );
	source.push_back( Source("", filename, source.size()) );
}

static FILE *fp;

static string MAKE_SOLVE()
{
	int i,j,symbol_id,statement,len,count, top, to;

	top = 1;  count = 1;
	sstack.clear(); sstack.push_back( SStack(0, "") );
	type_name.clear();
	headSubtr = NULL;

	for (i=0;i<word.size();i++)
	{
		while (sstack.size() < top+1) sstack.push_back( SStack(0, "") );

		symbol_id = mm[word[i].val];
		for (j=0;j<type_name.size();j++) {
			if (word[i].val == type_name[j].name)
				if (i==0 || word[i-1].val != "struct") {
					symbol_id = mm["TYPE_NAME"];
					word[i].type = "<keyword>";
				}
		}
		if (word[i].type == "<ident>")  symbol_id = mm[ident];
		if (word[i].type == "<number>") symbol_id = mm[number];
		if (word[i].type == "<string>") symbol_id = mm[stringg];


		if (OUT_OTHER_FILE == 1) {
			fprintf(fp, "(%d)\nnext input: ", count++);
			fprintf(fp, "%s %d \t\t\t\t\t", (word[i].val+word[i].type).c_str(), symbol_id);
		}

		to = action[sstack[top-1].state][symbol_id].to;
		switch (action[sstack[top-1].state][symbol_id].sta)
		{
			case 0:			// PUSH 移入
				if (OUT_OTHER_FILE == 1) {
					fprintf(fp, "push \t%d\t%s\t%s\t%d\n",to,word[i].type.c_str(),word[i].val.c_str(),word[i].line);
				}

				sstack[top].state = to;
				sstack[top].val = word[i].val;
				sstack[top].list.clear();
				sstack[top].list.push_back(sstack[top].val);

				sstack[top].subtr = new SyntaxTree();
				sstack[top].subtr->statement = -1;
				sstack[top].subtr->val = sstack[top].val;
				sstack[top].subtr->line = word[i].line;
				top++;

				// struct Str { Str *p; } // 当遇到{时 就要识别出Str是一种类型
				if (top > 3 && sstack[top-3].val == "struct" && sstack[top-1].val == "{") {
					type_name.push_back( SType_name(sstack[top-2].val) );
				}

				break;
			case 1:			// POP 规约
				statement = to;
				if (OUT_OTHER_FILE == 1) {
					fprintf(fp, "pop \tstatement= %d ",statement);
					fprintf(fp, "(");
					for (j=0;j<grammar[statement].size();j++) fprintf(fp, "%s",grammar[statement][j].c_str()); 
					fprintf(fp, ")\n");
				}

				len = grammar[statement].size(); if (grammar[statement][len-1] == null) len--;

				//SyntaxTree subtr;
				subtr = new SyntaxTree();
				subtr->statement = statement;
				subtr->val = "";
				subtr->tree.clear();
				subtr->line = sstack[top-len+2].subtr->line;
				for (j=top-len+2;j<top;j++) {
					subtr->tree.push_back(sstack[j].subtr);
				}

				top = top-len+3;
				sstack[top-1].state = mm[grammar[statement][0]];
				sstack[top-1].subtr = subtr;
				if (statement == 44) type_name.push_back( SType_name(sstack[top].val) );
				if (statement == 12 && sstack[top-1].val.substr(0, 7) == "typedef") {
					for (j=0;j<sstack[top].list.size();j++) {
						type_name.push_back( SType_name(sstack[top].list[j]) );
					}
				}
				if (statement == 65) {
					sstack[top-1].list.push_back(sstack[top+1].val);
				}

				i--;

				if (sstack[top-1].state == mm["<S>"]) {
					if (OUT_OTHER_FILE == 1) {
						fprintf(fp, "\n\n\naccepted\n\n\n");
					}
					headSubtr = sstack[top-1].subtr;
					return "syntax accept!";
				}

				// GOTO 跳转
				if (top > 1 && sstack[top-1].state >= end_symbol.size() && action[sstack[top-2].state][sstack[top-1].state].sta == 0)
				{
					sstack[top-1].state = action[sstack[top-2].state][sstack[top-1].state].to;
					if (OUT_OTHER_FILE == 1) {
						fprintf(fp, "sstack: ");
						for (j=0;j<top;j++) fprintf(fp, "%d ",sstack[j].state); fprintf(fp, "\n\n");

						fprintf(fp, "(%d)\nnext input: ", count++);
						fprintf(fp, " %s %d \t\t\t\t\t",(word[i+1].val+word[i+1].type).c_str(), symbol_id);
						fprintf(fp, "goto \t%d\n",sstack[top-1].state);
					}

				} else {
					if (OUT_OTHER_FILE == 1) {
						fprintf(fp, "error\n\n\n");
					}
					return "syntax error: "+source[word[i].line].file+": "+iTs(source[word[i].line].line)+": "+word[i].val;
				}
				break;
			case -1:		// error
				if (OUT_OTHER_FILE == 1) {
					fprintf(fp, "error\n\n\n");
				}
				return "syntax error: "+source[word[i].line].file+": "+iTs(source[word[i].line].line)+": "+word[i].val;
				break;
		}
		if (OUT_OTHER_FILE == 1) {
			fprintf(fp, "sstack: ");
			for (j=0;j<top;j++) fprintf(fp, "%d ",sstack[j].state); fprintf(fp, "\n");
		}
	}
	if (top == 1 && sstack[0].state == mm["<S>"]) {
		if (OUT_OTHER_FILE == 1) {
			fprintf(fp, "accepted\n\n\n");
		}
		headSubtr = sstack[top-1].subtr;
		return "syntax accept!";
	} else {
		if (OUT_OTHER_FILE == 1) {
			fprintf(fp, "error\n\n\n");
		}
		return "syntax error: "+filename+": at end of file";
	}
}

int syntax_main()
{
	syntax_out		= "./data/syntax.out";
	action_name		= "./action.init";

	number	= "constant";
	ident	= "identifier";
	stringg	= "string_literal";

	INPUT_GRAMMAR();

	if (MAKE_ACTION_INIT == 1) {
		MAKE_FIRST();
		MAKE_VINIT();
		MAKE_ITEMS();
		MAKE_ACTION();
		return 0;
	}

	INPUT_ACTION();
	INPUT_CODE();
	if (OUT_OTHER_FILE == 1) {
		fp = fopen(syntax_out.c_str(),"w");
	}
	string ss = MAKE_SOLVE();
	int success = 0;
	if (ss == "syntax accept!") {
		success = 0;
	} else {
		printf("%s\n", ss.c_str());
		success = 1;
	}
	if (OUT_OTHER_FILE == 1) {
		fprintf(fp, "%lf\n\n\n", clock()/1000000.0);
		fclose(fp);
	}
	return success;
}

