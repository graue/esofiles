There are several versions of this program:
cat1.w: reads one character and write it out, no check for EOF
cat2.w: pumps input to output, no check for EOF (makes use of the fact that
        "conditionals" without any stack just continue going, although it
        would be more logical to terminate the program with an error message)
cat3.w: reads one character, and if it isn't EOF, dump it out again
cat4.w: pumps input to output until EOF is reached

Versions 3 and 4 implicitly assume that EOF is -1.  Due to how hard it is to
change a Wierd program once it has been wired, I, with this very sentence,
define that Wierd programs will see EOF as -1, even on computers that normally
use a different method.

Versions 3 and 4 make use of the getput operator, which selects which
operatation to perform opposite from the specifications.  Since neither I nor
anybody else (like the writer of hello.w) wishes to change our programs, I
recommend leaving the interpreter alone and changing the specifications.  Also,
the angles work the opposite way (the first example in the specifications is
treated by the interpreter as 315 degrees).  Again, I recommend changing the
specifications.

The only true use of conditionals is in the EOF test candle (or if you prefer,
dynamite) of versions 3 and 4, although fake ones appear in versions 2 and 4
with the top-of-stack known to be zero just to turn the execution around.

It is also interesting to note that the loop to continue reading after the
first character seems like a big change but takes barely any space (although it
does cheat by temporarily creating a new thread which soon dies out), while
testing for EOF seems like a small change (after all, only one out of 256 input
characters will be affected) but takes a very long wire, primarily because the
input character is needed twice - once to test for EOF, once to print it out -
and since there is no stack-duplication operator (not that there is any room
for it, though gap-sparking could be made to have a side effect) and so I must
simulate it by putting the character in (1,1) (those coordinates have been
chosen because they are the easiest to push on the stack), then getting it
twice.

In cat4.w, the doomed thread (the one which appears when, quite literally,
looping, and soon vanishes again) might stumble across this character, but
Wierd has been designed in such a way that this can, at the worst, kill the
thread off one cycle earlier.  This proves that it is more robust than normal
languages.  In fact, the ONLY error condition possible in Wierd is if the
lop-left character is a space (although, as stated above, errors SHOULD also
appear for insufficient stack), and therefore Wierd is useful to teach
beginning programmers without getting them frustated about pages full of error
messages.  Now if you're talking about bugs that aren't caught by the language
but just have the wrong effect, THOSE are easy.

But although cat4.w is large, it's not a bad as hello.w.  One starts
understanding why many programs use external data files.  They not only
increases modifiability (for example, the program does not have to be changed
if you choose to use a different languange for the output), but also
significantly decrease program size.

This gap jumping is annoying.  I never yet ran into any need for it (if there
is nothing around that the program can go to instead, why not just fill the gap
with stars?), but sometimes the interpreter tries to gap jump when I want it to
kill the thread (which forces me to make the program LARGER to add space so no
gap jump occurs, which is the opposite of what it was meant to do), and worse,
I cannot see how it chooses when and where to jump.  I recommend that the whole
misfeature be removed.  By the way, according the the manual, "programs beyond
a certain complexity might simply be impossible to write, and would at least be
extremely impractical".  If they are impossible, gap jumping will not help (see
above question).  If practicality is an issue, I should be programming C, not
Wierd.

A more severe problem is that check-for-zero is the only available conditional.
This means that if a program wants to check, for exameple, if the top of the
stack is less than zero, then all 32768 options must be scanned.  Changing this
would imply having to change existing programs, but the problem is severe
enough that this is a necessary evil.  The earlier, the better (before even
more programs use the outdated version).

Although I do not do so myself, it is possible to include in-program comments
by including text in the program, either as the wire (with underscores
replacing spaces) or at a nonreachable distance from the program.

Finally, Wierd programs (or rather wires) are a good example of what sould NOT
be made with a text editor or anything remotely similar to it.  In fact, it
requires a special "wire editor" which can, as one of its basic operations,
stretch wires, taking any knotted wires along, and as another, cutting off
wires for later use and then place them again in a possibly rotated form.  It
is also useful if it would draw the angles next to the turns.  Sometime in the
future, when I have a lot of free time and feel excessively insane, I might try
it.  Don't hold your breath, though.
