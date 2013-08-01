/*
	The Sceql specification is not set in stone, so this
interpreter may change in the future. Version 0.1 of the specification
is implemented here.
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define NODESIZE  1000
#define LOOPDEPTH 1000

typedef unsigned char byte;

typedef struct byteqnode
{
	byte qdata[NODESIZE];
	int start; /* the first byte in use in qdata */
	int end;   /* the last byte in use in qdata */
	struct byteqnode *next;
} byteqnode;

typedef struct
{
	int len;
	byteqnode *head, *tail;
} bytequeue;

void error(char *complaint)
{
	fprintf(stderr, "%s\n", complaint);
	exit(1);
}

/* create a queue with a zero byte in it */
bytequeue *queue_create(void)
{
	bytequeue *myq;
	myq = malloc(sizeof (bytequeue));
	if (!myq)
		error("Out of memory");
	myq->len = 1;
	myq->head = myq->tail = malloc(sizeof (byteqnode));
	if (!myq->head)
		error("Out of memory");
	myq->tail->next = NULL;
	myq->tail->start = myq->tail->end = 0;
	myq->tail->qdata[0] = 0;
	return myq;
}

/* add a byte onto the back of the queue */
void queue_enqueue(bytequeue *q, byte val)
{
	byteqnode *node;
	node = q->tail;
	node->end++;
	if (node->end == NODESIZE)
	{
		node = q->tail = node->next = malloc(sizeof (byteqnode));
		if (!node)
			error("Out of memory");
		node->next = NULL;
		node->start = node->end = 0;
	}
	node->qdata[node->end] = val;
	q->len++;
}

byte queue_peek(bytequeue *q);

/* take a byte from the front of the queue */
byte queue_dequeue(bytequeue *q)
{
	byte result;

	result = queue_peek(q);

	q->head->start++;
	if (q->head->start == NODESIZE)
	{
		byteqnode *node = q->head->next;
		free(q->head);
		q->head = node;
		/* q->head->start should already be 0 */
	}
	q->len--;
	return result;
}

/* peek at a byte from the front of the queue */
byte queue_peek(bytequeue *q)
{
	if (q->len == 0)
		error("peeking at an empty queue");
	return q->head->qdata[q->head->start];
}

/* shove a new byte at the front of the queue replacing what's there */
void queue_shove(bytequeue *q, byte b)
{
	if (q->len == 0)
		error("shoving into an empty queue");
	q->head->qdata[q->head->start] = b;
}

void queue_delete(bytequeue *q)
{
	byteqnode *node;
	while (q->head)
	{
		node = q->head->next;
		free(q->head);
		q->head = node;
	}
	free(q);
}

#ifdef DEBUGGING
void queue_debug(bytequeue *q)
{
	int i;
	i = q->len;
	if (i > 0)
	{
		byte b;
		b = queue_dequeue(q);
		printf("%d", b);
		queue_enqueue(q, b);
	}
	for (i--; i > 0; i--)
	{
		byte b;
		b = queue_dequeue(q);
		printf(", %d", b);
		queue_enqueue(q, b);
	}
	putchar('\n');
}
#endif

typedef enum
{
	IN_NEXT   = 0x01,
	IN_DEC    = 0x02,
	IN_INPUT  = 0x03,
	IN_OUTPUT = 0x04,
	IN_INC    = 0x05,
	IN_GROW   = 0x06,
	IN_BEGIN  = 0x10,
	IN_END    = 0x11
#ifdef DEBUGGING
	,IN_DEBUG  = 0xff
#endif
} sceql_intype;

typedef struct
{
	sceql_intype in;
	int r;
} sceql_instruction;

typedef struct
{
	sceql_instruction *code;
	int len;
} sceql_program;

sceql_intype sceql_getinstruction(FILE *f)
{
	static sceql_intype uc2i[UCHAR_MAX + 1] = { IN_BEGIN };
	int c;
	sceql_intype i;
	if (uc2i[0] == IN_BEGIN)
	{
		uc2i[0] = 0;
		for (c = 1; c <= (int) UCHAR_MAX; c++)
		{
			i = 0;
			if (c == '\\')
				i = IN_BEGIN;
			else if (c == '/')
				i = IN_END;
			else if (c == '=')
				i = IN_NEXT;
			else if (c == '-')
				i = IN_DEC;
			else if (c == '_')
				i = IN_INC;
			else if (c == '!')
				i = IN_GROW;
			else if (c == '&')
				i = IN_INPUT;
			else if (c == '*')
				i = IN_OUTPUT;
#ifdef DEBUGGING
			else if (c == '`')
				i = IN_DEBUG;
#endif
			uc2i[c] = i;
		}
	}
	for (;;)
	{
		c = fgetc(f);
		if (c < 0 || c > 255)
			return 0;
		if ((i = uc2i[c]))
			break;
	}
	return i;
}

void sceql_load(sceql_program *p, FILE *f)
{
	sceql_instruction *code;
	sceql_intype in;
	unsigned long filesize;
	int last = 0, depth = 0, begin[LOOPDEPTH];

	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	if (filesize > INT_MAX)
		error("File too long");
	fseek(f, 0, SEEK_SET);

	code = p->code = malloc(filesize * sizeof (sceql_instruction));
	if (!code)
		error("Not enough memory");

	if (!(in = sceql_getinstruction(f)))
		error("Program contains no instructions");
	if (in == IN_END)
		error("Unbalanced slashes");
	if (in == IN_BEGIN)
		begin[depth++] = 0;
	code[0].in = in;
	code[0].r = 1;

	while ((in = sceql_getinstruction(f)))
	{
		if (in == IN_BEGIN)
		{
			last++;
			if (depth < LOOPDEPTH)
				begin[depth] = last;
			depth++;
		}
		else if (in == IN_END && depth <= LOOPDEPTH)
		{
			if (!depth)
				error("Unbalanced slashes");
			last++;
			code[last].r = last - begin[--depth];
			code[begin[depth]].r = code[last].r + 1;
		}
		else if (in == IN_END)
		{
			int mydepth = depth, start;
			for (start = last++; start >= 0; start--)
			{
				if (code[start].in == IN_BEGIN)
				{
					if (mydepth == depth)
						break;
					mydepth--;
				}
				else if (code[start].in == IN_END)
					mydepth++;
			}
			code[last].r = last - start;
			code[start].r = code[last].r + 1;
			depth--;
		}
		else
		{
			if (in == code[last].in)
			{
				code[last].r++;
				continue;
			}
			code[++last].r = 1;
		}
		code[last].in = in;
	}
	fclose(f);
	if (depth)
		error("Unbalanced slashes");
	p->len = last + 1;
}

void sceql_run(sceql_program *p)
{
	bytequeue *q;
	sceql_instruction *ip, *end;

	ip = p->code;
	end = ip + p->len; 

	q = queue_create();

	do
	{
		register sceql_intype in;
		register int r;

		in = ip->in;
		r = ip->r;

		/* run the instruction "in": not just once, but "r" times */

		if (in == IN_NEXT)
		{
			register byte b;
			for (; r; r--)
			{
				b = queue_dequeue(q);
				queue_enqueue(q, b);
			}
		}

		/* it just so happens that we can increment or decrement
		   r times with a one-liner */
		else if (in == IN_INC)
			queue_shove(q, queue_peek(q) + r);
		else if (in == IN_DEC)
			queue_shove(q, queue_peek(q) - r);

		else if (in == IN_GROW)
		{
			for (; r; r--)
				queue_enqueue(q, 0);
		}
		else if (in == IN_OUTPUT)
		{
			register int c;
			for (; r; r--)
			{
				c = queue_dequeue(q);
				putchar(c);
				queue_enqueue(q, c);
			}
		}
		else if (in == IN_INPUT)
		{
			register int c;
			for (; r; r--)
			{
				c = getchar();
				if (c == EOF)
					c = 0;
				queue_enqueue(q, c);
			}
		}
		else if (in == IN_BEGIN)
		{
			register byte b;
			if (!(b = queue_peek(q)))
			{
				ip += r;
				continue;
			}
		}
#ifdef DEBUGGING
		else if (in == IN_DEBUG)
			queue_debug(q);
#endif
		else /* in == IN_END */
		{
			ip -= r;
			continue;
		}
		ip++;
	} while (ip < end);

	queue_delete(q);
}

void sceql_freeup(sceql_program *p)
{
	free(p->code);
}

void sceql_do(FILE *f)
{
	sceql_program prog;
	sceql_load(&prog, f);
	sceql_run(&prog);
	sceql_freeup(&prog);
}

int main(int argc, char *argv[])
{
	FILE *codesrc = stdin;
	if (argc != 2)
		error("usage: sceql [sourcefile.sceql]");
	if (!(codesrc = fopen(argv[1], "r")))
	{
		error("Unable to open file");
	}
	sceql_do(codesrc);
	return 0;
}
