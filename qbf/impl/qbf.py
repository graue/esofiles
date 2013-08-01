#!/usr/bin/python

# A Quantum Brainfuck interpreter
# For now, very basic with no error checking
# Implements extra commands:
# '1' initializes the current cell to 1
# '0' initializes the current cell to 0
# '#' outputs the probabilities of all the different possible states

import qubit
import sys

DEBUG = True  # Adds some extra helpful commands

if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "<filename>"
    raise SystemExit

infile = file(sys.argv[1])

program = infile.read()
memory = qubit.Register(2)
qubit_positions = range(2)
mp = 0
cp = 0

while cp < len(program):
    cmd = program[cp]
    if cmd == '<':
        mp -= 1
    elif cmd == '>':
        mp += 1
        if mp >= memory.length:
            qubit_positions.append(memory.length)
            memory.add_bit()
        if mp >= 20:
            # just a precaution, feel free to remove this
            print "Pointer moved right too many qubits; terminating"
    elif cmd == '%':
        qubit.Hadamard.apply(memory, [qubit_positions[mp]])
    elif cmd == '!':
        qubit.CV.apply(memory,[qubit_positions[mp],qubit_positions[mp+1]])
    elif cmd == '&':
        temp = qubit_positions[mp]
        qubit_positions[mp] = qubit_positions[mp+1]
        qubit_positions[mp+1] = temp
    elif cmd == '.':
        # Output the qubit
        # For now, output it as a '1' or '0' and output causes observation
        output = memory.observe(qubit_positions[mp])
        print output,
    elif cmd == ',':
        # input a qubit
        i = raw_input("\nPlease enter 1 or 0: ")
        i = int(i)
        assert i == 1 or i == 0
        memory.set(qubit_positions[mp], i)
    elif cmd == '[':
        # observe the current qubit, loop if it's 1
        bit = memory.observe(qubit_positions[mp])
        if not bit:
            # skip till the end of the loop
            bracket_level = 1
            while bracket_level:
                cp += 1
                if program[cp] == '[': bracket_level += 1
                if program[cp] == ']': bracket_level -= 1
    elif cmd == ']':
        # just go back to the matching bracket
        bracket_level = -1
        while bracket_level:
            cp -= 1
            if program[cp] == '[': bracket_level += 1
            if program[cp] == ']': bracket_level -= 1
        cp -= 1
    # Now, some helpful debugging commands:
    elif DEBUG and cmd == '1':
        memory.set(qubit_positions[mp], 1)
    elif DEBUG and cmd == '0':
        memory.set(qubit_positions[mp], 0)
    elif DEBUG and cmd == '#':
        # pretty print memory contents
        print
        for i in range(len(memory.contents)):
            print '|'+''.join([str(b) for b in qubit.n2bits(i,memory.length)])+'>',
            print '%.2f' % (abs(memory.contents[i])**2)

    else:
        # do nothing
        pass
    cp += 1
