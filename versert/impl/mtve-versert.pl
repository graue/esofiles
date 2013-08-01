#! /usr/bin/perl -w

# from http://www.frox25.no-ip.org/~mtve/wiki/Versert.html

use strict;

open my ($f), $ARGV[0] or die "usage: $0 file\n";
my $txt = do { local $/; <$f> };
my @arr = map [/./g], $txt =~ /.*\n/g;

my ($x, $y, $dx, $dy, $px, $py, $a, $b, $t) = (0, 0, 1, 0, 0, 0, 0, 0);
$|++;

while (1) {
	$_ = $arr[$y][$x];
#	print "x=$x y=$y dx=$dx dy=$dy px=$px py=$py a=$a b=$b doing $_\n";
	die if !defined;
	/\d/	? $a = $_ :
	/\+/	? $b += $a :
	/-/	? $b -= $a :
	/\*/	? $b *= $a :
	/~/	? ($a, $b) = ($b, $a) :
	/>/	? $a < $b && (($a, $b) = ($b, $a)) :
	/</	? $a > $b && (($a, $b) = ($b, $a)) :
	/\//	? ($dx, $dy) = (-$dy, -$dx) :
	/\\/	? ($dx, $dy) = ($dy, $dx) :
	/\@/	? exit :
	/#/	? $b == 0 && ($x += $dx, $y += $dy) :
	/\./	? print chr $a :
	/:/	? print $a :
	/,/	? defined ($t = getc) && ($a = ord $t) :
	/;/	? $a = <> :
	/\{/	? $b = ord $arr[$py][$px] :
	/\|/	? ($px += $a, $py += $b) :
	/\}/	? $arr[$py][$px] = chr $b :
	1;
	$x += $dx;
	$y += $dy;
	$px = 0 if $px < 0;
	$py = 0 if $py < 0;
}
