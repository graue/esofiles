#!/usr/local/bin/perl -w

# muriel.pl - Cat's Eye Technologies' Muriel Interpreter
# An interpreter for Matthew Wescott's Muriel language
# (see http://demo.raww.net/muriel/ for more information)
# v2001.03.23 Chris Pressey, Cat's Eye Technologies

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

# USAGE: [perl] muriel[.pl] program.mur

### BEGIN muriel.pl ###

### QUOTIFIER ###

sub unquotify
{
  my $x = shift;
  my $a; my $b = ''; my $i;
  while(length($x)>0)
  {
    $a = substr($x, 0, 1);
    if ($a eq '\\')
    {
      $x = substr($x, 1);
      $a = substr($x, 0, 1);
      if ($a eq 'n')
      {
        $b .= "\n";
      }
      else
      {
        $b .= $a;
      }
      $x = substr($x, 1);
    } else
    {
      $b .= $a;
      $x = substr($x, 1);
    }
  }
  return $b;
}

sub quotify
{
  my $x = shift;
  my $a; my $b = ''; my $i;
  while(length($x)>0)
  {
    $a = substr($x, 0, 1);
    if ($a eq "\n" or $a eq "\r")
    {
      $b .= "\\n";
    }
    elsif ($a eq "\"")
    {
      $b .= "\\\"";
    }
    elsif ($a eq "\\")
    {
      $b .= "\\\\";
    }
    else
    {
      $b .= $a;
    }
    $x = substr($x, 1);
  }
  return $b;
}

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
  if ($program =~ /^(\d+)/)
  {
    $token = $1;
    $program = $';
  }
  elsif ($program =~ /^([A-Za-z])/)
  {
    $token = $1;
    $program = $';
  }
  elsif ($program =~ /^(\".*?[^\\]\")/ or $program =~ /^(\"\")/)
  {
    $token = $1;
    $program = $';
    $token = unquotify($token);
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
  print STDERR "*** ERROR: muriel: $msg\n";
}

### PARSER ###

# Program      ::= Instruction [";" Instruction].
sub program
{
  instruction();
  while($token eq ';')
  {
    scan();
    instruction();
  }
  # print "end program on $token\n";
}

# Instruction  ::= NumVarName ":" NumExpr
#                | StrVarName ":" StrExpr
#                | "." StrExpr
#                | "@" StrExpr.
sub instruction
{
  if ($token eq '.')
  {
    scan();
    my $q = strexpr();
    print STDOUT $q;
  }
  elsif ($token eq '@')
  {
    scan();
    # print "token: $token\n";
    my $q = strexpr();
    my $key;
    foreach $key (keys %var)
    {
      # print "=== $key === $var{$key}\n";
    }
    # print "<-- Executing $q -->\n\n"; <STDIN>;
    $program = $q;
    %var = ();
    scan();
    goto &program;
  }
  elsif ($token =~ /^[A-Z]$/)
  {
    my $t = $token;
    scan();
    expect(':');
    $var{$t} = strexpr();
  }
  elsif ($token =~ /^[a-z]$/)
  {
    my $t = $token;
    scan();
    expect(':');
    $var{$t} = numexpr();
  }
  elsif ($token eq '')
  {
    # end
  } else
  {
    error("Unknown token '$token'");
  }
}

# NumExpr      ::= NumTerm {"=" NumTerm | ">" NumTerm}.
sub numexpr
{
  my $q = numterm();
  while($token eq "=" or $token eq ">")
  {
    my $t = $token;
    scan();
    my $r = numterm();
    my $b = $q;
    if ($t eq '=') { $q = ($q == $r) || 0; }
    if ($t eq '>') { $q = ($q  > $r) || 0; }
    # print "compare: $b $t $r -> $q\n";
  }
  return $q;
}

# NumTerm      ::= NumFactor {"+" NumFactor | "-" NumFactor}.
sub numterm
{
  my $q = numfactor();
  while($token eq "+" or $token eq "-")
  {
    my $t = $token;
    scan();
    my $r = numfactor();
    if ($t eq '+') { $q += $r; }
    if ($t eq '-') { $q -= $r; }
  }
  return $q;
}

# NumFactor   ::= NumPrimitive {"*" NumPrimitive}.
sub numfactor
{
  my $q = numprimitive();
  while($token eq "*")
  {
    scan();
    $q *= numprimitive();
    # print "multiplication result: $q\n";
  }
  return $q;
}

# NumPrimitive ::= NumLiteral
#                | NumVarName
#                | "#" StrPrimitive
#                | "&" StrPrimitive
#                | "-" NumPrimitive
#                | "(" NumExpr ")".
sub numprimitive
{
  if ($token eq '#')
  {
    scan();
    return 0+strprimitive();
  }
  elsif ($token eq '&')
  {
    scan();
    my $q = length(strprimitive());
    # print "length: $q\n";
    return $q;
  }
  elsif ($token eq '-')
  {
    scan();
    return -1 * numprimitive();
  }
  elsif ($token eq '(')
  {
    scan();
    my $q = numexpr();
    expect(')');
    return $q;
  }
  elsif ($token =~ /^(\d+)$/)
  {
    my $q = 0+$1;
    scan();
    return $q;
  }
  elsif ($token =~ /^([a-z])$/)
  {
    my $q = $var{$1};
    # print "GET $1:$q\n";
    scan();
    return $q;
  }
  else
  {
    error("Illegal numeric '$token'");
    return 0;
  }
}

# StrExpr      ::= StrPrimitive {"+" StrPrimitive}.
sub strexpr
{
  my $q = strprimitive();
  while($token eq '+')
  {
    scan();
    $q .= strprimitive();
  }
  # print "strexpr: <<$q>>\n";
  return $q;
}

# StrPrimitive ::= StrLiteral
#                | StrVarName
#                | "$" NumPrimitive
#                | "%" StrPrimitive "," NumExpr "," NumExpr
#                | "|" StrPrimitive
#                | "~"
#                | "(" StrExpr ")".
sub strprimitive
{
  if ($token eq '$')
  {
    scan();
    return "" . numprimitive();
  }
  elsif ($token eq '%')
  {
    scan();
    my $q = strprimitive();
    expect(',');
    my $a = numexpr();
    expect(',');
    my $b = numexpr();
    my $r = substr($q, $a, $b-$a);
    $r = '' if $b <= $a;
    # print "substr: <<$a,$b=$r>>\n";
    return $r;
  }
  elsif ($token eq '|')
  {
    scan();
    my $q = quotify(strprimitive());
    return $q;
  }
  elsif ($token eq '~')
  {
    scan();
    my $q = <STDIN>;
    chomp $q;
    return $q;
  }
  elsif ($token eq '(')
  {
    scan();
    my $q = strexpr();
    expect(')');
    return $q;
  }
  elsif ($token =~ /^\"(.*?)\"$/s)
  {
    my $q = "" . $1;
    scan();
    return $q;
  }
  elsif ($token =~ /^([A-Z])$/)
  {
    my $q = $var{$1};
    # print "GET $1:$q\n";
    scan();
    return $q;
  }
  else
  {
    error("Illegal string '$token'");
    return "";
  }
}

### MAIN ###

$| = 1;
open FILE, "<$ARGV[0]";
$program = join('', <FILE>);
close FILE;
scan();
program();

### END of muriel.pl ###
