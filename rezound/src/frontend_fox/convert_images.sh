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
echo >> $H_FILE



# for each file create a data-member in the class
for i in $IMAGE_PATH/*.gif
do
	file=`basename $i`
	file=${file%\.*}
	echo "	FXIcon *$file;" >> $H_FILE
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
for i in $IMAGE_PATH/*.gif
do
	echo "static " >> $C_FILE
	$RESWRAP -n `basename ${i%\.*}`_icon $i >> $C_FILE
done

echo >> $C_FILE
echo "CFOXIcons *FOXIcons=NULL;" >> $C_FILE

echo >> $C_FILE
echo "CFOXIcons::CFOXIcons(FXApp *app) :" >> $C_FILE

# allocate each data member
for i in $IMAGE_PATH/*.gif
do
	echo "	"`basename ${i%\.*}`"(new FXGIFIcon(app,"`basename ${i%\.*}`"_icon))," >> $C_FILE
done
echo "	dummy(0)" >> $C_FILE

echo "{" >> $C_FILE
echo "}" >> $C_FILE
echo >> $C_FILE

# create destruct
echo "CFOXIcons::~CFOXIcons()" >> $C_FILE
echo "{" >> $C_FILE

# create a delete statement for each icon
for i in $IMAGE_PATH/*.gif
do
	echo "	delete "`basename ${i%\.*}`";" >> $C_FILE
done

echo "}" >> $C_FILE
echo >> $C_FILE


