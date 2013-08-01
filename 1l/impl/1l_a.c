/*
An interpreter for the 1L_a language.

This program is in the public domain. It may be used, modified, copied, 
distributed, sold, and otherwise exploited without restriction.

Last modified: Fri Oct 28 10:42:59 EDT 2005
Fixed an input bug in inbit().

graue@oceanbase.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static /*@null@*/ char *read_arbitrary_length_string(FILE *fp);
static void run(char **code, int lines, int maxline);

int main(int argc, char *argv[])
{
	char **code;
	FILE *src;
	int codespace, codelines = 0, maxline = 0;

	if (argc != 2)
	{
		fprintf(stderr, "Incorrect invocation\n");
		return 1;
	}

	src = fopen(argv[1], "r");
	if (src == NULL)
	{
		fprintf(stderr, "Unreadable file\n");
		return 2;
	}

	codespace = 10;
	code = malloc(codespace * sizeof(char*));
	if (code == NULL)
		return 3;

	for (;;)
	{
		int len;
		if (codelines == codespace)
		{
			codespace *= 2;
			code = realloc(code, codespace * sizeof(char*));
			if (!code)
				return 4;
		}
		code[codelines] = read_arbitrary_length_string(src);
		if (code[codelines] == NULL)
			break;
		len = (int) strlen(code[codelines]);
		if (len > maxline)
			maxline = len;
		codelines++;
	}
	(void) fclose(src);

	run(code, codelines, maxline);
	free(code);
	return 0;
}

static char *addmem(/*@null@*/ char *mem, int cursize, int newsize)
{
	mem = realloc(mem, (size_t) newsize);
	if (!mem)
	{
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
	memset(&mem[cursize], 0, (size_t) (newsize - cursize));
	return mem;
}

int inbit(void)
{
	static int pos = 0;
	static int byte;
	int rd;

	if (pos == 0)
	{
		rd = getchar();
		if (rd == EOF)
			return 0;
		pos = 8;
		byte = rd;
	}
	return !!(byte & (1<<--pos));
}

void outbit(int bit)
{
	static int pos = 8;
	static int byte = 0;
	assert(bit == 0 || bit == 1);

	byte |= (bit<<--pos);
	if (pos == 0)
	{
		pos = 8;
		putchar(byte);
		byte = 0;
	}
}

#define UP 31337
#define DOWN 666
#define LEFT 420
#define RIGHT 69

static void run(char **code, int lines, int maxline)
{
	char *mem = NULL;
	int memsize = 10, cell, bcell, dir = DOWN, codex, codey;
	int i, *lengths;

	if (lines < 1)
	{
		/* if the IP starts out of code space, the result is undefined,
		   but we handle it gracefully here */
		fprintf(stderr, "In 1L_a, the null program is not a quine.\n");
		return;
	}

	mem = addmem(mem, 0, memsize);
	bcell = 0, cell = 2; /* TL0 and TL1 are special */
	codex = 0;
	codey = 0;

	lengths = malloc(sizeof(int) * lines);

	if (lengths == NULL)
		exit(EXIT_FAILURE);

	for (i = 0; i < lines; i++)
		lengths[i] = (int) strlen(code[i]);

	for (;;)
	{
		char c;
		if (codex >= lengths[codey])
			c = ' ';
		else
			c = code[codey][codex];

		if (c != ' ') /* conditional turn */
		{
			/* first back up one */
			     if (dir == UP)   codey++;
			else if (dir == DOWN) codey--;
			else if (dir == LEFT) codex++;
			else /* RIGHT */      codex--;

			if ((mem[bcell] & (1<<cell)) != 0) /* turn right */
			{
				     if (dir == UP)    dir = RIGHT;
				else if (dir == RIGHT) dir = DOWN;
				else if (dir == DOWN)  dir = LEFT;
				else /* LEFT */        dir = UP;
			}
			else /* turn left */
			{
				     if (dir == DOWN)  dir = RIGHT;
				else if (dir == RIGHT) dir = UP;
				else if (dir == UP)    dir = LEFT;
				else /* LEFT */        dir = DOWN;
			}
		}
		else if (dir == UP) /* move the pointer to the right */
		{
			cell++;
			if (cell == 8)
			{
				cell = 0;
				bcell++;
				if (bcell == memsize)
				{
					mem = addmem(mem, memsize, memsize*2);
					memsize *= 2;
				}
			}
		}
		else if (dir == LEFT) /* move pointer left and flip bit */
		{
			cell--;
			if (cell == -1)
			{
				cell = 7;
				bcell--;
				if (bcell == -1)
				{
					fprintf(stderr, "Underflow error at "
						"%d, %d\n",
						codex, codey);
					exit(EXIT_FAILURE);
				}
			}
			mem[bcell] ^= (1<<cell);
			if (bcell == 0 && cell == 0) /* this is TL0 */
			{
				if (mem[0] & (1<<1)) /* TL1 is on; output */
					outbit(!!(mem[0] & (1<<2)));
				else /* TL1 is off; input to TL2 */
				{
					mem[0] &= ~(1<<2); /* zero TL2 */
					mem[0] |= (inbit()<<2); /* fill TL2 */
				}
			}
		}

		if (dir == RIGHT)
		{
			if (++codex == maxline)
				break;
		}
		else if (dir == LEFT)
		{
			if (codex-- == 0)
				break;
		}
		else if (dir == DOWN)
		{
			if (++codey == lines)
				break;
		}
		else
		{
			assert(dir == UP);
			if (codey-- == 0)
				break;
		}
	}

	free(lengths);
	free(mem);
}

static /*@null@*/ char *read_arbitrary_length_string(FILE *fp)
{
	char *p = NULL, *p2, *q;
	int size = 50;

	if ((p = malloc((size_t) size)) == NULL)
		return NULL;
	if (fgets(p, size, fp) == NULL)
		return NULL;

	while ((q = strchr(p, '\n')) == NULL)
	{
		size *= 2;
		if ((p2 = realloc(p, (size_t) size)) == NULL)
		{
			free(p);
			return NULL;
		}
		p = p2;
		if (fgets(&p[size/2-1], size/2+1, fp) == NULL)
			return NULL; /* invalid; file must end with newline */
	}

	*q = '\0';
	return p;
}
