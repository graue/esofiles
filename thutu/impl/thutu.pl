#!/bin/perl
# Thutu to Perl reference compiler.

# Preamble.
print << "EOF";
#!/bin/perl
# Perl output for Thutu file $0.

EOF
print << 'EOF';
$_="=1";
$row="";
$ninequit=0;
while(!$ninequit)
{
$row =~ s/=/=q/g;
$row =~ s/\t/=t/g;
$row =~ s/\n/=x/g; # The newline at the end of the line becomes =x
$row =~ s/\r/=r/g;
$row =~ s/\f/=f/g;
$row =~ s/\a/=a/g;
$row =~ s/\e/=e/g;
$row =~ s/([!-\/:-<>-@[-`{-~])/=$1/g; #`
defined($row) or $row = '=9';
$_ = $row . $_;
while(1) {
EOF

# The main loop.
$ilo=0;
$ilocheck=0;
while(<>)
{
    chomp; # An input newline will be =n, representing a newline is \n
    s/\t/        /;
    /^ *\#/ and next; # Ignore comments (lines starting with #).
    s/^( *)//;
    print $1; # Format the output with the same indentation as the input.
    $ilo > length $1 and print "last;}};\n"; # Indentation shows looping.
    $ilo < length $1 and $ilocheck and die "Indentation increased illegaly.";
    $ilo = length $1; 
    $ilocheck = 1;
    @regexps = split /(?<!\\)\//, $_, -1; # Split on / not preceded by \
    $regexps[-1] eq "" and $#regexps--;
    $lastexp = $regexps[-1];
    $#regexps--; # The last part is going to be special.
    $penexp = undef;
    $regsep = 'and';
    # print 'print "$_\r";'."\n"; # DEBUG
    if(/\/$/) # Lines ending with / are replacement lines.
    {
	$penexp = $regexps[-1];
	$#regexps--; # In this case, the penultimate part is also special.
    }
    elsif($lastexp eq "*") # Loop while the guards and some replacement match.
    {
	print "while(";
    }
    elsif($lastexp eq "!") # Loop while no guards but some replacement match.
    {
	print "while(not(";
	$regsep = 'or';
    }
    elsif($lastexp eq "^") # Check that no guards match at start of the loop.
    {
	$regsep = 'or';
    }
    foreach $regexp (@regexps)
    {
	$regexp and print "/$regexp/ $regsep "; # Guards are just Perl regexps.
    }
    if(defined($penexp)) # This is a replacement line.
    {
	print "s/$penexp/$lastexp/ and next;\n";
    }
    elsif($lastexp eq "<") # Jump back to the start of the block
    {
	print "next;\n";
    }
    elsif($lastexp eq ">") # Jump out of this block
    {
	print "last;\n";
    }
    elsif($lastexp eq "@") # If the guards are met, loop within this block.
    {                      # The guards are only checked at the start.
	$ilo++;
	$ilocheck = 0;
	print "do {while(1) {\n";
    }
    elsif($lastexp eq "^") # If no guards are met, loop within this block.
    {
	$ilo++;
	$ilocheck = 0;
	print "0 or do {while(1) {\n";
    }
    elsif($lastexp eq "!") # Loop while a replacement but no guards match.
    {
	$ilo++;
	$ilocheck = 0;
	print "0)) {do {\n";
    }
    elsif($lastexp eq "*") # Loop while the guards and a replacement match.
    {
	$ilo++;
	$ilocheck = 0;
	print "1) {do {\n";
    }
    elsif($lastexp eq ".") # Dedentation marker for multiple dedents
    {} # Do nothing.
    else {die "Unrecognized row modifier."};
};
# Finishing off.
print << 'EOF';
last; }
s/=9// and $ninequit=1;
if(s/(.*?)=x//) # =x marks the end of what to print out.
{
    $row=$1;
    $row =~ s/=t/\t/g;
    $row =~ s/=n/\n/g; # =n converts back to newline.
    $row =~ s/=r/\r/g;
    $row =~ s/=f/\f/g;
    $row =~ s/=a/\a/g;
    $row =~ s/=e/\e/g;
    $row =~ s/=([!-\/:-<>-@[-`{-~])/$1/g; #`
    $row =~ s/=q/=/g;
    # print "\n"; # DEBUG
    print $row;
}
$ninequit or $row=<>;
};

EOF
