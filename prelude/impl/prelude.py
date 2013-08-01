#!/usr/bin/python

import sys

NUMERIC_OUTPUT = True

try:
    filename = sys.argv[1]
except:
    print "Usage:", sys.argv[0], "<filename>"
    raise SystemExit

try:
    inFile = file(filename)
except:
    print "Error when opening", filename
    raise SystemExit

# code is kept as a list of voices, each voice a string

code = []
firstLine = True
eof = False
while not eof:
    # read batch of lines
    batch = []
    while 1:
        line = inFile.readline()
        if line == '': eof = True
        if line == '' or line.rstrip() == '*':
            break
        batch.append(line.rstrip())
    maxLen = max([len(b) for b in batch])
    batch = [b + ' '*(maxLen - len(b)) for b in batch]
    if firstLine:
        code = batch
        firstLine = False
    else:
        if len(batch) != len(code):
            print "Error in the program: number of voices changes"
            raise SystemExit
        for i in range(len(batch)):
            code[i] += batch[i]


class Stack:
    def __init__(self):
        self.data = []
    def push(self, value):
        if self.data or value != 0: self.data.append(value)
    def drop(self):
        if self.data: self.data.pop()
    def top(self):
        if self.data: return self.data[-1]
        return 0
    def pop(self):
        value = self.top()
        self.drop()
        return value




numVoices = len(code)
numInstructions = len(code[0])
stacks = [Stack() for x in range(numVoices)]
topValues = [0 for x in range(numVoices)]
# establish loop couplings
loopStack = []
loops = {}
for cp in range(numInstructions):
    curr = [voice[cp] for voice in code]
    if curr.count('(') + curr.count(')') > 1:
        print "Error in the program: More than one bracket; position", cp
        raise SystemExit
    if '(' in curr:
        loopStack.append((cp, curr.index('(')))
    if ')' in curr:
        if not loopStack:
            print "Error in the program: extraneous closing bracket; position", cp
            raise SystemExit
        openingPosition, openingVoice = loopStack.pop()
        loops[openingPosition] = cp
        loops[cp] = openingPosition, openingVoice

if loopStack:
    print "Error in the program: not enough closing brackets"
    raise SystemExit


# now, actually execute the program
cp = 0 # code pointer
while cp < numInstructions:
    # technically we're supposed to shuffle our voices to make sure to perform IO
    # in random order, but screw that for now
    next_cp = cp+1 # can be modified by ( )
    for voice in range(numVoices):
        i = code[voice][cp] # current instruction
        if i == '^':
            stacks[voice].push(topValues[(voice-1) % numVoices])
        elif i == 'v' or i == 'V':
            stacks[voice].push(topValues[(voice+1) % numVoices])
        elif i == '+':
            stacks[voice].push(stacks[voice].pop() + stacks[voice].pop())
        elif i == '-':
            b = stacks[voice].pop()
            a = stacks[voice].pop()
            stacks[voice].push(a-b)
        elif i == '#':
            stacks[voice].drop()
        elif i == '?':
            char = sys.stdin.read(1)
            if not char: char = '\0'
            stacks[voice].push(ord(char))
        elif i == '!':
            if NUMERIC_OUTPUT:
                print stacks[voice].pop()
            else:
                sys.stdout.write(chr(stacks[voice].pop()))
        elif i == '(':
            if stacks[voice].top() == 0:
                next_cp = loops[cp] + 1
        elif i == ')':
            openingPosition, openingVoice = loops[cp]
            if topValues[openingVoice] != 0:
                next_cp = openingPosition + 1
        elif i in '0123456789':
            stacks[voice].push(int(i))
    topValues = [stacks[i].top() for i in range(numVoices)]
    cp = next_cp






