"""
This program compiles Path programs to assembly.
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


#these can also be changed to "putchar" and "getchar",
#those can work with files, but getchar requires newlines to be input
putchar="_putch"
getchar="_getche"

#preamble
print """
EXTERN ExitProcess
IMPORT ExitProcess kernel32.dll
EXTERN %s
EXTERN %s

SEGMENT .data USE32
array times 30000 db 0

SEGMENT .code USE32
..start

mov EBX, array
push DWORD 0
jmp progstart

""" % (putchar, getchar)

movecharacters = "\\/v^<>"

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
        if char in movecharacters:
            proglocs[(x,y)] = jnum
            jnum += 1
            

skipnum = 0
checkjump = 0


def outputrowdata(y, row, myname, left, right, realx, realy):
    """Print out the code for a "row" of data."""
   
    global skipnum, checkjump
    amskipping = 0
    
    for x, char in enumerate(row):
    
       # if char not in "$+-}{,.v><^\\\/!#":
       #     continue
        skipinturn = 0
        
        xypos = (realx(x,y), realy(x,y)) #find out the REAL position
        

        #check each character
        if myname == "right" and char == "$":
            print "progstart:"
        if char == "+":
            print "inc BYTE [EBX]"
        if char == "-":
            print "dec BYTE [EBX]"
        if char == "}":
            print "add EBX, 4"
        if char == "{":
            print "CMP EBX, array"
            print "JE chjump_%i" % (checkjump)
            print "sub EBX, 4"
            print "chjump_%i:" % (checkjump)
            checkjump += 1
        if char == ",":
            print "call %s" % (getchar)
            print "mov [EBX], AL"
            print "cmp EAX, 3"
            print "JE NEAR endprogram"
        if char == ".":
            print "push DWORD [EBX]"
            print "call %s" % (putchar)
            print "pop EAX"
        if char == ">":
            print "CMP DWORD [EBX], 0"
            print "JNE NEAR right_%i" % (proglocs[xypos])
            if myname == "right":
                print "right_%i:" % (proglocs[xypos])
        if char == "<":
            print "CMP BYTE [EBX], 0"
            print "JNE NEAR left_%i" % (proglocs[xypos])
            if myname == "left":
                print "left_%i:" % (proglocs[xypos])
        if char == "^":
            print "CMP BYTE [EBX], 0"
            print "JNE NEAR up_%i" % (proglocs[xypos])
            if myname == "up":
                print "up_%i:" % (proglocs[xypos])
        if char == "v":
            print "CMP BYTE [EBX], 0"
            print "JNE NEAR down_%i" % (proglocs[xypos])
            if myname == "down":
                print "down_%i:" % (proglocs[xypos])
        if char == "\\":
            print "JMP NEAR %s_%i" % (right, proglocs[xypos])
            print "%s_%i:" % (myname, proglocs[xypos])
        if char == "/":         
            print "JMP NEAR %s_%i" % (left, proglocs[xypos])
            print "%s_%i:" % (myname, proglocs[xypos])
        if char == "!":
            print "JMP skip_%i" % (skipnum)
            skipnum += 1
            skipinturn = 1
        if char == "#":
            print "JMP NEAR endprogram"


        #jump-to points for the instructions that skip over others
        #these are annoyingly complicated, to account for the fact 
        #that there might be two in a row

        if amskipping:
            print "skip_%i:" % (skipnum - 1 - skipinturn)
            amskipping -= 1
        if char == "!":
            amskipping += 1
            
    #just make sure to clean up - no dangling references
    if amskipping:
        print "skip_%i:" % (skipnum - 1)
            
    print "JMP endprogram"    



#these parts output the rows of code, forwards and backwards, up and down


for y, row in enumerate(program):
    if [x for x in movecharacters if x in row] or \
       "$" in row or (y == 0 and not hasstart):
        outputrowdata(y, row, "right", "up", "down", 
                    lambda a,b:a, 
                    lambda a,b:b)


for y, row in enumerate(program):
    row = row[:]
    row.reverse()
    if [x for x in movecharacters if x in row]:
        outputrowdata(y, row, "left", "down", "up", 
                    lambda a,b:progwidth - a - 1,
                    lambda a,b:b)
            

for x, row in enumerate(zip(*program)):
    if [y for y in movecharacters if y in row]:
        outputrowdata(x, row, "down", "left", "right", 
                    lambda a,b:b, 
                    lambda a,b:a)       
       

for x, row in enumerate(zip(*program)):
    row = list(row)
    row.reverse()
    if [y for y in movecharacters if y in row]:
        outputrowdata(x, row, "up", "right", "left",
                    lambda a,b:b, 
                    lambda a,b:progheight - a - 1)    
        

print "endprogram:"
print "push DWORD 0"
print "call [ExitProcess]"