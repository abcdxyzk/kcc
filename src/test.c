#include <stdio.h>
#include <time.h>
int abc(int *k)
{
	*k = *k+1;
}

int main()
{
	int i;
	i = 7;
	abc(&i);
	i = printf("%d\n", i);
	for (i=0;i<1000000000;i++);
	printf("%d\n", clock());
	return 0;
}
/*
#include <stdio.h>
struct Node {
	int x, y;
} ;
struct Node p;
int main()
{
	int i, a = {i};
	p.y = 123;
	printf("%d\n", p.y);
	return 0;
}
*/
/*
#include <stdio.h>
int main()
{
	int i,j,k,a[11][11];
	char c;
	k = 2;
	c = (char)k;
	printf("%d\n", c);
	return 0;
}
*/
/*
#include <stdio.h>
int main()
{
	int i,j,k,a[11][11];
	k = 2;
	a[1][k+2] = 123;
	printf("%d\n", a[1][k*2]);
	return 0;
}
*/
/*
#include <stdio.h>
int main()
{
	int i,k,a[10];
	k = 1;
	for (i = 1; ; ) {
		k = k*i;
		i++;
		if (i <= 10) continue;
		break;
	}
	i = 10;
	do {
		k = k/i;
		i--;
	} while (i > 5);
	printf("%d\n", k);
}
*/
/*
#include <stdio.h>
int main()
{
	int i,k,a[10];
	k = 1;
	for (i = 1; ; ) {
		if (i > 10) break;
		k = k*i;
		i++;
	}
	i = 10;
	do {
		k = k/i;
		i--;
	} while (i > 5);
	printf("%d\n", k);
}
*/
/*
#include <stdio.h>
int main()
{
	int i,k,a[10];
	//a[1] = 1 && (a[2] = 2);
	a[2] = 0 ? 2 : 3;
	i = a[2]*2;
	printf("%d\n", i);
}
*/
/*
#include <stdio.h>
int n,m;

int abc(int i, int j)
{
	int k;
	k = i+j;
	return k;
}

int main()
{
	int i,k;
	i = abc(12+23, 45);
	printf("%d\n", i);
}
*/
/*
#include <stdio.h>
long n,m;
int f(int a);
int abc(int a[], int b[10])
{
}

int main()
{
	int i, c[10], *p;
	i = 1+2+3;
}
*/
/*
#include <stdio.h>
typedef int II; // 不该出现 int II

int main()
{
	II i;
	i = 1;
}
*/
/*
#include <stdio.h>
#define AA(a, b) 1+a+b

int main()
{
	int i,j,k= 1;
	i = k;
	printf("%d\n", i);
	return 0;
}
int abc()
{
}
*/
/*
#include <stdio.h>
#define B C
#define C(b,c) b+c

int main()
{
	int i = 123;
#if B (1,1) << 3 == 16
	printf("%d\n", i);
#endif
	printf("%d\n", B(1,1) << 3);
	return 0;
}
*/
/*
#include <stdio.h>
#define B C
#define C(b,c) b+c
int main()
{
	int i = 123;
#if -1+17 == 16
	printf("%d\n", i);
#endif
	printf("%d\n", B(1,1) << 3);
	return 0;
}
*/
/*
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

int main()
{
	int i,j,k;
	int *p = &j;
	j = k = 1;
	assert(j!=k);
	i = 1;
	printf("%d\n", i);
	return 0;
}
*/
/*
#include <stdio.h>
#include <stdlib.h>

#define S1 1+2
#define S2 22+33
#define S3(a, b) a+b
int abc()
{
	return 3;
}

int main()
{
	int i,j,k;
	j = S3(1, abc());
#if S2+5 > S1
	k = S1*S2;
#else
	k = S2+S1;
#endif
	printf("%d\n", k);
	return 0;
}
*/
/*
#include <stdio.h>
int abc()
{
	return 123;
}

int main()
{
	int i;
	i = abc() + abc() * 2;
	printf("%d\n", i);
	return 0;
}
*/
/*
#include <stdio.h>
#include <stdlib.h>
int main()
{
	int i,j,k;
	int *p;
#if 1
	k = 2;
	p = (int*)malloc(4);
	*p = 9;
	k = *p;
	free(p);
#elif 1
	k = 1;
#else
	k = 4;
#endif
	printf("%d\n", k);
	return 0;
}
*/
/*
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int a,b,*c;
	c = (int*)malloc(4);
	scanf("%d", &a);
	c = &a;
	printf("%d\n", *c);
	return 0;
}
*/
/*
#include <stdio.h>
#include <stdlib.h>
//void   *malloc (int size);

int main()
{
	int a,b,*c;
	c = (int*)malloc(4);
	*c = 123;
	a = printf("%d\n", *c);
	a = printf("%d\n", a);
	return 0;
}
*/
/*
#include <stdio.h>
int main()
{
	int i,j,k;
	i = j = k = 1;
#if 1
	#if 0
		j = 1;
	#else
		j = 2;
	#endif
#elif 0
	#if 0
		j = 3;
	#else
		j = 4;
	#endif
#else
#endif
	printf("%d\n", j);
	return 0;
}
*/
/*
#include <stdio.h>
#include <assert.h>
int main()
{
	int a,b,c;
	a = b = c= 5;
	if ((a=0) || (b=1) && (c=1));
	printf("%d %d %d\n", a,b,c);
	return 0;
}
*/
/*
#include <stdio.h>

typedef int INT,II;

typedef int CON,CON1;

typedef struct STR {
	int x,y;
} Node;

struct CC {
	int x,y;
	CC *p;
} No;

int abc()
{
	typedef int OOO;
}

int main()
{
	typedef int OO;
	//Node a;
	//long char c=1;
	Node tt;

	CON1 ss=1;

	tt.x = 1;

	printf("%d %d\n", tt.x, ss);
	//b=2;
	//a.x = 1;
	return 0;
}
*/
/*
#include <stdio.h>

int main()
{
	double a,b;
	a = 1; b = 2;
	printf("%lf %lf\n", a, b);
	return 0;
}
*/
/*
#include <stdio.h>

int main()
{
	int a,b;
	scanf("%d", &a);
	printf("%d\n", a);
	return 0;
}
*/
/*
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int a,b[2],c[3],d[4],e[5];

int abc(int a, int b)
{
	int i,j;
	i = 1; j = 2;
	i = a; j = b;
	printf("%d\n", a);
	return 0;
}

int main()
{
	abc(3, 4);
	return 0;
}
*/
/*
#include<stdio.h>
int main()
{
	int i,j,a[10][10];
	for(i=0;i<10;i++)
		for(j=0;j<=i;j++) if(i == 0 || j == 0 || i == j) a[i][j] = 1;
		else
			a[i][j] = a[i-1][j] + a[i-1][j-1];

	for(i=0;i<10;i++)
	{
		for(j=0;j<=i;j++) printf("%d ",a[i][j]);
		printf("\n");
	}

	return 0;
}
*/
/*
#include<stdio.h>

int out(int abc)
{
	printf("%d\n",abc);
	return abc;
}

int main()
{
	int i,*p,n=10,a[10][10];
	int d,f[10];
	printf("%d %d\n",n,a[5][5]);
	d = (int)out(123);
	d = 123;
	printf("%d\n",d);
	f[0] = 1;
	for(i=1;i<10;i++) f[i] = f[i-1]<<1;
	for(i=1;i<10;i++)
		//if(n>=f[i])
		printf("%d %d\n",i,f[i]);
	return 0;
}
*/
/*
#include<stdio.h>

double ab(double d)
{

}

int main()
{
	char c;
	int i,*p,n=10,a[10][10];
	double d1,d2,f[10];
	f[1] = 1.0; f[2] = 2.0;
	c = 2; i = 2;
	d1 = ab(d1);
	printf("%lf %lf %lf\n",f[1],f[2],f[1]);
	return 0;
}
*/
/*
#include<stdio.h>
#include<string.h>

int main()
{
	char a[55];
	char b[55];
	char c[80],*p;
	int i=0,j=0,k=0;
	strcpy(a, "acegikm");
	strcpy(b, "bdfhjlnpq");

	while(a[i]!='\0'&&b[j]!='\0')
	{
		if(a[i]!='\0') { c[k]=a[i++];}
		else
		c[k]=b[j++];
		k++;
	}
	c[k]='\0';
	if(a[i]=='\0')
		p=b+j;
	else
		p=a+i;
	strcat(c,p);
	puts(c);
	return 0;
}
*/
/*
#include<stdio.h>
#include<string.h>

char swap(char *p1, char *p2)
{
	char p[20];
	printf("%s %s\n", p1, p2);
	strcpy(p,p1);strcpy(p1,p2);strcpy(p2,p);
	printf("%s %s\n", p1, p2);
}
int main()
{
	char str1[20],str2[20],str3[20];
	printf("please input three strings\n");
	scanf("%s",str1);
	scanf("%s",str2);
	scanf("%s",str3);
	printf("SS %d %d\n", strcmp(str1, str2), strcmp(str1,str2) > 0);
	printf("%d\n", strcmp(str1,str3));
	printf("%d\n", strcmp(str2,str3));
	if(strcmp(str1,str2)>0) swap(str1,str2);
	printf("%s %s %s\n", str1, str2, str3);
	if(strcmp(str1,str3)>0) swap(str1,str3);
	printf("%s %s %s\n", str1, str2, str3);
	if(strcmp(str2,str3)>0) swap(str2,str3);
	printf("after being sorted\n");
	printf("%s\n%s\n%s\n",str1,str2,str3);
	return 0;
}
*/
/*
#include<stdio.h>

void move(double array[20], int n, int m)
{
	double *p,array_end;
	array_end=*(array+n-1);
	for(p=array+n-1;p>array;p--)
	*p=*(p-1);
	*p=array_end;
	m--;
	if(m>0) move(array,n,m);
}
int main()
{
	double number[20];
	int n,m,i;
	printf("the total numbers is:");
	scanf("%d",&n);
	printf("back m:");
	scanf("%d",&m);
	for(i=0;i<n-1;i++)
	scanf("%lf,",&number[i]);
	scanf("%lf",&number[n-1]);
	move(number,n,m);
	for(i=0;i<n-1;i++)
	printf("%.0lf ",number[i]);
	printf("%.0lf\n",number[n-1]);
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	char str1[20],str2[20],*p1,*p2;
	int sum=0;
	printf("please input two strings\n");
	scanf("%s%s",str1,str2);
	p1=str1;p2=str2;
	while(*p1!='\0')
	{
		if(*p1==*p2)
		{
			while(*p1==*p2&&*p2!='\0')
			{
				p1++;
				p2++;
			}
		}
		else
			p1++;
		if(*p2=='\0')
			sum++;
		p2=str2;
	}
	printf("%d\n",sum);
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	char *p,s[6];int n;
	p=s;
	gets(p);
	n=0;
	while(*(p)!='\0')
	{
		n=n*8+*p-'0';
		p++;
	}
	printf("%d\n",n);
	return 0;
}
*/
/*
#include<stdio.h>

void output(int b, int i)
{
	printf("%ld/%ld=809*%ld+%ld\n",b,i,i,b%i);
}
int main()
{
	int a,b,i;
	a=809;
	for(i=10;i<100;i++)
	{
		b=i*a+1;
		if(b>=1000&&b<=10000&&8*i<100&&9*i>=100)
		output(b,i);
	}
}
*/
/*
#include<stdio.h>

int main()
{
	int i,m,j,k,count;
	for(i=4;i<10000;i+=4)
	{
		count=0;
		m=i;
		for(k=0;k<5;k++)
		{
			j=i/4*5+1;
			i=j;
			if(j%4==0)
			count++;
			else
			break;
		}
		i=m;
		if(count==4)
		{
			printf("%d\n",count);
			break;
		}
	}
	return 0;
}
*/
/*
#include<stdio.h>

int length(char *p)
{
	int n;
	n=0;
	while(*p!='\0')
	{
		n++;
		p++;
	}
	return n;
}
int main()
{
	int len;
	char str[20];
	printf("please input a string:\n");
	scanf("%s",str);
	len=length(str);
	printf("the string %s has %d characters.\n",str,len);
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	int i,n;
	char k,m,num[55],*p;
	printf("please input the total of numbers:");
	scanf("%d",&n);
	p=num;
	for(i=0;i<n;i++)
		*(p+i)=i+1;
	i=0;
	k=0;
	m=0;
	while(m<n-1)
	{
		if(*(p+i)!=0) k++;
		if(k==3)
		{
			*(p+i)=0;
			k=0;
			m++;
		}
		i++;
		if(i==n) i=0;
	}
	while(*p==0) p++;
	printf("%d is left\n",*p);
	return 0;
}
*/
/*
#include<stdio.h>

void input(int numbe[10], int n, int number[10])
{
	int i;
	printf("please input %d number\n",n+1);
	for(i=0;i<n;i++)
		scanf("%d",&number[i]);
	scanf("%d",&number[9]);
}

void max_min(int array[10])
{
	int i;
	int *max,*min,k,l;
	int *p,*arr_end;

	arr_end=array+10;
	max=min=array;
	for(p=array+1;p<arr_end;p++)
	if(*p>*max) max=p;
	else if(*p<*min) min=p;

	k=*max;
	l=*min;
	//printf("max = %d  min = %d\n",k,l);
	//for(i=0;i<10;i++) printf("%d ",array[i]); printf("\n");
	p=&array[9];
	*max = *p; *p = k;
	//for(i=0;i<10;i++) printf("%d ",array[i]); printf("\n");
	p=&array[0];
	*min = array[0]; *p = l;
	//for(i=0;i<10;i++) printf("%d ",array[i]); printf("\n");
	return;
}

void output(int *array)
{
	int *p;
	for(p=array;p<array+9;p++)
	printf("%d ",*p);
	printf("%d\n",*p);
}

int main()
{
	int number[10];
	input(number, 9, number);
	max_min(number);
	output(number);
	return 0;
}
*/
/*
#include<stdio.h>

void swap(int *p1, int *p2)
{
	int p;
	p=*p1;*p1=*p2;*p2=p;
}
int main()
{
	int n1,n2,n3;
	int *pointer1,*pointer2,*pointer3;
	printf("please input 3 number: n1 n2 n3:");
	scanf("%d %d %d",&n1,&n2,&n3);
	pointer1=&n1;
	pointer2=&n2;
	pointer3=&n3;
	if(n1>n2) swap(pointer1,pointer2);
	if(n1>n3) swap(pointer1,pointer3);
	if(n2>n3) swap(pointer2,pointer3);
	printf("the sorted numbers are:%d %d %d\n",n1,n2,n3);
	return 0;
}
*/
/*
#include<stdio.h>
#include<time.h>

int n,ans=0,a[33],b[33],c[33];

void dfs(int kk)
{
     int i;
     if(kk == n)
     {
          ans++;
          return;
     }

     for(i=0;i<n;i++)
     if(a[i] == 0 && b[kk+i] == 0 && c[kk-i+n] == 0)
     {
          a[i] = 1;
          b[kk+i] = 1;
          c[kk-i+n] = 1;

          dfs(kk+1);

          a[i] = 0;
          b[kk+i] = 0;
          c[kk-i+n] = 0;
     }
}

int main()
{
     int i,j,k,l;
     int d;
     n = 8;
    for(i=0;i<n+n;i++) {
          a[i] = 0;
          b[i] = 0;
          c[i] = 0;
     }
     ans = 0;
    dfs(0);
    printf("%d\n",ans);
    d = clock();
    printf("%d\n",d);
    return 0;
}
*/
/*
#include <stdio.h>

int n,m,a[100000];

int main()
{
    int i,j;
    a[1] = 1; m = 1;
    n = 10;
    for(i=1;i<=n;i++)
    {
        for(j=1;j<=m;j++) a[j] *= i;
        for(j=1;j<m;j++)
        {
            a[j+1] = a[j+1] + a[j]/10;
            a[j] %= 10;
        }
        while(a[m] >= 10)
        {
            a[m+1] = a[m]/10;
            a[m] %= 10;
            m++;
        }
    }
    for(i=m;i>0;i--) printf("%d",a[i]);
    printf("\n");
    return 0;
}
*/
/*
#include<stdio.h>

char add(int p, int o)
{
//	return p;
	if(p < o) return o;
	return p;
}

int main()
{
	int i,j,*p,*q[11],a[10];
	double d;
	i = 12; j = 23;
	i = add(12, 23);
	printf("%d %d\n",add(12, 23),add(33, 23));
	return 0;
}
*/
/*
#include<stdio.h>
#include<time.h>

void add(int *p)
{
	*p = 57;
	p=p + 3;
	*p = 12;
	p-=6;
	*p = 123;
}

void out(double kk)
{
	printf("%lf\n",kk);
}

int main()
{
	int i,j;
	int *p,*q[11][11],a[100000];
	q[5][5] = a + 5;
	add(&a[5]);
	out(*q[5][5]);

	printf("%d %d\n",a[8],*(q[5][5]-3));
	for(i=0;i<1000;i++)
	{
		p = a;
		j = 0;
		do {
			//a[j] = j;
			*p = j;
			p++;j++;
		} while(j < 100000);
	}
	printf("%d\n",clock());

	return 0;
}
*/
/*
#include<stdio.h>

char out(int *kk, int ll)
{
	char c='a';
	*kk = 56;
	printf("%d %d\n",*kk,ll);

	return ll;
}

int main()
{
	int i,j;
	char k;
	int *p;
	p = &j;
	i = 123;
	k = out(p, i);
	printf("%d %d\n",j,k);
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	char *pc,*pcc,c,cc;
	int *pi,*pii,i,ii;
	double *pd,*pdd,d,dd;
	pc = &c;
	pi = &i;
	pd = &d;
	pcc = &cc;
	pii = &ii;
	pdd = &dd;
	c = 1;
	i = 2;
	d = 3;
	cc = 11;
	ii = 22;
	dd = 33;
	*pc = *pcc;
	printf("%d\n",*pc);
	*pc = *pii;
	printf("%d\n",*pc);

	*pi = *pcc;
	printf("%d\n",*pi);
	*pi = *pii;
	printf("%d\n",*pi);

	*pd = *pcc;
	printf("%lf\n",*pd);
	*pd = *pii;
	printf("%lf\n",*pd);
	*pd = *pdd;
	printf("%lf\n",*pd);

	c = *pcc;
	printf("%d\n",c);
	c = *pii;
	printf("%d\n",c);

	i = *pcc;
	printf("%d\n",i);
	i = *pii;
	printf("%d\n",i);

	d = *pcc;
	printf("%lf\n",d);
	d = *pii;
	printf("%lf\n",d);
	d = *pdd;
	printf("%lf\n",d);
	return 0;
}
*/
/*
#include<stdio.h>

char abc(char cc)
{
	return cc+1;
}

int abi(int ii)
{
	return ii+1;
}

double abd(double dd)
{
	return dd+1;
}

int main()
{
	char c,cc;
	int i,ii;
	double d,dd;
	cc = 'a';
	ii = 123;
	dd = 123.456;

	c = abc(cc);
	printf("%d\n",c);
	c = abc(ii);
	printf("%d\n",c);

	i = abi(cc);
	printf("%d\n",i);
	i = abi(ii);
	printf("%d\n",i);

	d = abd(cc);
	printf("%lf\n",d);
	d = abd(ii);
	printf("%lf\n",d);
	d = abd(dd);
	printf("%lf\n",d);
	return 0;
}
*/
/*
#include<stdio.h>
#include<math.h>

int main()
{
	int i;
	double d;
	i = 123;
	d = 100.123;
	d = sqrt(d);
	printf("%lf\n",d);
	d = sin(3.1415926/2.0);
	printf("%lf\n",d);
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	int i=1;
	int a[5] = {'1','2','3'};
	double b[10] = {1,2,456};
	//b[1] = 123;
	for(i=0;i<5;i++) printf("%d ",a[i]); printf("\n");
	for(i=0;i<5;i++) printf("%.1lf ",b[i]); printf("\n");
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	int i,j,k,l;
	printf("----------\n");
	switch(1)
	{
		case 1:
			printf("1\n");
			break;
		case 2:
			printf("2\n");
			break;
		case 3:
			printf("3\n");
			break;
		default:
			printf("66\n");
	}
	printf("----------\n");
	switch(1)
	{
		case 1:
			printf("1\n");
		case 2:
			printf("2\n");
		case 3:
			printf("3\n");
			break;
		default:
			printf("66\n");
	}
	printf("----------\n");
	switch(5)
	{
		case 1:
			printf("1\n");
		case 2:
			printf("2\n");
		default:
			printf("66\n");
		case 3:
			printf("3\n");
			break;
	}
	printf("----------\n");
	return 0;
}
*/
/*
#include<stdio.h>

#define  M  10
#define  exp i*i*i
typedef   int  integer  ;
int main()
{
	integer i,j,k,l,a[M];
	for(i=0;i<10;i++) a[i] = exp;
	for(i=0;i<10;i++) printf("%d %d\n",i,a[i]);
	return 0;
}
*/
/*
#include<stdio.h>

struct Node {
	char x,y,z;
} ;
struct Node p[33];

int main()
{
	int i,j,k;
	double d;
	for(i=0;i<10;i++) {
		p[i].x = i;
		p[i].y = i*i;
		p[i].z = p[i].x+p[i].y;
	}
	for(i=0;i<10;i++)
		printf("%d %d %d\n",p[i].x,p[i].y,p[i].z);
	return 0;
}
*/
/*
#include<stdio.h>

int i=1,j=3;
double a[10];
int main()
{
	double b[33][33];
	double *p,*o;
	p = &a[5];
	o = &b[5][5];
	*p = 123;
	*o = 456;
	printf("%lf %lf %lf %lf\n",*p,a[5],*o,b[5][5]);
	return 0;
}
*/
/*
#include<stdio.h>

struct Node {
	int x,y,z;
	int *p[10];
	double a[10];
} ;

int main()
{
	int i,j,k;
	double d;
	Node p[33],*q;
	char c = 123;
	p[3].a[3] = 333;
	p[2].a[2] = 222;
	p[5].a[5] = p[3].a[3] + p[2].a[2];
	printf("%lf %d\n",p[5].a[5],c);
	q = &p[5];
	p[5].y = 789;
	k = (*q).y;
	j = p[5].y;
	q = &p[4];
	q++;
	p[5].p[7] = &k;
	k = 765; printf("%d\n",*((*q).p[7]));
	p[5].a[7] = 777; printf("%lf\n",p[5].a[7]);
	printf("%d %lf\n",*(p[5].p[7]), p[5].a[7]);
	return 0;
}
*/
/*
#include<stdio.h>

struct Point {
	int x,y;
};

struct Node {
	int x,y,z;
	int *p[10];
	double a[10];
	Point *pp[10];
} ;

int main()
{
	Node st;
	Point pp;
	st.pp[7] = &pp;
	pp.y = 3456;
	printf("%d\n",(*(st.pp[7])).y);
	return 0;
}
*/
/*
#include<stdio.h>

struct Point {
	int x,*y;
};

struct Node {
	int x,y,z;
	int *p[10];
	double a[10];
	Point *pp[10];
} ;

int main()
{
	int yy;
	Node *st,ss;
	Point pp;
	st = &ss;
	(*st).pp[7] = &pp;
	(*(*st).pp[7]).y = &yy;
	yy = 333;
	printf("%d\n",*((*((*st).pp[7])).y));
	return 0;
}
*/
/*
#include<stdio.h>

struct Point {
	int a[10];
};

struct Node {
	int x,y;
	char c1,c2,c3;
};

int main()
{
	Node a,b;
	Point c;
	a.x = 1;
	a.y = 2;
	b = a;
	printf("%d %d\n",b.x,b.y);
	return 0;
}
*/
/*
#include<stdio.h>

struct Point {
	char ch[33][33];
};

struct Node {
	int x,y;
	char ch[33][33];
	Point po[33];
} p;

int main()
{
	char ch[33][33];
	int *pp;
	pp = &(p.y);
	*pp = 123;
	printf("%d\n",p.y);
	scanf("%s",p.po[3].ch[3]);
	printf("%s\n",p.po[3].ch[3]);
	return 0;
}
*/
/*
#include<stdio.h>

struct Node {
	int x,y,z;
} ;

void ty(Node a)
{
	printf("%d %d\n",a.x,a.y);
}

int main()
{
	Node a;
	a.x = 1; a.y = 2;
	a.z = 3;
	ty(a);
	return 0;
}
*/
/*
#include<stdio.h>

struct Node {
	int x,y,z;
} ;

void ty(Node *a)
{
	printf("%d %d\n",a->x,a->y);
}

int main()
{
	Node a;
	a.x = 1; a.y = 2;
	a.z = 3;
	ty(&a);
	return 0;
}
*/
/*
#include<stdio.h>
#include<stdlib.h>

struct Node {
	int x,y,z;
} ;

void ty(Node a)
{
	printf("%d %d\n",a.x,a.y);
}

int main()
{
	Node *a;
	a = (Node*)malloc(sizeof(Node));
	a->x = 1; a->y = 2;
	a->z = 3;
	ty(*a);
	return 0;
}
*/
/*
#include<stdio.h>
#include<stdlib.h>

struct NN {
	int x,y,z;
} ;

struct Node {
	int x,y,z;
	NN *ppp;
	NN *pp;
	NN *p;
} ;

int main()
{
	Node *a;
	NN nn;
	nn.z = 456;
	a = (Node*)malloc(sizeof(Node));
	a->p = (NN*)malloc(sizeof(NN));
	a->p->z = 123;
	printf("%d\n",a->p->z);
	a->p = &nn;
	printf("%d\n",a->p->z);
	return 0;
}
*/
/*
#include<stdio.h>

struct Node {
	int x,y,z;
	char c1,c2;
} ;

Node ty()
{
	Node a;
	a.x = 1; a.y = 2;
	a.c2 = 3;
	return a;
}

int main()
{
	Node a;
	a = ty();
	printf("%d %d\n",a.x,a.c2);
	return 0;
}
*/
/*
#include<stdio.h>
#include<stdlib.h>

struct Node {
	int x,y,z;
	char c1,c2;
} ;

int main()
{
	Node *a;
	double *p;
	p = (double*)malloc(sizeof(double));
	*p = 123;
	a = (Node*)malloc(sizeof(Node));
	a->x = 111;
	printf("%d %lf\n",a->x,*p);
	return 0;
}
*/
/*
#include<stdio.h>
#include<stdlib.h>

struct Node {
	int x,y,z;
	char c1,c2;
} ;

int main()
{
	Node *a,*b,c;
	a = (Node*)malloc(sizeof(Node));
	a->x = 111;
	b = (Node*)malloc(sizeof(Node));
	*b = *a;
	printf("%d\n",b->x);
	c.z=333;
	b = &c;
	printf("%d\n",b->z);
	*b = c;
	printf("%d\n",b->z);
	c = *a;
	printf("%d\n",c.x);
	return 0;
}
*/
/*
#include<stdio.h>
#include<stdlib.h>

struct Node
{
	int data;
	Node *next;
};

int main()
{
	Node *ptr,*head;
	int num,i;
	head = (Node*)malloc(sizeof(Node));
	ptr=head;
	printf("please input 5 numbers==>\n");
	for(i=0;i<=4;i++)
	{
		scanf("%d",&num);
		ptr->data=num;
		ptr->next=(Node*)malloc(sizeof(Node));
		if(i==4) ptr->next=NULL;
		else ptr=ptr->next;
	}
	ptr=head;
	while(ptr!=NULL)
	{
		printf("The value is ==>%d\n",ptr->data);
		ptr=ptr->next;
	}
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	FILE *fp;
	fp = fopen("aa.txt","w");
	fprintf(fp, "%d\n", 123);
	fprintf(fp, "%d\n", 123);
	fprintf(fp, "%d\n", 123);
	fclose(fp);
	return 0;
}
*/
/*
#include<stdio.h>

int main()
{
	freopen("a.txt","w",stdout);
	printf("1234567\n");
	return 0;
}
*/
/*
#include<stdio.h>
#include<stdlib.h>

int main()
{
	FILE *fp;
	char ch,filename[10];
	scanf("%s",filename);
	if((fp=fopen(filename,"w"))==NULL)
	{
		printf("cannot open file\n");
		exit(0);
	}
	ch=getchar();
	ch=getchar();
	while(ch!='#')
	{
		fputc(ch,fp);
		putchar(ch);
		ch=getchar();
	}
	fclose(fp);
	return 0;
}
*/
/*
#include <stdio.h>

void print(int i, double a, int j)
{
	printf("%d %lf %d\n", 1, a, 2);
}

int main()
{
	double a=3,b = 9.0;
	//a = a*b;
	print(1, a, 2);
	printf("%lf %lf %lf %.0lf %lf %lf %lf %lf %lf\n", 1.,2.,3.,4.,5.,6.,7.,8.,a);
	return 0;
}
*/
/*
#include <stdio.h>

int main()
{
	double a = 9.0;
	printf("%d %lf %d\n", 1, a, 2);
}
*/
/*
#include<stdio.h>

int main()
{
	FILE *fp;
	int i,j,n,ni;
	char c[160],t,ch;

	fp=fopen("A","w");
	fprintf(fp, "aaa");
	fflush(fp);
	fclose(fp);
	fp=fopen("B","w");
	fprintf(fp, "bbb");
	fflush(fp);
	fclose(fp);

	fp=fopen("A","r");
	printf(" A contents are :\n");
	for(i=0;(ch=fgetc(fp))!=EOF;i++)
	{
		c[i]=ch;
		putchar(c[i]);
	}
	fclose(fp);

	fp=fopen("B","r");
	printf("\n B contents are :\n");
	for(;(ch=fgetc(fp))!=EOF;i++)
	{
		c[i]=ch;
		putchar(c[i]);
	}
	fclose(fp);
	n=i;
	for(i=0;i<n;i++)
	for(j=i+1;j<n;j++)
	if(c[i]>c[j])
	{
		t=c[i];c[i]=c[j];c[j]=t;
	}
	printf("\n C file is:\n");
	fp=fopen("C","w");
	for(i=0;i<n;i++)
	{
		fputc(c[i],fp);
		putchar(c[i]);
	}
	putchar('\n');
	fclose(fp);
	return 0;
}
*/
