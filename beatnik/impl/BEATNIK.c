/* BEATNIK interpreter made by Catatonic Porpoise <graue@oceanbase.org>
   a long time ago. This has not been tested. This is public domain. */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

char *advance(char *, int);
char *retreat(char *, int);
int scrabble(char *);
unsigned char pop(void);
void push(unsigned char);
char characterget(void);

int main(int argc,char *argv[]) {
	FILE *f;
	char *p, cmd[10000];
	int i=0,j,k=1,s,t;

	if(argc<2) return 1;
	if((f=fopen(argv[1],"r"))==NULL) return 2;
	while((j=getc(f))!=EOF) {
		cmd[i]=j;
		i++;
	}
	fclose(f);
	cmd[i]=' ';i++;
	cmd[i]='f';i++;
	cmd[i]='o';i++;
	cmd[i]='x';i++;
	cmd[i]='y';
	p=cmd;

	while(k) {
		switch(scrabble(p)) {
			case 5: /* Push next word */
				p=advance(p,1);
				push(scrabble(p));
				p=advance(p,1);
				break;
			case 6: /* Pop number and discard it */
				pop();
				p=advance(p,1);
				break;
			case 7: /* Pop two numbers, add them, push result */
				push(pop()+pop());
				p=advance(p,1);
				break;
			case 8: /* Input character and push value */
				push(characterget());
				p=advance(p,1);
				break;
			case 9: /* Pop value and output character */
				putchar(pop());
				p=advance(p,1);
				break;
			case 10: /* Pop 2 values, subtract first from second, push result */
				t=pop();
				push(pop()-t);
				p=advance(p,1);
				break;
			case 11: /* Pop 2 values, swap them, and push them back */
				t=pop();
				s=pop();
				push(t);
				push(s);
				p=advance(p,1);
				break;
			case 12: /* Pop value, push it twice */
				t=pop();
				push(t);
				push(t);
				p=advance(p,1);
				break;
			case 13: /* Pop value, skip ahead n+1 words if value is zero */
				p=advance(p,1);
				t=scrabble(p);
				if(!pop()) p=advance(p,t);
				else p=advance(p,1);
				break;
			case 14: /* Pop value, skip ahead n+1 words if value isn't zero */
				p=advance(p,1);
				t=scrabble(p);
				if(pop()) p=advance(p,t);
				else p=advance(p,1);
				break;
			case 15: /* Pop value, skip back n words if value is zero */
				p=advance(p,1);
				t=scrabble(p);
				if(!pop()) p=retreat(p,t+1);
				else p=advance(p,1);
				break;
			case 16: /* Pop value, skip back n words if value isn't zero */
				p=advance(p,1);
				t=scrabble(p);
				if(pop()) p=retreat(p,t+1);
				else p=advance(p,1);
				break;
			case 17: /* Stop the program */
				k=0;
				break;
			default: /* Another word-length is a no-op */
				p=advance(p,1);
		}
	}

	return 0;
}

int scrabble(char *p) {
	int i,scrabble[]={1,3,3,2,1,4,2,4,1,8,5,1,3,1,1,3,10,1,1,1,1,4,4,8,4,10};
	for(i=0;*p!=0;p++) {
		i+=isalpha(*p)?scrabble[tolower(*p)-'a']:0;
		if(isspace(*p)) break;
	}
	return i;
}

char *advance(char *p, int t) {
	int i;
	for(i=0;i<t;i++) {
		p++;
		while(!isspace(*p)) p++;
		while(isspace(*p)) p++;
	}
	return p;
}

char *retreat(char *p, int t) {
	/* Note: this function assumes *p is the beginning of a word, so *(p-1)
		will be whitespace */
	int i;
	p--;
	for(i=0;i<t;i++) {
		while(isspace(*p)) p--;
		while(!isspace(*p)) p--;
	}
	p++;
	return p;
}

unsigned char stack[3000];
int sp=0;
#include <stdlib.h>

unsigned char pop(void) {
	unsigned char c;
	if(sp) {
		c=stack[--sp];
		stack[sp]=0;
		return c;
	}
	printf("FATAL ERROR: no more values on stack\n");
	exit(3);
	return 0;
}

void push(unsigned char c) {
	stack[sp++]=c;
}

#ifdef DOSCRAP

/* The below is for DOS only */

#include <conio.h>

char characterget(void) {
	char c;
	c=getch();
	if(c==13) c=getch();
	return c;
}

#else

char characterget(void) { return getchar(); }

#endif
