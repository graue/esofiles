# A qubit library
# Author: Nikita Ayzikovsky (lament)
# You may use this freely, but it's neither GPL nor public domain (yet)

# Note that everything starts out initialized to 1. This needs to be
# changed if this is to be used for something other than Quantum Brainfuck.
# Other than that, the library is completely generic. Just add your own
# gates and go ahead.

# The things that should be actually used from outside are
# clases Gate and Register.

import random # of course!

def dotproduct(a,b):
    """Because I'm too lazy to look for a lin. algebra module"""
    return sum([x*y for (x,y) in zip(a,b)])

def mvmult(matrix, vector):
    """Because I'm too lazy to look for a lin. algebra module"""
    #no error checking for now
    result = []
    for row in matrix:
        result.append(dotproduct(row, vector))
    return result

def bit(num, bitnum):
    return int((num&(1<<bitnum)) != 0)

def n2bits(n, length):
    return [bit(n, x) for x in range(length)]

def bits2n(bitlist):
    result = 0
    for x in range(len(bitlist)):
        result += bitlist[x] * (1<<x)
    return result

class Gate:
    def __init__(self, num_bits, matrix):
        self.num_bits = num_bits
        self.N = 1<<num_bits
        self.matrix = matrix
    def apply(self, register, bitlist):
        """Applies this gate to bits from bitlist (size of bitlist should
        be N) in the register object given"""
        # let's do this the least efficient way possible
        # apply the gate to bits in bitlist for each individual combination
        # of other bits
        # We have register.length total bits, so register.length-N
        # bit combinations

        new_contents = register.contents[:]

        num_bits = register.length
        allbits = range(num_bits)
        otherbits = [x for x in allbits if x not in bitlist]
        # Iterate over each combination of other bits:
        for i in range(2**len(otherbits)):
            other_bit_values = n2bits(i, len(otherbits))
            positions = []
            for j in range(self.N):
                current_position = [0] * num_bits
                for index, value in zip(otherbits, other_bit_values):
                    current_position[index] = value
                for index, value in zip(bitlist,n2bits(j, self.N)):
                    current_position[index] = value
                positions.append(bits2n(current_position))
            values = [register.contents[x] for x in positions]
            values = mvmult(self.matrix, values)
            for x, index in zip(positions, range(self.N)):
                new_contents[x] = values[index]
        register.contents = new_contents

class Register:
    def __init__(self, length):
        """Initialize to all 1s as per Quantum Brainfuck specs"""
        self.length = length
        self.contents = [0] * (2**length)
        self.contents[-1] = 1
    def add_bit(self):
        """Adds a new qubit, containing 1 and not entangled with anything"""
        new_contents = []
        new_contents = [0] * len(self.contents) + self.contents
        self.contents = new_contents
        self.length += 1
    def observe(self, bit_index):
        """Observes the value of the qubit at the given index,
        changing that bit to |0> or |1> and updating all probabilities
        accordingly. Returns 0 or 1"""

        # first, find out the probability that the qubit is set.
        prob = 0
        for i in range(2 ** self.length):
            prob +=  abs(bit(i, bit_index) * self.contents[i]) ** 2
        # prob is now set to the probability of bit set to 1
        # now "Observe"
        if random.random() <= prob:
            bit_value = 1
        else:
            bit_value = 0
        # now that we know the "observed" value, adjust all other
        # probabilities to account for it.
        if prob == 0 or prob == 1:
            # don't need adjustment
            return bit_value

        adjustment = (1 / prob) ** 0.5
        for i in range(2 ** self.length):
            if bit(i, bit_index) == bit_value:
                self.contents[i] = self.contents[i] * adjustment
            else:
                self.contents[i] = 0
        return bit_value

    def set(self, bit_index, bit_value):
        """Sets the indexed bit to 'value', which should be 1 or 0"""
        for i in range(2 ** self.length):
            if bit(i, bit_index) == bit_value:
                # take the 'sister' bit combination and add its
                # probability to this one
                if bit_value:
                    sister = i & (~ 1<<bit_index)
                else:
                    sister = i | 1<<bit_index
                total_prob = self.contents[i] **2 + self.contents[sister]**2
                self.contents[i] = math.sqrt(total_prob)
                self.contents[sister] = 0


#############################################################

import math
st = 1/math.sqrt(2)

Hadamard = Gate(1, [[st, st],
                    [st, -st]])

CNOT = Gate(2, [[1, 0, 0, 0],
                [0, 1, 0, 0],
                [0, 0, 0, 1],
                [0, 0, 1, 0]])

CV = Gate(2, [[1, 0, 0, 0],
              [0, 1, 0, 0],
              [0, 0, 1, 0],
              [0, 0, 0, 1j]])

if __name__ == '__main__':
    # just testing stuff
    r = Register(2)
    Hadamard.apply(r, [0])
    print r.contents
    CNOT.apply(r,[1,0])
    print r.contents
    r.set(0, 1)
    print r.contents
