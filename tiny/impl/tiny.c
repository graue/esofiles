/*
NAME
    tiny.c -- interpreter for the Tiny programming language
DESCRIPTION
    This is an entire ANSI C89 program that interprets programs written
    in the language Tiny.
LICENSE TERMS
    Copyright (C) 2006 Ron Hudson
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
****************************************************************************/
/*
** Tiny  - A small Basic like RPN language
** See tiny.html for syntax details and examples
**
** Version History
** 0.0.1  Begin keeping a verion history                        6-june-2006
**        Moved Gatherformat code to inside if not string print part
**        becuase printing a single qoute should not be a problem.
**        (By the theory that if you are printing you aren't doing
**        anything else
**
** 0.1.0  Changed by Alex Smith: delinting, change to dynamic   6-june-2006
**        memory, automatic pretty-printing. The changes are
**        marked AIS in comments, apart from the automatic
**        formatting. A few functions were ANSIfied to make
**        the program strictly C89-conforming.
*/

#define VERSION "0.1.01" 
/* RAH    Re-spacing, and examining code - making comments      7-june-2006
** 
**
*/


#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>


#define TRUE 1
#define FALSE 0
#define PUT 1
#define GET 0
/* AIS #define STEPLIMIT 999*/
/* AIS #define STACKLIMIT 30*/
/* AIS #define ARRAYELEMENTS 999*/
#define LISTFORMAT "  %07ld%s"
#define PUT 1
#define GET 0
#define CMDPROMPT ">:"
#define NUMPROMPT ""

/* This macro by AIS. It uses arradjust (defined below) to ensure that a
   dynamic array has enough elements to store a value in musthave, or to
   read a value from musthave. It does not zero any new data created. */
#define ARRADJUST(a,musthave) a = arradjust(a, sizeof *a, musthave, &size##a);

typedef struct statement
{
  long            lino;
  char            text[80];
}               STATENODE;

/* Global Variables */
/* AIS: I changed some indexes from int to size_t, to allow as much
   dynamic memory to be allocated as possible. laststep was changed
   to long (the numeric type used by the program). */
long            varz[27];             /* Variables                */
/*long ustack[STACKLIMIT];    AIS     /+ $ stack                  */
/*long darray[ARRAYELEMENTS]; AIS     /+ User Array               */
size_t          ustackindex;          /* index to free stack item */
/*long compstack[STACKLIMIT]; AIS     /+ computational stack      */
size_t          compstackindex;       /* index to free stack item */
/*STATENODE progmem[STEPLIMIT]; AIS   /+ Program memory           */
long            laststep;             /* first free step          */
char            numberformat[20];     /* Number printout format   */

/* Dynamic global variables (this section by AIS) */
/* These are explicitly initialized to 0, to mark their deallocated
   status. This ought to happen anyway, but some compilers are buggy
   in this regard.
   (The convention used in this program is that used by realloc; any
   dangling pointers should immediately be replaced by null pointers,
   and a null pointer means that no memory is allocated for that
   particular job.) */
long           *ustack = 0;
long           *darray = 0;
long           *compstack = 0;
STATENODE      *progmem = 0;
size_t          sizeustack = 0;
size_t          sizedarray = 0;
size_t          sizecompstack = 0;
size_t          sizeprogmem = 0;

/* function prototypes */
void            setup(void); /* setup system */
void            cleanupatexit(void); /* AIS: cleanup system */
void           *arradjust(void *array, size_t elesize,
			  size_t musthaveele, size_t * arrsize); 
/* AIS: Ensures an
 * element is
 * addressable */

void            addprogramstep(long lino, char text[80]);
void            listprogram(void);
void            loadprogram(char filename[20]);
void            saveprogram(char filename[20]);
void            execprogram(void);
long            cpop(void); /* Pop compstack       */
long            spop(void); /* Pop storage stack   */
void            cpush(long in); /* Push comp stack     */
void            spush(long in); /* Push storage stack  */
long            inputnumber(void);


int main(int argc, char *argv[])  {
  /* AIS: Changed some int to size_t, changed comments for clarity           */
  
  char            instring[80]; /* input buffer                              */
  char            text[80];     /* Parsed input statement w/o line number    */
  long            lino;         /* Parsed line number                        */
  char            c;            /* character temporary                       */
  int             going;        /* flag is interpreter still going else exit */
  size_t          place;        /* index used while inputing a line          */
  int             i, j;         /* General counter and array pointer         */
  int             before;       /* flag, before/after line number            */

  atexit(cleanupatexit);        /* AIS: to ensure memory is freed            */

  /*
  ** See if we are running executive mode Vs Interactive mode. 
  */
  if (argc > 1)  {
    setup();
    loadprogram(argv[1]);
    execprogram();
    exit(EXIT_FAILURE);   /* Always?? What if the program completes (.) */
  }

  printf("\n\nTiny --  Interactive Mode\n\n");
  printf("Version :%s\n",VERSION);
  printf("Tiny is provided under the \nGNU General Public License \n");
  printf("See the file COPYING for details\n\n");

  setup();
  going = TRUE;
  do  {
    /*
    ** Read and store lines. This first part reads a line charactor by
    ** charactor, breaks it into line number and line. Then 
    ** addprogramstep() is used to place the line into the program memory.
    ** Commands are detected.
    */
    
    /* clear input buffer */
    for (i = 0; i < 80; i++) {
      instring[i] = '\0';
    }

    /* read input a string at a time */
    place = 0;

    /* print prompt */
    printf(CMDPROMPT);

    /* grab charactors until end of line or line too long */
    do {
      c = fgetc(stdin);
      instring[place] = c;
      place = place + 1;
      if (place > 80) {
	printf("\n\n-- Line too long \n");
      }
    } while (c != '\0' && c != '\n' && place < 80);


    /* detect and execute a command */
    if (instring[0] == '#') {

      /* bye (exit interpreter ) */
      if (tolower(instring[1]) == 'b') {
	going = FALSE;
	printf("\n\n-- End of Session \n");
      }

      /* run program #r */
      if (tolower(instring[1]) == 'r') {
	execprogram();
      }

      /* #l list program */
      if (tolower(instring[1]) == 'l') {
	listprogram();
      }

      /* #s filename   Save */
      if (tolower(instring[1] == 's')) {
	/* 
	** discard '#s' and copy filename into text[]
	** todo- need to actually parse out multiple spaces
	*/
	i = 0;
	j = 0;
	before = FALSE;
	while (i < strlen(instring))  {
	  if (instring[i] == ' ')  {
	    before = TRUE;
	    /* skip the space */
	    i++;
          }


	  if (before)  {
	    /* AIS: Added test for period, a common filename character. */
	    if (isalnum(instring[i]) || instring[i] == '.')  {
	      text[j] = instring[i];
	      j++;
	      text[j] = '\0';
            } else  {
	      text[j] = '\0'; /* AIS */
            }	    
	  }
	  i++;
	}
	saveprogram(text);
      }


      /* #n new, Clear program */
      if (tolower(instring[1] == 'n')) {
	setup();
      }


      /* #o filename  old program (load the program) */
      if (tolower(instring[1]) == 'o') {
	/* discard '#o' and copy filename into text[] */
	i = 0;
	j = 0;
	before = FALSE;
	while (i < strlen(instring)) {
	  if (instring[i] == ' ') {      /* looking for end of name */
	    before = TRUE;
	    /* skip the space */
	    i++;
	  }

	  if (before)  {
	    if (isalnum(instring[i])) {
	      text[j] = instring[i];     /* move a char from input to name */
	      j++;                       /* next space in name             */
	      text[j] = '\0';            /* ensure string ends in null     */
	    }
	  }
	  i++;
	}
	loadprogram(text);
      }
    }


    /* Perhaps it's a statement to store */
    if (isdigit(instring[0])) {
      /* separate statement into lino and text */
      lino = 0;
      j = 0;
      before = TRUE;
      for (i = 0; i <= strlen(instring); i++) {
	/* locate break after line number */
	if (!isdigit(instring[i]))  {
	  before = FALSE;
	}
	/* if still line number */
	if (before) {
	  /* collect each digit of line number */
	  lino = lino * 10 + (instring[i] - '0');
	} else  {
	  /* else collect text from rest of string */
	  text[j] = instring[i];
	  j = j + 1;
	}
      }
      text[j] = '\0';  /* ensure an end of line sentinel */

      /* store line in program structure here... */
      addprogramstep(lino, text);
      printf(LISTFORMAT, lino, text); /* echo program step */
    }
  } while (going);
  return 0;
}


void setup(void)  {
  int             i;


  /* 
  ** initialize all 26 variables.
  */

  for (i = 0; i < 27; i++) {
    varz[i] = 0;
  }


  /* AIS: initialize dynamic arrays (by realloc to 0) */
  ustack = realloc(ustack, 0);
  sizeustack = 0;
  darray = realloc(darray, 0);
  sizedarray = 0;
  compstack = realloc(compstack, 0);
  sizecompstack = 0;
  progmem = realloc(progmem, 0);
  sizeprogmem = 0;

  /* initialize number format */
  strcpy(numberformat, "%ld");
}



/* AIS: Atexit cleanup procedure. */
void cleanupatexit(void)  {

  /* Free memory and zero the pointers. */
  ustack = realloc(ustack, 0);
  sizeustack = 0;
  darray = realloc(darray, 0);
  sizedarray = 0;
  compstack = realloc(compstack, 0);
  sizecompstack = 0;
  progmem = realloc(progmem, 0);
  sizeprogmem = 0;
}


/* AIS: Expand an array if needed. */
void *arradjust(void *a, size_t elesize, size_t musthave, size_t * arrsize) {
  void           *temp;

  if (*arrsize > musthave) {
    return a;   /* Array is already big enough */
  }

  /*
   * Determine new arrsize. Algorithm: Add 4 to arrsize if arrsize < 32, to
   * save memory if only a few elements are used; Double arrsize if arrsize *
   * elesize < SIZE_MAX/3, to save time when large amounts of data are used;
   * Add 1024 to arrsize otherwise, to prevent overwrapping size_t.
   */

  if (*arrsize < 32)
    *arrsize += 4;
#ifdef SIZE_MAX
  else if (*arrsize * elesize < SIZE_MAX / 3)
    *arrsize *= 2;
#else
  else if (*arrsize * elesize < ULONG_MAX / 3)
    *arrsize *= 2;
#endif
  else
    *arrsize += 1024;
  temp = realloc(a, *arrsize * elesize);
  if (!temp)  {
    printf("Tiny-- Out of memory\n"); /* This resembles some of the other
      * error messages */
    free(a);
    a = 0;
    exit(EXIT_FAILURE);
  }
  a = temp;
  return a;
}


/* This procedure adds a line to the program  */
void  addprogramstep(long lino, char text[80])
{
    ARRADJUST(progmem, laststep); /* AIS */
    progmem[laststep].lino = lino;
    strcpy(progmem[laststep].text, text);
    laststep++;

}

/* This function commented out as it is unused. It has not been
   converted to use dynamic memory. (AIS)


   void NEWaddprogramstep(long lino, char text[80]) {
   int i;
   if (strlen(text) == 0) {
   /+ find and delete lino +/
   } else {
   /+ Search forward +/
   i=0;
   while (lino < progmem[i].lino ) i++;
   if (progmem[i].lino == lino) {
   /+
   ++ Replace this line (lino is already correct, just copy in
   ++ the new text.
   +/
   strcpy(progmem[i].text, text);
   } else {
   /+ Insert this line here +/
   }
   }
   }
*/


void listprogram(void)
{
  int             i;
  for (i = 0; i < laststep; i++)
  {
    printf(LISTFORMAT, progmem[i].lino, progmem[i].text);
  }
  printf("\n");
}


void  loadprogram(char filename[20])  {

  FILE            *fp;
  char            instring[80];
  char            text[80];
  int             before;
  int             i, j;
  long            lino;
  char           *eflag;

  fp = fopen(filename, "r");
  if (fp == NULL)
  {
    printf("Tiny--Can't open file [%s] \n", filename);
  } else
  {
    do
    {
      eflag = fgets(instring, 80, fp);
      if (eflag != NULL)
      {
	/* separate statement into lino and text */
	lino = 0;
	j = 0;
	before = TRUE;
	for (i = 0; i <= strlen(instring); i++)  {
	  /* locate break after line number */
	  if (!isdigit(instring[i]))  {
	    before = FALSE;
	  }
	  /* if still line number */
	  if (before) {
	    /* collect each digit of line number */
	    lino = lino * 10 + (instring[i] - '0');
	  } else  {
	    /* else collect text from rest of string */
	    text[j] = instring[i];
	    j++;
	  }
	}
	text[j] = '\0';  /* ensure an end of line sentinel */
	addprogramstep(lino, text);
	/* printf(LISTFORMAT,lino,text); */
      }
    } while (eflag != NULL);
    fclose(fp);
  }
}


/* AIS: Removed illegal trailing semicolons on these functions. */
long cpop(void) {
  compstackindex--;
  return compstack[compstackindex];
}

long spop(void) {
  ustackindex--;
  return ustack[ustackindex];
}

void cpush(long in) {
  ARRADJUST(compstack, compstackindex); /* AIS */
  compstack[compstackindex] = in;
  compstackindex++;
}

void spush(long in) {
  ARRADJUST(ustack, ustackindex); /* AIS */
  ustack[ustackindex] = in;
  ustackindex++;
}


/*
** execprogram
**
** This executes the program that is currently loaded.
**
*/
void execprogram(void) {
  long            x, y;        /* Temporary                               */
  long            exlino;      /* effective lino (@ register)             */
  long            thenumber;   /* Used to collect numeric constants       */
  char            xtext[80];   /* Program text being interpreted          */
  char            xchar;       /* actual char being interpreted           */
  char            xstr[2];     /* when we need a string xchar instead     */
  int             i;           /* loop indexes                            */
  int             running;     /* flag - running                          */
  int             progmemstep; /* index into progmem                      */
  int             putget;      /* put get flag                            */
  int             indirect;    /* indirect flag (array?)                  */
  int             numbuild;    /* building a number                       */
  int             StringPrint; /* Printing                                */

  /* int AIS: unused  BackSlash; /+ Prev Char was a backslash       */
  /* int AIS: unused Parencomment; /+ Paren comment level             */
  /* int AIS: unused Poundcomment; /+ Pound Comment                   */

  long            arrayindex; /* Index into user array                   */
  int             gatherformat; /* Flag used while reading format strings  */


  /* setup */
  putget = GET;
  numbuild = FALSE;
  StringPrint = FALSE;
  /* BackSlash    = FALSE; AIS: Unused */
  indirect = FALSE;
  /* Parencomment = 0;    AIS: Unused /+ not inside parens */
  /* Poundcomment = FALSE; AIS: Unused /+ No Poundsign comment yet */
  gatherformat = FALSE;

  /* Get first Line Number */
  progmemstep = 0;
  exlino = progmem[progmemstep].lino;
  running = TRUE;
  do {
    /* locate line from @ */
    progmemstep = 0;
    while (progmem[progmemstep].lino != exlino) {
      progmemstep++;
      /* if past lastline then error message stop */
      if (progmemstep > laststep)  {
	printf("Tiny-- Attempt to jump to %ld, line not found\n", exlino);
	running = FALSE;
	break;
      }
    }

    /* fetch line */
    strcpy(xtext, progmem[progmemstep].text);
    if (strlen(xtext) == 0) {
      printf("Tiny-- Execute past end of program\n");
      running = FALSE;
    }

    /* set @ to line number of next line */
    exlino = progmem[progmemstep + 1].lino;

    /* interpret line */
    for (i = 0; i < strlen(xtext); i++) {
      xchar = xtext[i]; 

      if (xchar == '#')  {
	break;  
      }

      if (StringPrint)
      {
	if (xchar == '\\')
	  {
	    i++;
	    xchar = xtext[i];
	    switch (xchar)  {

	    case 'n':
	      printf("\n");
	      break;

	    case 't':
	      printf("\t");
	      break;

	    case '\\':
	      printf("\\");
	      break;

	    case 'e':
	      printf("\033");
	      break;

	    case '"':
	      printf("\"");
	      break;

	    case '\'':
	      printf("'");
	      break;

	    default:
	      printf("\n**backslash what? %c\n", xchar);
	    }
	  } else  {
	    if (xchar == '"')
	      {
		StringPrint = FALSE;
	      } else  {
		printf("%c", xchar);
	      }
	  }
      } else  {


	/*
	** Gatherformat, this section handles the format string * looking
	** for a single qoute and gathering everythg beween * the first
	** single qoute and the second.
	*/
	if (gatherformat) {
	  if (xchar == '\'')
	    {
	      xchar = '\0';
	      gatherformat = FALSE;
	    } else  {
	      xstr[0] = xchar;
	      xstr[1] = '\0';
	      strcat(numberformat, xstr);
	      xchar = '\0';
	    }
	}
	if (xchar == '\'') {
	  /*
	  ** When first we notice a single qoute, clear the numberformat 
	  ** and begin to gather new characters in numberformat.
	  */
	  gatherformat = TRUE;
	  strcpy(numberformat, "\0");
	}

	/* user Array usage */
	if (xchar == '(') {
	    indirect = TRUE;
	}

	if (xchar == ')') {
	  indirect = FALSE;
	}

	/* period is the "stop program " */
	if (xchar == '.') {
	  running = FALSE;
	  break;
	}
	
	if (isdigit(xchar)) {
	  if (numbuild)  {
	    /*
	    ** should probably try to detect numeric constant that
	    ** is too big here.
	     */
	    thenumber = (10 * thenumber) + (xchar - '0');
	  } else  {
	    numbuild = TRUE;
	    thenumber = xchar - '0';
	  }
	}

	/* first non digit after a number */
	if (!isdigit(xchar)) {
	  if (numbuild) {
	    cpush(thenumber);
	    numbuild = FALSE;
	  }
	}

	/* Get/Put Variables */
	xchar = tolower(xchar);
	if (xchar <= 'z' && 'a' <= xchar) {
	  if (putget == GET)  {
	      if (indirect) {
		arrayindex = varz[xchar - 'a'];
		if (  arrayindex >= 0) {
		  ARRADJUST(darray, arrayindex); /* AIS */
		  cpush(darray[arrayindex]);
		} else {
		  printf("TINY %ld -- Recall from Array out of range %ld \n", 
			 exlino, arrayindex);
		}
	      } else {
		cpush(varz[xchar - 'a']);
	      }
	  } else {
	    if (indirect) {
	      arrayindex = varz[xchar - 'a'];
	      if ( arrayindex >= 0) { 
		ARRADJUST(darray, arrayindex); /* AIS */
		darray[arrayindex] = cpop();
		cpush(darray[arrayindex]);
	      } else  {
		printf("TINY %ld -- Store to Arry out of range %ld \n", 
		       exlino, arrayindex);
	      }
	    } else {
	      varz[xchar - 'a'] = cpop();
	      cpush(varz[xchar - 'a']);
	    }
	  }
	}
	
	switch (xchar)  {
	case '[':
	  compstackindex = 0;
	  putget = GET;
	  break;

	case ']':
	  putget = PUT;
	  break;

	case '"':
	  StringPrint = TRUE;
	  break;

	case '+':
	  cpush(cpop() + cpop());
	  break;

	case '*':
	  cpush(cpop() * cpop());
	  break;

	case '!':
	  cpush(!cpop());
	  break;

	case '%':
	  x = cpop();
	  y = cpop();
	  cpush(y % x);
	  break;

	case '^':
	  x = cpop();
	  y = cpop();
	  cpush((long) pow((double) y, (double) x));
	  break;

	case '-':
	  x = cpop();
	  y = cpop();
	  cpush(y - x);
	  break;

	case '/':
	  x = cpop();
	  y = cpop();
	  if (x == 0)  {
	    printf("Tiny -- %ld div by zero! Black hole forming!\n", exlino);
	  } else {
	    cpush(y / x);
	  }
	  break;

	case '<':
	  x = cpop();
	  y = cpop();
	  cpush(y < x);
	  break;

	case '>':
	  x = cpop();
	  y = cpop();
	  cpush(y > x);
	  break;

	case '=':
	  cpush(cpop() == cpop());
	  break;

	case '&':
	  cpush(cpop() && cpop());
	  break;

	case '|':
	  cpush(cpop() || cpop());
	  break;

	  /* Special Variables */
	case '~':
	  if (putget == GET) {
	    cpush(labs( rand())); 
	  } else {
	    x = cpop();
	    cpush(x);
	    if (x == 0) {
	      x = (int) time(NULL);
	    }
	    srand(x); 
	  }
	  break;

	case '?':
	  if (putget == GET) {
	    printf("%s", NUMPROMPT); /* AIS: Satisfying a reasonable
				      * compiler warning */
	    x = inputnumber();
	    cpush(x);
	  } else {
	    x = cpop();
	    cpush(x);
	    printf(numberformat, x);
	  }
	  break;

	case '@':
	  if (putget == GET)  {
	    cpush(exlino);
	  } else {
	    x = cpop();
	    cpush(x);
	    if (x != 0) {
	      exlino = x;
	    }
	  }
	  break;

	case '$':
	  if (putget == GET) {
	    cpush(spop());
	  } else {
	    x = cpop();
	    cpush(x);
	    spush(x);
	  }
	  break;
	}
      }
    }

    if ((compstackindex + 1) == 0) { 
      /* 
      ** AIS: Changed condition to allow
      ** for unsigned compstackindex 
      */
    
      printf("*** Tiny -- Comp Stack underflow \n");
      running = FALSE;
    }
  } while (running);  /* execute do loop */
}    /* execprogram */


long inputnumber(void) {

#define CR '\n'   /* AIS: ANSIfication */
#define BS '\b'   /* AIS: ANSIfication */

  long            val;
  char            c;
  int             nflag;
  val = 0;
  nflag = FALSE;
  do {
    c = getchar();
    if (c != CR) {
      if (c == BS) {
	val = val / 10;
      }
      if (c == '-') {
	nflag = TRUE;
      }
      if (c <= '9' && c >= '0') {
	val = (val * 10) + (c - '0'); 
      }
    }
  } while (c != CR);

  if (nflag) {
    val = 0 - val;
  }
  return val;
}


void saveprogram(char filename[20]) {

  FILE           *fp;
  long            i;


  fp = fopen(filename, "w");
  /* AIS: Check for program not openable */
  if (!fp)  {
    perror(filename);   /* rah why not print a tiny style error message */
    return;
  }
  for (i = 0; i < laststep; i++) {
    fprintf(fp, LISTFORMAT, progmem[i].lino, progmem[i].text);
  }
  fclose(fp);
  /* AIS: File Saved message */
  printf("Tiny-- File saved to %s.\n", filename);
}
