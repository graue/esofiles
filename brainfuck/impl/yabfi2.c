/*  yabfi2 -- yet another optimizing brainfuck interpreter
 * 
 *  Copyright (C) 2004 Sascha Wilde
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *------------------------------------------------------------------------- 
 * 
 * this is a simple BF interpreter, doing some on the fly optimization.
 * with madelbrot.b this is nearly four times faster than yabfi.c
 * 
 *-------------------------------------------------------------------------
 * $Id: yabfi2.c,v 1.8 2004/06/15 19:36:50 wilde Exp $ */

#define MEMSIZE 30000

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

int
main(int argc,char** argv)
{
  int i, imax, c, lc, fd, *lut, j;
  static struct stat fd_stat;
  char *src;
  unsigned char *p, *pmin, *pmax;

  if ( 2 != argc ) {
    fprintf (stderr, "Usage: %s BF-SOURCE\n", argv[0]); exit(1); }

  if ((fd = open(argv[1], O_RDONLY)) == -1) {
    perror ("Cant open file\n"); exit(1); }

  (void) fstat (fd, &fd_stat);
  if ((src = (char*) mmap (0, (size_t) fd_stat.st_size, 
			   PROT_READ, MAP_PRIVATE, fd, 0)) == (void*) -1)
    {
      close (fd);
      perror (NULL);
      exit(1);
    }
  close (fd);

  lut = (int*) calloc (fd_stat.st_size, sizeof(int));

  imax = (fd_stat.st_size - 1);

  p = pmin = (unsigned char*) calloc (MEMSIZE, 1);
  pmax = pmin + (MEMSIZE - 1);

  i = 0;
  while (i <= imax )
    {
      switch (src[i])
	{
 	case '>': 
	  if (lut[i]) {
	      p += lut[i];
	      i += lut[i];
	  } else {
	    j = i;
	    do {
	      ++lut[j];
	    } while (src[++i] == '>');
	    p += lut[j];
	  }
 	  break; 
 	case '<':
	  if (lut[i]) {
	      p -= lut[i];
	      i += lut[i];
	  } else {
	    j = i;
	    do {
	      ++lut[j];
	    } while (src[++i] == '<');
	    p -= lut[j];
	  }
 	  break; 
 	case '+':
	  if (lut[i]) {
	      *p += lut[i];
	      i += lut[i];
	  } else {
	    j = i;
	    do {
	      ++lut[j];
	    } while (src[++i] == '+');
	    *p += lut[j];
	  } 
 	  break; 
 	case '-':
	  if (lut[i]) {
	      *p -= lut[i];
	      i += lut[i];
	  } else {
	    j = i;
	    do {
	      ++lut[j];
	    } while (src[++i] == '-');
	    *p -= lut[j];
	  }
 	  break; 
	case '.': putchar(*p);
	  ++i;
	  break;
	case ',': c = getchar();
	  if (c == EOF)
	    *p = 0;
	  else
	    *p = c;
	  ++i;
	  break;
	case '[':
	  if (*p == 0)
	    if (lut[i])
	      i = lut[i];
	    else {
	      j = i;
	      lc = 0;
	      while ((src[++i] != ']') || (lc != 0))
		if (src[i] == '[') 
		  ++lc;
		else if (src[i] == ']')
		  --lc;
	      lut[j] = ++i;
	    }
	  else
	    ++i;
	  break;
	case ']':
	  if (*p != 0)
	    if (lut[i])
	      i = lut[i];
	    else {
	      j = i;
	      lc = 0;	
	      while ((src[--i] != '[') || (lc != 0))
		if (src[i] == ']') 
		  ++lc;
		else if (src[i] == '[')
		  --lc;
	      lut[j] = ++i;
	    }
	  else
	    ++i;
	  break;
	default: ++i;
	}
      if ((p < pmin) || (p > pmax)) {
	fprintf (stderr, "Range error in BF code!"); exit(1); }
    }
  free (pmin);
  free (lut);
}
