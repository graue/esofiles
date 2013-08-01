#!/usr/local/bin/perl -w

# strelnokoff.pl - Cat's Eye Technologies' Strelnokoff Interpreter
# v2001.03.24 Chris Pressey, Cat's Eye Technologies

# Copyright (c)2001, Cat's Eye Technologies.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#   Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 
#   Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
# 
#   Neither the name of Cat's Eye Technologies nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission. 
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE. 

# USAGE: [perl] strelnokoff[.pl] program.skf

### BEGIN strelnokoff.pl ###

### SCANNER ###

$program = '';
$token = '';
sub scan
{
  if ($program =~ /^\s+/)
  {
    $program = $';
    goto &scan;
  }
  if ($program =~ /^REM.*?\n/)
  {
    $program = $';
    goto &scan;
  }
  if ($program =~ /^(\d+)/)
  {
    $token = $1;
    $program = $';
  }
  elsif ($program =~ /^([A-Za-z_][A-Za-z0-9_]*)/)
  {
    $token = $1;
    $program = $';
  }
  elsif ($program =~ /^(\'.\')/)
  {
    $token = $1;
    $program = $';
  }
  elsif ($program =~ /^(.)/)
  {
    $token = $1;
    $program = $';
  }
  else
  {
    # end of program
    $token = '';
    $program = '';
  }
  # print "Scanned: $token\n";
}

sub expect
{
  my $expected = shift;
  if ($token eq $expected)
  {
    scan();
  } else
  {
    error("Expected '$expected' not '$token'");
  }
}

sub error
{
  my $msg = shift;
  print STDERR "*** ERROR: strelnokoff: $msg\n";
}

### SYMBOL TABLE ###

%sym = ();

### PARSER ###

# Strelnokoff = {Assignment}.
# Assignment  = Variable [Index] "=" Expression0.
# Expression0 = Expression1 {"=" Expression1 | ">" Expression1}.
# Expression1 = Expression2 {"+" Expression2 | "-" Expression2}.
# Expression2 = Primitive   {"*" Primitive   | "/" Primitive}.
# Primitive   = ["PRINT" | "INPUT"] ["CHAR"] Variable [Index]
#             | IntegerLiteral | CharLiteral
#             | "(" Expression0 ")".
# Index       = "[" Expression0 {"," Expression0} "]".

# Program      ::= {Assignment}.
sub program
{
  my @p = ();
  while($token ne '')
  {
    my $x = assignment();
    push @p, $x;
    # print join(', ', @$x);
  }
  # print "end program on $token\n";
  return \@p;
}

# Assignment   ::= Variable [Index] "=" Expression0.

sub assignment
{
  my $varname = $token;
  scan();
  if ($token eq '[')
  {
    varindex();
  }
  expect('=');
  return [':=', $varname, expression0()];
  # print "$varname = $sym{$varname}\n";
}

# Expression0 = Expression1 {"=" Expression1 | ">" Expression1}.

sub expression0
{
  my $q = expression1();
  while($token eq "=" or $token eq ">")
  {
    my $t = $token;
    scan();
    my $r = expression1();
    my $b = $q;
    if ($t eq '=') { $q = ['=', $q, $r]; }
    if ($t eq '>') { $q = ['>', $q, $r]; }
    # print "compare: $b $t $r -> $q\n";
  }
  return $q;
}

# Expression1 = Expression2 {"+" Expression2 | "-" Expression2}.

sub expression1
{
  my $q = expression2();
  while($token eq "+" or $token eq "-")
  {
    my $t = $token;
    scan();
    my $r = expression2();
    if ($t eq '+') { $q = ['+', $q, $r]; }
    if ($t eq '-') { $q = ['-', $q, $r]; }
  }
  return $q;
}

# Expression2 = Primitive   {"*" Primitive   | "/" Primitive}.

sub expression2
{
  my $q = primitive();
  while($token eq "*" or $token eq "/")
  {
    my $t = $token;
    scan();
    my $r = primitive();
    if ($t eq '*') { $q = ['*', $q, $r]; }
    if ($t eq '/') { $q = ['/', $q, $r]; }
  }
  return $q;
}

# Primitive   = ["PRINT" | "INPUT"] ["CHAR"] Variable [Index]
#             | IntegerLiteral | CharLiteral
#             | "(" Expression0 ")".

sub primitive
{
  my $mode = 0; # listen up, kids: this is called *context* :-)
  if ($token eq 'PRINT')
  {
    $mode = 1;
    scan();
  }
  elsif ($token eq 'INPUT')
  {
    $mode = 2;
    scan();
  }
  if ($token eq 'CHAR')
  {
    $mode = 3 if $mode == 1;
    $mode = 4 if $mode == 2;
    scan();
  }
  if ($token =~ /^(\d+)$/)
  {
    my $q = 0+$1;
    scan();
    return ['print', 'int',  $q] if $mode == 1;
    return ['print', 'char', $q] if $mode == 3;
    return $q;
  }
  elsif ($token =~ /^\'(.)\'$/)
  {
    my $q = ord($1);
    scan();
    return ['print', 'int',  $q] if $mode == 1;
    return ['print', 'char', $q] if $mode == 3;
    return $q;
  }
  elsif ($token eq '(')
  {
    scan();
    my $q = expression0();
    expect(')');
    return ['print', 'int',  $q] if $mode == 1;
    return ['print', 'char', $q] if $mode == 3;
    return $q;
  }
  else
  {
    $sym{$token} = 0 if not exists $sym{$token};
    $q = [':', $token, 0];
    scan();
    if($token eq '[')
    {
      varindex();
    }
    return ['print', 'int',  $q] if $mode == 1;
    return ['print', 'char', $q] if $mode == 3;
    return $q;
  }
}

# Index       = "[" Expression0 {"," Expression0} "]".
sub varindex
{
  error("arrays not implemented");
  expect('[');
  my $q = expression0();
  while($token eq ',')
  {
    scan();
    $q .= expression0();
  }
  expect(']');
  return $q;
}

### EVALUATOR ###

sub dumpic
{
  my $x = shift;
  if(ref($x) eq 'ARRAY')
  {
    my $c = $x->[0];
    my $q = $x->[1] || 0;
    my $r = $x->[2] || 0;
    print "[$c ";
    dumpic($q);
    print " ";
    dumpic($r);
    print "] ";
  } else
  {
    print $x;
  }
}

sub evaluate
{
  my $x = shift;
  if(ref($x) eq 'ARRAY')
  {
    my $c = $x->[0];
    # print "--> command: $c\n"; # <STDIN>;
    my $q = $x->[1] || 0;
    my $r = $x->[2] || 0;
    if    ($c eq '+') { $q = evaluate($q) + evaluate($r) }
    elsif ($c eq '-') { $q = evaluate($q) - evaluate($r) }
    elsif ($c eq '*')
    {
      # multiplication is interesting in strelnokoff
      # because it is short circuiting :-)
      $q = evaluate($q);
      if ($q != 0)
      {
        $q *= evaluate($r);
      }
    }
    elsif ($c eq '/')
    {
      # division is also interesting
      # because division by 0 yields 0
      $q = evaluate($q);
      $r = evaluate($r);
      if ($r != 0)
      {
        $q = int($q / $r);
      } else
      {
        $q = 0;
      }
    }
    elsif ($c eq '=')
    {
      if(evaluate($q) == evaluate($r))
      {
        $q = 1;
      } else
      {
        $q = 0;
      }
    }
    elsif ($c eq '>')
    {
      if(evaluate($q) > evaluate($r))
      {
        $q = 1;
      } else
      {
        $q = 0;
      }
    }
    elsif ($c eq 'print')
    {
      $r = evaluate($r);
      if ($q eq 'char') { print chr($r); } else { print $r; }
      $q = $r;
    }
    elsif ($c eq ':=')
    {
      $sym{$q} = evaluate($r);
      $q = $sym{$q};
    }
    elsif ($c eq ':')
    {
      $q = $sym{$q};
    }
    else
    {
      error("unknown runtime command $c");
    }
    return $q;
  } else
  {
    return $x;
  }
}

### MAIN ###

$| = 1;
open FILE, "<$ARGV[0]";
$program = join('', <FILE>);
close FILE;
scan();
$assignments = program();
$done = 0;
while (not $done)
{
  my $no = int(rand(1) * ($#{$assignments}+1));
  my $assignment = $assignments->[$no];
  # print "Assignment # $no\n";
  # dumpic($assignment); <STDIN>;
  evaluate($assignment);
}

### END of strelnokoff.pl ###
