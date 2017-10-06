#!/bin/bash

function conv # input file
{
	file=$1
	shift 1

	# "Content-Type: text/plain; charset=iso-8859-1\n"
	enc=$(grep -i -m1 "Content-Type:.*charset=" "$file" | cut -f2 -d'=' | cut -f1 -d'\')

	if [[ "$enc" == "UTF-8" ]]; then
		return
	fi

	cat "$file" | while read -r l
	do
		if [[ "$l" =~ ^\"Content-Type: ]]; then
			echo '"Content-Type: text/plain; charset=UTF-8\n"'
			continue
		fi

		echo -E "$l" | iconv -f "$enc" -t "UTF-8" -
		
	done >o
	mv o "$file"

	echo $enc
	
}

while [[ $# -gt 0 ]]; do
	conv $1
	shift 1

done

