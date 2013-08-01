/* brainfuck to Prelude compiler. Public domain by Alex Smith 2006. */
#include <stdio.h>
#include <stdlib.h>
/*

This is a brainfuck to Prelude compiler.

Brainfuck commands

+     Increment the current cell.
-     Decrement the current cell.
<     Make the previous cell current.
>     Make the next cell current.
[     If the current cell is 0, skip past the matching ].
]     If the current cell isn't 0, skip back tothe matching [.
,     Input a character to the current cell.
          This implementation assumes 0 on EOF.
.     Output the character at the current cell.

Any other character is a comment.


Prelude commands

+     Pop two values, add them, and push the result.
-     Pop two values, subtract them, and push the result.
          Top value subtracted from the second in this implementation.
#     Pop a value and discard it.
(     If the top value on the stack is zero, skip past the matching ).
)     If the top value on the stack is nonzero, skip back past the matching (.
^     Push the top value from the stack of the voice above this one.
v     Push the top value from the stack of the voice below this one.
          This implementation won't generate these aiming outside the program.
          This implementation assumes these push 0 on an empty stack.
?     Input a character and push it.
          This implementation assumes 0 on EOF.
!     Pop a value and output it as a character.
0..9  Pushes the appropriate one-digit number.

.     NOP (all other unrecognized characters are also NOPs)

\n*\n This sequence is ignored.
\n    End input for one voice, switch to next voice.

*/

char* voice1;
char* voice2;
char* tp;
int v1s, v2s, v1c, v2c;
int c;
int cc;
int curvoice=1;
char loopstack[120];
char lsp=0;

void resync(int tovoice)
{
  int ns=max(v1c,v2c)+5;
  if(ns>v1s)
    {tp=realloc(voice1,ns); if(!tp) goto errhandle; voice1=tp; v1s=ns;}
  if(ns>v2s)
    {tp=realloc(voice2,ns); if(!tp) goto errhandle; voice2=tp; v2s=ns;}
  while(v1c<v2c) voice1[v1c++]='.';
  while(v2c<v1c) voice2[v2c++]='.';
  if(curvoice==1&&tovoice==2)
  {
    voice2[v2c++]='^';
    voice1[v1c++]='#';
  }
  if(curvoice==2&&tovoice==1)
  {
    voice1[v1c++]='v';
    voice2[v2c++]='#';
  }
  curvoice=tovoice;
  return;
errhandle:
  free(voice1);
  free(voice2);
  fprintf(stderr,"Out of memory.\n");
  exit(EXIT_FAILURE);
}

int main(void)
{
  /* This compiles from stdin to stdout. */
  voice1=malloc(8); v1s=8; v1c=0;
  if(!voice1)
  {
    fprintf(stderr,"Out of memory.\n");
    return EXIT_FAILURE;
  }
  voice2=malloc(8); v2s=8; v2c=0;
  if(!voice2)
  {
    free(voice1);
    fprintf(stderr,"Out of memory.\n");
    return EXIT_FAILURE;
  }
  while((c=getchar())!=EOF)
  {
    cc=0;
    while(c=='-'||c=='+')
    {
      if(c=='-') cc--;
      if(c=='+') cc++;
      if(cc==-9||cc==9) break;
      c=getchar();
    }
    if(cc)
    {
      if(curvoice==1)
      {
        if(v1s-v1c<3)
          {v1s*=2; tp=realloc(voice1,v1s); if(!tp) break; voice1=tp;}
        voice1[v1c++]=abs(cc)+'0';
        voice1[v1c++]=cc>0?'+':'-';
      }
      else
      {
        if(v2s-v2c<3)
          {v2s*=2; tp=realloc(voice2,v2s); if(!tp) break; voice2=tp;}
        voice2[v2c++]=abs(cc)+'0';
        voice2[v2c++]=cc>0?'+':'-';
      }
    }
    if(c=='>')
    {
      if(curvoice!=1) resync(1);
      curvoice=2; /* voice1's stack is 'lefter' than voice2's */
    }
    if(c=='<')
    {
      if(curvoice!=2) resync(2);
      curvoice=1; /* voice1's stack is 'lefter' than voice2's */
    }
    if(c==',')
    {
      resync(curvoice); /* leaves at least 2 chars breathing room */
      if(curvoice==1)
        {voice1[v1c++]='#'; voice1[v1c++]='?';}
      else
        {voice2[v2c++]='#'; voice2[v2c++]='?';}
    }
    if(c=='.')
    {
      resync(curvoice);
      if(curvoice==1)
        {voice1[v1c++]='!'; voice2[v2c++]='^';}
      else
        {voice2[v2c++]='!'; voice1[v1c++]='v';}
      curvoice=3-curvoice;
    }
    if(c=='[')
    {
      loopstack[lsp++]=curvoice;
      if(lsp==120)
      {
        fprintf(stderr,"Loops too deeply nested.\n");
        free(voice1);
        free(voice2);
        return EXIT_FAILURE;
      }
      resync(curvoice);
      if(curvoice==1)
        {voice1[v1c++]='('; voice2[v2c++]='.';}
      else
        {voice2[v2c++]='('; voice1[v1c++]='.';}
    }
    if(c==']')
    {
      resync(loopstack[--lsp]);
      if(curvoice==1)
        {voice1[v1c++]=')'; voice2[v2c++]='.';}
      else
        {voice2[v2c++]=')'; voice1[v1c++]='.';}      
    }
  }
  if(c!=EOF)
  {
    fprintf(stderr,"Out of memory.\n");
    free(voice1);
    free(voice2);
    return EXIT_FAILURE;
  }
  resync(curvoice); /* make both voices the same length */
  voice1[v1c]=0; voice2[v2c]=0;
  printf("%s\n%s\n",voice1,voice2);
  free(voice1);
  free(voice2);
  return EXIT_SUCCESS;
}
