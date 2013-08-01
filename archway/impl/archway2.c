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
		return EXIT_FAILURE;
	}

	src = fopen(argv[1], "r");
	if (src == NULL)
	{
		fprintf(stderr, "Unreadable file\n");
		return EXIT_FAILURE;
	}

	codespace = 10;
	code = malloc(codespace * sizeof(char*));
	if (code == NULL)
		return EXIT_FAILURE;

	for (;;)
	{
		int len;
		if (codelines == codespace)
		{
			codespace *= 2;
			code = realloc(code, codespace * sizeof(char*));
			if (code == NULL)
				return EXIT_FAILURE;
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
	int memsize = 10, cell, dir = RIGHT, codex, codey;
	int i, *lengths;

	mem = addmem(mem, 0, memsize);
	cell = 0;
	codex = 0;
	codey = lines - 1;

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

		if (c == '+')
			((int) mem[cell])++;
		else if (c == '-')
			((int) mem[cell])--;
		else if (c == '<')
		{
			if (cell == 0)
			{
				fprintf(stderr, "Underflow error\n");
				exit(EXIT_FAILURE);
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
			(void) putchar(mem[cell]);
		else if (c == ',')
		{
			int d;
			d = getchar();
			if (d == EOF)
				mem[cell] = '\0';
#if '\n' != 10
			else if (d == '\n')
				mem[cell] = (char) 10;
#endif
			else
				mem[cell] = (char) d;
		}
		else if (mem[cell] != '\0' && c == '/')
		{
			if (dir == UP)
			{
				codey++; /* back one */
				dir = RIGHT;
			}
			else if (dir == RIGHT)
			{
				codex--; /* back one */
				dir = UP;
			}
			else if (dir == LEFT)
			{
				codex++; /* back one */
				dir = DOWN;
			}
			else
			{
				assert(dir == DOWN);
				codey--; /* back one */
				dir = LEFT;
			}
		}
		else if (mem[cell] != '\0' && c == '\\')
		{
			if (dir == UP)
			{
				codey++; /* back one */
				dir = LEFT;
			}
			else if (dir == LEFT)
			{
				codex++; /* back one */
				dir = UP;
			}
			else if (dir == RIGHT)
			{
				codex--; /* back one */
				dir = DOWN;
			}
			else
			{
				assert(dir == DOWN);
				codey--; /* back one */
				dir = RIGHT;
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
	int size = 56;

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
