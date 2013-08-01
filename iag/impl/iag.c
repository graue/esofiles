
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STACKSIZE 256
#define PROGSIZE 65000

void error( char *complaint )
{
	fprintf( stderr, "%s\n", complaint );
	exit( 1 );
}

void system_call( int nr, int mp, int mem[] )
{
	int c;
	switch( nr ) {
	case 2:
		fputc( mem[mp], stdout );
		break;
	case 3: 
		c = fgetc( stdin );
		if ( c == EOF ) c = 0;
		mem[mp] = c;
		break;
	case 4:
		mem[mp] = time( NULL );
		break;
	case 5:
		exit( 0 );
		break; /* Not needed */
	}
}

void dump_debug( int acc, int mp, int mem[], int memsize )
{
	int i;
	printf( "\nDebug: [%d] ", acc );
	for( i = 0; i < memsize; ++i )
		printf( "%c%d", ( mp == i ? '*' : ' ' ), mem[i] );
	putchar( '\n' );
}

void stack_debug( int sp, char *stack[] )
{
	int i;
	for( i = sp; i >= 0; --i ) {
		char *f = stack[i];
		printf ( "\ns[%d]: %c%c%c%c%c%c%c", i, 
			 f[0], f[1], f[2], f[3], f[4], f[5], f[6] );
	}
	putchar( '\n' );
}

int execute( char *ip, char *end )
{
	char *stack[STACKSIZE];
	int *mem = malloc( sizeof(int) );
	int memsize = 1;
	int sp = 0, mp = 0, acc = 0;

	mem[0] = 0;
	while ( ip < end ) {
		switch ( *ip ) {

		case '<':
			if ( --mp < 0 ) error( "Memory underflow" );
			break;

		case '=':
			mem[mp] = acc = acc - mem[mp];
			if ( ++mp >= memsize ) {
				int nmemsize = memsize * 2 + 10;
				mem = realloc( mem, sizeof(int) * nmemsize );
				while ( memsize < nmemsize )
					mem[memsize++] = 0;
			}
			break;

		case '|':
			if ( acc > 0 ) system_call( acc, mp, mem );
			else if ( acc == 0 ) {
				stack[sp++] = ip;
				if ( !mem[mp] )
					while ( ++ip < end )
						if ( *ip == '|' ) break;
				acc = 1;
			} else {
				sp += acc;
				if ( sp < 0 ) error( "Stack underflow" );
				ip = stack[sp] - 1;
				acc = 0;
			}
			break;

		case 'd':
			dump_debug( acc, mp, mem, memsize );
			break;

		case 's':
			stack[sp] = ip;
			stack_debug( sp, stack );
			break;

		case 'x':
			return 0;

		}
		++ip;
	}
	return 0;
}

int main( int ac, char* av[] )
{
	char prog[PROGSIZE] = { 0 };
	int c;
	char *progp = prog;
	FILE *pfile;
	
	if ( ac < 2 ) error( "Please give program file as argument" );
	pfile = fopen( av[1], "r" );
	if ( !pfile ) error( "Not a valid file" );
	while ( ( c = fgetc( pfile )) != EOF ) *progp++ = c;

	return execute( prog, progp );
}
