/**
  * auto_test.c
  * 自动测试test.c里的代码
  * test.c中每段程序用 / *  * / 分隔
  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	int i,j,k,l,incode;
	char c, ch[10000];
	FILE *fpin, *fpout;

	fpin = fopen("test.c", "r");
	incode = 0;
	while (fgets(ch, 10000, fpin)) {
		if (strcmp(ch, "/*\n") == 0) {
			fpout = fopen("test_1.c", "w");
			incode = 1;
		} else if (strcmp(ch, "*/\n") == 0) {
			fclose(fpout);
			incode = 0;
			printf("===================\n");
			system("./kcc test_1.c -o test_1");
			system("./test_1");
			printf("===================\n");
			printf("按q退出，按Enter继续\n");
			c = getchar();
			if (c == 'q' || c == 'Q') {
				break;
			}
		} else if (incode == 1) {
			fputs(ch, fpout);
			printf("%s", ch);
		}
	}
	return 0;
}
