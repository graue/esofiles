#!/usr/local/bin/perl -w

# randsmet[.pl] v2000.03.01 Cat's Eye Technologies
# Generate a random SMETANA program
# This work is part of the Public Domain.

### MAIN ###

$| = 1;
if ($#ARGV == -1)
{
  print "usage: randsmet <integer>\n";
  exit(1);
}
$steps = shift @ARGV;
for($i=1;$i<=$steps;$i++)
{
  print "Step $i. ";
  if (rand > 0.5)
  {
    print "Go to step " . int(rand($steps)+1) . ".\n";
  } else
  {
    print "Swap step " . int(rand($steps)+1) . " with step " .
          int(rand($steps)+1) . ".\n";
  }
}

### END ###

