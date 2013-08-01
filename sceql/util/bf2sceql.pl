#!/usr/bin/perl -w
use strict;

# Convert a brainfuck program to Sceql.
# http://esolangs.org/wiki/Sceql
# Written by Scott Feeney/Graue in 2012. Public domain. No warranty.

# Brainfuck dialect is 8-bit wrapping, input 0 on EOF,
# newline behavior not specified (depends on Sceql implementation).
# Tape is unbounded to the right but going left past the initial cell causes
# undefined behavior.

# This works by converting first to the intermediate language 'sceqlfuck',
# created by Keymaker in the process of proving Sceql's Turing-completeness.
# Thanks to Keymaker for that language (and the translations from sceqlfuck
# into Sceql).

# To convert brainfuck to sceqlfuck:
# Treating the two stacks as an array is easy, but whenever we move to the
# right we have to explicitly ask for more space as needed.
# So we model the tape as:
#  [c1 1 c2 1 c3 1 c4 ... 1] [ci ... 1 cn 0]
#                             ^^
# where cX is the value at cell X, cell i is active, n cells have been
# allocated, and each 1 value signals that there is another allocated cell
# to the right.
# The first [] is stack two with topmost value on the right.
# The second [] is stack one with topmost value on the left, and is always
# active when evaluating a bf command.

# So, to initialize, we want things looking like
#  [] [c1 0]
#      ^^    where c1 = 0.
# This is simple, just push two zero values onto stack one.

my %bf2sqf = (
	'init' => '**', # see above
	'+' => '+',
	'-' => '-',
	'[' => '[',
	']' => ']',

	# To move left, move a 1 from stack two to stack one,
	# then move a cell value from stack two to stack one.
	'<' => '*+*=![-=+=]!=',

	# To move right, move a cell value from stack one to stack two,
	# then if top of stack one is 0 (and not 1), push a 0 and 1,
	# then move a 1 from stack one to stack two.
	'>' => '=*=[-=+=]!-[+**]!=*+=',

	',' => '!,', # Replace cell value by popping, then pushing input.
	'.' => '.'
);

my %sqf2sceql = (
	'init' => '!!!!!!!!!_==_===_==_=',
	'+' => '_',
	'-' => '-',
	'*' => '=\==/=\=/=\==/=\=/=!!\==/=\=/=\==/=\=/=_=',
	'!' => '=\==/=\=/=\==/=\=/_==\-/==',
	'=' => '=\==/=\=/==',
	'[' => "\\",
	']' => '/',
	',' => '&!=\==/=\=/=\==/=\=/===_\==/=\=/=\==/=\=/==',
	'.' => '*\==/=\=/=\==/=\=/=='
);

my $bfcode = join '', <>;

my $sqfcode = $bf2sqf{'init'};
foreach my $c (split(//, $bfcode)) {
	$sqfcode .= $bf2sqf{$c} if defined $bf2sqf{$c};
}

my $sceqlcode = $sqf2sceql{'init'};
foreach my $c (split(//, $sqfcode)) {
	$sceqlcode .= $sqf2sceql{$c} if defined $sqf2sceql{$c};
}

print "$sceqlcode\n";
