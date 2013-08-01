#!/usr/bin/env python
"""\
Udage Interpreter in Python, revision 1
Copyright (c) 2005, Kang Seonghoon (Tokigun).

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
"""

import sys

class UdageDict(dict):
    def __init__(self, default=None, object=None):
        if object is None: dict.__init__(self)
        else: dict.__init__(self, object)
        self.default = default

    def __getitem__(self, key):
        try: return dict.__getitem__(self, key)
        except KeyError: return self.default

class UdageIO:
    def __init__(self, encoding='utf-8'):
        self.encoding = encoding

    def getunicode(self):
        buf = ''
        while 1:
            char = sys.stdin.read(1)
            if char == '': return -1
            buf += char
            try:
                return ord(buf.decode(self.encoding))
            except UnicodeError:
                try:
                    reason = sys.exc_info()[1].reason
                    if not ('end of data' in reason or 'incomplete' in reason):
                        return None
                except: pass

    def getchar(self):
        char = self.getunicode()
        if char is None:
            return 0xfffd
        else:
            return char == 13 and 10 or char

    def putchar(self, value):
        try: sys.stdout.write(unichr(value).encode(self.encoding))
        except: sys.stdout.write('[U+%04X]' % value)

class UdageInterpreter:
    def __init__(self, code, io=None):
        if not isinstance(code, unicode):
            code = unicode(code)
        self.code = code
        self.io = io

    def init(self):
        self.pointer = UdageDict(0)
        self.value = UdageDict(False)
        self.current = 0

    def getswitch(self, key):
        return self.value[self.pointer[key] or key]

    def setswitch(self, key, value):
        self.value[self.pointer[key] or key] = value

    def step(self):
        if self.current >= len(self.code): return False

        char = self.code[self.current]
        if self.getswitch(char):
            ptr = self.current
            while ptr < len(self.code) and char == self.code[ptr]:
                ptr += 1
            length = ptr - self.current
            if length == 1:
                self.setswitch(char, False)
                self.current += 1
            elif length == 2:
                ptr += 3
                if ptr > len(self.code): raise RuntimeError, "incomplete instruction"
                operand1 = self.getswitch(self.code[ptr-3])
                operand2 = self.getswitch(self.code[ptr-2])
                target = self.code[ptr-1]
                result = not (operand1 and operand2)
                self.setswitch(char, result)
                if not result:
                    direction = self.getswitch(target)
                    if direction:
                        ptr = self.code.rfind(target, 0, self.current)
                    else:
                        ptr = self.code.find(target, ptr)
                    if ptr < 0: raise RuntimeError, "cannot jump to undefined instruction"
                    ptr += 1
                self.current = ptr
            elif length == 3 or length == 4:
                endptr = self.code.find(char, ptr)
                if endptr < 0: raise RuntimeError, "incomplete instruction"
                number = self.code[ptr:endptr]
                value = reduce(lambda x,y:x*2+int(self.getswitch(y)), number, 0)
                if length == 3:
                    self.pointer[char] = value
                elif value > 0:
                    self.io.putchar(value)
                else:
                    value = self.io.getchar()
                    for char in number[::-1]:
                        self.setswitch(char, bool(value & 1))
                        value >>= 1
                self.current = endptr + 1
            else:
                raise RuntimeError, "invalid instruction"
        else:
            self.setswitch(char, True)
            self.current += 1

        return True

    def execute(self):
        self.init()
        while self.step(): pass

################################################################################

def version(apppath):
    print __doc__
    return 0

def help(apppath):
    print __doc__[:__doc__.find('\n\n')]
    print
    print "Usage: %s [options] <filename> [<param>]" % apppath
    print
    print "--help, -h"
    print "    prints help message."
    print "--version, -V"
    print "    prints version information."
    print "--fileencoding, -e"
    print "    set encoding of source code. (default 'utf-8')"
    print "--ioencoding"
    print "    set encoding for input/output."
    return 0

def main(argv):
    import getopt, locale

    try:
        opts, args = getopt.getopt(argv[1:], "hVe:",
                ['help', 'version', 'fileencoding=', 'ioencoding=',])
    except getopt.GetoptError:
        return help(argv[0])

    fencoding = 'utf-8'
    ioencoding = locale.getdefaultlocale()[1]
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            return help(argv[0])
        if opt in ('-V', '--version'):
            return version(argv[0])
        if opt in ('-e', '--fileencoding'):
            fencoding = arg
        if opt == '--ioencoding':
            ioencoding = arg
    if len(args) == 0:
        print __doc__[:__doc__.find('\n\n')]
        return 1

    # encoding fix
    if fencoding == 'euc-kr': fencoding = 'cp949'
    if ioencoding == 'euc-kr': ioencoding = 'cp949'

    filename = args[0]
    param = args[1:]
    try:
        if filename == '-':
            data = sys.stdin.read()
        else:
            data = file(filename).read()
    except IOError:
        print "Cannot read file: %s" % filename
        return 1
    try:
        code = data.decode(fencoding)
    except LookupError:
        print "There is no encoding named '%s'." % fencoding
        return 1
    except UnicodeDecodeError:
        print "Unicode decode error!"
        return 1

    io = UdageIO(ioencoding)
    interpreter = UdageInterpreter(code.strip(), io)
    interpreter.execute()
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))

