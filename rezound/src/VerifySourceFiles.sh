#!/bin/sh


# This function is not terribly powerful; it merely checks to see
# that $2 is included in the .h file or in the .cpp if there is no
# .h file for that .cpp.   And it makes sure that that is the $3'rd
# #include of that file.
function checkFilesForInclude # $1 is path to recur on; $2 is file to check for; $3 is line it should appear on
{
	# This section makes sure that common.h is include somewhere 
	# within every .cpp's .h file or in that .cpp file if it 
	# doesn't have a .h file.
	# Then, it reports a warning if the include of common.h isn't
	# before all other includes
	function checkForInclude
	{
		output=`grep -n "^#include.*[\"\<].*$2[\"\>]" $1`
		if [[ $? -eq 0 ]]
		then # was in file
			#output the first line number that it was found on
			echo $output | head -1 | cut -f1 -d':'
			return 1
		else # was not in file
			echo "0"
			return 0
		fi
	}

	checkForInclude=$2
	checkIncludedOnLine=$3

	#for each .cpp in $1/*/*/.../*  it looks for the $2 in that .cpp and its .h
	for i in `find $1 -name "*.cpp"`
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

		lineNo1=`checkForInclude $fileToCheck $checkForInclude`
		if [[ $? -ne 0 ]]
		then # file did contain the include

			# make sure its the $checkIncludedOnLine'st include in the file
			lineNo2=`grep "^#include" $fileToCheck | grep -n . | grep $2 | cut -f1 -d':'`
			if [[ $lineNo2 -ne $checkIncludedOnLine ]]
			then	
				echo "include of $checkForInclude is not include #$checkIncludedOnLine in $fileToCheck"
			fi

			# now warn that its not necessary in the $fileToCheckNotIn
			if [[ ! -z "$fileToCheckNotIn" ]]
			then
				checkForInclude $fileToCheckNotIn $checkForInclude >/dev/null
				if [[ $? -ne 0 ]]
				then
					echo "include of $checkForInclude is unnecessarily included in $fileToCheckNotIn"
				fi
			fi

		else # file did not contain the include

			# now make sure it's before other includes
			echo "include of $checkForInclude was not found in $fileToCheck"
		fi
	done
}

checkFilesForInclude . common.h 1
checkFilesForInclude ./frontend_fox fox_compat.h 2

echo "done"
