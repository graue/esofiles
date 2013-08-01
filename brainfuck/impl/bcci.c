/*  bcci.c -- Brainfuck Component Competition interpreter :)
    Version 1.2, 12/19/04
    Daniel B Cristofani
    email cristofdathevanetdotcom
    http://www.hevanet.com/cristofd/brainfuck/  */

/*Make any use you like of this software. I can't stop you anyway. :)*/

/*  This interpreter uses byte cells, and gives an error on overflow or
    underflow, or if the pointer is moved out of the array (whose size
    is defined in ARRAYSIZE). It translates newlines, and leaves cell
    values unchanged on end-of-file. It also reports the pointer's final
    location and the final values of all cells visited during execution,
    plus the number of brainfuck commands in the program, the number of
    commands executed (using Faase's [] semantics), and the amount of
    memory used, plus a composite score which is the product of these
    last three numbers.
    The pseudo-command '#' will zero all three numbers; this is to avoid
    scoring any framework which is needed to set things up before the
    execution of the component being tested, in the competition.  */


#include <stdio.h>
#include <float.h>
#include <math.h>
#define ARRAYSIZE 65536
#define CODESIZE 65536

unsigned long stackp=CODESIZE, targets[CODESIZE];
unsigned char array[ARRAYSIZE], code[CODESIZE];
long p=0, q=0, length, c, max;
unsigned long census[256];
char *filename="";
FILE *prog;
double count=0, maxcount;

void err(char *s){
    fprintf(stderr, "Error detected at byte %d of %s: %s!\n", q,filename,s);
    exit(1);
}

int main(int argc, char **argv){
    if (argc > 2) err("Too many arguments");
    if (argc < 2) err("I need a program filename");
    if(!(prog = fopen(argv[1], "r"))) err("Can't open that file");
    filename=argv[1];
    length = fread(code, 1, CODESIZE, prog);
    fclose(prog);
    maxcount=pow(FLT_RADIX, DBL_MANT_DIG);
    for(q=0; q<length; q++){
        ++census[code[q]];
        if (code[q]=='[') targets[--stackp]=q;
        if (code[q]==']'){
            if(stackp<CODESIZE) targets[targets[q]=targets[stackp++]]=q;
            else err("Unmatched ']'");
        }
        if (code[q]=='#') for (p='+'; p<=']'; p++) census[p]=0;
    }
    if(stackp<CODESIZE) q=targets[stackp], err("Unmatched '['");
    for(q=0,p=0;q<length;q++){
        switch(code[q]){
            case '+': if(array[p]++>=255) err("Overflow"); break;
            case '-': if(array[p]--<=0) err("Underflow"); break;
            case '<': p--; if(p<0) err("Too far left"); break;
            case '>': if(++p>max)max=p;if(p>=ARRAYSIZE)err("Too far right");
                          break;
#if '\n' == 10
            case ',': if((c=getchar())!= EOF) array[p]=c; break;
            case '.': putchar(array[p]); break;
#else
            case ',': if((c=getchar())!=EOF) array[p]=c=='\n'?10:c; break;
            case '.': putchar(array[p]==10?'\n':array[p]); break;
#endif
            case '[': if(!array[p]) q=targets[q]; break;
            case ']': if(array[p]) q=targets[q]; break;
            case '#': count=-1; max=0; break;
            default: count--;
        }
        if(count++>=maxcount) err("Command count overflow");
    }
    printf("Pointer: %ld\nFinal memory state:\n", p);
    for (p=0; p<=max;++p) printf(" %3d", array[p]);
    printf("\n\n");
    length=census['+']+census['-']+census['<']+census['>'];
    length+=census[',']+census['.']+census['[']+census[']'];
    printf("Program length: %ld.\nCommands executed: %.0f.\n", length,count);
    printf("Memory used: %ld.\nScore: %.0f.\n", max+1, length*count*(max+1));
    exit(0);
}

