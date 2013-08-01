/*
 * a crude implementation of a trigger interpreter.
 *
 * Author: Bertram Felgenhauer (aka int-e), 2005-07-26,27
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned char prog[1048576];
unsigned char trigger[256];

int main(int argc, char **argv)
{
    int pc = 0, max, bits=0, input=0;
    FILE *in;

    srand(time(NULL));

    if (argc<2) {
        fprintf(stderr, "Usage: %s program [input]\n", argv[0]);
        exit(1);
    }
    in = fopen(argv[1], "r");
    if (!in) {
        fprintf(stderr, "Could not open '%s'.", argv[1]);
        exit(1);
    }
    max = fread(prog, 1, sizeof(prog), in);
    fclose(in);
    in = NULL;
    input = EOF;
    if (argc>2) {
        in = fopen(argv[2], "r");
    }
    if (max == sizeof(prog)) {
        fprintf(stderr, "Warning: only the first %d bytes of the program "
                "were read.", sizeof(prog));
    }
    while (pc<max) {
        if (pc<max-1 && prog[pc] == prog[pc+1]) {
            if (pc<max-2 && prog[pc] == prog[pc+2]) {
                if (pc<max-3 && prog[pc] == prog[pc+3]) {
                    /* AAAA pattern -> input */
                    if (!bits) {
                        if (in)
                            input = getc(in);
                        bits = 8;
                    }
                    if (input != EOF) {
                        bits--;
                        trigger[prog[pc]] = !!(input & (1<<bits));
                    }
                    pc += 4;
                } else {
                    /* AAA pattern -> output */
                    putchar(prog[pc]);
                    pc += 3;
                }
            } else {
                /* AAB pattern -> conditional jump */
                if (pc>max-3) {
                    /* just AA at end of program -- exit silently */
                    return 0;
                }
                if (trigger[prog[pc]]) {
                    /* jump */
                    int nxt = pc+3;
                    int prv = pc-1;
                    for ( ; ; nxt++, prv--) {
                        int l = prv >= 0  && prog[pc+2] == prog[prv];
                        int r = nxt < max && prog[pc+2] == prog[nxt];

                        if (l && (!r || rand()/(RAND_MAX/2))) {
                            pc = prv;
                            break;
                        } else if (r) {
                            pc = nxt;
                            break;
                        }
                        if (nxt >= max && prv < 0) {
                            /* label not found */
                            pc += 3;
                            break;
                        }
                    }
                } else {
                    /* skip */
                    pc += 3;
                }
            }
        } else {
            /* A pattern -> toggle */
            trigger[prog[pc]] ^= 1;
            pc++;
        }
    }
    return 0;
}
