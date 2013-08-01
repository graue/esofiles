#!/usr/local/bin/perl -w

########################################################
### smurf.pl - An interpreter for the Smurf language ###
### Version 1.0 - 24 April 2001                      ###
### Copyright (c) 2001, Matthew Westcott.            ###
### All rights reserved.                             ###
########################################################

### Read program in from named file ###

$prog='';
while (<>) {
	chomp;
	$prog .= $_
};

@sstack=();
%svars=();

while ($prog ne '') {
	if ($prog =~ /^([^\"])/) {	# Process all instructions except "

		$prog=substr($prog,1);	# Remove the executed instruction
					# (We do this before executing the instruction,
					# because otherwise we'll strip off the first character
					# of a new program executed with 'x')
		EXECOP: {

### Whitespace - skip ###
			if ($1 =~ /\s/) {
				last EXECOP;
			}

### Concatenation (+) operator ###
			if ($1 eq '+') {
				$str1=&popstr;
				$str2=&popstr;
				push (@sstack,$str2.$str1);
				last EXECOP;
			}

### Get variable (g) operator ###
			if ($1 eq 'g') {
				$varname = &popstr;
				if (exists $svars{$varname}) {
				  $vardata = $svars{$varname}
				} else {
				  $vardata = ''}
				push (@sstack,$vardata);
				last EXECOP;
			}

### Head (h) operator ###
			if ($1 eq 'h') {
				$full = &popstr;
				if (length($full) == 0) {
					print "\n\"Roll out that special head\nThis is our favourite one\"\n";exit;
				}
				push (@sstack,substr($full,0,1));
				last EXECOP;
			}

### Input (i) operator ###
			if ($1 eq 'i') {
				$instring = <STDIN>;
				chomp $instring;
				push (@sstack,$instring);
				last EXECOP;
			}

### Output (o) operator ###
			if ($1 eq 'o') {
				$outstring = &popstr;
				print $outstring;
				last EXECOP;
			}

### Put variable (p) operator ###
			if ($1 eq 'p') {
				$varname = &popstr;
				$varcont = &popstr;
				$svars{$varname} = $varcont;
				last EXECOP;
			}

### Quotify (q) operator ###
			if ($1 eq 'q') {
				$_=&popstr;
				s/\\/\\\\/g;	# substitute \n
				s/\n/\\n/g;
				s/\"/\\\"/g;
				push (@sstack,'"'.$_.'"');
				last EXECOP;
			}

### Tail (t) operator ###
			if ($1 eq 't') {
				$full = &popstr;
				if (length($full) == 0) {
					print "\n\"I'm not done\nAnd I won't be till my head falls off\"\n";exit;
				}
				push (@sstack,substr($full,1));
				last EXECOP;
			}

### Execute (x) operator ###
			if ($1 eq 'x') {
				$prog=&popstr;
				$prog =~ s/\n//;	# don't allow newlines in the middle of strings
				@sstack=();
				%svars=();
				last EXECOP;
			}

### Unrecognised instruction ###
			print "\n\"It's hard to understand me from the language I use\nThere's no word in English for my style\"\n";exit;
		};
	} else {

### Parse quoted expression ###

		$prog=substr($prog,1);	# remove initial "
		$quote='';
		while (substr($prog,0,1) ne '"') {
			# Match everything up to the first escaped character or the terminating ", whichever is sooner - there must be one, or it's a syntax error.
			if ($prog !~ /^([^\\\"]*)([\\\"].*)$/ ) {
				print "\n\"I was just talking and someone interrupted\nOr was it a loud explosion?\"\n";exit;
			};
			$quote .= $1;
			$prog = $2;
			if ($prog =~ /^\\\\(.*)$/) { 		# match \\
				$quote .= "\\";
				$prog = $1;
			} elsif ($prog =~ /^\\\"(.*)$/) {	# match \"
				$quote .= '"';
				$prog = $1;
			} elsif ($prog =~ /^\\n(.*)$/) {	# match \n
				$quote .= "\n";
				$prog = $1;
			} elsif ($prog =~ /^\\(.*)$/) {		# match other occurrences of \
				$quote .= "\\";
				$prog = $1;
			};
		};
		$prog=substr($prog,1);	# remove final "
		push (@sstack,$quote);	# push quoted string onto stack
	}
}

### Attempt to pop a string from the stack ###

sub popstr {
	if (scalar (@sstack) == 0) {
		print "\n\"When the indicator says you're out of gas\nShould you continue driving anyway?\"\n";exit;
	} else {
		pop @sstack;
	}
}
