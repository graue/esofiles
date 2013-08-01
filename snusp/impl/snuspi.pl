#!/usr/bin/perl

# A Modular SNUSP Interpreter
# Copyright (C) 2004  Rick Klement

use strict;

my ($dy, $p, $dir, $run, $code, @data, @stack, $op) = (1, 0, 1, 1, '', 0);
$code .= $_, $dy < 2 + length and $dy = 2 + length while <STDIN>;
$code =~ s/^.*/$& . ' ' x ($dy - 2 - length $&) . "\n"/gem;
my $ip = index $code, '$'; # find first $ or first char
$ip = 0 if $ip < 0;
my %instructions = (
  '>'  => sub { ++$p },                                             # RIGHT
  '<'  => sub { $run-- if --$p < 0 },                               # LEFT
  '+'  => sub { ++$data[$p] },                                      # INCR
  '-'  => sub { --$data[$p] },                                      # DECR
  ','  => sub { $data[$p] = ord shift @ARGV },                      # READ
  '.'  => sub { print chr $data[$p], "\n" },                        # WRITE
  '/'  => sub { $dir = abs $dir == 1 ? -$dy * $dir : $dir / -$dy},  # RULD
  '\\' => sub { $dir = abs $dir == 1 ? $dy * $dir : $dir / $dy},    # LURD
  '!'  => sub { $ip += $dir },                                      # SKIP
  '?'  => sub { $ip += $dir unless $data[$p] },                     # SKIPZ
  '@'  => sub { push @stack, [ $ip + $dir, $dir ] },                # ENTER
  '#'  => sub { @stack ? ($ip, $dir) = @{pop @stack} : $run-- },    # LEAVE
  "\n" => sub { $run-- });                                          # STOP

while($run and $ip >= 0 and $ip < length $code)
  {
  $op = $instructions{my $ch = substr $code, $ip, 1} and &$op;
  #print "op: $ch (@data)[$p]\n"; # uncomment for trace
  $ip += $dir;
  }
exit $data[$p];
