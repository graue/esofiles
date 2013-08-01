There are two versions of this program:
asciiadd.w: reads two characters, adds their ASCII values, and prints them out
add.w:      like above, but treats 0 as origin, so 1+2 is 3 rather than c

Note that add.w uses the 0 in the top-left corner (which is operationally
equivalent to the star used elsewhere) for the origin.  This character can be
changed to an arbitrary one, but 0 has been chosen because it gives the most
intuitive effect.

See cat.doc for general Wierd notes.
