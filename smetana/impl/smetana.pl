#!/usr/bin/perl -w

### BEGIN smetana.pl ###

# smetana[.pl] v2004.0227 Cat's Eye Technologies
# Interpreter/Debugger for the SMETANA language
# This work is part of the Public Domain.

### GLOBALS ###

$[ = 1;
@step = ();
$cycle = 1;
$curstep = 1;
$maxcycle = 0;

### SUBROUTINES ###

sub load_source_file # into global list variable @step
{
  my $fn = shift @_;
  my $rs = 1; my $x = '';

  @step = ();
  open FILE, "<$fn";
  while(defined($x = <FILE>))
  {
    if ($x =~ /^\s*Step\s*(\d+)\s*\.\s*(.*?)$/io)
    {
      my $x = $2;
      if ($1 == $rs)
      {
        if ($x =~ /^Go\s*to\s*step\s*(\d+)\s*\.\s*$/io)
        {
          push @step, [1, $1];
        } elsif ($x =~ /^Swap\s*step\s*(\d+)\s*with\s*step\s*(\d+)\s*\.\s*$/io)
        {
          push @step, [0, $1, $2];
        } else
        {
          print STDERR "SMETANA: Triple Boo!  " .
	               "Insulting instruction in step $rs.\n";
          return;
        }
        $rs++;
      } else
      {
        print STDERR "SMETANA: Double Boo!  " .
	             "Line $rs contains wrong step, step $1.\n";
        return;
      }
    } else
    {
      print STDERR "SMETANA: Boo!  Line $rs does not contain a step.\n";
      return;
    }
  }
}

sub display_program
{
  my $i = 1;
  while (defined($step[$i]))
  {
    print "Step $i. ";
    if ($step[$i][1])
    {
      print "Go to step " . $step[$i][2] . ".\n";
    } else
    {
      print "Swap step " . $step[$i][2] . " with step " . $step[$i][3] . ".\n";
    }
    $i++;
  }
  print "\n";
}

### MAIN ###

$| = 1;
if ($#ARGV == 0)
{
  print <<'EOT';
smetana[.pl] v2004.0227 - Interpreter/Debugger for the SMETANA language
Chris Pressey, Cat's Eye Technologies.
This work is part of the Public Domain.

Usage:
  [perl] smetana[.pl] inputfile {-d binfile | -m integer}
  inputfile: text file to use as SMETANA source file.
  -d binfile: optional program to shell between program states (e.g. cls)
  -m integer: optional limit to number of states (to avoid infinite loops)
EOT
  exit(1);
}

load_source_file shift @ARGV;

while (defined $ARGV[1])
{
  $a = shift @ARGV;
  if ($a eq '-d')
  {
    $xbefore = shift @ARGV;
  } elsif ($a eq '-m')
  {
    $maxcycle += shift @ARGV;
  } else
  {
    print STDERR "SMETANA: What the..?  " .
                 "Unsupported command line option '$a'.\n";
  }
}

while ($maxcycle == 0 or $cycle <= $maxcycle)
{
  system $xbefore if $xbefore;
  print "Current cycle: $cycle.  Current step: $curstep.\n";
  display_program();
  if ($step[$curstep][1])
  {
    $curstep = $step[$curstep][2]; 
  } else
  {
    my $a = $step[$curstep][2];
    my $b = $step[$curstep][3];
    my $temp = $step[$a];
    $step[$a] = $step[$b];
    $step[$b] = $temp;
    $curstep++;
  }
  $cycle++;
  last if $curstep > $#step;
}

system $xbefore if $xbefore;
print "Final cycle: $cycle.  Final step: $curstep.\n";
display_program();

### END of smetana.pl ###
