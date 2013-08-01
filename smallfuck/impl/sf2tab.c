/*
 * Copyright (c)2005 Cat's Eye Technologies.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Cat's Eye Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

/*
 * sf2tab.c
 * Smallfuck to lookup-table compiler.
 * $Id$
 */

/*
 * Modified by Catatonic Porpoise <graue@oceanbase.org>
 * to reject program or tape sizes smaller than 1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int prog_size = 256;
int tape_size = 8;
int debug_level = 0;

enum result {
	HALT,
	HANG,
	OFF_LEFT,
	OFF_RIGHT
};

struct state_bucket {
	struct state_bucket *next;
	char *t;			/* tape contents */
};

struct state_bucket **state = NULL;	/* array of bucket ptrs */

void
clear_state(void)
{
	int i;

	/* init state if this is first access */
	if (state == NULL) {
		state = malloc(prog_size * sizeof(struct state_bucket *));
		for (i = 0; i < prog_size; i++) {
			state[i] = NULL;
		}
	}

	for (i = 0; i < prog_size; i++) {
		struct state_bucket *b = state[i];
		while (b != NULL) {
			struct state_bucket *t = b;
			b = b->next;
			free(t->t);	/* free tape contents */
			free(t);	/* free bucket itself */
		}
		state[i] = NULL;
	}
}

void
save_state(int pc, char *t)
{
	struct state_bucket *b;

	if (debug_level >= 2) {
		fprintf(stderr, "saving state (%d, '%s')\n", pc, t);
	}

	b = malloc(sizeof(struct state_bucket));
	b->t = strdup(t);
	b->next = state[pc];
	state[pc] = b;
}

int
check_state(int pc, char *t)
{
	struct state_bucket *b = state[pc];

	while (b != NULL) {
		if (memcmp(t, b->t, tape_size) == 0)
			return 1;	/* yes, deja vu */
		b = b->next;
	}

	return 0;			/* no, this is fresh */
}

enum result
run(char *p, char *t)
{
	int hd = 0, pc = 0, bc = 0;

	clear_state();
	for (;;) {
		save_state(pc, t);		/* record our state */

		if (debug_level >= 3 && p[pc] != '\0') {
			fprintf(stderr, "executing '%c'\n", p[pc]);
		}

		switch (p[pc]) {		/* dispatch on instr */
		case '\0':
			return HALT;

		case '>':
			hd++;
			if (hd > tape_size - 1)
				return OFF_RIGHT; /* ran off end of tape */
			break;
		case '<':
			hd--;
			if (hd < 0)
				return OFF_LEFT; /* ran off beg of tape */
			break;

		case '*':
			if (t[hd] == ' ')
				t[hd] = '*';
			else
				t[hd] = ' ';
			break;

		case '[':
			if (t[hd] == ' ') {
				bc = 1;
				while (bc > 0) {
					pc++;
					if (p[pc] == '[') {
						bc++;
					} else if (p[pc] == ']') {
						bc--;
					}
				}
			}
			break;
			
		case ']':
			bc = 1;
			while (bc > 0) {
				pc--;
				if (p[pc] == '[') {
					bc--;
				} else if (p[pc] == ']') {
					bc++;
				}
			}
			pc--;
			break;

		default:
			break;
		}

		pc++;
		if (check_state(pc, t))		/* total deja vu? */
			return HANG;
	}
}

void
load(char *filename, char *p)
{
	FILE *f;
	int pi = 0;
	char c;

	f = fopen(filename, "r");
	if (f == NULL) {
		fprintf(stderr, "Error: can't open '%s'\n", filename);
		exit(1);
	}

	for (;;) {
		if (feof(f) || pi == (prog_size - 1))
			break;
		c = (char)fgetc(f);
		if (c == '*' || c == '<' || c == '>' || c == '[' || c == ']')
			p[pi++] = c;
	}
	p[pi] = '\0';

	if (debug_level >= 2) {
		fprintf(stderr, "loaded program: '%s'\n", p);
	}
}

void
usage(void)
{
	fprintf(stderr, "Usage: sf2tab [-d] [-p progsize] [-t tapesize] filename\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	char *p;		/* program */
	char *t, *b;		/* tape and backup tape */
	int r = 1;		/* run # */
	int h;			/* halt flag */
	int i;			/* misc counter */
	int done = 0;		/* all runs done flag */
	int ch;			/* getopt character */

	/* get cmdline args */
	while ((ch = getopt(argc, argv, "dp:t:")) != -1) {
		switch ((char)ch) {
		case 'd':
			debug_level++;
			break;
		case 'p':
			prog_size = atoi(optarg);
			if (prog_size < 1)
			{
				fprintf(stderr, "Invalid program size, "
					"using 1\n");
				prog_size = 1;
			}
			break;
		case 't':
			tape_size = atoi(optarg);
			if (tape_size < 1)
			{
				fprintf(stderr, "Invalid tape size, using 1\n");
				tape_size = 1;
			}
			break;
		case '?':
		default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;

	if (argc <= 0)
		usage();

	/* set up program */
	p = malloc(prog_size);
	load(argv[0], p);

	/* set up tape and backup tape */
	t = malloc(tape_size + 1);
	b = malloc(tape_size + 1);
	memset(t, ' ', tape_size);
	memset(b, ' ', tape_size);
	t[tape_size] = '\0';
	b[tape_size] = '\0';
	
	while (!done) {
		/* back up tape, run program, print result */
		memcpy(b, t, tape_size);
		if (debug_level >= 1) {
			fprintf(stderr, "run #%d, using tape '%s'\n", r, t);
		}
		h = run(p, t);
		printf("{ \"%s\", %d, \"%s\" },\n", b, h, t);
		fflush(stdout);

		/* restore tape backup and get next tape */
		memcpy(t, b, tape_size);
		i = tape_size - 1;
		
	loop:	if (t[i] == ' ') {
			t[i] = '*';
		} else {
			t[i] = ' ';
			i--;
			if (i >= 0)
				goto loop;
			else
				done = 1;
		}
		r++;
	}

	exit(0);
}
