#!/usr/bin/perl

# HUNTER - concurrent maze-space traversal language
# v2002.01.26 Chris Pressey, Cat's Eye Technologies

# Copyright (c)2002, Cat's Eye Technologies.
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

# usage: [perl] hunter[.pl] [-no-eat] [-delay xxx] hunter-playfield-file
# requirements: Curses, or ANSI terminal, or Win32 Console.

# history: v2000.08.05 - started prototyping (from worb.pl).
#          v2000.08.07 - added deterministic traversal.
#                        mouse always tries: E, N, W, S.
#          v2000.10.24 - cleaned up code, added rewriting.
#          v2000.12.08 - added support for virtual console.
#          v2000.12.15 - refined support for virtual console.
#          v2001.01.24 - adapted to new virtual console modules.
#          v2002.01.26 - fixed behaviour of eating of cheese
#                        added command line options
#                        does not rely on "\n" in screen driver

# Uncomment this line to use a specific display driver.
# BEGIN { $_Console::Virtual::setup{display} = 'ANSI'; }

use _Console::Virtual 2001.0123
     qw(getkey display gotoxy clrscr clreol
        normal inverse bold update_display);

### GLOBALS ###

@mouse = ();
@playfield = ();
@mouse_at_cache = ();
@rule = ();

$x = 0; $y = 0;
$no_eat = 0;     # compatibility flag
$delay = 300;

### SUBS ###

sub draw_playfield
{
  gotoxy(1,1);
  my $i; my $j; my $p;
  for($j = 0; $j <= $maxy; $j++)
  {
    for($i = 0; $i <= $maxx; $i++)
    {
      if (is_mouse_at($i,$j))
      {
        display('m');
      } else
      {
        display($playfield[$i][$j]);
      }
    }
    gotoxy(1, $j+2);
  }
}

sub is_mouse_at
{
  my $x = shift; my $y = shift;
  return $mouse_at_cache[$x][$y] || 0;
}

sub vacant
{
  my $x = shift; my $y = shift;
  return 0 if $playfield[$x][$y] eq '#';
  return 0 if is_mouse_at($x,$y);
  return 1;
}

### MAIN ###

while ($ARGV[0] =~ /^\-\-?(.*?)$/)
{
  my $opt = $1;
  shift @ARGV;
  if ($opt eq 'no-eat')
  {
    $no_eat = 1;
  }
  elsif ($opt eq 'delay')
  {
    $delay = 0+shift @ARGV;
  }
  else
  {
    die "Unknown command-line option --$opt";
  }
}

open PLAYFIELD, $ARGV[0];
while(defined($line = <PLAYFIELD>))
{
  my $i;
  chomp($line);
  if ($line =~ /^\*(.*?)\>(.*?)$/)
  {
    push @rule, [$1, $2];
  } else
  {
    for($i = 0; $i < length($line); $i++)
    {
      my $c = substr($line, $i, 1);
      if (ucfirst($c) eq 'M')
      {
        $c = ' ';
        push @mouse,
        {
          'x'     => $x,
          'y'     => $y,
          'been'  => [[]],
          'seen'  => '',
          'stack' => [ 1 ],
          'dead'  => 0,
          'out'   => '',
        };
        $mouse_at_cache[$x][$y] = 1;
      }
      $playfield[$x][$y] = $c;
      $x++; if ($x > $maxx) { $maxx = $x; }
    }
    $x = 0;
    $y++; if ($y > $maxy) { $maxy = $y; }
  }
}
close PLAYFIELD;

clrscr();

draw_playfield();

$start_time = time();
$tick = 1;
while(1)
{
  my $mouse;
  my $pole;
  my $deadmice = 0; # first time I've ever used THAT as a variable name! ;-)
  foreach $mouse (@mouse)
  {
ResetMouse:

    if ($mouse->{dead})
    {
      $deadmice++;
      next;
    }

    my $tos = $mouse->{stack}[$#{$mouse->{stack}}];
    my $new_x = 0; my $new_y = 0;

    if ($tos < 5)
    {
      if ($tos == 1)
      {
        $new_x = $mouse->{x} + 1;
        $new_y = $mouse->{y};
      }
      elsif ($tos == 2)
      {
        $new_x = $mouse->{x};
        $new_y = $mouse->{y} - 1;
      }
      elsif ($tos == 3)
      {
        $new_x = $mouse->{x} - 1;
        $new_y = $mouse->{y};
      }
      elsif ($tos == 4)
      {
        $new_x = $mouse->{x};
        $new_y = $mouse->{y} + 1;
      } else
      {
        die "Can't be!";
      }

      if((defined($mouse->{been}[$new_x][$new_y]) and $mouse->{been}[$new_x][$new_y])
        or not vacant($new_x, $new_y))
      {
        $mouse->{stack}[$#{$mouse->{stack}}]++;
        next;
      }

      push @{$mouse->{stack}}, 1;
      $mouse->{been}[$mouse->{x}][$mouse->{y}] = 1;
    }
    else
    {
      $tos = pop @{$mouse->{stack}};
      if ($#{$mouse->{stack}} == -1)
      {
        $mouse->{been} = [[]];
        push @{$mouse->{stack}}, 1;
        goto ResetMouse;
      }
      $tos = $mouse->{stack}[$#{$mouse->{stack}}];
      $mouse->{been}[$mouse->{x}][$mouse->{y}] = 0;
      if ($tos == 1)
      {
        $new_x = $mouse->{x} - 1;
        $new_y = $mouse->{y};
      }
      elsif ($tos == 2)
      {
        $new_x = $mouse->{x};
        $new_y = $mouse->{y} + 1;
      }
      elsif ($tos == 3)
      {
        $new_x = $mouse->{x} + 1;
        $new_y = $mouse->{y};
      }
      elsif ($tos == 4)
      {
        $new_x = $mouse->{x};
        $new_y = $mouse->{y} - 1;
      }
      $mouse->{stack}[$#{$mouse->{stack}}]++;
      if (not vacant($new_x, $new_y))
      {
        next;
      }
    }

    if ($mouse->{out} =~ /^(.)/)
    {
      $playfield[$mouse->{x}][$mouse->{y}] = $1;
      $mouse->{out} = $';
    }

    gotoxy($mouse->{x}+1, $mouse->{y}+1);
    display($playfield[$mouse->{x}][$mouse->{y}]);
    $mouse_at_cache[$mouse->{x}][$mouse->{y}] = 0;
    $mouse->{x} = $new_x;
    $mouse->{y} = $new_y;
    $mouse_at_cache[$mouse->{x}][$mouse->{y}] = 1;
    gotoxy($mouse->{x}+1, $mouse->{y}+1);
    display('m');

    my $item = $playfield[$mouse->{x}][$mouse->{y}];
    if ($item eq '!')
    {
      $mouse->{dead} = 1;
      $playfield[$mouse->{x}][$mouse->{y}] = 'w'; # mouse carcass
      gotoxy($mouse->{x}+1, $mouse->{y}+1);
      display($playfield[$mouse->{x}][$mouse->{y}]);
    }
    elsif ($item ne ' ')
    {
      $mouse->{seen} .= $item;
      if ($item =~ /^\d$/)
      {
        $playfield[$mouse->{x}][$mouse->{y}] = ' ' unless $no_eat;
      }
    }

    my $r; my $dr = 0;
    while (not $dr)
    {
      $dr = 1;
      foreach $r (@rule)
      {
        my $q = quotemeta($r->[0]);
        if ($mouse->{seen} =~ /$q$/)
        {
          $mouse->{seen} = $`;
          $mouse->{out} .= $r->[1];
          $dr = 0;
        }
      }
    }
  }
  if ($deadmice == $#mouse+1)
  {
    exit(0);
  }
  update_display();
  for($i = 1; $i < $delay; $i++)
  {
    gotoxy(1,20);
  }
}

### END of hunter.pl ###
