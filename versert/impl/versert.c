/*
 * TokigunStudio Versert Reference Interpreter
 * Copyright (c) 2005, Kang Seonghoon (Tokigun).
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#define BLOCK_WIDTH 64
#define BLOCK_HEIGHT 64

typedef signed int value_t;
typedef unsigned char cell_t;
typedef cell_t block_t[BLOCK_WIDTH][BLOCK_HEIGHT];

typedef struct {
	/* vectors */
	value_t x, y, dx, dy, px, py;
	/* registers */
	value_t a, b;
	/* code space */
	value_t minx, miny, maxx, maxy;
	int nblock[4];
	block_t **block[4];
} context_t;

/******************************************************************************/

context_t *context_alloc() {
	context_t *ptr;
	int i;
	
	ptr = malloc(sizeof(context_t));
	if(ptr != NULL) {
		ptr->x = 0; ptr->y = 0;
		ptr->dx = 1; ptr->dy = 0;
		ptr->px = 0; ptr->py = 0;
		ptr->a = 0; ptr->b = 0;
		ptr->minx = 0; ptr->miny = 0;
		ptr->maxx = 1; ptr->maxy = 1;
		for(i=0; i<4; i++) {
			ptr->block[i] = malloc(sizeof(block_t*));
			ptr->block[i][0] = NULL;
			ptr->nblock[i] = 1;
		}
	}
	return ptr;
}

void context_free(context_t *context) {
	int i, j;
	
	for(i=0; i<4; i++) {
		for(j=0; j<context->nblock[i]; j++) {
			if(context->block[i][j] != NULL)
				free(context->block[i][j]);
		}
		free(context->block[i]);
	}
	free(context);
}

int context_findcell(context_t *context, int *sign, int *block, value_t *x, value_t *y) {
	int temp;

	*sign = 0;
	if(*x < 0) { *sign |= 1; *x = ~*x; }
	if(*y < 0) { *sign |= 2; *y = ~*y; }

	temp = (*x / BLOCK_WIDTH) + (*y / BLOCK_HEIGHT);
	*block = temp * (temp + 1) / 2 + (*y / BLOCK_HEIGHT);
	*x %= BLOCK_WIDTH;
	*y %= BLOCK_HEIGHT;

	return (*block < context->nblock[*sign]);
}

cell_t context_getcell(context_t *context, value_t x, value_t y) {
	int sign, block;

	if(context_findcell(context, &sign, &block, &x, &y)) {
		block_t *current = context->block[sign][block];
		if(current != NULL) {
			return (*current)[x][y];
		}
	}
	return 32;
}

int context_putcell(context_t *context, value_t x, value_t y, cell_t c) {
	int sign, block;
	value_t realx, realy;
	block_t *current;

	realx = x; realy = y;
	if(!context_findcell(context, &sign, &block, &x, &y)) {
		block_t **ptr, **last;

		block++;
		ptr = realloc(context->block[sign], sizeof(block_t*) * block);
		if(ptr == NULL) return 0;
		context->block[sign] = ptr;
		last = ptr + block;
		ptr += context->nblock[sign];
		while(ptr < last) *ptr++ = NULL;
		context->nblock[sign] = block;
		block--;
	}

	current = context->block[sign][block];
	if(current == NULL) {
		int i, j;
		
		current = malloc(sizeof(block_t));
		if(current == NULL) return 0;
		for(i=0; i<BLOCK_WIDTH; i++)
			for(j=0; j<BLOCK_HEIGHT; j++)
				(*current)[i][j] = 32;
		context->block[sign][block] = current;
	}
	(*current)[x][y] = c;

	if(realx < context->minx) context->minx = realx;
	if(realx >= context->maxx) context->maxx = realx + 1;
	if(realy < context->miny) context->miny = realy;
	if(realy >= context->maxy) context->maxy = realy + 1;

	return 1;
}

int context_load(context_t *context, FILE *fp) {
	value_t x, y;
	cell_t ch, prev;

	if(fp == NULL) return 0;

	x = y = ch = 0;
	while(!feof(fp)) {
		prev = ch;
		ch = fgetc(fp);
		if(ch == '\n' && prev == '\r') continue;
		if(ch == '\n' || ch == '\r') {
			x = 0; y++;
		} else {
			context_putcell(context, x, y, ch);
			x++;
		}
	}
	return 1;
}

int context_step(context_t *context) {
	value_t temp;
	cell_t cur;

	cur = context_getcell(context, context->x, context->y);
	switch(cur) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		context->a = cur - '0';
		break;
	case '+':
		context->b += context->a;
		break;
	case '-':
		context->b -= context->a;
		break;
	case '*':
		context->b *= context->a;
		break;
	case '~':
		temp = context->a;
		context->a = context->b;
		context->b = temp;
		break;
	case '<':
		if(context->a > context->b) {
			temp = context->a;
			context->a = context->b;
			context->b = temp;
		}
		break;
	case '>':
		if(context->a < context->b) {
			temp = context->a;
			context->a = context->b;
			context->b = temp;
		}
		break;
	case '/':
		temp = -context->dx;
		context->dx = -context->dy;
		context->dy = temp;
		break;
	case '\\':
		temp = context->dx;
		context->dx = context->dy;
		context->dy = temp;
		break;
	case '@':
		return 0;
	case '#':
		if(context->b == 0) {
			context->x += context->dx;
			context->y += context->dy;
		}
		break;
	case '.':
		putchar(context->a & 0xff);
		break;
	case ':':
		printf("%d", context->a);
		break;
	case ',':
		temp = getchar();
		if(temp != EOF) context->a = temp;
		break;
	case ';':
		if(scanf("%d", &temp) > 0) context->a = temp;
		break;
	case '{':
		context->b = context_getcell(context, context->px, context->py);
		break;
	case '|':
		context->px += context->a;
		context->py += context->b;
		break;
	case '}':
		if(!context_putcell(context, context->px, context->py, (cell_t)(context->b & 0xff))) {
			return -1;
		}
		break;
	}

	context->x += context->dx;
	context->y += context->dy;
	if(context->x < context->minx) context->x += context->maxx - context->minx;
	if(context->y < context->miny) context->y += context->maxy - context->miny;
	if(context->x >= context->maxx) context->x += context->minx - context->maxx;
	if(context->y >= context->maxy) context->y += context->miny - context->maxy;

	return 1;
}

/******************************************************************************/

int main(int argc, char **argv) {
	FILE *fp;
	context_t *context;
	int goahead;

	if(argc != 2) {
		printf("TokigunStudio Versert Reference Interpreter\n");
		printf("Copyright (c) 2005, Kang Seonghoon (Tokigun).\n\n");
		printf("Usage: %s <filename>\n", argv[0]);
		return 0;
	}

	fp = fopen(argv[1], "rb");
	if(fp == NULL) {
		printf("Fatal Error: cannot open file.\n");
		return 1;
	}
	
	context = context_alloc();
	if(context == NULL) {
		printf("Fatal Error: no enough memory.\n");
		fclose(fp);
		return 2;
	}
	context_load(context, fp);
	fclose(fp);

	while(1) {
		goahead = context_step(context);
		if(goahead == 0) break;
		if(goahead == -1) {
			printf("Fatal Error: no enough memory.\n");
			context_free(context);
			return 2;
		}
	}
	context_free(context);
	return 0;
}
