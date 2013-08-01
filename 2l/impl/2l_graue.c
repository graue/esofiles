/*
Interpreter for Gregor Richards's 2L language.
Define POINTER_REVERSE for the incorrect up/down swap in Gregor's interpreter.

Written in 2005 by Scott Feeney aka Graue

http://esolangs.org/wiki/2L
http://esolangs.org/wiki/User:Graue

To the extent possible under law, the author(s) have dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along
with this software. If not, see:
http://creativecommons.org/publicdomain/zero/1.0/
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

#define UP 31337
#define DOWN 666
#define LEFT 420
#define RIGHT 69

static void run(char **code, int lines, int maxline)
{
	char *mem = NULL;
	int memsize = 10, cell, dir = DOWN, codex, codey;
	int i, *lengths;

	mem = addmem(mem, 0, memsize);
	cell = 2; /* TL0 and TL1 are special */
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

		if (c == '*') /* all IO and tape operations */
		{
#ifdef POINTER_REVERSE
			if (dir == DOWN)
#else
			if (dir == UP)
#endif
			{
				/* move the pointer to the right */
				cell++;
				if (cell == memsize)
				{
					mem = addmem(mem, memsize, memsize*2);
					memsize *= 2;
				}
			}
#ifdef POINTER_REVERSE
			else if (dir == UP)
#else
			else if (dir == DOWN)
#endif
			{
				/* move the pointer to the left */
				if (cell == 0)
				{
					fprintf(stderr, "Underflow error\n");
					exit(EXIT_FAILURE);
				}
				cell--;
			}
			else if (cell == 1)
			{
				/* trying to change TL1 */
				assert(dir == LEFT || dir == RIGHT);

				if ((int) mem[0] != 0) /* output a character */
					(void) putchar(mem[0]);
				else /* input a character */
				{
					int d;
					d = getchar();
					if (d == EOF)
						d = 0;
					mem[0] = (char) d;
				}
			}
			else if (dir == LEFT)
			{
				/* decrement the current cell */
				((int) mem[cell])--;
			}
			else
			{
				/* increment the current cell */
				assert(dir == RIGHT);
				((int) mem[cell])++;
			}
		}
		else if (c == '+') /* conditional turn */
		{
			/* first back up one */
			     if (dir == UP)   codey++;
			else if (dir == DOWN) codey--;
			else if (dir == LEFT) codex++;
			else /* RIGHT */      codex--;

			if ((int) mem[cell] != 0) /* turn right */
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
