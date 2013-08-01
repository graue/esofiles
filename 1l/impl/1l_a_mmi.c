/*
This program is in the public domain. It may be used, modified, copied, 
distributed, sold, and otherwise exploited without restriction.

On my system this is compiled with the command
	cc 1l_a_mmi.c -o 1l_a_mmi `libpng-config --cflags --ldflags`

Should be compliant with 1L_a105 specification.
The PNG loading functionality has not been extensively tested.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <png.h>

static /*@null@*/ char *read_arbitrary_length_string(FILE *fp);
static void run(char **code, int lines, int maxline);
static void loadtext(const char *filename, char ***pcode,
	int *pcodelines, int *pmaxline);
static void loadimg(const char *filename, char ***pcode,
	int *pcodelines, int *pmaxline);

static void die(const char *obituary)
{
	fprintf(stderr, "%s\n", obituary);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	char **code;
	char *p;
	int codelines, maxline;

	if (argc != 2)
		die("Incorrect invocation");

	p = strstr(argv[1], ".png");
	if (p && p[4] == '\0')
		loadimg(argv[1], &code, &codelines, &maxline);
	else
		loadtext(argv[1], &code, &codelines, &maxline);

	run(code, codelines, maxline);
	free(code);
	return 0;
}

static void loadtext(const char *filename, char ***pcode,
	int *pcodelines, int *pmaxline)
{
	char **code;
	char blankchar;
	FILE *src;
	int codespace, codelines = 0, maxline = 0;
	int i;

	src = fopen(filename, "r");
	if (src == NULL)
		die("Unreadable file");

	codespace = 10;
	code = malloc(codespace * sizeof(char *));
	if (code == NULL)
		die("Out of memory");

	for (;;)
	{
		int len;
		if (codelines == codespace)
		{
			codespace *= 2;
			code = realloc(code, codespace * sizeof(char *));
			if (!code)
				die("Out of memory");
		}
		code[codelines] = read_arbitrary_length_string(src);
		if (code[codelines] == NULL)
			break;

		/* top left char is blank */
		if (codelines == 0)
			blankchar = code[0][0] ? code[0][0] : ' ';

		len = (int) strlen(code[codelines]);
		if (len > maxline)
			maxline = len;

		for (i = 0; i < len; i++)
		{
			code[codelines][i] = 1 +
				(code[codelines][i] != blankchar);
		}

		codelines++;
	}
	(void) fclose(src);

	if (codelines < 1 || maxline < 1)
		die("Program size is too small");

	*pcode = code;
	*pcodelines = codelines;
	*pmaxline = maxline;
}

static unsigned int getrowpixel(png_bytep row, int bpp, int pixel)
{
	if (bpp == 1)
		return !!(row[pixel/8] & (1<<(7-(pixel%8))));
	if (bpp == 2)
		return (row[pixel/4] >> (2*(3-(pixel%4)))) & 3;
	if (bpp == 4)
		return (row[pixel/2] >> (4*(1-(pixel%2)))) & 15;
	assert(bpp % 8 == 0);
	if (bpp == 8)
		return row[pixel];
	if (bpp == 16)
		return (row[pixel*2+1]<<8) + row[pixel*2];
	if (bpp == 24)
		return (row[pixel*3+2]<<16)
			+ (row[pixel*3+1]<<8)
			+ row[pixel*3];
	assert(bpp == 32);
	return (row[pixel*4+3]<<24)
		+ (row[pixel*4+2]<<16)
		+ (row[pixel*4+1]<<8)
		+ row[pixel*4];
}

static void loadimg(const char *filename, char ***pcode,
	int *pcodelines, int *pmaxline)
{
	FILE *src;
	char header[8];
	png_structp pngptr;
	png_infop infoptr;
	png_bytep *rowptrs;

	int height, width;
	int bpp;
	char **code;

	unsigned int blankpixel;

	int i, j;

	src = fopen(filename, "rb");
	if (src == NULL)
		die("Unreadable file");

	if (fread(header, 1, 8, src) < 8
		|| png_sig_cmp(header, 0, 8) != 0)
	{
		die("Not a valid PNG file");
	}

	pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (!pngptr)
		die("Cannot initialize libpng");

	infoptr = png_create_info_struct(pngptr);
	if (!infoptr)
		die("Cannot create PNG info struct");

	png_init_io(pngptr, src);
	png_set_sig_bytes(pngptr, 8); /* we already read 8 bytes */

	png_read_png(pngptr, infoptr,
		PNG_TRANSFORM_STRIP_ALPHA /* discard alpha channel */
		, NULL);

	/* get info about the image */
	width = png_get_image_width(pngptr, infoptr);
	height = png_get_image_height(pngptr, infoptr);
	bpp = png_get_bit_depth(pngptr, infoptr)
		* png_get_channels(pngptr, infoptr);

	if (width < 1 || height < 1)
		die("Program code is too small");

	if (bpp > 32) /* doesn't fit in an int */
		die("Bits per pixel of greater than 32 are not supported");

	/* get access to the image itself */
	rowptrs = png_get_rows(pngptr, infoptr);

	blankpixel = getrowpixel(rowptrs[0], bpp, 0);

	code = malloc(sizeof(char *) * height);
	if (code == NULL)
		die("Out of memory");

	for (i = 0; i < height; i++)
	{
		code[i] = malloc(width);
		if (code[i] == NULL)
			die("Out of memory");

		for (j = 0; j < width; j++)
		{
			code[i][j] = 1 +
				(getrowpixel(rowptrs[i], bpp, j)
				!= blankpixel);
		}
	}

	/* finish up with libpng */
	png_destroy_read_struct(&pngptr, &infoptr, NULL);

	(void) fclose(src);

	*pcode = code;
	*pcodelines = height;
	*pmaxline = width;
}

static char *addmem(/*@null@*/ char *mem, int cursize, int newsize)
{
	mem = realloc(mem, (size_t) newsize);
	if (!mem)
		die("Out of memory");
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

	/* Programs must be at least 1x1.
	 * This should already be guaranteed by the source code
	 * loaders, but just to make sure, and to document the
	 * assumption, we verify it here.
	 */
	assert(lines >= 1);
	assert(maxline >= 1);

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
			c = 0;
		else
			c = code[codey][codex];

		if (c == 2) /* conditional turn */
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
