#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

int main(int argc,char **argv)
{
printf("%d\n",argc);
FILE*fp =fopen("log.txt","r");
assert(argv[1]);
int pre_readline=atoi(argv[1]);
char buffer[1024];
for(int i=0;i<pre_readline;i++)
fgets(buffer,1024,fp);

char c;
int bra=0;
while((c=fgetc(fp))!=EOF)
{
if(c=='(') bra++;
else if(c==')') bra--;
if(c=='('||c==')')
printf("%c val=%d\n",c,bra);
assert(bra>=0&&bra<=5);
}

}
