#!/bin/sh
:<<"ABOUT"
This "slashes/impl/slashes.awkly.sh" is copied from
the following version of "GH-TpaeFawzen/slashes-portable-shellscript-awkly/slashes.sh":
<<https://github.com/GH-TpaeFawzen/slashes-portable-shellscript-awkly/blob/cd61ab20ab1976218612bca9f467335ef1b0496a/slashes.sh>>
The script is originally distributed under CC0-1.0.

scripted by TpaeFawzen (<<https://github.com/GH-TpaeFawzen>>)
copied by TpaeFawzen (<<https://github.com/GH-TpaeFawzen>>)

ABOUT

set -ue
umask 0022
export LC_ALL=C
export PATH="$(command -p getconf PATH 2>/dev/null)${PATH+:}${PATH-}"
case $PATH in :*) PATH=${PATH#?};; esac
export UNIX_STD=2003

sedlyLF="$(printf '\\\012_')"; sedlyLF="${sedlyLF%_}"
{
	od -A n -t x1 -v "${1:--}" |
		tr ABCDEF abcdef |
		tr -Cd '0123456789abcdef\n' |
		sed 's/../&'"$sedlyLF"'/g' |
		grep . |
		tr '\n' ,
	echo
} |
awk '
{
	for(;;){
		if(match($0,/^(2[^f],|5[^c],|[^25].,)+/)){
			printf substr($0,1,RLENGTH);
			$0=substr($0,RLENGTH+1);
			continue;
		}
		if(match($0,/^5c,..,/)){
			printf substr($0,1+3,3);
			$0=substr($0,RLENGTH+1);
			continue;
		}
		if(match($0,\
			"^2f,"\
			"(2[^f],|5[^c],|[^25].,|5c,..,)*2f,"\
			"(2[^f],|5[^c],|[^25].,|5c,..,)*2f,")\
		){
			rEndAt=RLENGTH;
			match($0,"^2f,(2[^f],|5[^c],|[^25].,|5c,..,)*2f");
			pEndAt=RLENGTH;
			oneCharLen=3;
			rBeginAt=pEndAt+oneCharLen-1;
			pBeginAt=1+oneCharLen;
			s=substr($0,pBeginAt,pEndAt-pBeginAt-oneCharLen+2);
			d=substr($0,rBeginAt,rEndAt-rBeginAt-oneCharLen+1);

			programContinueFrom=rEndAt+1;
			$0=substr($0,programContinueFrom);

			# You know what, many impls of utils with ERE
			# might not support what looks like /(..)\1/ 
			for(;escPos=match(s,/5c,...,/);)
				s=substr(s,1,escPos-3) substr(s,escPos+3);
			for(;escPos=match(d,/5c,...,/);)
				d=substr(d,1,escPos-3) substr(d,escPos+3);

			# print s,d>"/dev/stderr";

			for(;$0~s;)
				sub(s,d);

			continue;
		}
		break;
	}
}' | 
tr , '\n' |
awk '
BEGIN{
	for(i=1;i<256;i++){
		hex=sprintf("%02x",i);
		fmt[hex]=sprintf("%c",i);
		fmtl[hex]=1;
	}

	fmt["25"]="%%";
		fmtl["25"]=2;
	fmt["5c"]="\\\\\\\\";
		fmtl["5c"]=4;
	fmt["00"]="\\\\000";
		fmtl["00"]=5;
	fmt["0a"]="\\\\n";
		fmtl["0a"]=3;
	fmt["0d"]="\\\\r";
		fmtl["0d"]=3;
	fmt["09"]="\\\\t";
		fmtl["09"]=3;
	fmt["0b"]="\\\\v";
		fmtl["0b"]=3;
	fmt["0c"]="\\\\f";
		fmtl["0c"]=3;
	fmt["20"]="\\\\040";
		fmtl["20"]=5;
	fmt["22"]="\\\"";
		fmtl["22"]=2;
	fmt["27"]="\\'"'"'";
		fmtl["27"]=2;
	fmt["2d"]="\\\\055";
		fmtl["2d"]=5;
	for(i=48;i<58;i++){
		fmt[sprintf("%02x",i)]=sprintf("\\\\%03o",i);
		fmtl[sprintf("%02x",i)]=5;
	}

	ORS="";
	LF=sprintf("\n");
	printfLen=7; # as "printf "
	maxlen=int('"$(getconf ARG_MAX)"'/2)-printfLen;
	arglen=0;
}
{
	# TBH I have no ideas why 4
	if(arglen+4>maxlen){
		print LF;
		arglen=0;
	}
	print fmt[$0];
	arglen+=fmtl[$0];
}
END{
	if(NR) print LF;
	else printf"\"\"\n";
}

' |
xargs -n 1 printf

# finally
exit 0
