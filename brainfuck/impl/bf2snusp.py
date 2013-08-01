"""
This program converts BrainF**k files to SNUSP.
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

def strreverse(string):
    """Reverse the input string - reverse the redirection characters too."""
    string.reverse()
    for x in range(len(string)):
        if string[x] == "\\":
            string[x] = "/"
        elif string[x] == "/":
            string[x] = "\\"        

#input the program, ignore unimportant characters
validchars = "[].,+-<>"
program = []
for line in fileinput.input():
    for char in line:
       if char in validchars:
           program.append(char)


def recursiveput(string):
    """Take the data in the string and generate a program from it (recursively)"""
    string = string[:]
    lines = [[]]

    while string:
        try: #check if we must recurse any more
            sloc = string.index("[")
        except:
            lines[0].extend(string)
            break
    
        lines[0].extend(string[:sloc]) #set up for the function call
        lines[0].append("!")
        lines[0].append("/")
        string = string[sloc + 1:]
        
        numpushes = 1 #find matching "]"
        currchar = 0
        while numpushes > 0:
            if string[currchar] == "[":
                numpushes += 1
            if string[currchar] == "]":
                numpushes -= 1
            currchar += 1
        
        newval = recursiveput(string[:currchar - 1])
        
        
        maxlen = 0
        for line in newval:
            if len(line) > maxlen:
                maxlen = len(line)
                
        for line in newval: #pad the lines and reverse them
            line.extend([" "] * (maxlen - len(line)))
            strreverse(line)
            
        for val in range(len(newval) - len(lines) + 1): #make enough extra lines to fit
            lines.append([])
            
        for line in lines[1:]: #space-pad the new lines
            if len(line) < len(lines[0]):
                line.extend([" "] * (len(lines[0]) - len(line)))
            
        del lines[1][-1]
        lines[1].append("\\")
            
        lines[0].extend(["="]*(len(newval[0]) - 1))
        lines[0].append("?")
        lines[0].append("\\")

        for line, rline in zip(lines[1:], newval):
            line.extend(rline)
        lines[1].append("/")

        string = string[currchar:]

    return lines

programvals = recursiveput(program)

for line in programvals:
    print "".join(line)
        
    
    
