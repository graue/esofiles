#!/usr/bin/env python

# nhclient.py
# Network Headache client
# Written by Marinus Oosters


import sys, re, socket, getopt;

### parser ###
# The parsing happens at the client end to reduce server workload

def isnum(c): return re.compile('[0-9]').match(c);
def isvar(c): return re.compile('[A-Z]').match(c);
def isop(c): return re.compile('[\-\*\$\~\?\|\+]').match(c);
def iscmd(c): return re.compile('[\!\;\:\#\&\>\/\.\=\`\^\\\]').match(c);
def iscode(c): return (isop(c) or iscmd(c));
def onearg(c): return re.compile('[\?\!\;\:\&\>\/\|\^]').match(c);
def noarg(c): return re.compile('[\#\.]').match(c);

def ireplace(str,old,new): return re.compile(old,re.I).sub(new,str);

def parse(input, recursing=0):
	# remove all spaces
	input = re.compile(' ').sub('',input);

	# change the multi-character NH commands into single-character NH Intermediate
	input = ireplace(input,'SET',''); # SET is removed, we'll use its '='.
	input = ireplace(input,'IN',';');
	input = ireplace(input,'OUT',':');
	input = ireplace(input,'SKIP','#');
	input = ireplace(input,'LABEL','&');
	input = ireplace(input,'DO','>');
	input = ireplace(input,'FORGET','/');
	input = ireplace(input,'EXIT','.');
	
	# my own additions: IF..THEN, IF..THEN JUMP, JUMP
	input = ireplace(input,'IF',''); # it really can't do much with the IF...
	input = ireplace(input,'THENJUMP','\\\\');
	input = ireplace(input,'THEN','`');
	input = ireplace(input,'JUMP','^');

	lines = input.split('\n');
	lineno = 0;

	output = '';
	
	for line in lines:
		lineno += 1; # for error reporting
		if (line.replace(' ','')==''): continue;
		if (line[0] == '#'): continue; # lines starting with # are comments
		l = len(line) - 1;
		n = '';
		vals = [];
		op = [];
		# parse a line
		while (l >= 0):
			if isnum(line[l]):
				n = "'";
				while isnum(line[l]) and l>=0: n=line[l]+n; l-=1;
				n = "'" + n;
#				print "** NUMBER RECOGNIZED: %s" % n;
				vals.append(n);
			elif isvar(line[l]):
				n = '"';
				while isvar(line[l]) and l>=0: n=line[l]+n; l-=1;
				n = '"' + n;
#				print "** VARIABLE RECOGNIZED: %s" % n;
				vals.append(n);
			elif iscode(line[l]):
				op.append(line[l]);
#				print "** OPERATOR RECOGNIZED: %s" % line[l];
				l -= 1;
			elif line[l] == ')':
				# find the '(':
				parcnt = 1;
				enclosed = '';
				while parcnt:
					l-=1;
					if (l<0):
						print "error: braces don't match on line %d."%lineno;
						raise "BracesError";
					if (line[l] == '('): parcnt -= 1;
					if (line[l] == ')'): parcnt += 1;
					if (parcnt): enclosed = line[l] + enclosed;
					#recursive parsing
					
				try: vals.append(parse(enclosed, 1));
				except:
					print "error: recursive parsing '%s' failed on line %d"%(enclosed,lineno);
					raise "RecursingError";
				l -= 1;
			elif line[l] == '(':
				print "error: braces don't match on line %d."%lineno;
				raise "BracesError";
			else:
				print "error: unrecognized character %s on line %d"%(line[l],lineno);
				raise "UnrecognizedCharacterError";

		# write it out
		
		vals_used = 0;
#		print "values: %d" % len(vals);
		last_op = '';
		for o in op:
#			print "op: %s vals_used: %d"%(o,vals_used);
			try:
				if not noarg(o):
					if not isop(last_op): output += vals[vals_used]; vals_used += 1;
					if not onearg(o): output += vals[vals_used]; vals_used += 1;
				output += o;
			except IndexError:
				print "error: missing argument(s) for op %s at line %d"%(o,lineno);
				raise "ArgumentError";
			last_op = o;
		if not recursing: output += '%';

	return output;
## usage
def help():
	print """
nhclient.py --- Network Headache client
Written by Marinus Oosters
Usage:
        nhclient.py [-h] [-n] [-o] [-p port] file hostname

	-h              This screen
	-p port         Connect to port [port]. By default 31337.
	-n              The input file contains NH intermediate code, don't parse.
	-o              Only parse, and output the intermediate code. Don't connect.
""";


## main

def main(argv):
	port = 31337;
	noparse = False;
	onlyparse = False;
	file = None;
	hostname = None;
	progtext = None;
	# get options
	try: opt, arg = getopt.getopt(argv[1:],'hp:no');
	except getopt.error, msg:
		print msg;
		print "For help, use -h.";
		sys.exit();
	for o,a in opt:
		if o == '-h':
			help();
			sys.exit();
		elif o == '-p':
			port = int(a);
		elif o == '-n':
			noparse = True;
		elif o == '-o':
			onlyparse = True;
	if noparse and onlyparse:
		print "error: conflicting arguments.\nFor help, use -h.";
		sys.exit();
	try:
		file = arg[0];
		if not onlyparse: hostname = arg[1];
	except:
		print "Argument error.\nFor help, use -h.";
		sys.exit();
	
	# read the file
	try: 
		fh=open(file, 'r');
		fcontents = fh.read();
		if not noparse: progtext = parse(fcontents);
		else: progtext = fcontents;
	except IOError:
		print "Error reading file %s." % file;
		sys.exit();
	
	if onlyparse:
		print progtext;
		sys.exit();
	
	progtext += chr(0xFE);
	
	# connect
	sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM);
	try: sock.connect((hostname,port));
	except: print "Error: can't connect."; sys.exit();
	
	if (sock.send('NHRDP') == 0):
		print "Connection broken.";
		sys.exit();
		
	ret = sock.recv(5);
	if ret != 'NHSRV':
		print "Error: server refused to execute program.";
		sys.exit();
	# any input?
	if (re.compile('.*\;.*').match(progtext)):
		sys.stdout.write("Enter program input, end with EOF: ");
		sys.stdout.flush();
		input = sys.stdin.read();
		sock.send("I" + input + chr(0xFE));
		
	if (sock.send('P' + progtext) == 0):
		print "Connection broken.";
		sys.exit();
	
	while(1):
		ch = sock.recv(1);
		if (ch == '' or ch == chr(0xFE)):
			sys.exit();
	 	else:	sys.stdout.write(ch);
	
		
			
if __name__=='__main__':main(sys.argv);			
