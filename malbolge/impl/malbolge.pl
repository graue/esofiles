#! /usr/bin/perl

use strict;
use warnings 'all';

my	$xlat1 = '+b(29e*j1VMEKLyC})8&m#~W>qxdRp0wkrUo[D7,XTcA"lI.v%{gJh4G\\-=O@5`_3i<?Z\';FNQuY]szf$!BS/|t:Pn6^Ha';
my	$xlat2 = '5z]&gqtyfr$(we4{WP)H-Zn,[%\\3dL+Q;>U!pJS72FhOA1CB6v^=I_0/8|jsb9m<.TVac`uY*MK\'X~xDl}REokN:#?G"i@';
my	$size = 9**5;
my	@op = (
	[ 4, 3, 3, 1, 0, 0, 1, 0, 0 ],
	[ 4, 3, 5, 1, 0, 2, 1, 0, 2 ],
	[ 5, 5, 4, 2, 2, 1, 2, 2, 1 ],
	[ 4, 3, 3, 1, 0, 0, 7, 6, 6 ],
	[ 4, 3, 5, 1, 0, 2, 7, 6, 8 ],
	[ 5, 5, 4, 2, 2, 1, 8, 8, 7 ],
	[ 7, 6, 6, 7, 6, 6, 4, 3, 3 ],
	[ 7, 6, 8, 7, 6, 8, 4, 3, 5 ],
	[ 8, 8, 7, 8, 8, 7, 5, 5, 4 ],
);

my	@xlat1 = $xlat1 =~ /./g;
my	@xlat2 = $xlat2 =~ /./g;
my	@mem;

sub	op
{
	my	($x, $y) = @_;
	my	$i = 0;
	my	$j = 1;

	$i += $op[$y / $j % 9][$x / $j % 9] * $j, $j *= 9
		for 0 .. 4;
	$i;
}

sub	run
{
	my	$a = 0;
	my	$c = 0;
	my	$d = 0;

	while (1) {
		next	if $mem[$c] < 33 || $mem[$c] > 126;
		$_ = $xlat1[($mem[$c] - 33 + $c) % 94];
		# print "c=$c d=$d mem[c]=$mem[$c] mem[d]=$mem[$d] a=$a op=$_\n";
		/j/	? $d = $mem[$d]
		: /i/	? $c = $mem[$d]
		: /\*/	? $a = $mem[$d] = int ($mem[$d] / 3) + $mem[$d] % 3 * $size / 3
		: /p/	? $a = $mem[$d] = op ($a, $mem[$d])
		: /</	? print chr $a % 256
		: /\//	? $a = eof () ? $size - 1 : getc ()
		: /v/	? last
		: ();
		warn "attempt to exploit security hole\n"
			if $mem[$c] < 33 || $mem[$c] > @xlat2 + 32;
		$mem[$c] = ord $xlat2[$mem[$c] - 33];
		$c++; $c %= $size;
		$d++; $d %= $size;
	}
}

sub	load
{
	my	$i = 0;

	local $/ = \1;
	while (<>) {
		next	if /\s/;
		$_ = ord;
		die "invalid character in source file\n"
			if $_ > 32 && $_ < 127
			 && index ('ji*p</vo', $xlat1[($_ - 33 + $i) % 94]) < 0;
		die "input file too long\n"
			if $i == $size;
		$mem[$i++] = $_;
	}
	warn "input file is too short\n"
		if $i < 2;
	# print $xlat1[($mem[$_] - 33 + $_) % 94]	for 0..$#mem; exit;
	$mem[$i] = op ($mem[$i - 1], $mem[$i - 2]), $i++
		while $i < $size;
}

load;
run;
