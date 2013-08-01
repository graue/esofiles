/*
This program is in the public domain. It may be used, modified, copied,
distributed, sold, and otherwise exploited without restriction.

graue@oceanbase.org
http://www.oceanbase.org/graue/

A mildly intelligent Brainfuck to C translator. Use -std=c99.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

static char *symbols = "+-><][.,";

typedef enum
{
	BF_INC,
	BF_DEC,
	BF_RIGHT,
	BF_LEFT,
	BF_CLOSE,
	BF_OPEN,
	BF_OUTPUT,
	BF_INPUT,
	BF_EOF
} bfcom_t;

static bfcom_t getbfc(void);
static bfcom_t peekbfc(void);

int main(void)
{
	bfcom_t com;
	int tabs = 0;
	const int memspace = 32768;

#define DOTABS \
	for (int i = 0; i < tabs; i++) \
		putchar('\t')

	printf("#include <stdio.h>\n\n");
	printf("int main(void)\n{\n");
	printf("\tunsigned char mem[%d];\n", memspace);
	printf("\tint dp = 0;\n");
	printf("\tint c;\n\n");
	tabs++;

	while ((com = getbfc()) != BF_EOF)
	{
		if (com == BF_INPUT)
		{
			DOTABS;
			printf("c = getchar();\n");
			DOTABS;
			printf("if (c != EOF)\n");
			DOTABS;
			printf("\tmem[dp] = c;\n");
			continue;
		}

		if (com == BF_OUTPUT)
		{
			DOTABS;
			printf("putchar(mem[dp]);\n");
			continue;
		}

		if (com == BF_CLOSE)
		{
			if (tabs <= 1)
				fprintf(stderr, "error: unmatched ]\n");
			else
			{
				tabs--;
				DOTABS;
				printf("}\n");
			}
			continue;
		}

		if (com == BF_OPEN)
		{
			DOTABS;
			printf("while (mem[dp])\n");
			DOTABS;
			printf("{\n");
			tabs++;
			continue;
		}

		if (com == BF_INC || com == BF_DEC)
		{
			unsigned int addval = 0;
			int icount = 0, dcount = 0;

			for (;;)
			{
				bfcom_t nextcom;

				addval += (com == BF_INC ? 1 : -1);
				icount += (com == BF_INC);
				dcount += (com == BF_DEC);

				nextcom = peekbfc();
				if (nextcom != BF_INC && nextcom != BF_DEC)
					break;
				com = getbfc();
			}

			addval %= 256;

			if (addval == 0)
				continue;

			DOTABS;

			if (addval == 1)
				printf("mem[dp]++;\n");
			else if (addval == 255)
				printf("mem[dp]--;\n");
			else if (icount > dcount)
				printf("mem[dp] += %d;\n", addval);
			else // (icount < dcount)
				printf("mem[dp] -= %d;\n", 256 - addval);

			continue;
		}

		assert(com == BF_RIGHT || com == BF_LEFT);
		{
			int addval = 0;

			for (;;)
			{
				bfcom_t nextcom;

				addval += (com == BF_RIGHT ? 1 : -1);

				nextcom = peekbfc();
				if (nextcom != BF_RIGHT && nextcom != BF_LEFT)
					break;
				com = getbfc();
			}

			if (addval == 0)
				continue;

			DOTABS;
			if (addval == 1)
				printf("dp++;\n");
			else if (addval == -1)
				printf("dp--;\n");
			else if (addval > 0)
				printf("dp += %d;\n", addval);
			else // (addval < 0)
				printf("dp -= %d;\n", -addval);

			continue;
		}
	}

	while (tabs > 1)
	{
		fprintf(stderr, "error: unmatched [\n");
		tabs--;
		DOTABS;
		printf("}\n");
	}

	printf("\treturn 0;\n");
	printf("}\n");

	return 0;
}

static bfcom_t waitingbfc;
static int isbfcwaiting = 0;

static bfcom_t getbfc(void)
{
	char c = ' '; // not a command
	char *p;

	if (isbfcwaiting)
	{
		isbfcwaiting = 0;
		return waitingbfc;
	}

	while (!(p = strchr(symbols, c)))
		if ((c = getchar()) == EOF)
			return BF_EOF;

	return (bfcom_t) (p - symbols);
}

static bfcom_t peekbfc(void)
{
	if (isbfcwaiting)
		return waitingbfc;

	waitingbfc = getbfc();
	isbfcwaiting = 1;

	return waitingbfc;
}
