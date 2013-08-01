#!/usr/local/bin/perl -w

# argh.pl - argh debugger
#
# (C) 2004 Laurent Vogel - covered by the GPL version 2 or later 
# version 1.0   2004-06-26
#
# notes:
# - undefined cell value is 32 (ASCII space)
# - this apparently works on the existing examples
# - It does not check right and left bounds currently

use strict;

my $inputleft = '';
my $inputbatch = 0;    # one if reading from file

my $final = 0;
my $debug = 0;     # one if use the debugger
my $ansiesc = 0;   # one if use terminal escape sequences


my $bold = "\033[7m";
my $norm = "\033[m";

sub err($)
{
  my($a)=@_;
  $a =~ s/\n$//;
  print STDERR "error: $a\n";
  exit(1);
}

sub usage($) {
  my($fatal)=@_;
  print <<EOF;
usage: argh [ <options> ] <file>
options are:
  -d   debug
  -a   use ANSI terminal escape sequences
  -f <file>   read input from this file
EOF
  exit(1) if($fatal);
}

# read arguments

my $fname = '';
while(0 < scalar @ARGV) {
  my $a = shift @ARGV;
  if ($a =~ s/^-//) {
    while ($a ne '') {
      $a =~ s/(.)//;
      my $c = $1;
      if ($c eq 'd') {
        $debug++;
      } elsif ($c eq 'a') {
        $ansiesc = 1;
      } elsif ($c eq 'f') {
        my $f = shift @ARGV;
        usage(1) if (!defined $f);
        open(FILE,"<$f") or die "cannot open $f: $!\n";
        while(<FILE>) {
          $inputleft .= $_;
        }
        $inputbatch = 1;
        close(FILE);
      } else {
        usage(1);
      }
    }
  } else {
    usage(1) if ($fname ne '');
    $fname = $a;
  }
}
usage(1) if ($fname eq '');

# the hash of break points. A location is a number a;
# x = a mod 80; y = a / 80.
my $break = {};
  
my @code = ();
my @stack = ();
my $csz;
my $ssz = 0;
# position in code
my $loc = 0;
my $dir = 0;

my $op = '';


sub readcode($)
{
  my($fname)=@_;

  open(CODE, $fname) or err("cannot open $fname");
  my $p = 0;
  
  while (<CODE>) {
    s/[\012\015]+//g;
    my $n = length $_;
    my $i;
    for ($i = 0; $i < $n; $i++) {
      $code[$p++] = ord(substr($_, $i, 1));
    }
    for (; $i < 80; $i++) {
      $code[$p++] = 32;
    }
    $csz = $p;
  }
  close(CODE);
}

readcode($fname);

my $outseq = { 
  ord("'") => "'", 
  ord("\\") => "\\",
  ord("\n") => "n",
  ord("\t") => "t",
  ord("\r") => "r"
};

sub out($)
{
  my($a)=@_;
  if ($debug) {
    print "Output: \"";
    if(exists $outseq->{$a}) {
      print "\\$outseq->{$a}";
    } elsif($a >= 32 && $a <= 126) { 
      printf("%c",$a); 
    } else { 
      printf("\\%03o", $a);
    }
    print "\"\n";
  } else {
    printf("%c", $a);
  }
}

my $ateof = 0;

my $eof = -1;

sub in()
{
  return -1 if ($ateof);
  if($inputleft eq '') {
    if($inputbatch) {
      $ateof = 1;
      return $eof; 
    } else {
      if($debug) {
        print "Input>";
      }
      $inputleft = <>;
      if(!defined $inputleft) {
        $ateof = 1;
        return $eof; 
      }
  #    $inputleft =~ s/\\n/\n/;
  #    $inputleft =~ s/\\t/\t/;
  #    $inputleft =~ s/\\([0-7][0-7]?[0-7]?)/chr(oct($1))/ge;
  #    $inputleft =~ s/\\x([0-9a-fA-F][0-9a-fA-F])/chr(hex($1))/ge;
    }
  }
  my $c = substr($inputleft, 0, 1);
  $inputleft = substr($inputleft, 1);
#  print STDERR "got input '$c' (" . ord($c) . ")\n"; 
  return ord($c);
}

sub dispcode()
{
  my $i;
  my $j;
  my $y = int($loc / 80) ;
  
  my $x = $loc % 80;
  my $a = "      ($x,$y): "; 
  $a =~ s/.*(.......: )/$1/;
  for ($i = $ssz-1; $i >= 0; $i--) {
    my $c = $stack[$i];
    if ($c < 32 || $c > 126) {
      $a .= "$c ";
    } else {
      $a .= sprintf("%c ", $c);
    }
  }
  $a .= "\n";
  
  for($j = $y-2; $j < $y+2; $j++) {
    next if ($j <0 || $j*80 >= $csz);
    for ($i = 0; $i < 79; $i++) {
      if ($i == $x && $j == $y && $ansiesc) {
        $a .= $bold;
      }
      my $c = $code[$j*80+$i];
      if ($c < 32 || $c > 126) {
        $c = '.';
      } else {
        $c = sprintf("%c", $c);
      }
      $a .= $c;
      if ($i == $x && $j == $y && $ansiesc) {
        $a .= $norm;
      }
    }
    $a .= "\n";
    if ($j == $y && !$ansiesc) {
      $a .= " " x $x . "^\n";
    }
  }
  print $a;
}
  

my $done = 0;
my $stackchanged = 0;

sub init()
{
  $loc = 0;
  $dir = 0;
}

my $dirs = {
  'h' => -1,
  'j' => 80,
  'k' => -80,
  'l' => 1
};

my $turn = {
  -1 => -80, -80 => 1, 1 => 80, 80 => -1
};

sub write_above($)
{
  my($a)=@_;
  if ($loc-80 <0) { $done = 1; return; } 
  $code[$loc-80] = $a;
}

sub write_below($)
{
  my($a)=@_;
  if ($loc+80 > $csz) { 
    my $i; for($i = 0; $i < 80; $i++) { $code[$csz++] = 32; }
  }
  $code[$loc+80] = $a;
}

sub read_above()
{
  if ($loc-80 <0) { $done = 1; return; } 
  return $code[$loc-80];
}

sub read_below()
{
  if ($loc+80 > $csz) { 
    my $i; for($i = 0; $i < 80; $i++) { $code[$csz++] = 0; }
  }
  return $code[$loc+80];
}

sub step()
{
  if ($csz == 0) { $done = 1; return; }
  
  my $inst = $code[$loc];
  
  if ($inst <= 32 || $inst >= 126) {
    $done = 1; return;
  }
  $inst = sprintf("%c", $inst);
  
  if ($inst =~ /[hjkl]/) { 
    $dir = $dirs->{$inst};
  } elsif ($inst =~ /[HJKL]/) {
    $inst =~ y/HJLK/hjlk/;
    $dir = $dirs->{$inst};
    if ($ssz <= 0) {
      $done = 1; return;
    }
    do {
        $loc += $dir;
    } while ($loc >= 0 && $loc < $csz 
	     && $code[$loc] != $stack[$ssz-1]);
    if ($loc < 0 || $loc > $csz) {
      $done = 1; return;
    }
  } elsif ($inst eq 'x') { 
    if ($ssz <= 0) {
      $done = 1; return;
    }
    if ($stack[$ssz-1] > 0) { 
      $dir = $turn->{$dir}; 
    }
  } elsif ($inst eq 'X') { 
    if ($ssz <= 0) {
      $done = 1; return;
    }
    if ($stack[$ssz-1] < 0) { 
      $dir = $turn->{$turn->{$turn->{$dir}}}; 
    }
  } elsif ($inst eq 'q') { 
    $done = 1; return;
  } elsif ($inst eq 's') { 
    $stack[$ssz++] = read_below();
  } elsif ($inst eq 'S') { 
    $stack[$ssz++] = read_above();
  } elsif ($inst eq 'd') { 
    $stack[$ssz] = $stack[$ssz-1];
    $ssz++;
  } elsif ($inst eq 'D') { 
    if($ssz <= 0) { $done = 1; return; }
    $ssz--;
  } elsif ($inst eq 'a') { 
    if($ssz <= 0) { $done = 1; return; }
    $stack[$ssz-1] += read_below();
  } elsif ($inst eq 'A') { 
    if($ssz <= 0) { $done = 1; return; }
    $stack[$ssz-1] += read_above();
  } elsif ($inst eq 'r') { 
    if($ssz <= 0) { $done = 1; return; }
    $stack[$ssz-1] -= read_below();
  } elsif ($inst eq 'R') { 
    if($ssz <= 0) { $done = 1; return; }
    $stack[$ssz-1] -= read_above();
  } elsif ($inst eq 'f') { 
    if($ssz <= 0) { $done = 1; return; }
    write_below($stack[--$ssz]);
  } elsif ($inst eq 'F') {
    if($ssz <= 0) { $done = 1; return; }
    write_above($stack[--$ssz]);
  } elsif ($inst eq 'p') { 
    out(read_below());
  } elsif ($inst eq 'P') { 
    out(read_above());
  } elsif ($inst eq 'g') { 
    write_below(in());
  } elsif ($inst eq 'G') { 
    write_above(in());
  } elsif ($inst eq 'e') { 
    write_below($eof);
  } elsif ($inst eq 'E') { 
    write_above($eof);
  } else {
    $done = 1; return;
  }
  if ($loc + $dir > $csz) { write_below(0); }
  $loc += $dir;
}
    


if($debug) {
  my $lastk = 's';   # default command is step
  init();
  while(!$done) {
    dispcode();
    my $arg = '';
    my $k = <>;
    if(!defined $k) {
      print "exit\n";
      exit(0);
    } 
    $k =~ s/[\012\015]//g;
    if($k =~ s/^([0-9]+)//) {
      $arg = $1;
    } 
    if($k =~ s/^,([0-9]+)//) {
      $arg = $arg + 80 * $1;
    } 
    if($k eq '') {
      $k = $lastk;
    }
    if($k eq 's') {
      my $count = 1;
      $count = $arg if($arg ne '') ;
      while(--$count >= 0 && !$done) {
        step();
      }
    } elsif($k eq 'n') {
      my $count = 1;
      $count = $arg if($arg ne '') ;
      while(--$count >= 0) {
        my $lastloc = $loc;
        while($loc == $lastloc && !$done) {
          step();
        }
      }
    } elsif($k eq 'g') {
      if($arg ne '') {
        while($loc != $arg && !$done) {
          step();
        }
      }
    } elsif($k eq 'S') {
      $stackchanged = 0;
      while(!$stackchanged && !$done) {
        step();
      }
    } elsif($k eq 'q') {
      exit(0);
    } elsif($k eq 'b') {
      $arg = $loc if($arg eq '');
      $break->{$arg} = 1;
    } elsif($k eq 'B') {
      $arg = $loc if($arg eq '');
      delete $break->{$arg};
    } elsif($k eq 'c') {
      step();
      while(! exists $break->{$loc} && !$done) {
        step();
      }
    } else {
      print <<EOF;
debugger commands: [ <arg> ] <command>
<arg> is an optional decimal number used as argument
<commands> are  (behaviour in bracket when <arg> present):
  s   step  (step <arg> times)
  n   step until the line number changes (do this <arg> times)
  g   (step until reaching line number <arg>)
  S   step until the stack changes
  c   continue
  b   sets a breakpoint on current line  (on line number <arg>)
  B   deletes breakpoint on current line  (on line number <arg>)
  q   quit
if <command> is empty the previous command is executed (with current
<arg> if any). 

EOF
    }
    $lastk = $k;
  }
} else {
  init();
  while(!$done) {
    step();
  }
}
