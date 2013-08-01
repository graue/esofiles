"""
This program compiles SNUSP programs to C.
Copyright (C) 2004  John Bauman

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
"""

import fileinput


#preamble
print """#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

#ifdef WIN32
#define gtch() getche()
#else
#include <termios.h>
#define gtch() getchar()
#endif

int main(int argc, char **argv) {
int *pvals = malloc(30000 * sizeof(int));
jmp_buf envs[1000];
jmp_buf *penvs = envs;

#ifndef WIN32
struct termios old_tio, new_tio;
/* get the terminal settings for stdin */
tcgetattr(STDIN_FILENO,&old_tio);

/* we want to keep the old setting to restore them at the end */
new_tio=old_tio;

/* disable canonical mode (buffered i/o) */
new_tio.c_lflag &=(~ICANON);
   
/* set the new settings immediately */
tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
#endif

goto progstart;
"""


program = []
hasstart = 0
progwidth = 0
for line in fileinput.input():
    n = [c for c in line]
    
    if line.find("$") >= 0:
        hasstart = 1
    program.append(n)
    if progwidth < len(n):
        progwidth = len(n)
        
progheight = len(program)

for line in range(len(program)): #pad the lines out
    program[line] += " " * (progwidth - len(program[line]))
    
if not hasstart:
    print "progstart:"
    
    
proglocs = {}
jnum = 0

for y, line in enumerate(program):
    for x, char in enumerate(line):
        if char == "\\" or char == "/":
            proglocs[(x,y)] = jnum
            jnum += 1
            
jumpnum = 0
callnum = 0
skipnum = 0



def outputrowdata(y, row, myname, left, right, realx, realy):
    """Print out the code for a "row" of data."""
   
    global skipnum, callnum, jumpnum
    amskipping = amcalling = amjumping = 0
    
    for x, char in enumerate(row):
        skipinturn = 0
        callinturn = 0
        jumpinturn = 0
        
        xypos = (realx(x,y), realy(x,y)) #find out the REAL position
        

        #check each character
        if myname == "right" and char == "$":
            print "progstart:"
        if char == "+":
            print "(*pvals)++;"
        if char == "-":
            print "(*pvals)--;"
        if char == ">":
            print "pvals++;"
        if char == "<":
            print "pvals--;"
        if char == ",":
            print "*pvals = gtch();"
            print "if (*pvals == 3) goto endprogram;"
        if char == ".":
            print "putchar(*pvals);"
        if char == "\\":
            print "goto %s_%i;" % (right, proglocs[xypos])
            print "%s_%i:" % (myname, proglocs[xypos])
        if char == "/":
            print "goto %s_%i;" % (left, proglocs[xypos])
            print "%s_%i:" % (myname, proglocs[xypos])
        if char == "?":
            print "if (*pvals == 0)"
            print "goto jump_%i;" % (jumpnum)
            jumpnum += 1
            jumpinturn = 1
        if char == "!":
            print "goto skip_%i;" % (skipnum)
            skipnum += 1
            skipinturn = 1
        if char == "@":
            print "if (penvs > envs + 999) {"
            print 'printf("Too many @ calls - Ran out of space.");'
            print "goto endprogram;"
            print "}"
            print "if (setjmp(*penvs)) {"
            print "goto call_%i;" % (callnum)
            print "} else {"
            print "penvs++;"
            print "}"
            callnum += 1
            callinturn = 1
        if char == "#":
            print "penvs--;"
            print "if (penvs < envs) "
            print "goto endprogram;"
            print "longjmp(*penvs, 1);"


        #jump-to points for the instructions that skip over others
        #these are annoyingly complicated, to account for the fact 
        #that there might be two in a row
        if amjumping:
            print "jump_%i:" % (jumpnum - 1 - jumpinturn)
            amjumping -= 1
        if char == "?":
            amjumping += 1

        if amcalling:
            print "call_%i:" % (callnum - 1 - callinturn)
            amcalling -= 1
        if char == "@":
            amcalling += 1

        if amskipping:
            print "skip_%i:" % (skipnum - 1 - skipinturn)
            amskipping -= 1
        if char == "!":
            amskipping += 1
            
    #just make sure to clean up - no dangling references
    if amjumping:
        print "jump_%i:" % (jumpnum - 1)
    if amcalling:
        print "call_%i:" % (callnum - 1)
    if amskipping:
        print "skip_%i:" % (skipnum - 1)
            
    print "goto endprogram;"    



#these parts output the rows of code, forwards and backwards, up and down


for y, row in enumerate(program):
    if "/" in row or "\\" in row or "$" in row or (y == 0 and not hasstart):
        outputrowdata(y, row, "right", "up", "down", 
                    lambda a,b:a, 
                    lambda a,b:b)


for y, row in enumerate(program):
    row = row[:]
    row.reverse()
    if "/" in row or "\\" in row:
        outputrowdata(y, row, "left", "down", "up", 
                    lambda a,b:progwidth - a - 1,
                    lambda a,b:b)
            

for x, row in enumerate(zip(*program)):
    if "/" in row or "\\" in row:
        outputrowdata(x, row, "down", "left", "right", 
                    lambda a,b:b, 
                    lambda a,b:a)       
       

for x, row in enumerate(zip(*program)):
    row = list(row)
    row.reverse()
    if "/" in row or "\\" in row:
        outputrowdata(x, row, "up", "right", "left", 
                    lambda a,b:b, 
                    lambda a,b:progheight - a - 1)    
        

print """

endprogram:
#ifndef WIN32
tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
#endif

return 0;
}
"""
