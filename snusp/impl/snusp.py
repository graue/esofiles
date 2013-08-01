"""
This program is a complete Windows Bloated SNUSP interpreter.
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
import sys
import random
import msvcrt

try:
    import psyco #greatly speeds it up
    psyco.full()
except:
    pass

#0 = core, 1 = modular, 2 = bloated
version = 2

class Thread:
    """Store data (instruction pointer, etc.) for a thread)"""
    def __init__(self, insp, dire, memindex, memlevel, callstack):
        self.insp = insp
        self.dire = dire
        self.memindex = memindex
        self.memlevel = memlevel
        self.callstack = callstack
            
            
def findnext(dire, x, y):
    """Find where to go to next."""
    if dire == 0:
        return x + 1, y
    if dire == 1:
        return x, y + 1
    if dire == 2:
        return x - 1, y
    if dire == 3:
        return x, y - 1
    print "Invalid direction"
    
def gotonext():
    """Go to the next location."""
    global currx, curry, dire
    currx, curry = findnext(dire, currx, curry)

program = []
currx = curry = 0
maxlen = 0
for line in fileinput.input():
    n = [c for c in line]
    if currx == 0 and curry == 0 and line.find("$") >= 0:
        currx = line.index("$")
        curry = len(program)
    program.append(n)
    if maxlen < len(n):
        maxlen = len(n)

for line in range(len(program)): #pad the lines out
    program[line] += " " * (maxlen - len(program[line]))

dire = 0
stack = [[0]]
currindex = 0
currlevel = 0
callstack = []

threads = [Thread((currx, curry), dire, currindex, currlevel, callstack)]

currthread = 0



def savethreaddata():
    """Save all of the exposed thread data into the thread object."""
    threads[currthread].insp = (currx, curry)
    threads[currthread].dire = dire
    threads[currthread].memindex = currindex
    threads[currthread].memlevel = currlevel
    threads[currthread].callstack = callstack
    
def loadnextthread():
    """Load all of a thread's data from its associated object."""
    global currthread, currx, curry, dire, currindex, callstack, currlevel
    
    if not threads:
        return
        
    currthread = (currthread + 1) % len(threads)
    
    currx = threads[currthread].insp[0]
    curry = threads[currthread].insp[1]
    dire = threads[currthread].dire
    currindex = threads[currthread].memindex
    currlevel = threads[currthread].memlevel
    callstack = threads[currthread].callstack 

while threads:

    loadnextthread()
    
    if curry < 0 or curry >= len(program) or currx < 0 or currx >= len(program[curry]):
        del(threads[currthread])
        continue
        
        
    currchar = program[curry][currx]
    #print currx, curry, stack[currindex]
    
    blocked = 0
    if currchar == ">":
        currindex += 1
        if len(stack[currlevel]) <= currindex: #lengthen the stack if it's not long enough
            stack[currlevel].append(0)
            
    elif currchar == "<":
        currindex -= 1
        
    elif currchar == "+":
        stack[currlevel][currindex] += 1
        
    elif currchar == "-":
        stack[currlevel][currindex] -= 1
        
    elif currchar == "/":
        dire = 3 - dire #rotate
        
    elif currchar == "\\":
        dire = dire ^ 1 #rotate a different way
        
    elif currchar == "!": #skip a space
        gotonext()
        
    elif currchar == "?": #skip a space if the current space has a zero
        if not stack[currlevel][currindex]:
            gotonext()
            
    elif currchar == ",": #read input 
        if msvcrt.kbhit():
            stack[currlevel][currindex] = ord(msvcrt.getche())
        else:
            blocked = 1
        
    elif currchar == ".": #write output
        sys.stdout.write(chr(stack[currlevel][currindex]))
        
    elif currchar == "@" and version >= 1: #append the current location
        callstack.append((dire, currx, curry))
        
    elif currchar == "#" and version >= 1: #pop off the current location and move there
        if len(callstack) == 0:
            del(threads[currthread])
            continue
        dire, currx, curry = callstack.pop()
        gotonext()
        
    elif currchar == "&" and version >= 2: #make a new thread
        gotonext()
        threads.append(Thread((currx, curry), dire, currindex, currlevel, []))
    
    elif currchar == "%" and version >= 2: #get a random number
        stack[currlevel][currindex] = random.randrange(stack[currlevel][currindex])
    
    elif currchar == ":": #move up a memory level
        currlevel += 1
        if len(stack) <= currlevel:
            stack.append([0] * (currindex + 1))
            
        if len(stack[currlevel]) <= currindex:
            stack[currlevel].extend([0] * (currindex + 1 - len(stack[currlevel])))
    
    elif currchar == ";": #move down a memory level
        currlevel -= 1
        if len(stack[currlevel]) <= currindex:
            stack[currlevel].extend([0] * (currindex + 1 - len(stack[currlevel])))           
    
      
    if not blocked:
        gotonext()
        
    savethreaddata()
