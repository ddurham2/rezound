#!/bin/bash

# to change to something other than gif, replace .gif and FXGIFIcon throughout the script

set +u

if [ -z $2 ]
then
	echo "usage: $0 /path/reswrap /path/to/icons" >&2
	exit 1
fi

RESWRAP=$1
IMAGE_PATH=$2

if [ ! -x $RESWRAP ] 
then
	echo "$0: $RESWRAP either does not exist or is not executable -- perhaps delete config.cache and rerun configure"
	exit 1
fi

H_FILE=CFOXIcons.h.tmp
C_FILE=CFOXIcons.cpp

# create the CFOXIcons.h file 
echo "/* script generated source code */" > $H_FILE
echo "#ifndef __Images_H__" >> $H_FILE
echo "#define __Images_H__" >> $H_FILE
echo "#include \"../../config/common.h\"" >> $H_FILE
echo  >> $H_FILE
echo "#include <fox/fx.h>" >> $H_FILE
echo  >> $H_FILE
echo "class CFOXIcons" >> $H_FILE
echo "{" >> $H_FILE
echo "public:" >> $H_FILE
echo "        CFOXIcons(FXApp *app);" >> $H_FILE
echo "        virtual ~CFOXIcons();" >> $H_FILE
echo  >> $H_FILE
echo "        FXIcon *getByName(const char *name) const;" >> $H_FILE
echo >> $H_FILE


# this function encodes filenames with spaces or other non-printable characters into something that will be a valid C-variable name
function filenameToVarname # $1 is a [path/]filename
{
	name=`basename "${1%\.*}"`	# remove extention and pathname

	# for each characters in the name
	for ((t=0;t<${#1};t++))
	{
		c=${name:t:1}

		# these encode characters to other valid C-variable characters
		c=${c/\ /_}
		c=${c/\[/_}
		c=${c/\]/_}
		c=${c/\,/_}
		c=${c/\-/_}

		echo -n $c
		#echo -n $c >&2
	}
	#echo >&2
}

# for each file create a data-member in the class
ls $IMAGE_PATH/*.gif | while read i
do
	varname=`filenameToVarname "$i"`
	echo "	FXIcon *"$varname";" >> $H_FILE
done

echo "	int dummy;" >> $H_FILE
echo "};" >> $H_FILE
echo  >> $H_FILE
echo "extern CFOXIcons *FOXIcons;" >> $H_FILE
echo >> $H_FILE
echo "#endif" >> $H_FILE



# only overwrite the h file if it needs to be so it won't cause a whole bunch of things to unnecessarily recompile
if [ -a ${H_FILE%\.tmp} ]
then
	diff $H_FILE ${H_FILE%\.tmp} >/dev/null
	if [ $? -ne 0 ]
	then
		mv $H_FILE ${H_FILE%\.tmp}
	else
		rm $H_FILE
	fi
else
	mv $H_FILE ${H_FILE%\.tmp}
fi




# generate the cpp file
echo "/* script generated source code */" > $C_FILE
echo "#include \"${H_FILE%\.tmp}\"" >> $C_FILE
echo >> $C_FILE

# reswrap should have come with the fox package
# create a data array for each icon
ls $IMAGE_PATH/*.gif | while read i
do
	varname=`filenameToVarname "$i"`
	echo "static " >> $C_FILE
	$RESWRAP -n "${varname}_icon" "$i" >> $C_FILE
done


echo >> $C_FILE
echo "CFOXIcons *FOXIcons=NULL;" >> $C_FILE

# create constructor, CFOXIcons()
echo >> $C_FILE
echo "CFOXIcons::CFOXIcons(FXApp *app) :" >> $C_FILE

	# allocate each data member
	ls $IMAGE_PATH/*.gif | while read i
	do
		varname=`filenameToVarname "$i"`
		echo "	${varname}(new FXGIFIcon(app,${varname}_icon))," >> $C_FILE
	done
	echo "	dummy(0)" >> $C_FILE

echo "{" >> $C_FILE
echo "}" >> $C_FILE
echo >> $C_FILE

# create destructor, ~CFOXIcons()
echo "CFOXIcons::~CFOXIcons()" >> $C_FILE
echo "{" >> $C_FILE

	# create a delete statement for each icon
	ls $IMAGE_PATH/*.gif | while read i
	do
		varname=`filenameToVarname "$i"`
		echo "	delete "$varname";" >> $C_FILE
	done

echo "}" >> $C_FILE
echo >> $C_FILE

# create method, FXIcon *getByName(const char *name)
echo "#include <string.h>" >> $C_FILE
echo "FXIcon *CFOXIcons::getByName(const char *name) const" >> $C_FILE
echo "{" >> $C_FILE

	# create an if statement to return a data member for each filename
	ls $IMAGE_PATH/*.gif | while read i
	do
		varname=`filenameToVarname "$i"`
		file=`basename "${i%\.*}"`
		echo "	if(strcmp(name,\"$file\")==0) return $varname;" >> $C_FILE
	done


echo "	return NULL;" >> $C_FILE
echo "}" >> $C_FILE
echo >> $C_FILE

exit 0
