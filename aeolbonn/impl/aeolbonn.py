# Aeolbonn interpreter
# written by Keymaker
# run at your own risk, I can't program


import sys
import random

if len(sys.argv) < 2:
    print "Can run nothing, no Aeolbonn program specified."
    sys.exit()

file = sys.argv[1]

debug = False
if len(sys.argv) > 2:
    if sys.argv[2] == "debug":
        debug = True

try:
    f = open(file)
except IOError:
    print "Error in reading the program."
    sys.exit()

program = []
memory = [False] * 20000

while 1:
    line = f.readline()
    if line == "":
        break
    else:
        if line[len(line)-1] == '\n':
            program.append(line[:len(line)-1])
        else:
            program.append(line)

length = len(program)

pointer = 0
asterisk = 0
flip = False

def execnumber(n):
    global memory
    global pointer
    global flip

    n = int(n)

    if (n % 2) == 0:
        if flip == True:
            pointer = n - 1
    else:
        memory[n] = not memory[n]
        flip = memory[n]

while pointer < length:
    if program[pointer].isdigit() == True:
        execnumber(program[pointer])
    else:
        if program[pointer] == "<":
            if asterisk == 0:
                print "Error, trying to decrease zero on line",pointer
                sys.exit()

            asterisk = asterisk - 1
        elif program[pointer] == ">":
            asterisk = asterisk + 1
        elif program[pointer] == "?":
            if random.randint(0, 1) == 0:
                flip = False
            else:
                flip = True
        elif program[pointer] == "*":
            execnumber(asterisk)
        else:
            line = program[pointer]
            if len(line) == 0:
                print "Error, invalid data on line",pointer
                sys.exit()

            if line[0] == ":":
              if len(line) == 1:
                sys.stdout.write("\n")
              else:
                sys.stdout.write(line[1:])
            else:
                print "Error, invalid data on line",pointer
                sys.exit()

    pointer = pointer + 1

if debug == True:
    print "Memory:",memory
    print "Asterisk:",asterisk
    print "Flip:",flip
    print "Pointer:",pointer
