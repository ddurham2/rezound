#!/bin/bash

RESWRAP=$1
IMAGE_PATH=$2

echo "/* script generated source code */" > images.cpp
echo "#include \"images.h\"" >> images.cpp
echo >> images.cpp


# reswrap should have come with the fox package
for i in $IMAGE_PATH/*.gif
do
	$RESWRAP -n `basename ${i%\.gif}_gif` $i >> images.cpp
done


echo "/* script generated source code */" > images.h
echo "#ifndef __Images_H__" >> images.h
echo "#define __Images_H__" >> images.h
echo "#include \"../../config/common.h\"" >> images.h
echo  >> images.h
grep "^const unsigned char" images.cpp | sed 's/^const /extern const /' | sed 's/={/;/' >> images.h
echo >> images.h
echo "#endif" >> images.h
