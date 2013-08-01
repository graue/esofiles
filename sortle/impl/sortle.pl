#!/usr/bin/perl -w

# Sortle interpreter - Implements the esoteric programming language "Sortle"
# Written in 2012 by Scott Feeney aka Graue
# Last modified Mar. 12, 2012 - Optimize regex matching for groups of just dots
#
# http://esolangs.org/wiki/Sortle
# http://esolangs.org/wiki/User:Graue
#
# To the extent possible under law, the author(s) have dedicated all copyright
# and related and neighboring rights to this software to the public domain
# worldwide. This software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain Dedication along
# with this software. If not, see:
# http://creativecommons.org/publicdomain/zero/1.0/
#
# If this implementation disagrees with something explicitly stated in the
# language spec, it is a bug in the implementation, not the spec. Please let
# me know of any such bugs you find! You can email me via my user page on the
# wiki (linked above). Thanks.

use strict;

#STILL TO DO:
# maybe use bignums for integer calculations and conversions(?)
# test some more
# clean up debug msgs

# $verbose can be:
#  0: no debug info - only print the final expression's name (if program halts)
#  1: show state after each evaluation - useful for non-halting programs
#  2: show state AND show progress of stack during evals
#  3: all that AND show progress of regex matches
#  1000: all that AND random debugging crap
my $verbose = 0;

# Leave strings untouched, while converting integers to strings,
# with the odd requirement that in Sortle, the integer 0
# becomes the null string.
#
# All input strings should be prefixed with '"'.
# Output string is also prefixed with '"'.
#
sub sortlestring($) {
	my $s = $_[0];

	return $s if substr($s, 0, 1) eq '"';
	return '"' if $s == 0;
	return "\"$s";
}

# Convert to integer without any annoying warning about the argument
# not being numeric (just use 0 then).
# If input is a string, and it is prefixed with ", this marker will be removed
# prior to conversion.
sub toint($) {
	my $s = "$_[0]";
	$s = substr($s, 1) if substr($s, 0, 1) eq '"';
	my $x;
	if (($x) = $s =~ /^([+-]?\d+)/) {
		$x = int $x;
	} else {
		$x = 0;
	}
	return $x;
}

# return 1 if both strings are equal, 0 otherwise,
# considering the character "." in $a to be equivalent to any character in $b.
sub stringequaldot($$) {
	my ($a, $b) = @_;
	return 0 if length $a != length $b;
	for (my $x = 0; $x < length $a; $x++) {
		my $ac = substr $a, $x, 1;
		next if $ac eq ".";
		my $bc = substr $b, $x, 1;
		return 0 if $ac ne $bc;
	}
	return 1;
}

sub matchregex($$;$);

# match a verified well-formed regex against a specific string
# (no substrings)
# return: string containing captures, or undef if no match
sub matchregex($$;$) {
	my ($regex, $s, $captureall) = @_;

	if ($verbose >= 3) {
		print "    ";
		# print - if non-recursive
		print (defined($captureall) ? " " : "-");

		print "matchregex \"$regex\" \"$s\" ";
		print "capt:$captureall" if defined $captureall;
		print "\n";
	}

	# $captureall is only set when this is called recursively.
	# if not set, turn it off if regex contains () and on otherwise.
	if (!defined $captureall) {
		$captureall = $regex !~ /\(/ || 0;
	}

	# a null string is matched only by an empty regex,
	# which captures the empty string
	if ($s eq "") {
		if ($regex eq "") { return ""; }
		else { return undef; }
	}

	# otherwise, if regex is empty it's a non-match
	return undef if $regex eq "";

	# separate out any non-grouped characters at beginning of regex
	# not followed by a modifier (@ or !)
	my ($reghead) = $regex =~ /^([^([@!]+)(?![@!])/;
	if (defined $reghead) {
		my $stringhead = substr $s, 0, length $reghead;
		print "     (found reghead: $reghead vs. $stringhead)\n" if $verbose >= 1000;
		return undef if !stringequaldot($reghead, $stringhead);
		print "     (heads match)\n" if $verbose >= 1000;

		# head matches, see if the rest does
		my $tailmatch = matchregex((substr $regex, length $reghead),
			(substr $s, length $reghead), $captureall);
		return undef if !defined $tailmatch;

		# so it did match and captured $tailmatch. If capturing all,
		# prepend our part-match to that, otherwise leave it alone
		return ($captureall ? $stringhead . $tailmatch : $tailmatch);
	}

	# at the beginning of the regex,
	# we have either a group, or a single character with modifier
	# (which we can treat as a [] group with modifier).
	# first, get the group.
	my ($group) = $regex =~ /^(\[[^[]*\])/;
	($group) = $regex =~ /^(\([^(]*\))/ if !defined $group;
	$group = "[" . substr($regex, 0, 1) . "]" if !defined $group;

	# now get its modifier, if any.

	my $mod;
	my $tailidx = length $group; # string offset in regex after group

	# if group was a single character, and we added [] around it,
	# account for that.
	print "     group is >>>$group<<<\n" if $verbose >= 1000;
	print "     length of group before adjusting is $tailidx\n" if $verbose >= 1000;
	$tailidx -= 2 if index("([", substr($regex, 0, 1)) == -1;

	print "     length of group is $tailidx and length of regex is "
		. length($regex) . "\n" if $verbose >= 1000;
	if ($tailidx == length $regex) { $mod = ""; }
	elsif (substr($regex, $tailidx, 1) eq "@") {
		$mod = "@";
		$tailidx++;
	} elsif (substr($regex, $tailidx, 1) eq "!") {
		$mod = "!";
		$tailidx++;
	} else { $mod = ""; }

	# min and max number of times the group must repeat
	my $minrepeat = (($mod eq "@") ? 0 : 1);
	my $maxrepeat = (($mod eq "!") ? -999 : 1);

	# content of group:
	# if $group is "(foo)" or "[foo]", $content is "foo"
	my $content = substr $group, 1, -1;

	# match group with fewest number of repetitions that allows
	# rest of string to match (minimal munch)
	my $tailmatch = undef; # set to partial capture when tail matched

	my $matchcount; # num chars matched in the string
	if ($content =~ /^\.+$/ && $maxrepeat < 0) {
		# optimize: group is entirely dots, like the common (.)! pattern
		# with no maximum repetition count
		my $dotstep = length $content; # step size, e.g. (..)! can only
		                               # match an even number of chars
		my $dots;
		for ($dots = $minrepeat*$dotstep; $dots <= length $s; $dots += $dotstep) {

			# guaranteed the dots match... does the tail match?
			$tailmatch = matchregex(substr($regex, $tailidx),
				substr($s, $dots), $captureall);
			last if defined $tailmatch;
		}
		$matchcount = $dots if defined $tailmatch;
	} else {
		# general case
		my ($acc, $reps);
		for ($acc = "", $reps = 0;
		  $reps-1 != $maxrepeat;
		  $acc .= $content, $reps += 1) {

			print "     ACC is >>>$acc<<< REPS is $reps NEED [$minrepeat, $maxrepeat]\n" if $verbose >= 1000;
			# do we have enough repetitions yet?
			next if $reps < $minrepeat; # nope, build up $acc first

			# does the repeated group match?
			last if !stringequaldot($acc, substr($s, 0, length $acc));

			# does the tail match?
			$tailmatch = matchregex(substr($regex, $tailidx),
				substr($s, length $acc), $captureall);
			last if defined $tailmatch;
		}
		$matchcount = length $acc if defined $tailmatch;
	}

	# if tail not matched for any allowed number of repetitions,
	# then the whole regex doesn't match
	return undef if !defined $tailmatch;

	# 1. If capturing all, return what we matched here, plus the tail.
	if ($captureall) {
		return substr($s, 0, $matchcount) . $tailmatch;
	}
	# 2. Otherwise, if this group was (), return what we matched here.
	elsif (substr($group, 0, 1) eq "(") {
		return substr($s, 0, $matchcount);
	}
	# 3. Group [] and not capturing all, so return (possibly empty) tail
	#    only.
	else { return $tailmatch; }
}

# evaluate a regex (against $s OR all other expression names)
# and return string result: capture if matched, "" otherwise
# if () are not used in regex, the capture is the whole string
sub evalregex($$$$) {
	my ($regex, $s, $ip, $exprref) = @_;

	# check regex for nested groups
	die "Nested groups not allowed in regex \"$regex\""
		if $regex =~ /\([^)]*[([]/
		|| $regex =~ /\[[^\]]*[([]/;

	# or unclosed []s
	my $opencount = 0;
	$opencount++ while $regex =~ /\[/g;
	$opencount-- while $regex =~ /\]/g;
	die "Unclosed [] group in regex \"$regex\"" if $opencount != 0;

	# or unclosed or multiple ()s
	$opencount = 0;
	$opencount++ while $regex =~ /\(/g;
	die "Multiple () groups not allowed in regex \"$regex\""
		if $opencount > 1;
	$opencount-- while $regex =~ /\)/g;
	die "Unclosed () group in regex \"$regex\"" if $opencount != 0;

	if ($s ne "") {
		# Test every substring of $s, starting with one-byte
		# substrings from left to right.
		my $match = undef; # set to captured string when matched
		for (my $len = 1; $len <= length $s && !defined $match;
		  $len++) {
			for (my $start = 0; length $s - $len >= $start
			  && !defined $match; $start++) {
				$match = matchregex($regex,
					(substr $s, $start, $len));
			}
		}
		return $match || "";
	}

	# op1 is "", so search all expression names other than the current
	# expression ($ip).
	# The .pdf spec doesn't specify an order, but the earlier sortle.txt
	# spec says expression names are searched in reverse order, starting
	# with the one before the current expression.
	# So we'll do that. To start, find the index of the current ip.
	my @expnames = reverse sort keys %{ $exprref };
	my $idx;
	for ($idx = 0; $expnames[$idx] ne $ip; $idx++) {
		die "Internal error" if $idx == $#expnames;
	}

	$idx = ($idx + 1) % (scalar @expnames);
	my $match = undef; # set to captured string when matched
	while ($expnames[$idx] ne $ip && !defined $match) {
		$match = matchregex($regex, $expnames[$idx]);
		$idx = ($idx + 1) % (scalar @expnames);
	}
	return $match || "";
}

# evaluate an expression (array reference)
# and return string result. 2nd arg - ip; 3rd - reference to exprs hash
sub evalexpr($$$) {
	my @expr = @{ $_[0] };
	my $ip = $_[1];
	my $exprref = $_[2];
	my @stack = ();

	foreach my $tok (@expr) {
		if ($verbose >= 2) {
			# print stack and token
			print "  -stack:";
			foreach my $elmt (@stack) {
				print " [$elmt]";
			}
			print "\n";
			print "   token: $tok\n";
		}

		# literal string
		# Note: This preserves the " at the beginning (not end)
		# when pushing on stack. This is done because there is
		# seemingly no way to distinguish string "0" from number 0
		# in Perl (it's too eager to convert one to the other).
		# We need this because in Sortle, the number 0 coerced to a
		# string becomes not "0" but "".
		# Thus, on the stack, we would store "0" as ["0]
		# and 0 as [0], without the [].
		# toint() will remove the leading " if needed
		# (but does not require it), and sortlestring() adds the "
		# if not present.
		if (my ($s) = $tok =~ /^(".*)"$/) {

			# handle escape sequences: \ab => character 0xab
			$s =~ s/\\([[:xdigit:]]{2})/chr hex $1/ge;

			# (To be pedantic, we could check for uses of \
			#  outside a valid escape sequence, and report an
			#  error - but I'm too lazy, so that can just be
			#  considered undefined behavior.)

			push @stack, $s;
			next;
		}

		# literal number
		if ($tok =~ /^[+-]?\d+$/) {
			push @stack, toint($tok);
			next;
		}

		# must be an operator... all of which take 2 operands
		die "Stack empty" if 1+$#stack < 2;
		my $op1 = pop @stack;
		my $op2 = pop @stack;

		if (index("+*/%", $tok) != -1) {
			# operator requires and produces numbers
			$op1 = toint($op1);
			$op2 = toint($op2);

			push(@stack, $op1 + $op2) if $tok eq "+";
			push(@stack, $op1 * $op2) if $tok eq "*";
			push(@stack, int($op1 / $op2)) if $tok eq "/";
			push(@stack, $op1 % $op2) if $tok eq "%";
		} else {
			# operator requires and produces strings
			$op1 = substr sortlestring($op1), 1;
			$op2 = substr sortlestring($op2), 1;

			push(@stack, '"' . $op2 . $op1) if $tok eq "~";

			if ($tok eq "^" || $tok eq "\$") {
				if ($op1 gt $op2) { push(@stack, '"' . $op1); }
				else { push(@stack, '"' . $op2); }
			}

			push(@stack, '"' . evalregex($op2, $op1, $ip, $exprref))
				if $tok eq "?";
		}
	}

	# Need exactly one expression on stack
	if (scalar @stack != 1) {
		die "Expression left " . (scalar @stack) . " values on stack";
	}
	print "  -final stack: [$stack[0]]\n\n" if $verbose >= 2;

	# Convert the one expression to a string, if needed
	my $final = $stack[0];
	$final = sortlestring($final) if $final !~ /^"/;

	# Remove the " prefix when returning the string
	return substr($final, 1);
}

# print current state
# usage: printstate($ip, \%exprs)
sub printstate($$) {
	my $ip = $_[0];
	my %exprs = %{$_[1]};
	foreach my $key (sort keys %exprs) {
		print "*" if $ip eq $key; # expression to evaluate next
		print "$key :=";
		foreach my $x (@{ $exprs{$key} }) {
			print " $x";
		}
		print "\n";
	}
}

# run one step of the program
# usage: advancestate(\$ip, \%exprs)
sub advancestate($$) {
	my $ipref = $_[0];
	my $oldname = $$ipref; # needed in case the expression suicides
	my $exprref = $_[1];
	my $newname = evalexpr([@{ $exprref->{$$ipref} }], $$ipref,
		$_[1]);

	# store expression under new name, unless new name is blank (suicide)
	$exprref->{$newname} = $exprref->{$$ipref}
		unless $newname eq "";

	# delete old name, unless it's the same as the new name
	delete $exprref->{$$ipref}
		unless $$ipref eq $newname;

	# find next expression in sorted order, wrapping, after $newname.
	# exception: if $newname is blank (expression has committed suicide),
	# we want the one after where the suicided expression WAS.
	my $isnext = 0;
	undef $$ipref;
	foreach my $x (sort keys %$exprref) {
		if ($newname eq "" && $x gt $oldname) {
			# expression suicided, but would have been before
			# this one.
			$$ipref = $x;
			last;
		}
		if ($x eq $newname) {
			$isnext = 1;
			next;
		}
		if ($isnext == 1) {
			$$ipref = $x;
			$isnext = 0;
			last;
		}
	}
	if (!defined $$ipref) { # means we need to wrap around
		$$ipref = (sort keys %$exprref)[0];
	}
}



# Top-level interpreter

my %exprs = (); # code expressions

# for concatenating multiple lines when a line ends with a \
my $partialline;

# read source code from stdin or named input files
while (my $line = <>) {
	chomp $line;
	if (defined $partialline) {
		$line = $partialline . " " . $line;
		undef $partialline;
	}
	next if $line =~ /^\s*#/ || $line =~ /^\s*$/; # skip blank lines

	# if the line ends with a \, but the \ is NOT within a comment,
	# delete the \ and prepend this line to what's on the next line
	# ($tmp stores $line with quoted strings removed, to test if a
	#  comment is present)
	my $tmp = $line;
	$tmp =~ s/"[^"]*"//g; # remove closed quoted strings from $tmp
	$tmp =~ s/"[^"]*$//; # remove an unclosed string, if present
	if ($tmp !~ /#/ && $line =~ /\\$/) { # no comment, but a \ on end
		$partialline = substr $line, 0, -1;
		next;
		# rest of line will be read, and line parsed, on next loop
	}

	# split expression name and value
	my ($name, $contents) = $line =~
		/^\s*([A-Za-z][A-Za-z0-9]*)\s*:=(.+)$/;
	die "Sortle syntax error" if !defined $name || !defined $contents;
	$contents =~ s/^(\s*)//;
	$contents =~ s/(\s*)$//;

	# split tokens at whitespace (except inside quoted strings)
	# Note: this code will allow strings to contain escaped double-quotes
	#       e.g "this is a \"string\"", which technically isn't how Sortle
	#       works. But it is undefined behavior because backslashes are
	#       only supposed to be used in Sortle strings when followed by
	#       two hex digits, e.g. \0a for newline. Backslashes should be
	#       escaped in the same manner, i.e. "this is a \5c\22string\22\5c"
	my @tokens = $contents =~ m/\s* ("(?:(?!(?<!\\)").)*" | '(?:(?!(?<!\\)').)*' | \S+)/gx;
	die "Empty expression" if (scalar @tokens) <= 0;

	# check that tokens are valid, and remove comments
	my @terms = ();
	foreach my $tok (@tokens) {
		last if $tok =~ /^#/; # comment: ignore further tokens on line
		push @terms, $tok;

		next if $tok =~ /^[+-]?\d+$/; # number
		next if $tok =~ /^".*"$/; # string
		next if length $tok == 1
			&& index("+*/%^~?\$", $tok) != -1; # operator
		die "Invalid token: $tok";
	}
	@{ $exprs{$name} } = @terms;
}
die "Last line cannot end with \\" if defined $partialline;
die "No expressions defined" if scalar keys %exprs == 0;

my $ip = (sort keys %exprs)[0];

my $evalcount = 0;
while (scalar keys %exprs > 1) {
	if ($verbose >= 1) {
		printstate($ip, \%exprs);
		print "$evalcount expressions evaluated\n\n";
	}

	advancestate(\$ip, \%exprs);
	++$evalcount;
}

if ($verbose >= 1) {
	printstate($ip, \%exprs);
	print "$evalcount expressions evaluated\n\n";
}

print((keys %exprs)[0] . "\n");
