// lnusp.c
// lnusp interpreter
// by Marinus Oosters

/* I assumed the following:
 * - Line format is three bytes which form a number, then one byte that is
 *   ignored, then the program code, starting at index 1.
 *          NNN?code.code.code.code.code.....
 * - Array offsets are one-based.
 * - The data space and program space are separate
 * - Anything that is not a command, is ignored.
 * - If the program pointer hits the north at one of the
 *   special places (x=8, x=24, x=41) that subroutine will be
 *   executed before the direction is reversed.
 * - If the program pointer exits the program space and no subroutine
 *   is at that place, this is an error.
 *
 * Modified a bit by Graue.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char memory[8000][8000];
char prgm[8000][8000];

typedef enum {n,ne,e,se,s,sw,w,nw} direction;
typedef struct {int x; int y; direction d;} vector;

vector next(vector vv) {
	vector v=vv;
	switch(v.d) {
		case n: v.y--; break;
		case ne: v.y--; v.x++; break;
		case e: v.x++; break;
		case se: v.y++; v.x++; break;
		case s: v.y++; break;
		case sw: v.y++; v.x--; break;
		case w: v.x--; break;
		case nw: v.y--; v.x--; break;
	}
	return v;
}
vector reverse(vector vv) {
	vector v=vv;
	switch(v.d) {
		case n: v.d=s; break;
		case ne: v.d=sw; break;
		case e: v.d=w; break;
		case se: v.d=nw; break;
		case s: v.d=n; break;
		case sw: v.d=ne; break;
		case w: v.d=e; break;
		case nw: v.d=se; break;
	}
	return v;
}

vector l45(vector v) { // 45 degrees left
	v.d = (v.d - 1) % 8;
	return v;
}
vector r45(vector v) { // 45 degrees right
	v.d = (v.d + 1) % 8;
	return v;
}
char getpvalue(vector v) { return prgm[v.y][v.x]; }
unsigned char getvalue(vector v) { return memory[v.y][v.x]; }
void setvalue(vector v,unsigned char c) { memory[v.y][v.x] = c; }

vector mp, ip;

int main (int argc, char **argv) {
	FILE *f;
	char line[8004], *l=line;
	char c;
	int i,y=1;
	vector saved;
	// zero memory
	bzero(memory,sizeof(memory));
	bzero(prgm,sizeof(prgm));
	if (argc != 2) {
		fprintf(stderr,"Usage: %s sourcefile\n", argv[0]);
		exit(1);
	}
	if (!(f=fopen(argv[1],"r"))) {
		fprintf(stderr,"Error: can't open %s\n", argv[1]);
		exit(1);
	}
	bzero(line,8004);
	while (!feof(f)) {
		c=fgetc(f);
		if (c=='\n') {
			*l=0; l=line;
			line[3] = 0; // 0,1,2 = number; 4 and onwards = code
			for (i=0;i<atoi(line);i++) {
				strcpy(&prgm[y++][1],&line[4]);
			}
		} else *(l++)=c;
	}
	// initialize
	mp.x = ip.x = 1; // 1-based
	mp.y = ip.y = 1;
	mp.d = ip.d = se; // starts going south-east
	saved.x=-1; // -1: nothing is saved
	// interpret
	while (1) {
		switch(getpvalue(ip)) {
			case '+': setvalue(mp,(getvalue(mp)+1)%256); break;
			case '*': mp.d = ip.d; mp = next(mp); break;
			case '?': if (getvalue(mp)) ip = l45(ip); break;
			case '!': if (!getvalue(mp)) ip = l45(ip); break;
			case '@': if (ip.d==n) ip = r45(ip);
				  else if (saved.x==-1) {
					  saved=ip;
					  ip.d=n;
				  } else {
					  ip=saved;
					  saved.x=-1;
				  }
				  break;
		}
		ip = next(ip);
		if (ip.x<1) {
			fprintf(stderr,"Instruction pointer left code area\n");
		  	exit(0);
		}
		if (ip.y<1) {
			switch(ip.x) {
				case 8: if (feof(stdin))
					setvalue(mp,0);
					else setvalue(mp,getchar());
					ip = reverse(ip);
					break;
				case 24: putchar(getvalue(mp));
					 ip = reverse(ip);
					 break;
				case 41: exit(0);
				default: fprintf(stderr,
				      "Instruction pointer left code area.\n");
					 exit(0);
			}
		}
	}
	return 0;
}

