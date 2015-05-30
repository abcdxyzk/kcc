2015-05-30:
		v0.7.1 增加注释、Makefile
2013-04-15:
		v0.7.0
2013-04-15:
		修改一些结构，重写类型检查、语义分析，支持include

2013-03-03:
		v0.6.0

2013-03-03:
		commit: 完善#define，#if表达式等的预处理；完善编译选项
2013-02-26:
		commit: 修改def，增加ifdef,ifndef,undef，忽略pragma
2013-02-24:
		commit: k++更名kcc；源文件不再可以缺省；增加参数，-o可执行文件，-nt不输出中间文件；不再输出_start段；将原来每一部分都从文件读入改为共用内存，文件输出变为可选项。
2013-02-07:
		commit: 整合常用函数，不再输出run.sh
2013-02-07:
		commit: 将文件mode修改正确的600
2013-02-07:
		commit: 增加k++.h，将define移到k++.h里
2013-02-06:
		commit: 将MAKE_ACTION_INIT的define位置由syntax.cpp移到k++.cpp
2013-02-06:
		commit: 修改文件结构后修改成可执行
2013-02-06:
		commit: 修改文件结构，去掉7个文件夹，将8个文件放到外层
2013-02-06:
		commit: 增加注释 和 修改代码分格

2013-02-01:
		v0.5.0

2013-02-01:
		commit: 修改返回值 修复free和assert和|| &&
2013-01-27:
		commit: 可以计算简单#if表达式
2013-01-11:
		commit: 修正对&&和||的错误处理
2013-01-03:
		commit: 重新处理typedef，改一条文法，使处理类似|typedef struct Str { ... } Node;|更简单
		下一步处理 # include, #  define, #if, # else, #elif, #endif #ifdef  #ifndef if (a && b)  if (a || b) 等
