#!/bin/bash
if [ "$1" ]
then
	export LST=$@
else
	export LST=*.fyb
fi

export COUNT=0
for j in *.fyb
do
        export COUNT=$(( COUNT + 1 ))
done

for i in $LST
do
	echo "$i"
	export PTS=0
	for j in *.fyb
	do
		export A="`../fukyorbrane $i $j`"
		export B="`../fukyorbrane $j $i`"
		export A="`echo \"$A\" | cut -d ' ' -f 1`"
		export B="`echo \"$B\" | cut -d ' ' -f 1`"
		
		if [ "$A" = "$B" -a "$A" != "It's" ]
		then
			if [ "$A" = "$i" ]
			then
				echo "  [x] $j"
				export PTS=$(( PTS + 1 ))
			else
				echo "  [ ] $j"
				export PTS=$(( PTS - 1 ))
			fi
		else
			echo "  [d] $j"
		fi
	done
        
        export PTS=$(( (PTS * 100) / COUNT ))
	echo "  total points: $PTS
"
done

