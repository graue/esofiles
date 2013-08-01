/* Prelude interpreter. Public domain by Alex Smith 2006. */
#include <stdio.h>
#include <stdlib.h>

/* Some DOS implemetations need a special keyword to handle large static arrays.
   implhuge enables this keyword to be given on the command line. */
#ifndef implhuge
#define implhuge
#endif

#define VOICEMAX 10
#define NOTEMAX 99999

/*

This is a Prelude intepreter. Not all language features are implemented yet, however.

Prelude commands

+     Pop two values, add them, and push the result.
-     Pop two values, subtract them, and push the result.
          Top value subtracted from the second in this implementation.
#     Pop a value and discard it.
(     If the top value on the stack is zero, skip past the matching ).
)     If the top value on the stack is nonzero, skip back past the matching (.
^     Push the top value from the stack of the voice above this one.
v     Push the top value from the stack of the voice below this one.
          This implementation pushes 0 if these aim outside the program.
          This implementation assumes these push 0 on an empty stack.
?     Input a character and push it.
          This implementation assumes 0 on EOF.
!     Pop a value and output it as a character.
0..9  Pushes the appropriate one-digit number.

N     NOP (all other unrecognized characters are also NOPs)

\n*\n This sequence is ignored. /not yet implemented/
\n    End input for one voice, switch to next voice.

*/

char implhuge voice[VOICEMAX][NOTEMAX]={0}; /* fixed array for now */
char implhuge stack[VOICEMAX][NOTEMAX]={0}; /* fixed array for now */
int           kp[VOICEMAX]={0};             /* stack pointers. Points to TOS. */

char eoget(void)
{
  int c;
  c=getchar();
  if(c==EOF) return 0;
  return (char)c;
}

int main(int argc, char** argv)
{
  FILE* in;
  int sp,cv,nv,c,f;
  if(argc!=2)
  {
    fprintf(stderr,"Usage: prelude inputfile\n");
    return EXIT_FAILURE;
  }
  in=fopen(argv[1],"r");
  if(!in)
  {
    perror(argv[1]);
    return EXIT_FAILURE;
  }
  cv=0; c=0;
  while(c!=EOF)
  {
    sp=0;
    while((c=getc(in))!=EOF)
    {
      voice[cv][sp++]=c;
      if(sp>NOTEMAX-1) {fprintf(stderr,"Out of memory.\n"); return EXIT_FAILURE;}
      if(c=='\n') break;
    }
    cv++;
    if(cv>VOICEMAX-1) {fprintf(stderr,"Out of memory.\n"); return EXIT_FAILURE;}
  }
  fclose(in);
  nv=cv; sp=0;
  while(1)
  {
    /* add more 0s to the stacks if required */
    cv=0;
    while(cv<nv)
    {
      if(!kp[cv])
      {
        stack[cv][1]=stack[cv][0];
        stack[cv][0]=0;
        kp[cv]=1;
      }
      cv++;
    }
    /* Make two scans of each note, to fake simultaneous calculation.
       First, in the case of ^ and v, the next location on the stack is
       changed, but the stack pointer doesn't move.
       Second, other updates are done. */
    cv=0;
    while(cv<nv)
    {
      if(voice[cv][sp]=='^')
      {
        if(cv!=0)
          stack[cv][kp[cv]+1]=stack[cv-1][kp[cv-1]];
        else
          stack[cv][kp[cv]+1]=0;
      }
      if(voice[cv][sp]=='v')
      {
        if(cv!=nv-1)
          stack[cv][kp[cv]+1]=stack[cv+1][kp[cv+1]];
        else
          stack[cv][kp[cv]+1]=0;
      }
      cv++;
    }
    cv=0;
    while(cv<nv)
    {
      if(voice[cv][sp]=='#') kp[cv]--;
      if(voice[cv][sp]=='!') putchar(stack[cv][kp[cv]--]);
      if(voice[cv][sp]=='?') stack[cv][++kp[cv]]=eoget();
      if(voice[cv][sp]=='^') kp[cv]++;
      if(voice[cv][sp]=='v') kp[cv]++;
      if(voice[cv][sp]=='+')
      {
        kp[cv]--; stack[cv][kp[cv]]+=stack[cv][kp[cv]+1];
      }
      if(voice[cv][sp]=='-')
      {
        kp[cv]--; stack[cv][kp[cv]]-=stack[cv][kp[cv]+1];
      }
      if(voice[cv][sp]>='0'&&voice[cv][sp]<='9')
        stack[cv][++kp[cv]]=voice[cv][sp]-'0';
      if(voice[cv][sp]=='\n') return EXIT_FAILURE;
      if(voice[cv][sp]=='('&&!stack[cv][kp[cv]])
      {
        sp++;
        f=1;
        while(voice[0][sp+1]!='\n')
        {
          c=0;
          while(c<nv)
          {
            if(voice[c][sp]==')') f--;
            if(voice[c][sp]=='(') f++;
            c++;
          }
          sp++;
          if(!f) break;
        }
      }
      if(voice[cv][sp]==')'&&stack[cv][kp[cv]])
      {
        sp--;
        f=1;
        while(sp)
        {
          c=0;
          while(c<nv)
          {
            if(voice[c][sp]==')') f++;
            if(voice[c][sp]=='(') f--;
            c++;
          }
          sp--;
          if(!f) break;
        }
      }
      cv++;
    }
    sp++;
  }
}
