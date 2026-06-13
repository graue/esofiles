#!/usr/bin/sh
./wiki-formatter.sed < "$1" | xclip -selection c -rmlastnl