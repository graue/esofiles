/*
This program is hereby placed in the public domain. It may be used,
modified, copied, distributed, sold, and otherwise exploited without
restriction.

graue@oceanbase.org
http://www.oceanbase.org/graue/
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char *read_arbitrary_length_string(FILE *fp);
void run(char **code, int lines, int maxline);

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
	if (!code)
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
		len = strlen(code[codelines]);
		if (len > maxline)
			maxline = len;
		codelines++;
	}
	fclose(src);

	run(code, codelines, maxline);
	free(code);
	return 0;
}

char *addmem(char *mem, int cursize, int newsize)
{
	mem = realloc(mem, newsize);
	if (!mem)
	{
		fprintf(stderr, "Out of memory\n");
		exit(-1);
	}
	memset(&mem[cursize], 0, newsize - cursize);
	return mem;
}

#define UP 69
#define DOWN 420
#define LEFT 666
#define RIGHT 31337

void run(char **code, int lines, int maxline)
{
	char *mem = NULL;
	int memsize = 10, cell, dir = RIGHT, codex, codey;
	int i, *lengths;

	mem = addmem(mem, 0, memsize);
	cell = 0;
	codex = 0;
	codey = lines - 1;

	lengths = malloc(sizeof(int) * lines);
	for (i = 0; i < lines; i++)
		lengths[i] = strlen(code[i]);

	for (;;)
	{
		char c;
		if (codex >= lengths[codey])
			c = ' ';
		else
			c = code[codey][codex];

		if (c == '+')
			mem[cell]++;
		else if (c == '-')
			mem[cell]--;
		else if (c == '<')
		{
			if (cell == 0)
			{
				fprintf(stderr, "Underflow error\n");
				exit(-1);
			}
			cell--;
		}
		else if (c == '>')
		{
			cell++;
			if (cell == memsize)
			{
				mem = addmem(mem, memsize, memsize*2);
				memsize *= 2;
			}
		}
		else if (c == '.')
			putchar(mem[cell]);
		else if (c == ',')
		{
			int d;
			d = getchar();
			if (d == EOF)
				mem[cell] = 0;
#if '\n' != 10
			else if (d == '\n')
				mem[cell] = 10;
#endif
			else
				mem[cell] = (char) d;
		}
		else if (mem[cell] && c == '/')
		{
			if (dir == UP)
				dir = RIGHT;
			else if (dir == RIGHT)
				dir = UP;
			else if (dir == LEFT)
				dir = DOWN;
			else if (dir == DOWN)
				dir = LEFT;
		}
		else if (mem[cell] && c == '\\')
		{
			if (dir == UP)
				dir = LEFT;
			else if (dir == LEFT)
				dir = UP;
			else if (dir == RIGHT)
				dir = DOWN;
			else if (dir == DOWN)
				dir = RIGHT;
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

char *read_arbitrary_length_string(FILE *fp)
{
	char *p = NULL, *q;
	int size = 50;

	if ((p = malloc(size)) == NULL)
		return NULL;
	if (fgets(p, size, fp) == NULL)
		return NULL;

	while ((q = strchr(p, '\n')) == NULL)
	{
		size *= 2;
		if ((p = realloc(p, size)) == NULL)
			return NULL;
		if (fgets(&p[size/2-1], size/2+1, fp) == NULL)
			return NULL; /* invalid; file must end with newline */
	}

	*q = '\0';
	return p;
}
