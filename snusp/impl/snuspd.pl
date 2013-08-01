#!/usr/bin/perl

# A Modular SNUSP Debugger
# Copyright (C) 2004  Rick Klement

use Curses;
use Term::ReadKey;
use strict; # animate.pl - show calculation using Curses
# allow backing up out of STOP

my @restart = ( $^X, $0, @ARGV );
my $filename = shift;
open IN, $filename or die "$! opening $filename";
my $input = do{ local $/; <IN> };
close IN;

my ($dy, $dir, $p, @data, @stack, $op, $code, $ch) = (1, 1, 0, 0);
$code .= $_, $dy < length and $dy = length for $input =~ /^.*\n/gm;
$code =~ s/^.*/$& . ' ' x ($dy - length $&) . "\n"/gem;
$dy += 2;
my %lurd = (-1, -$dy, -$dy, -1, 1, $dy, $dy, 1);
my $ip = $code =~ /\$/ * $-[0]; # find first $ or first char
my @out = ();
my %instructions = (
  '>'  => sub { $data[++$p] += 0 },                                  # RIGHT
  '<'  => sub { --$p >= 0 or $dir = 0 },                             # LEFT
  '+'  => sub { ++$data[$p] },                                       # INCR
  '-'  => sub { --$data[$p] },                                       # DECR
  ','  => sub { $data[$p] = ord shift @ARGV },                       # READ
  '.'  => sub { push @out, chr $data[$p] },                          # WRITE
  '/'  => sub { $dir = -$lurd{$dir} },                               # RULD
  '\\' => sub { $dir =  $lurd{$dir} },                               # LURD
  '!'  => sub { $ip += $dir },                                       # SKIP
  '?'  => sub { $ip += $dir if $data[$p] == 0 },                     # SKIPZ
  '@'  => sub { push @stack, [ $ip + $dir, $dir ] },                 # ENTER
  '#'  => sub { @stack ? ($ip, $dir) = @{pop @stack} : ($dir = 0) }, # LEAVE
  "\n" => sub { $dir = 0 });                                         # STOP

initscr();
ReadMode 3;

my $y = 0;
addstr($y++, 0, $&) while $code =~ /.+/g;
addstr(++$y + 2, 0, "(space)togglepause (g)oto n (Enter)step (BS)backstep");
addstr($y + 3, 0, "(r)estart (q)uit (+)fast (-)slow");
my $count = 0;
my @history;
my $key;
my $sleep = 0.1;
my $pause = 0;
my $number = 0;

while(1)
  {
  if($ip < 0) {$ip = 0; $dir = 0}
  if($ip >= length $code) {$ip = length($code) - 1; $dir = 0}
  $pause = 1 if $dir == 0;
  if($dir and (not $pause or $key eq "\n"))
    {
    $pause = 1 if $number and $count == $number - 1 or $key eq "\n";
    $op = $instructions{$ch = substr $code, $ip, 1} and &$op;
    $ip += $dir;
    $history[$count++] ||= [$ip, $dir, $p, [@data], [@stack], [@ARGV], [@out] ];
    }
  my $n = 0;
  my $brace = join '', map { $n++ == $p ? "[$_]" : " $_ " } @data;
  my $s = "data: $brace   out: @out  t: $count  n: $number";
  addstr($y, 0, $s);
  clrtoeol();
  move( int($ip / $dy), $ip % $dy);
  refresh;
  $key = ReadKey($pause ? 0 : $sleep);
  if($key eq 'q' or $key eq 'r') {last}
  elsif($key eq '+') {$sleep = 0.01}
  elsif($key eq '-'){$sleep = 0.1}
  elsif($key eq "\e"){$number = 0}
  elsif($key =~ /\d/){$number = 10 * $number + $key}
  elsif($key eq ' '){$pause = not $pause}
  elsif($key eq 'g' || $key eq "\x08" and $number < @history)
    {
    $count = $key eq 'g' ? $number : $count - 2;
    ($ip, $dir, $p, my $data, my $stack, my $argv, my $out) =
      @{$history[$count++]};
    @data = @$data;
    @stack = @$stack;
    @ARGV = @$argv;
    @out = @$out;
    }
  }

ReadMode 0;
endwin();
exec @restart if $key eq 'r';
