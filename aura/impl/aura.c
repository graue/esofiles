/* esoteric.voxelperfect.net archive note:
   this was at http://nhi.netfirms.com/aura.html previously */

/* Aura revision 0.03 */
/* send comments to temperanza@softhome.net */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

unsigned short int c, i = 1, q, x;
short int n = 1;
char a[5000], f[5000], b, o, *s=f;

int main(int argc,char *argv[])
{
    FILE *z;

    q=argc;

    if((z=fopen(argv[1],"r"))) {
        while( (b=getc(z))>0 )
            *s++=b;
        *s=0;
    }

    x = strlen(f);

    while (f[i]) {

        /* x = (unsigned short int) f[i] % 8; */

        for (c = 0; c <= strlen(f); c++)
        {
            if (!isgraph(f[c]) && (f[c] != ' '))
            {
                a[c] = (f[c] % 8) + 48;
            }
            else a[c] = f[c];
        }

        printf("%s %d %d \n",a,n,i);

        if ((i == x) || (i == 0)) i = abs(i - x);
        i += n;

        switch(o=1,((unsigned short int) f[i] % 8)) {
        case 0:
            if (f[i] == 0) exit(EXIT_SUCCESS);
            f[i+n] /= f[i];                                  break;
        case 1: f[i+n] -= f[i];                              break;
        case 2: f[i+n]--;                                    break;
        case 3: f[i+n] += getchar();                         break;
        case 4: n -= (n*2);                                  break;
        case 5:
            b = (char) f[i-n];
            if (isgraph(b) || (b == ' '))
                printf("%c",b);
            f[i+n]++;                                        break;
        case 6: f[i+n] += f[i];                              break;
        case 7: f[i+n] *= f[i];                              break;
        default: o=0;
        }

        printf("\n");
    }

    return 0;
}
