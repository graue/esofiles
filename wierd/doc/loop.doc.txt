loop1.w is an infinite loop, which I have tried to write with minimal size (with
the restraints that any garbage put on the stack must be taken off again, but
that no popping intructions may be performed when there is still data on the
stack).  It seems at first that the first row and column can be ommited, but
this doesn't work, and I suspect broken gap jumping to be the cause.

loop2.w is also an infinite loop, but with the extra restraint that no threads
may be started.  Notice how the horizontal line at the bottom can be entered by
two parallel paths.  The leftmost path is used the first time, afterwards the
rightmost path is used.  Both have the same angle, therefore the same effect.

For more information on this broken gap jumping, and the language in general,
see cat.doc.
