#!/bin/sh
# probably should go with something more generic like /bin/sh

# to change to something other than gif, replace .gif and FXGIFIcon throughout the script

set +u

if [ -z $2 ]
then
	echo "usage: $0 /path/reswrap /path/to/icons" >&2
	exit 1
fi

# absolutize these paths
RESWRAP=$(cd "$(dirname "$1")"; pwd)/$(basename "$1")
IMAGE_PATH=$(cd "$2"; pwd)
cd "$(dirname "$0")"

# patterns to match that are instantiated NOT to guess at the alpha color
override_alpha_exceptions="logo icon_logo_32 icon_logo_16 plugin_wave"

if [ ! -x $RESWRAP ] 
then
	echo "$0: $RESWRAP either does not exist or is not executable -- perhaps delete config.cache and rerun configure"
	exit 1
fi

# now determine if reswrap should be run with -n or -r depending on the version of reswrap
maj_ver=`reswrap -v 2>&1 | head -1 | cut -f2 -d' ' | cut -f1 -d'.'`
# older version print "reswrap version 1.x.x"
test $maj_ver = "version" && maj_ver=`reswrap -v 2>&1 | head -1 | cut -f3 -d' ' | cut -f1 -d'.'`
if test $maj_ver -le 2; then 	# versions 1 and 2
	RESWRAP="$RESWRAP -n"
else				# version 3
	RESWRAP="$RESWRAP -r"
fi


H_FILE=CFOXIcons.h.tmp
C_FILE=CFOXIcons.cpp

# create the CFOXIcons.h file 
echo "/* script generated source code */" > $H_FILE
echo "#ifndef __CFOXIcons_H__" >> $H_FILE
echo "#define __CFOXIcons_H__" >> $H_FILE
echo "#include \"../../config/common.h\"" >> $H_FILE
echo  >> $H_FILE
echo "#include \"fox_compat.h\"" >> $H_FILE
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
filenameToVarname() # $1 is a [path/]filename.ext
{
	# remove path and extension and translate chars
	basename "${1%\.*}" | tr ' [],-' '_____'
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
if [ -e ${H_FILE%\.tmp} ]
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
	$RESWRAP "${varname}_icon" "$i" >> $C_FILE
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
		if `echo "$override_alpha_exceptions" | grep "${varname}" >/dev/null`
		then	# don't guess at alpha color (seems to happen by default if I don't override it)
			echo "	${varname}(new FXGIFIcon(app,${varname}_icon,FXRGB(255,0,255),IMAGE_ALPHACOLOR))," >> $C_FILE
		else	# guess at alpha color
			echo "	${varname}(new FXGIFIcon(app,${varname}_icon))," >> $C_FILE
		fi
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
