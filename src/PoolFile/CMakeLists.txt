cmake_minimum_required(VERSION 3.1)	
project(misc)

add_library(PoolFile STATIC
	CMultiFile.cpp
	CMultiFile.h
	#TPoolAccesser.cpp
	TPoolAccesser.h
	#TPoolFile.cpp
	TPoolFile.h
	#TStaticPoolAccesser.cpp
	TStaticPoolAccesser.h
)

