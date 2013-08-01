#!/usr/bin/perl -w

# Interpreter in Perl for the CHIQRSX9+ language.
# April 3-5, 2002 by Ørjan Johansen.
# You may use this program as you wish.

use 5.004; # Requires at least Perl 5.4.

sub input {
  @input = <STDIN> unless defined(@input);
  return @input;
}

if ($#ARGV > 0) { die "Usage: $0 <progname>\n"; }
if ($#ARGV) {
  $prog = join '', input;
}
else {
  open PROG, '<', $ARGV[0] or die "Could not open $ARGV[0]: $!";
  $prog = join '', <PROG>;
}

sub beer {
  for (my ($i,$s,$n)=($_[0],'s',''); $i ne 'No more'; ) {
    my $t=(my $j=$i-1||"No more")eq 1?'':'s';
    print <<VERSE; ($i,$s,$n)=($j,$t,"\n");
$n$i bottle$s of beer on the wall,
$i bottle$s of beer,
Take one down, pass it around,
$j bottle$t of beer on the wall.
VERSE
  }
}

sub turing {
  srand;
  my $lang = int rand 256;
  my $prog = '';
  for (split //, $_[0]) {
    $prog .= chr +(ord($_)+$lang & 255)
  }
  $_[0]='';
  return $prog;
}

sub interpret {
  my $accumulator;
  $rest = $_[0];
  while ($rest =~ s/^(.)//) {
    $_=$1;
    /[ \t\n]/ or
    /c/i ? print(input) :
    /h/i ? print("Hello, world!\n") :
    /i/i ? interpret(join '', input) :
    /q/i ? print($_[0]) :
    /r/i ? do {for (input) {tr/A-Za-z/N-ZA-Mn-za-m/; print;}} :
    /s/i ? print(sort &input()) :
    /x/i ? eval(turing($rest)) :
    /9/  ? beer(99) :
    /\+/  ? $accumulator++ :
    die <<UNKCMD;
Unknown command '$_'.  Legal commands are:
 C      Copies input to output.
 H      Prints "Hello, world!"
 I      Interprets input as program source.
 Q      Prints the program source code.
 R      Encrypts input with ROT-13.
 S      Sorts input lines.
 X      Makes the programming language Turing-complete.
 9      Prints the lyrics to "99 Bottles of Beer on the Wall."
 +      Increments the accumulator.
UNKCMD
  }
}

interpret $prog;
