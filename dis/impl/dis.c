/* Interpreter for Dis--Malbolge variant.                             */
/* '98 Ben Olmstead.                                                  */
/*                                                                    */
/* Malbolge is the name of Dante's inner circle of Hell; Dis is his   */
/* name for Hell's capital city.  This interpreter isn't even         */
/* Copylefted; I hereby place it in the public domain.  Have fun...   */
/*                                                                    */
/* Note: in keeping with the idea that programming in Malbolge or Dis */
/* is meant to be hell, there is no debugger.                         */
/*                                                                    */
/* By the way, this code assumes that short is 16 bits.  I haven't    */
/* seen any case where it isn't, but it might happen.  If short is    */
/* longer than 16 bits, it will still work, though it will take up    */
/* considerably more memory.                                          */
/*                                                                    */
/* If you are compiling with a 16-bit Intel compiler, you will need   */
/* >64K data arrays; this means using the HUGE memory model on most   */
/* compilers, but MS C, as of 8.00, possibly earlier as well, allows  */
/* you to specify a custom memory-model; the best model to choose in  */
/* this case is /Ashd (near code, huge data), I think.  Also, you     */
/* will need to use the appropriate HUGE version of malloc.           */

#include <stdio.h>
#include <ctype.h>
#include <malloc.h>

void exec( unsigned short *mem );

int main( int argc, char **argv )
{
  FILE *f;
  unsigned short i = 0, j;
  int x;
  unsigned short *mem;
  if ( argc != 2 )
  {
    fprintf( stderr, "invalid command line\n" );
    return ( 1 );
  }
  if ( ( f = fopen( argv[1], "r" ) ) == NULL )
  {
    fprintf( stderr, "can't open file\n" );
    return ( 1 );
  }
#ifdef _MSC_VER
  mem = (unsigned short *)_halloc( 59049, sizeof(unsigned short) );
#else
  mem = (unsigned short *)malloc( sizeof(unsigned short) * 59049 );
#endif
  if ( mem == NULL )
  {
    fclose( f );
    fprintf( stderr, "can't allocate memory\n" );
    return ( 1 );
  }
  while ( ( x = getc( f ) ) != EOF )
  {
    switch ( x )
    {
      case '*':
      case '^':
      case '>':
      case '|':
      case '{':
      case '}':
      case '_':
      case '!':
        if ( i == 59049 )
        {
          fprintf( stderr, "input file too long\n" );
          free( mem );
          fclose( f );
          return ( 1 );
        }
        mem[i++] = x;
        break;
      case '(':
        while ( x != ')' )
        {
          x = getc( f );
          if ( x == EOF )
          {
            fprintf( stderr, "unmatched (\n" );
            free( mem );
            fclose( f );
            return ( 1 );
          }
        }
        break;
      default:
        if ( !isspace( x ) )
        {
          fprintf( stderr, "input file contains invalid characters\n" );
          free( mem );
          fclose( f );
          return ( 1 );
        }
    }
  }
  fclose( f );
  while ( i < 59049 ) mem[i++] = 0;
  exec( mem );
  free( mem );
  return ( 0 );
}

void exec( unsigned short *mem )
{
  unsigned short a = 0, c = 0, d = 0, i, j;
  int x;
  static const unsigned short p3[10] =
    { 1, 3, 9, 27, 81, 243, 729, 2187, 6561, 19683 };
  for (;;)
  {
    switch ( mem[c] )
    {
      case '*': d = mem[d]; break;
      case '^': c = mem[d]; break;
      case '>': a = mem[d] = mem[d] / 3 + mem[d] % 3 * 19683; break;
      case '|':
        i = 0;
        for ( j = 0; j < 10; j++ )
          i += ( a / p3[j] % 3 - mem[d] / p3[j] % 3 + 3 ) % 3 * p3[j];
        a = mem[d] = i;
        break;
      case '{':
        if ( a == 59048 ) return;
#if '\n' != 10
        if ( x == 10 ) putchar( '\n' ); else
#endif
        putchar( a );
        break;
      case '}':
        x = getchar();
#if '\n' != 10
        if ( x == '\n' ) a = 10; else
#endif
        if ( x == EOF ) a = 59048; else a = x;
        break;
      case '!': return;
    }
    if ( c == 59048 ) c = 0; else c++;
    if ( d == 59048 ) d = 0; else d++;
  }
}
