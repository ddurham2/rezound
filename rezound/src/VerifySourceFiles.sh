#!/bin/sh

# This section makes sure that common.h is include somewhere 
# within every .cpp's .h file or in that .cpp file if it 
# doesn't have a .h file.
# Then, it reports a warning if the include of common.h isn't
# before all other includes
function checkForInclude
{
	output=`grep -n "^#include.*[\"\<].*common\.h[\"\>]" $1`
	if [[ $? -eq 0 ]]
	then # was in file
		echo $output | head -1 | cut -f1 -d':'
		return 1
	else # was not in file
		echo "0"
		return 0
	fi
}

for i in `find . -name "*.cpp"`
do
	cppFile=$i
	hFile=${i%.cpp}.h

	# determine which file in which to look for the include
	fileToCheck=""
	fileToCheckNotIn=""
	if [[ ! -a $hFile ]]
	then
		fileToCheck=$cppFile
	else
		fileToCheck=$hFile
		fileToCheckNotIn=$cppFile
	fi

	lineNo1=`checkForInclude $fileToCheck`
	if [[ $? -ne 0 ]]
	then # file did contain the include

		# make sure its the first include in the file
		lineNo2=`grep -n "^#include" $fileToCheck | head -1 | cut -f1 -d':'`
		if [[ $lineNo1 -gt $lineNo2 ]]
		then	
			echo "include of common.h is not the first include in $fileToCheck"
		fi

		# now warn that its not necessary in the $fileToCheckNotIn
		if [[ ! -z "$fileToCheckNotIn" ]]
		then
			checkForInclude $fileToCheckNotIn >/dev/null
			if [[ $? -ne 0 ]]
			then
				echo "include of common.h is unnecessarily included in $fileToCheckNotIn"
			fi
		fi

	else # file did not contain the include

		# now make sure it's before other includes
		echo "include of common.h was not found in $fileToCheck"
	fi



done






echo "done"
