#include "kcc.h"

// 不输出中间文件
int OUT_OTHER_FILE;

// 输入源代码的文件名
string filename;

// 输出的汇编文件
string assemblyname;

// 输出的目标文件
string objectname;

// 输出的可执行文件
string outfilename;

// 如果MAKE_ACTION_INIT=1，表示输出文法对应的跳转表action.init
int MAKE_ACTION_INIT = 0;

int main(int argc, char** argv)
{
	if (argc == 2 && strcmp(argv[1], "make_action") == 0) {
		MAKE_ACTION_INIT = 1;
		syntax_main();
		printf("just for make action.init\n");
		return 0;
	}

	int i,j,l;
	char ch[1033];
	FILE *fp;

	OUT_OTHER_FILE = 1;
	filename  = "";
	assemblyname = "./data/assembly.s";
	objectname = "./data/assembly.o";
	outfilename = "";

	for (i=1;i<argc;i++) {
		string str = argv[i];
		l = str.length();
		string str1 = str.substr(max(0, l-2)), str2 = str.substr(max(0,l-4));
		for (j = 0; j < str1.length(); j++) if (str1[j] >= 'A' && str1[j] <= 'Z') str1[j] += 32;
		for (j = 0; j < str2.length(); j++) if (str2[j] >= 'A' && str2[j] <= 'Z') str2[j] += 32;
		if (str1 == ".c" || str2 == ".cpp") {
			filename  = argv[i];
		} else if (strcmp(argv[i], "-n") == 0) {
			OUT_OTHER_FILE = 0;
			#ifdef __linux_system__
				assemblyname = "/tmp/assembly.s";
				objectname = "/tmp/assembly.o";
			#endif
			#ifdef __windows_system__
				char path[333];
				GetTempPath(333, path);
				assemblyname = path;
				assemblyname += "assembly.s";
				objectname = path;
				objectname += "assembly.o";
			#endif
		} else if (strcmp(argv[i], "-o") == 0) {
			if (i+1 >= argc) {
				printf("argument \"-o\" ERROR");
				return 11;
			}
			outfilename = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "--version") == 0) {
			printf("%s\n", VERSION);
			printf("Copyright (C) 2011-2013 kk, All Rights Reserved.\n");
			return 0;
		} else if (strcmp(argv[i], "--help") == 0) {
			printf("Usage: kcc [options] file\n");
			printf(" -o <file>   Place the output into <file>\n");
			printf(" -n          No out temp files, it's say no out data/*\n");
			printf(" --version   Display compiler version information\n");
			printf(" --help      Out help information\n");
			return 0;
		} else {
			printf("argument \"%s\" ERROR\n", argv[i]);
			return 12;
		}
	}

	if (filename == "") {
		printf("no input file\n");
		return 10;
	}

	if (outfilename == "") {
		for (i=filename.size()-1; i>0 && filename[i] != '.';i--);
		for (j=i-1;j>=0 && filename[j] != '.' && filename[j] != '/' && filename[i] != '\\';j--);
		for (j++;j<i;j++) outfilename += filename[j];
		#ifdef __windows_system__
			outfilename += ".exe";
		#endif
	}

	if (preprocessor_main())
		return 1;

	if (lexical_main())
		return 2;

	if (syntax_main())
		return 3;

	if (semantic_main())
		return 4;

	if (assembly_main())
		return 5;

	int pid, status;

#ifdef __windows_system__
	sprintf(ch, ".\\bin\\as.exe %s -o %s", assemblyname.c_str(), objectname.c_str());
	status = system(ch);
	if (status != 0) {
		return 5;
	}

	sprintf(ch, ".\\bin\\ld.exe ./lib/crt2.o ./lib/crtbegin.o -L./lib %s -lstdc++ -lmingw32 -lgcc -lmoldname -lmingwex -lmsvcrt -luser32 -lkernel32 -ladvapi32 -lshell32 -lmingw32 -lgcc -lmoldname -lmingwex -lmsvcrt -o %s ./lib/crtend.o", objectname.c_str(), outfilename.c_str());
	status = system(ch);
	if (status != 0) {
		return 6;
	}
#endif

#ifdef __linux_system__
	pid = fork();
	if (pid == 0) {
		execlp("as", "as", assemblyname.c_str(), "-o", objectname.c_str(), NULL);
	}
	waitpid(0, &status, 0);
	if (status != 0) {
		return 5;
	}

	pid = fork();
	if (pid == 0) {
	#ifdef __linux_32_system__
		execlp("ld", "ld", "-dynamic-linker", "/lib/ld-linux.so.2", "./lib/crt1.o", "./lib/crti.o", "./lib/crtbegin.o", "-L/lib/", "-L/usr/lib/", objectname.c_str(), "-lm", "-lc", "./lib/crtend.o", "./lib/crtn.o", "-o", outfilename.c_str(), NULL);
	#endif
	#ifdef __linux_64_system__
		execlp("ld", "ld", "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", "./lib/crt1.o", "./lib/crti.o", "./lib/crtbegin.o", "-L/lib/", "-L/usr/lib/", objectname.c_str(), "-lm", "-lc", "./lib/crtend.o", "./lib/crtn.o", "-o", outfilename.c_str(), NULL);
	#endif
	}
	waitpid(0, &status, 0);
	if (status != 0) {
		return 6;
	}
#endif
	return 0;
}

