/* pointy.c
 * Pointy interpreter
 * by Marinus Oosters
 *
 * see http://esoteric.voxelperfect.net/wiki/Pointy
 *
 * Memory cells are 32-bit, unsigned integers.
 * Labels are not case-sensitive. 
 * Comments start with ; (because that's what they did on the esolang wiki page),
 * or with # (so I can do #!/), and end with a newline.
 */

#include <stdio.h>

#define MEMSZ	 0x10000
#define N_LBL	 0x100 // max number of labels in a program
#define LBL_LEN  20 // max label length
#define LINE_LEN 200 // max line length
#define N_LINES  0x10000 // max number of lines in a program;
#define ARG_LEN  140 // max argument length
/* err */
void die(char *errmsg) {
	fprintf(stderr, "error: %s\n", errmsg);
	exit(0);
}

/* labels */
typedef struct {
	char name [LBL_LEN];
	int line;
} label;

char c[50];
int lbl_amt=0;
label lbls[N_LBL];

int getlbl(char *l) { // get line no for label name
	int i,r=-1;
	for (i=0; i<lbl_amt; i++) {
		if (!strcasecmp(lbls[i].name,l)) {
			r = lbls[i].line;
		       	break;
		}
	}
	if (r == -1) {
		sprintf(c,"%s does not exist.",l);
		die(c);
	}
	return r;
}

void setlbl(char *l, int ln) { // set label
	if (lbl_amt < N_LBL) {
		lbls[lbl_amt].line = ln;
		strcpy(lbls[lbl_amt++].name,l);
	} else {
		sprintf(c,"cannot store %s, too many labels (max %n).",l,N_LBL);
		die(c);
	}
}

// memory
unsigned int memory[MEMSZ];

// dereference
int value(char *n) { // gets value for number/pointer in n
	int r=0,i,a=0;
	char *t=n;
	while (*t && !isdigit(*t)) t++;
	r = atoi(t); // got the number, start dereferencing
	while (*n && !isdigit(*n)) if (*(n++)=='*') a++;
	for (;a;a--) {
		if (r<0||r>MEMSZ) {
			sprintf(c,"cannot dereference, %d is outside allocated memory (%d-%d)",r,0,MEMSZ);
			die(c);
		}
		r = memory[r];
	}
	return r;
}

// some more error checking
int memval(int n) {
	if (n<0||n>MEMSZ) {
		sprintf(c,"cannot dereference, %d is outside allocated memory (%d-%d)",n,0,MEMSZ);
		die(c);
	}
	return memory[n];
}

void setval(int n, int v) {
	if (n<0||n>MEMSZ) {
		sprintf(c,"cannot assign, %d is outside allocated memory (%d-%d)",n,0,MEMSZ);
		die(c);
	}
	memory[n]=v;
}

// program code
typedef enum { LBL, CPY, INC, DEC, OUT, INP } instr_t;
typedef struct {
	instr_t instruction;
	char op1[ARG_LEN];
	char op2[ARG_LEN];
} line;
line pgm [N_LINES];
int n_of_lines=0, ip=0;

// read program
void readprogram(char *filename){
	char buf[LINE_LEN], *lc=buf, ch, *ptr;
	bzero(pgm,sizeof(pgm));
	bzero(buf,LINE_LEN);
	
	// read in the program
	FILE *f;
	f=fopen(filename, "r");
	if (!f) {
		sprintf(c,"cannot open %s for reading",filename);
		die(c);
	}
	while (!feof(f)) {
		ch = fgetc(f);
		if (ch==';'||ch=='#') while (!feof(f) && (ch=fgetc(f))!='\n');
		if (ch=='\n' || feof(f)) {
			// try to parse.
			*lc = 0;
			lc = buf;
			while (*lc && isspace(*lc)) lc++;
			if (!*lc) { // empty line or only a comment
				bzero(buf,LINE_LEN);
				lc=buf;
				continue;
			}
			// get instruction
			if (!strncasecmp(lc,"LBL",3)) pgm[n_of_lines].instruction = LBL;
			else if (!strncasecmp(lc,"CPY",3)) pgm[n_of_lines].instruction = CPY;
			else if (!strncasecmp(lc,"INC",3)) pgm[n_of_lines].instruction = INC;
			else if (!strncasecmp(lc,"DEC",3)) pgm[n_of_lines].instruction = DEC;
			else if (!strncasecmp(lc,"OUT",3)) pgm[n_of_lines].instruction = OUT;
			else if (!strncasecmp(lc,"INP",3)) pgm[n_of_lines].instruction = INP;
			else {
				strncpy(c,lc,3);
				c[3] = 0;
				sprintf(c,"%d: '%s' is not a valid instruction.",n_of_lines+1,c);
				die(c);
			}
			
			// get first argument
			lc += 3; 
			while (*lc && isspace(*lc)) lc++;
			if (!*lc) {
				sprintf(c,"%d: no arguments given.",n_of_lines);
				die(c);
			}
			ptr = pgm[n_of_lines].op1;
			while (*lc && !isspace(*lc)) *(ptr++) = *(lc++);
				
			
			// if CPY or DEC, get second output.
			if (pgm[n_of_lines].instruction == CPY || pgm[n_of_lines].instruction == DEC) {
				while (*lc && isspace(*lc)) lc++;
				if (!*lc) {
					sprintf(c,"%d: instruction requires two arguments, but only one was given.",n_of_lines+1);
					die(c);
				}
				ptr = pgm[n_of_lines].op2;
				while (*lc && !isspace(*lc)) *(ptr++) = *(lc++);
			}

			// we're done.
			n_of_lines++;
			bzero(buf,LINE_LEN);
			lc = buf;
		} else *(lc++)=ch;
	}

	// find all the labels and store them
	for (ip = 0; ip < n_of_lines; ip++) 
		if (pgm[ip].instruction == LBL) 
			setlbl(pgm[ip].op1, ip);
}	

/* Of a 209-line interpreter, the actual interpreting function is only 17 lines... */
void run() {
	// run the program
	for (ip = 0; ip < n_of_lines; ip++) {
		switch (pgm[ip].instruction) {
			case LBL: break; // don't do anything, labels have already been stored.
			case CPY: setval(value(pgm[ip].op2),value(pgm[ip].op1)); break;
			case INC: setval(value(pgm[ip].op1),memval(value(pgm[ip].op1))+1); break;
			case DEC: if (memval(value(pgm[ip].op1))) {
					  setval(value(pgm[ip].op1), memval(value(pgm[ip].op1))-1);
				  } else {
					  ip = getlbl(pgm[ip].op2);
				  }; break;
			case OUT: putchar(value(pgm[ip].op1));break;
			case INP: setval(value(pgm[ip].op1),feof(stdin)?0:getchar());break;
		}
	}
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr,"%s - Pointy interpreter\nWritten by Marinus Oosters\n\tusage: %s program\n\n",
				argv[0],argv[0]);
		exit(0);
	}
	readprogram(argv[1]);
	run();
	return 0;
}
