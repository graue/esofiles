#!/usr/bin/env python
#
# NULLRUN -- NULL Interpreter in Python (2005-01-18)
# Copyright (c) 2005, Kang Seonghoon <tokigun@gmail.com>.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

from sys import stdin, stdout, argv, exit
try:
    from primes import list as plist
except ImportError:
    # first 100 primes (default)
    plist = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
        67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139,
        149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223,
        227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293,
        307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383,
        389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
        467, 479, 487, 491, 499, 503, 509, 521, 523, 541]

def sprp(n, a):
    if n < 2 or n & 1 == 0: return False
    d = n - 1
    while d & 1 == 0:
        d >>= 1
        if pow(a, d, n) + 1 == n: return True
    return pow(a, d, n) == 1

def isprime(n):
    if n < 2: return False
    elif n < 4: return True
    elif not sprp(n, 2): return False
    elif n < 2047: return True
    elif not sprp(n, 3): return False
    elif n < 1373653: return True
    elif not sprp(n, 5): return False
    elif n < 25326001: return True
    elif n == 3215031751 or not sprp(n, 7): return False
    elif n < 118670087467: return True
    elif not sprp(n, 11): return False
    elif n < 2152302898747: return True
    elif not sprp(n, 13): return False
    elif n < 3474749660383: return True
    elif not sprp(n, 17): return False
    elif n < 341550071728321: return True
    else: raise ValueError

def factor_g(include_builtin_list = True):
    if include_builtin_list:
        for x in plist: yield x
    k = plist[-1] + 7
    if k % 6 == 2:
        yield k-5
        k -= 2
    while True:
        yield k-1
        yield k+1
        k += 6

def factor(n):
    if n < 2: return n
    for i in factor_g():
        if n % i == 0: return i

def nprime(n):
    if n <= plist[-1]:
        try: return plist.index(n) + 1
        except: return -1
    else:
        k = len(plist) + 1
        for i in factor_g(False):
            if i >= n: break
            try:
                if isprime(i):
                    plist.append(i)
                    k += 1
            except:
                return -1
        if isprime(n):
            plist.append(n)
            return k
        else:
            return -1

def interpret(prog):
    q = [[], [], []]; qp = 0
    x = long(prog); y = 1
    while x > 1:
        n = factor(x); x /= n; y *= n
        n = nprime(n) % 14
        if n == 0:
            break
        elif n == 1:
            qp = (qp + 1) % 3
        elif n == 2:
            qp = (qp - 1) % 3
        elif n == 3:
            stdout.write(len(q[qp]) and chr(q[qp][-1]) or '\0')
        elif n == 4:
            tmp = ord(stdin.read(1))
            if len(q[qp]): q[qp][-1] = tmp
            else: q[qp].append(tmp)
        elif n == 5:
            if len(q[qp]):
                y -= q[qp][-1]
                if y < 0: y = 0
        elif n == 6:
            if len(q[qp]): y += q[qp][-1]
        elif n == 7:
            if len(q[qp]): q[qp][-1] = (q[qp][-1] + y) & 255
            else: q[qp].append(y & 255)
        elif n == 8:
            if len(q[qp]): q[(qp+1)%3].insert(0, q[qp].pop())
            else: q[(qp+1)%3].append(0)
        elif n == 9:
            if len(q[qp]): q[(qp-1)%3].insert(0, q[qp].pop())
            else: q[(qp-1)%3].append(0)
        elif n == 10:
            try: q[qp].pop()
            except: pass
        elif n == 11:
            q[qp].insert(0, y & 255)
        elif n == 12:
            if len(q[qp]) == 0 or q[qp][-1] == 0:
                n = factor(x); x /= n; y *= n
        elif n == 13:
            x, y = y, x

def main(argv):
    if len(argv) != 2:
        print "NULLRUN, interpreter of NULL programming language"
        print "Kang Seonghoon (Tokigun) @ TokigunStudio 2005"
        print
        print "Usage: python %s <filename>" % argv[0]
        print "Source file can be Python expression. (e.g. 2*3*5**8)"
        print "You can use Python-style comment in each line of code."
        print
        print "Size of the prime database (primes.py) is %d." % len(plist)
        return 0
    
    try:
        code = file(argv[1]).read()
    except:
        print "Error: can't read file \"%s\"." % argv[1]
        return 1
    
    rcode = ''
    for line in code.splitlines():
        pos = line.find('#')
        if pos >= 0: line = line[:pos]
        rcode += line + ' '
    code = eval(rcode, {}, {})
    
    interpret(code)
    return 0

if __name__ == '__main__': exit(main(argv))

