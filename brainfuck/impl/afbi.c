/*
 * afbi -- another fast brainfuck interpreter
 * usage:
 * afbi program.b
 *
 *
 * Copyright (c) 2005 Jannis Harder
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */



#include <stdio.h>
#include <string.h>

typedef struct{
	int instruction;
	int arg_a;
	int arg_b;
	void* arg_c;
} bfi;

enum { Iadd,Imov,Iprint,Iget,IloopO,IloopC,Izero,Iexit,sizeof_I};

#define program_realloc program=(bfi*)realloc(program,sizeof(bfi)*(program_s<<=2))
#define loopstack_realloc loopstack=(int*)realloc(loopstack,sizeof(int)*(loopstack_s<<=2))
#define nexti if((++program_p)>=program_s)program_realloc
#define nexts if((++loopstack_p)>=loopstack_s)loopstack_realloc
#define next ci = *(++tmp);goto *(ins[ci.instruction]);

int main (int argc, const char * argv[]) {
    FILE * inf = fopen(argv[1],"r");
	void * ins[sizeof_I];
	bfi * program;
	int program_s;
	int program_p=-1;
	int * loopstack;
	int loopstack_s;
	int loopstack_p=-1;
	int incs[100];
	int incp=50;
	int i,j;
	bfi * tmp;
	char inbyte;
	char inbyte2;
	char *mem;
	int mem_s;
	char *mem_max;
	char *mem_min;
	char *mem_p;
	bfi ci;
	program_s=1000;
	program = (bfi*)malloc(sizeof(bfi)*program_s);
	loopstack_s=100;
	loopstack = (int*)malloc(sizeof(int)*loopstack_s);
	
	
	while((inbyte=getc(inf))!=EOF){
		rescan:
		switch(inbyte){
			case '+':
			case '-':
			case '<':
			case '>':
				incp=50;
				memset(incs,0,sizeof(int)*100);
				do {
					switch(inbyte){
						case '+':
							incs[incp]++;
							incs[incp]&=0xFF;
						break;
						case '-':
							incs[incp]--;
							incs[incp]&=0xFF;
						break;
						case '>':
							incp++;
						break;
						case '<':
							incp--;
						break;
						break;
					}
				} while ((inbyte=getc(inf))!=EOF && (inbyte=='+' || inbyte=='-' || inbyte=='<'  || inbyte=='>' )&& incp>0 && incp<100);
				for(i=0;i<100;i++){
					if(incs[i]){
						nexti;
						tmp= &program[program_p];
						tmp->instruction=Iadd;
						tmp->arg_a=incs[i];
						tmp->arg_b=i-50;
					}
				}
				if(incp!=50){
	
					nexti;
					tmp= &program[program_p];
					tmp->instruction=Imov;
					tmp->arg_a=incp-50;
				}
				goto rescan;
			break;
			case '.':
				nexti;
				tmp= &program[program_p];
				tmp->instruction=Iprint;
			break;
			case ',':
				nexti;
				tmp= &program[program_p];
				tmp->instruction=Iget;
			break;
			case '[':
				inbyte=getc(inf);
				inbyte2=getc(inf);
				nexti;
				tmp= &program[program_p];
				if((inbyte=='+' || inbyte=='-')&&inbyte2==']'){
					tmp->instruction=Izero;
				}
				else{
					ungetc(inbyte2,inf);
					ungetc(inbyte,inf);
					
					
					tmp->instruction=IloopO;
					nexts;
					loopstack[loopstack_p]=program_p;
				}
			break;
			case ']':
				nexti;
				tmp= &program[program_p];
				tmp->instruction=IloopC;
				tmp->arg_a=loopstack[loopstack_p--];
				program[tmp->arg_a].arg_a=program_p;
			break;
		}
	}
	if(loopstack_p+1) exit(1);
	nexti;
	program[program_p].instruction=Iexit;
	
	for(i=0;i<program_p;i++){
		tmp=&program[i];
		if(tmp->instruction==IloopC||tmp->instruction==IloopO)
			tmp->arg_c=(void*)(&program[tmp->arg_a]);
	}
	
	free(loopstack);
	mem_s=3000;
	mem=(char*)malloc(mem_s);
	memset(mem,0,mem_s);
	mem_max=mem+mem_s-100;
	mem_min=mem_p=mem+100;
	
	tmp=program-1;
	
	
	ins[Iadd]=&&lIadd;
	ins[Imov]=&&lImov;
	ins[Iprint]=&&lIprint;
	ins[Iget]=&&lIget;
	ins[IloopO]=&&lIloopO;
	ins[IloopC]=&&lIloopC;
	ins[Izero]=&&lIzero;
	ins[Iexit]=&&lIexit;
	
		
		next
		lIadd:
			mem_p[ci.arg_b]+=ci.arg_a;
		
		next
		lImov:
			mem_p+=ci.arg_a;
			if(mem_max>mem_p){
				mem=(char*)realloc(mem,mem_s*2);
				memset(mem+mem_s,0,mem_s);
				mem_s<<=2;
				mem_max=mem+mem_s-100;
			}
			else if(mem_min>mem_p){
				free(mem);
				free(program);
				exit(2);
			};
		
		next
		lIprint:
			putchar(*mem_p);
		
		next
		lIget:
			#ifndef RAW_IO
				inbyte=getchar();
				if(inbyte==0 || inbyte==-1)
					inbyte^=0xFF;
				*mem_p=inbyte;
			#else
				*mem_p=getchar();
			#endif
		
		next
		lIloopO:
			if(!(*mem_p))
				tmp=(bfi*)ci.arg_c;
		
		next
		lIloopC:
			if(*mem_p)
				tmp=(bfi*)ci.arg_c;
		
		next
		lIzero:
			*mem_p=0;
		
		next
		lIexit:
			free(mem);
			free(program);
			exit(0);

	
	
	
    return 0;
}
