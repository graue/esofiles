#!/usr/bin/sed -f
1i\
<pre><nowiki>
s/&/\&amp;/g
s/</\&lt;/g
s/>/\&gt;/g
$a\
</nowiki></pre>