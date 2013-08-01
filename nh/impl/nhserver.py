#!/usr/bin/env python

# nhserver.py
# Network Headache server
# Written by Marinus Oosters

import sys, re, socket, getopt, thread;

# variables

variables = {};
port = 31337;
statport = 0;

progs_running = 0;
progs = [];

# thread locks
proglock = thread.allocate_lock();
varlock = thread.allocate_lock();

# some functions
def isnum(c): return re.compile('[0-9]').match(c);
def isvar(c): return re.compile('[A-Z]').match(c);

def getValue(c):
	if isnum(str(c)): return int(c);
	elif isvar(str(c)):
		try: return int(variables[c]);
		except: return 0;

# Program class
# note: expects NH Intermediate, not Network Headache.
# The parsing and translating is to be done by the client.
class Program:
	stack = [];
	ipstack = [];
	labels = {};
	program = '';
	ended = False;
	skip = False; #set to True if last operation was SKIP
	ip = 0;
	lastcmd = -1;
	client = 0;
        input = '';
	
	done = False;
	def inputfunc(self):
		try:
			ch = self.input[0];
			self.input = self.input[1:];
		except: ch = chr(0);
		return ch;
	
	def outputfunc(self,x): 
		try: self.client.send(x);
		except:
			self.ended = True; # connection is broken

	def __init__(self,pgm,c,input): 
		self.program = pgm;
		self.client = c;
		self.input = input;
	def reset(self):
		self.ip = 0;
		self.stack = [];
		self.ipstack = [];
		self.labels = {};
		self.ended = False;
		self.skip = False;
		self.lastcmd = -1;
		self.inputting = False;
		self.done = False;
		
	# execute until the next %
	def step(self):
		if not self.ip < len(self.program): self.ended = True;
		if self.ended: return;
		self.done = False;
		while self.ip < len(self.program):
			self.skip = False;
			if (self.program[self.ip] == '%'):
				self.lastcmd = self.ip;
				self.stack = [];
				self.ip += 1;
				self.done = True;
				return;
			elif (self.program[self.ip] == "'" or self.program[self.ip] == '"'):
				match = self.program[self.ip];
				val = "";
				self.ip += 1;
				try:
					while (self.program[self.ip] != match):
						val += self.program[self.ip];
						self.ip += 1;
				except IndexError:
					self.outputfunc("001 Identifier not closed.\n");
					self.ended = True;
					return;
				self.stack.append(val);
			elif (self.program[self.ip] == '-'):
				try:
					a = getValue(self.stack.pop());
					b = getValue(self.stack.pop());
					self.stack.append(abs(a-b));
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '+'):
				try:
					a = getValue(self.stack.pop());
					b = getValue(self.stack.pop());
					self.stack.append(a+b);
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '*'):
				try:
					a = getValue(self.stack.pop());
					b = getValue(self.stack.pop());
					self.stack.append(a*b);
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '$'):
				try:
					a = getValue(self.stack.pop());
					b = getValue(self.stack.pop());

					# "Bits are alternated from the left and right operands, with the
					# least significant bit of the right operand becoming the least
					# significant bit of the output".

					c = 0;
					for bit in [0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30]: c = (c & ~(1<<bit)) | (b&1<<bit);
					for bit in [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31]: c = (c & ~(1<<bit)) | (a&1<<bit);

					self.stack.append(int(c));
				except IndexError: 
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '~'): # great, another weird bitwise operator
				try:
					a = getValue(self.stack.pop());
					b = getValue(self.stack.pop());

					# "Only bits in the left operand corresponding to set bits in the
					# right operand are used to affect the result, and these bits are
					# justified towards the least significant and padded with zeros."

					c = 0;
					bits_chosen = 0;
					for bit in range(0,32):
						if (b>>bit)&1: 
							c = (c & ~(1<<bits_chosen)) | ((a>>bit)&1<<bits_chosen);
							bits_chosen += 1;

					self.stack.append(c);
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '?'): # yet another bitwise operator, thankfully a supported one
				try:
					a = getValue(self.stack.pop());
					self.stack.append(a^(2*a)); # well, the documentation said (a) XOR (2a)
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '|'):
				try:
					a = getValue(self.stack.pop());
					if a: self.stack.append(0);
					else: self.stack.append(1);
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '='):
				try:
					a = self.stack.pop();
					if not isvar(a):
						self.outputfunc("003 Tried to write a value to another value.\n");
						self.ended = True;
						return;
					b = getValue(self.stack.pop());
					variables[a] = b;
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == ';'):
				try:
					a = self.stack.pop();
					if not isvar(a):
						self.outputfunc("003 Tried to write a value to another value.\n");
						self.ended = True;
						return;
					variables[a] = ord(self.inputfunc());
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == ':'):
				try:
					a = getValue(self.stack.pop());
					self.outputfunc(chr(int(a)));
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '#'):
				self.skip = True;
			elif (self.program[self.ip] == '&'):
				try:
					a = getValue(self.stack.pop());
					self.labels[a] = self.lastcmd;
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '>'):
				try:
					a = getValue(self.stack.pop());
					try:
						self.ipstack.append(self.lastcmd);
						self.ip = self.labels[a];
						self.lastcmd=self.ip;
						self.stack = [];
					except KeyError: pass;
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '`'):
				try:
					a = getValue(self.stack.pop());
					b = getValue(self.stack.pop());
					if (a):
						try:
							self.ipstack.append(self.lastcmd);
							self.ip = self.labels[b];
							self.lastcmd = self.ip;
							self.stack = [];
						except KeyError: pass;
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '^'):
				try:
					a = getValue(self.stack.pop());
					try:
						i=0;
						while (a>0):
							i += 1;
							if self.program[i] == '%': a -= 1;
						self.ip = i-1;
					except:
						self.outputfunc("004 Jumped past program end\n");
						self.ended = True;
						return;
				except:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '\\'):
				try:
					a = getValue(self.stack.pop());
					b = getValue(self.stack.pop());
					if (a):
						try:
							i=0;
							while (b>0):
								i += 1;
								if self.program[i] == '%': b -= 1;
							self.ip = i-1;
						except:
							self.outputfunc("004 Jumped past program end\n");
							self.ended = True;
							return;
				except:
					self.outputfunc("002 Stac is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '/'):
				try:
					a = getValue(self.stack.pop());
					for i in range(0,a): self.ipstack.pop();
				except IndexError:
					self.outputfunc("002 Stack is empty.\n");
					self.ended = True;
					return;
			elif (self.program[self.ip] == '.'):
				try: 
					self.ip = int(self.ipstack.pop());
					self.stack = [];
				except IndexError:
					# program end
					self.outputfunc("\n");
					self.ended = True;
					return;
			self.ip+=1;
			if not self.ip < len(self.program): self.done = True; return;
		self.done = True;
		return 0;

## help
def help():
	print """
nhserver.py -- Network Headache server
Written by Marinus Oosters
Usage:
        nhserver.py [-h] [-p port] [-s port]

	-h		This screen
	-p port		Listen on port [port] (by default 31337).
	-s port         Listen on port [port] for status (it doesn't by default).
""";

# status
def status_srvr(c):
	global variables;
	str = 'nhserver version 0.1 | by Marinus Oosters\n';
	str += "There are %d programs running.\n" % progs_running;
	str += "-------------------------------------------------------------\n";
	str += "Currently defined variables on this system:\n";
	for n in variables.keys(): str += "%20s:%20s\n" % (n, variables[n]);
	str += "-------------------------------------------------------------\n";
	c.send(str);
	c.close();


# status accepting thread
def status_thread():
	while (1):
		(clnt,ap) = statlsock.accept();
		status_srvr(clnt);
		
# program running thread
def progrthread(c):
	global progs_running;
	s = c.recv(5);
	if s != 'NHRDP':
		# The client does not send the welcoming message. Disconnect it.
		c.close();
		return;
	c.send("NHSRV"); # no IP banning, and it can't be full either.
	s = "";
	ch = c.recv(1);
	input = '';
	# input/program?
	if (ch == 'I'):
		ch = c.recv(1);
		while (ch != chr(0xFE)):
			input += ch;
			ch = c.recv(1);
			if (ch == ''):
				c.close();
				return;
		ch = c.recv(1);
	if (ch == 'P'):
		ch = c.recv(1);
		while (ch != chr(0xFE)):
			s += ch;
			ch = c.recv(1);
			if (ch == ''):
				# the client quit
				c.close();
				return;
	
	# we now have program code. Make the object and add it to the list.
	thisProgram = Program(s,c,input);
	proglock.acquire();
	progs.append(thisProgram);
	progs_running += 1;
	proglock.release();

	# keep the connection alive until the program has quit.
	while not thisProgram.ended: pass;
	try:
		c.send(chr(0xFE));
		c.close();
	except: pass;

# multitasking program thread
def prog_thread():
	# this function will loop, executing all programs in the list.
	skip = False;
	global progs_running;
	while(1):
		# nothing will be done, if no programs are running.
		if not progs_running: continue;
		for prog in progs:
			proglock.acquire();
			if prog.ended: 
				progs_running -= 1;
				progs.remove(prog);
			else:
				if not skip: 
					prog.step();
					skip = prog.skip;
				else: skip = False;
			proglock.release();

## sockets
statlsock = socket.socket(socket.AF_INET,socket.SOCK_STREAM);
proglsock = socket.socket(socket.AF_INET,socket.SOCK_STREAM);

## main function
def main(argv):
	global statport, port;
	# get options
	try: opt, arg = getopt.getopt(argv[1:],'hp:s:');
	except getopt.error, msg:
		print msg;
		print "For help use -h.";
		sys.exit();
	for o,a in opt:
		if o == '-h':
			help();
			sys.exit();
		elif o == '-p':
			port = int(a);
		elif o == '-s':
			statport = int(a);
	
	if statport: 
		# if a status port has been given, set up its port and start the thread
		statlsock.bind(('',statport));
		statlsock.listen(10);
		thread.start_new_thread(status_thread, ());
	
	# start the multitasking thread
	thread.start_new_thread(prog_thread,());

	# use the main thread for setting up program connections
	proglsock.bind(('',port));
	proglsock.listen(5);
	while(1):
		(clnt,ap) = proglsock.accept();
		thread.start_new_thread(progrthread,(clnt,))

if __name__=="__main__": main(sys.argv);
