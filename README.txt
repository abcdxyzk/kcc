kcc => kk's C language Compiler  下载：右击另存为
2013-01-11:
		commit: 修正对&&和||的错误处理
2013-01-03:
		commit: 重新处理typedef，改一条文法，使处理类似|typedef struct Str { ... } Node;|更简单
		下一步处理 # include, #  define, #if, # else, #elif, #endif #ifdef  #ifndef if (a && b)  if (a || b) 等
