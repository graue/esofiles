#!/usr/bin/perl

# REVERSE interpreter, by Marinus Oosters

# This implements the REVERSE programming language, as described at
# http://www.geocities.com/brianscsmith/reverse.html



$#ARGV and die("reverse.pl: usage: reverse.pl program.\n");

@program = ();
%vvars = ();
%wvars = ();
%xvars = ();

$ptr = 0;
$x = 1;

open(pgm,$ARGV[0]) or die("reverse.pl: error: can't open ".$ARGV[0].".\n");
@lines = <pgm>;

# the specification is overloading -, ugh. 
# this is an ugly workaround.
# the intention of this part is to change all -'s that come directly after a
# modifier to _'s, so the 'parser' below can tell the difference.
 
$pstr = join(' ',@lines);
$pstr =~ s/(\+|-|\/|\^|%|\*|=)-/\1_/;
@ptemp = split(' ',$pstr);

foreach (@ptemp) { $_ ne '' and  @program = (@program,$_); }

while($ptr>-1 and$ptr<=$#program) {
	$cmd = $program[$ptr];
	# check for commands without a left variable
	if ($cmd=~/^reverse$/i) {if($x==1){$x=-1;}else{$x=1;}}
	elsif ($cmd=~/^reverse/i) {
		$val = 0;
		for (substr($cmd,7,1)) {
			/\>/ && do { 
				for (substr($cmd,8,1)) {
					/v/i && do { $val = ($vvars{substr($cmd,9)}<0); last;};
				 	/w/i && do { $val = ($wvars{substr($cmd,9)}<0); last;};
					/x/i && do { $val = ($xvars{substr($cmd,9)}<0); last;};
					die("$cmd : variable expected.\n");
				}
				last;
			};
			/\</ && do {
				for (substr($cmd,8,1)) {
					/v/i && do { $val = ($vvars{substr($cmd,9)}>0); last;};
					/w/i && do { $val = ($wvars{substr($cmd,9)}>0); last;};
					/x/i && do { $val = ($xvars{substr($cmd,9)}>0); last;};
					die("$cmd : variable expected.\n");
				}
				last;
			};
			/\=/ && do {
				for (substr($cmd,8,1)) {
					/v/i && do { $val = ($vvars{substr($cmd,9)}==0); last;};
					/w/i && do { $val = ($wvars{substr($cmd,9)}==0); last;};
					/x/i && do { $val = ($xvars{substr($cmd,9)}==0); last;};
					die("$cmd : variable expected.\n");
				}
				last;
			};
			/\!/ && do {
				for (substr($cmd,8,1)) {
					/\>/ && do {
						for (substr($cmd,9,1)) {
							/v/i && do {$val=!($vvars{substr($cmd,10
									)}<0);last;};
							/w/i && do {$val=!($wvars{substr($cmd,10
									)}<0);last;};
							/x/i && do {$val=!($xvars{substr($cmd,10
									)}<0);last;};
							die("$cmd : variable expected.\n");
						}
						last;
					};
					/\</ && do {
						for (substr($cmd,9,1)) {
							/v/i && do {$val=!($vvars{substr($cmd,10
									)}>0);last;};
							/w/i && do {$val=!($wvars{substr($cmd,10
									)}>0);last;};
							/x/i && do {$val=!($xvars{substr($cmd,10
									)}>0);last;};
							die("$cmd : variable expected.\n");
						}
						last;
					};
					/\=/ && do {
						for (substr($cmd,9,1)) {
							/v/i && do {$val=!($vvars{substr($cmd,10
									)}==0);last;};
							/w/i && do {$val=!($wvars{substr($cmd,10
									)}==0);last;};
							/x/i && do {$val=!($xvars{substr($cmd,10
									)}==0);last;};
							die ("$cmd : variable expected.\n");
						}
						last;
					};
					die ("$cmd : <, >, or = expected after !.\n");
				}
				last;
			};
			die("$cmd : nothing, <, >, = or ! expected after REVERSE.\n");
		}
		if($val){if($x==1){$x=-1;}else{$x=1;}}
	}	
	elsif (substr($cmd,0,3)=~/get/i) { 
		for (substr($cmd,3,1)) {
			/v/i && do {
				$c = <STDIN>;
				chomp($c);
				$vvars{substr($cmd,4)} = int($c); last;
			};
			/w/i && do {
				$c = <STDIN>;
				$wvars{substr($cmd,4)} = $c; last; };
			/x/i && do {$xvars{substr($cmd,4)} = ord(getc()) % 128; last; };
			die ("$cmd : variable expected.\n");
		}
	} elsif (substr($cmd,0,3)=~/put/i) {
		for (substr($cmd,3,1)) {
			/v/i && do { print ' '.$vvars{substr($cmd,4)}; last; };
			/w/i && do { print ' '.$wvars{substr($cmd,4)}; last; };
			/x/i && do { print chr($xvars{substr($cmd,4)}); last; };
			print substr($cmd,4);last;
		}
	} elsif ($cmd=~/^skip$/i) {
		$ptr += $x;
	}
	
	# modifiers
	
	else {
		@values = split(/[\+|\-|\*|\/|\^|\%|\=]/, $cmd);
		@ops = ();
		foreach (split("",$cmd)) {
			if (/[\+|\-|\*|\/|\^|\%|\=]/) {
				@ops = (@ops, $_);
			}
		}

		($#values - $#ops == 1) or die("$cmd : syntax error\n");

		# all but the last value will be modified, so except for the last one,
		# all of them must be variables.

		for ($c=0;$c<$#values;$c++) {
			(substr($values[$c],0,1)=~/[v|w|x]/i) or die("$cmd : variable expected.\n");
		}
		
		# execute the commands

		for ($c=$#ops;$c>=0;$c--) {

			
			# first get the value that won't change
			my $val=0;
			for (substr($values[$c+1],0,1)) {
				/v/i && do {$val = $vvars{substr($values[$c+1],1)}; last;};
				/w/i && do {$val = $wvars{substr($values[$c+1],1)}; last;};
				/x/i && do {$val = $xvars{substr($values[$c+1],1)}; last;};
				/[0-9]/ && do {$val = $values[$c+1];last;};
				/\_/ && do {$val = -(substr($values[$c+1],1));last;};
				die("$cmd : syntax error\n");
			}	
			# execute

			for (substr($values[$c],0,1)) {
				/v/i && do {
					for ($ops[$c]) {
						/\+/ && do {$vvars{substr($values[$c],1)}+=$val;last;};
						/\-/ && do {$vvars{substr($values[$c],1)}-=$val;last;};
						/\*/ && do {$vvars{substr($values[$c],1)}*=$val;last;};
						/\// && do {$vvars{substr($values[$c],1)}/=$val;last;};
						/\^/ && do {$vvars{substr($values[$c],1)}**=$val;last;};
						/\%/ && do {$vvars{substr($values[$c],1)}%=$val;last;};
						/\=/ && do {$vvars{substr($values[$c],1)}=$val;last;};
						last;
					}
					$vvars{substr($values[$c],1)}=int($vvars{substr($values[$c],1)});
					last;
				};
				/w/i && do {
					for ($ops[$c]) {
						/\+/ && do {$wvars{substr($values[$c],1)}+=$val;last;};
						/\-/ && do {$wvars{substr($values[$c],1)}-=$val;last;};
						/\*/ && do {$wvars{substr($values[$c],1)}*=$val;last;};
						/\// && do {$wvars{substr($values[$c],1)}/=$val;last;};
						/\^/ && do {$wvars{substr($values[$c],1)}**=$val;last;};
						/\%/ && do {$wvars{substr($values[$c],1)}%=$val;last;};
						/\=/ && do {$wvars{substr($values[$c],1)}=$val;last;};
						last;
					}
					last;
				};
				/x/i && do {
					for ($ops[$c]) {
						/\+/ && do {$xvars{substr($values[$c],1)}+=$val;last;};
						/\-/ && do {$xvars{substr($values[$c],1)}-=$val;last;};
						/\*/ && do {$xvars{substr($values[$c],1)}*=$val;last;};
						/\// && do {$xvars{substr($values[$c],1)}/=$val;last;};
						/\^/ && do {$xvars{substr($values[$c],1)}**=$val;last;};
						/\%/ && do {$xvars{substr($values[$c],1)}%=$val;last;};
						/\=/ && do {$xvars{substr($values[$c],1)}=$val;last;};
						last;
					}
					$xvars{substr($values[$c],1)}=int($xvars{substr($values[$c],1)});
					$xvars{substr($values[$c],1)} %= 128;
					last;
				};
				
				last;
			}
		}
	}
	$ptr += $x;
}

