#!perl

$id = "a";

sub compile {
	my ($toplevel, $i) = @_;
	my ($c, $s, $sub_s);
	$s = "\nbf(in|l|ent|cr|out) {\n";
	$sub_s = "";
	if (!$toplevel) {
		$s .= "\t8zeros(cr|allzero)set allzero |[\n";
	}
	for (;;) {
		if (!read(STDIN,$c,1) || $c eq ']') {
			if (!$toplevel) {
				$s .= "\t\tbf(in|l|ent|cr|out)$i\n\t] ent\n";
			}
			return "$s} (in|l|ent|cr|out)$i\n$sub_s";
		} elsif ($c eq '+') {
			$s .= "\t\tinc8(cr)8ced\n";
		} elsif ($c eq '-') {
			$s .= "\t\tdec8(cr)8cni\n";
		} elsif ($c eq '<') {
			$s .= "\t\tmove8from(l|cr)to\n";
		} elsif ($c eq '>') {
			$s .= "\t\tmove8from(cr|l)to\n";
		} elsif ($c eq '.') {
			$s .= "\t\tcopy8from(cr|out)to zero|out\n";
		} elsif ($c eq ',') {
			$s .= "\t\tmove8from(cr|ent)to in ent move8from(in|cr)to\n";
		} elsif ($c eq '[') {
			++$id;
			$s .= "\t\tbf(in|l|ent|cr|out)$id\n";
			$sub_s .= compile(0, $id);
		}
	}
}

print compile(1, $id);

print "
8zeros(x|b) {
	x|[x|[x|[x|[x|[x|[x|[x|[b|b]|x]|x]|x]|x]|x]|x]|x]|x
} (x|b)set

move8from(a|b) {
	a t a t a t a t a t a t a t a
	b t b t b t b t b t b t b t b
} (a|b)to

copy8from(a|b) {
	a[u|u]t u v a[u|u]t u v a[u|u]t u v a[u|u]t u v
	a[u|u]t u v a[u|u]t u v a[u|u]t u v a[u|u]t u v
	t a t a t a t a t a t a t a t a
	v b v b v b v b v b v b v b v b
} (a|b)to

dec8(s) {
	<< If s is all zeroes, dec will recurse endlessly. To avoid >>
	<< that, add 256 here if bits 0..7 are all 0. We need to do >>
	<< this anyway at one end or the other, so that bits beyond >>
	<< #7 are not affected if the inc/dec rolls over.           >>
	s|[s|[s|[s|[s|[s|[s|[s|[inc(s)ced]|s]|s]|s]|s]|s]|s]|s]|s

	dec(s)cni
} (s)8cni

inc(s) {
	s [ inc(s)ced ] | s
} (s)ced

str(s|count) {
        s [
                s temp s temp s temp s temp
                s temp s temp s temp s temp
                count str(s|count)len | count
                temp s temp s temp s temp s
                temp s temp s temp s temp s
        ] s
} (s|count)len

copyrev(a|b) {
	a [
		a[u|u]t u v a[u|u]t u v a[u|u]t u v a[u|u]t u v
		a[u|u]t u v a[u|u]t u v a[u|u]t u v a[u|u]t u v
		v b v b v b v b v b v b v b v b
		zero | b
		copyrev(a|b)stream
		t a t a t a t a t a t a t a t a
	] a
} (a|b)stream

dump(s|ent) {
	s [
		s ent s ent s ent s ent
		s ent s ent s ent s ent
		dump(s|ent)stream
	] ent
} (s|ent)stream

(ent|in) {
	<< run the brainfuck program >>
	bf(in|l|ent|cr|temp_out)a
	<< copy the output >>
	copyrev(temp_out|out)stream
	<< unrun the brainfuck program to erase temp_out, cr, and l, and empty the bit bucket >>
	a(temp_out|cr|ent|l|in)fb
	<< dump the input into the bit bucket >>
	dump(in|ent)stream
} (out|ent)
";
