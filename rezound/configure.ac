# Process this file with autoconf to build a configure script
#
# ReZound configure.ac, by David W. Durham based on a previous
# configure.in written by Anthony Ventimiglia and heavily 
# modified by David W. Durham
#
# Copyright (C) 2002 - David W. Durham
# 
# This file is part of ReZound, an audio editing application, 
# 
# ReZound is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
# 
# ReZound is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
# 
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA  02111-1307, USA
# Process this file with autoconf to produce a configure script.

AC_INIT(rezound, 0.8.3beta, [http://sourceforge.net/tracker/?atid=105056&group_id=5056])
AC_CONFIG_AUX_DIR(config)
# ??? AC_CONFIG_SRCDIR([config/config.h])
AM_CONFIG_HEADER([config/config.h])
AM_INIT_AUTOMAKE
AC_DISABLE_SHARED	# This makes libtool only build static libs 
AC_GNU_SOURCE 		# enable posix extensions in glibc

### handle --enable-largefile flag
AC_ARG_ENABLE(
	largefile,
	[  --enable-largefile      enable 64bit file I/O support; NOTE: all dependancies file I/O libraries will also need to have largefile support enabled],
	[ # action if either enable or disable flag is given
		if [[ "$enableval" = "yes" ]]
		then 
			AC_SYS_LARGEFILE
		fi
	]
)

AC_LANG(C++)





############################################################################
# Checks for programs                                                      #
############################################################################
CXXFLAGS= # AC_PROG_CXX automatically sets CXXFLAGS to -g -O2 if g++ is the C++ compiler and CXXFLAGS hasn't been set, so by setting CXXFLAGS, we can avoid this.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_LEX 
	expr z"$LEX" : z"flex.*" >/dev/null || AC_MSG_ERROR([flex not found (and I have not tested if lex will work)])
AC_PROG_YACC 
	expr z"$YACC" : z"bison.*" >/dev/null || AC_MSG_ERROR([bison not found (and yacc will not work)])

AM_PROG_LIBTOOL # turn on using libtool

AC_PATH_PROG(LAME, lame)
	test -z "$LAME" && AC_MSG_WARN([lame not found on \$PATH -- this is necessary for loading and saving MP3s -- it is called 'notlame' on some distroes -- the source can be downloaded from http://lame.sourceforge.net])

AC_PATH_PROG(CDRDAO, cdrdao)
	test -z "$CDRDAO" && AC_MSG_WARN([cdrdao not found on \$PATH -- this is necessary for burning audio files to CD -- http://cdrdao.sourceforge.net])






############################################################################
# Set some compilter flags if they're available                            #
############################################################################
ajv_CXX_FLAG(-g)
ajv_CXX_FLAG(-Wall)
ajv_CXX_FLAG(-Wno-unused-function)
ajv_CXX_FLAG(-Wno-unused-variable)
ajv_CXX_FLAG(-Wno-unused)






############################################################################
# Supply header files if their missing (and we have an implementation)     #
############################################################################

# this is used by the sections below AND ajv_CHECK_HEADER_SSTREAM
builddir=`pwd`  # I'm assuming that the pwd will always be what $(top_builddir) will be set to in AC_OUTPUT

# check for stdint.h
AC_CHECK_HEADER(stdint.h,
	[rm -f $builddir/src/misc/missing/generated/stdint.h],
	[
		mkdir -p $builddir/src/misc/missing/generated 2>/dev/null
		cp $srcdir/src/misc/missing/stdint.h-missing $builddir/src/misc/missing/generated/stdint.h
	]
)

# check for sstream (I would like to do it just like stdint.h except sstream is only found if c++/g++ is used to try to compile, and AC_CHECK_HEADER disregards that I used AC_LANG(C++) )
# ajv_CHECK_HEADER_SSTREAM
AC_CHECK_HEADER(sstream,
	[rm -f $builddir/src/misc/missing/generated/sstream],
	[
		mkdir -p $builddir/src/misc/missing/generated 2>/dev/null
		cp $srcdir/src/misc/missing/sstream-missing $builddir/src/misc/missing/generated/sstream
	]
)





############################################################################
# Checks for header files                                                  #
############################################################################
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdlib.h string.h sys/ioctl.h sys/vfs.h sys/statvfs.h sys/statfs.h unistd.h])






############################################################################
# Checks for typedefs, structures, and compiler characteristics            #
############################################################################
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

	temp="$CPPFLAGS"
	CPPFLAGS="-I$builddir/src/misc/missing/generated $CPPFLAGS"
AC_CHECK_TYPES([uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t], [], [AC_MSG_ERROR([Necessary fixed size integer types not found])],[#include <stdint.h>])
	CPPFLAGS="$temp"

AC_CHECK_SIZEOF([float],[])
test `grep SIZEOF_FLOAT confdefs.h | cut -f3 -d' '` -ne 4 && AC_MSG_ERROR([Type float is not 4 bytes])

AC_CHECK_SIZEOF([double],[])
test `grep SIZEOF_DOUBLE confdefs.h | cut -f3 -d' '` -ne 8 && AC_MSG_ERROR([Type double is not 8 bytes])

#################################
# Among other things, if char is not size 1 then I will need to address:
#   poolNames in poolFile
#   signatures in CSound, CMultiFile and TPoolFile
#   saved cue names
#   saved user notes in the ASoundTranslator derived classes
#################################
AC_CHECK_SIZEOF([char],[])
test `grep SIZEOF_CHAR confdefs.h | cut -f3 -d' '` -ne 1 && AC_MSG_ERROR([Type char is not 1 byte])






# define WORDS_BIGENDIAN if this is a big endian platform
AH_TEMPLATE(WORDS_BIGENDIAN)
AC_C_BIGENDIAN([AC_DEFINE(WORDS_BIGENDIAN)])








############################################################################
# Checks for library functions/classes                                     #
############################################################################
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT

# make -lm get added to the LIBS
AC_CHECK_LIB(m, sqrt,,AC_MSG_ERROR([compatible libc math library not found])) 

AC_LANG_PUSH(C)
AC_CHECK_FUNCS([floor ftruncate memmove memset mkdir modf pow realpath sqrt strchr strdup strerror strrchr strstr strtol statfs],[],[AC_MSG_WARN([Some functions not found.  Compile may fail.  Alternate methods may need to be implemented.])])
AC_LANG_POP(C)


# check for -lpthread being valid
AC_CHECK_LIB(
	pthread,
	pthread_create,
	[ # -lpthread is valid, so check for mutexes and rwlocks to be implemented
		AC_CHECK_LIB(pthread, pthread_mutex_init,,AC_MSG_ERROR([compatible posix thread library not found containing mutexes]))
		AC_CHECK_LIB(pthread, pthread_rwlock_init,,AC_MSG_ERROR([compatible posix thread library not found containing rwlocks]))
		# until libtool gets its act together and decides to correctly support -pthread or not, I am forcing the -lpthread to link with
		LDFLAGS="$LDFLAGS -Xlinker -lpthread"
	],
	[ # -lpthread is NOT valid, so check if -lc_r gets us pthread functionality (BSD uses -lc_r instead of -lpthread (how annoying))
		AC_CHECK_LIB(
			c_r,
			pthread_create,
			[ # -lc_r is valid, so check for mutexes and rwlocks to be implemented
				AC_CHECK_LIB(c_r, pthread_mutex_init,,AC_MSG_ERROR([compatible posix thread library not found containing mutexes]))
				AC_CHECK_LIB(c_r, pthread_rwlock_init,,AC_MSG_ERROR([compatible posix thread library not found containing rwlocks]))
				# until libtool gets its act together and decides to correctly support -pthread or not, I am forcing the -lc_r to link with
				LDFLAGS="$LDFLAGS -Xlinker -lc_r"
			],
			[ # -lc_r is also NOT valid, so we can't find a pthread lib
				AC_MSG_ERROR([compatible posix thread library not found])
			]
		)
	]
)







############################################################################
# Handle the flags that choose which audio I/O system to use               #
############################################################################
audioIOMethod=""

AC_MSG_NOTICE([Tests for methods to use for audio I/O will follow below.  The method actually used will be determined at run-time depending on the order of the values in the ~/.rezound/registry.dat file in AudioOutput(or Input)Methods and will depend on which method first initializes successfully.  Alternatively, there is an --audio-method=... flag to the rezound binary to choose which method to try first.  See 'rezound --help' for more information.])

# handle the --enable-oss flag
AH_TEMPLATE(ENABLE_OSS)
AC_ARG_ENABLE(
	oss,
	[  --enable/disable-oss            enable/disable ReZound to use OSS for audio I/O ],
	[ # action if either enable or disable flag is given
		if [[ "$enableval" = "yes" ]]
		then 
			AC_DEFINE(ENABLE_OSS)
			AC_MSG_RESULT([*** enabling OSS for audio I/O])
		else
			AC_MSG_RESULT([*** disabling OSS for audio I/O])
		fi
	],
	[ # action if neither enable or disable flag is given
			AC_DEFINE(ENABLE_OSS)
			AC_MSG_RESULT([*** enabling OSS for audio I/O])
	]
)

# handle the --enable-portaudio flag
AH_TEMPLATE(ENABLE_PORTAUDIO)
AC_ARG_ENABLE(
	portaudio,
	[  --enable/disable-portaudio      enable/disable ReZound to use PortAudio for audio I/O ],
	[
		if [[ "$enableval" = "yes" ]]
		then 
			# --enable-portaudio was given
			# Require that the portaudio library can be found
			AC_CHECK_LIB(
				portaudio,
				Pa_Initialize,

				origLIBS="$LIBS"
				LIBS="-lportaudio $LIBS"
				
				# libportaudio found, now check for portaudio header file
				[AC_CHECK_HEADER(
					portaudio.h,
					[
						AC_DEFINE(ENABLE_PORTAUDIO)
						AC_MSG_RESULT([*** enabling libportaudio for audio I/O])
					],
					[
						LIBS="$origLIBS"
						AC_MSG_ERROR([***** portaudio.h header file not found (http://www.portaudio.com) *****])
					]
				)], 

				AC_MSG_ERROR([***** libportaudio.a not found (http://www.portaudio.com) *****])
			)
		else
			AC_MSG_RESULT([*** disabling libportaudio for audio I/O])
		fi
	],
	[ # action if neither enable or disable flag was given
		# if libportaudio and portaudio.h are found, then enable it
		AC_CHECK_LIB(
			portaudio,
			Pa_Initialize,

			origLIBS="$LIBS"
			LIBS="-lportaudio $LIBS"
			
			# libportaudio found, now check for portaudio header file
			[AC_CHECK_HEADER(
				portaudio.h,
				[
					AC_DEFINE(ENABLE_PORTAUDIO)
					AC_MSG_RESULT([*** enabling libportaudio for audio I/O])
				],
				[
					LIBS="$origLIBS"
				]
			)]
		)
	]
)

# handle the --enable-jack flag
AH_TEMPLATE(ENABLE_JACK)
AC_ARG_ENABLE(
	jack,
	[  --enable/disable-jack           enable/disable ReZound to use JACK for audio I/O ],
	[ # action if either enable or disable flag is given
		if [[ "$enableval" = "yes" ]]
		then 
			PKG_CHECK_MODULES(
				JACK,
				jack,
				[
					AC_DEFINE(ENABLE_JACK)
					AC_MSG_RESULT([*** enabling JACK for audio I/O])
				],
				AC_MSG_ERROR([***** libjack.a or jack/jack.h not found (http://jackit.sourceforge.net) *****])
			)
			
			AC_MSG_RESULT([*** See http://sourceforge.net/docman/display_doc.php?docid=15095&group_id=5056 for more help on using ReZound with JACK])
		else
			AC_MSG_RESULT([*** disabling JACK for audio I/O])
		fi
	],
	[ # action if neither enable or disable flag was given
		# if the jack lib is found, then enable it
		PKG_CHECK_MODULES(
			JACK,
			jack,
			[
				AC_DEFINE(ENABLE_JACK)
				AC_MSG_RESULT([*** enabling JACK for audio I/O])
			],
			[
				# disable default action of exiting if not found
				AC_MSG_RESULT([*** JACK not enabled for audio I/O])
			]
		)
	]
)


# ...
# handle other possible audio I/O system flags here
# ...







############################################################################
# Check for Audio File Format I/O Libraries                                #
############################################################################

### Audiofile ##############################################################

# used to conditionally activate code in ClibaudiofileSoundTranslator.*
AH_TEMPLATE(HAVE_LIBAUDIOFILE)
# this macro detects libaudiofile and does an AC_SUBST on AUDIOFILE_CFLAGS and AUDIOFILE_LIBS, so I put these variables in src/backend/Makefile.am for substitution (also audiofile comes with with a pkg-config file, I could use that)
AM_PATH_AUDIOFILE(0.2.2,AC_DEFINE(HAVE_LIBAUDIOFILE),AC_MSG_NOTICE([audiofile website: http://oss.sgi.com/projects/audiofile]))



### Ogg Vorbis Libraries ##################################################

AC_LANG_PUSH(C) # xiph needs to include vorbisenc.h as well if C++ is going to compile the code

AH_TEMPLATE(HAVE_LIBOGG)
# this macro detects libogg and does an AC_SUBST on OGG_CFLAGS and OGG_LIBS, so I put these variables in src/backend/Makefile.am for substitution
XIPH_PATH_OGG(AC_DEFINE(HAVE_LIBOGG),AC_MSG_NOTICE([Ogg Vorbis website: http://www.xiph.org]))

AH_TEMPLATE(HAVE_LIBVORBIS)
# this macro detects libvorbis and does an AC_SUBST on VORBIS_CFLAGS, VORBIS_LIBS, VORBISFILE_LIBS and VORBISENC_LIBS, so I put these variables in src/backend/Makefile.am for substitution
XIPH_PATH_VORBIS(AC_DEFINE(HAVE_LIBVORBIS),AC_MSG_NOTICE([Ogg Vorbis website: http://www.xiph.org]))

AC_LANG_POP(C)





### FLAC Library ##########################################################

# used to conditionally activate code in CFLACSoundTranslator.*
AH_TEMPLATE(HAVE_LIBFLACPP)
AH_TEMPLATE(HAVE_LIBFLAC)

# look for libFLAC and if it's found then look for libFLAC++
AM_PATH_LIBFLAC(
	[
		AC_DEFINE(HAVE_LIBFLAC)
		AM_PATH_LIBFLACPP(AC_DEFINE(HAVE_LIBFLACPP),AC_MSG_NOTICE([FLAC website: http://flac.sourceforge.net]))
	],
	AC_MSG_NOTICE([FLAC website: http://flac.sourceforge.net])
)






############################################################################
# Handle the flags for the FFTW library                                    #
############################################################################

# check for the rfftw library installed.  Eventually this macro call
# should define HAVE_FFTW and enable or disable the FFT based actions
# if it's not found, then I should print a noticable warning that FFT
# actions will be disabled and why
#  ??? email about an official way to test for fftw
		# ??? actually I don't think I need fftw, just rfftw.. remove fftw and see if that's so
AC_CHECK_HEADER(
	fftw.h,
	AC_CHECK_LIB(fftw, fftw_create_plan, , AC_MSG_WARN([***** libfftw.a not found -- any FFT based actions will be disabled (http://www.fftw.org) *****]),-lfftw),
	AC_MSG_WARN([***** fftw.h header file not found -- any FFT based actions will be disabled (http://www.fftw.org) *****])
	)

AC_CHECK_HEADER(
	rfftw.h,
	AC_CHECK_LIB(rfftw, rfftw_create_plan, , AC_MSG_WARN([***** librfftw.a not found -- any FFT based actions will be disabled (http://www.fftw.org) *****]),-lrfftw),
	AC_MSG_WARN([***** rfftw.h header file not found -- any FFT based actions will be disabled (http://www.fftw.org) *****])
	)







############################################################################
# Check for the Frontend GUI Library                                       #
############################################################################

# This is a check for a class in a C++ library. To pinpoint the specific version we need we could change what class is linked.
# it sets $1_CXXFLAGS and $1_LIBS to either something useful or nothing
ajv_CXX_CHECK_LIB(FOX,FXApp,fox/fx.h,[http://fox-toolkit.org])
AC_SUBST(FOX_CXXFLAGS)
AC_SUBST(FOX_LIBS)

# Check for FOX's reswrap program. 
AC_PATH_PROG(RESWRAP, reswrap)
	test -z "$RESWRAP" && AC_MSG_ERROR([reswrap not found (it comes with fox) -- perhaps you need to add reswrap's location to your PATH])





################################################################################
# Supply header files (again) if their missing (and we have an implementation) #
################################################################################

# check for FXBackBufferedCanvas.h and use my implementation if not found
if ! test -z "$FOX_LIBS"; then
	AC_CHECK_HEADER(fox/FXBackBufferedCanvas.h,
		[rm -f $builddir/src/misc/missing/generated/fox/FXBackBufferedCanvas.h],
		[
			mkdir -p $builddir/src/misc/missing/generated/fox 2>/dev/null
			cp $srcdir/src/misc/missing/FXBackBufferedCanvas.h-missing $builddir/src/misc/missing/generated/fox/FXBackBufferedCanvas.h
		]
	)
fi









############################################################################
# #define some install paths to be hard coded into the binary              #
############################################################################

AC_DEFINE_DIR(INSTALL_PREFIX,prefix,[ This is the prefix passed to configure])

tmp=`tmp1=\`pwd\` ; cd $srcdir ; pwd ; cd $tmp1`
AC_DEFINE_UNQUOTED(SOURCE_DIR,"$tmp",[ This is the top source directory for pre-installed testing])

AC_DEFINE_DIR(DATA_DIR,datadir,[ This the --datadir (share) set by configure ])







############################################################################
# Internationalization and Localization                                    #
############################################################################

AM_GNU_GETTEXT_VERSION([0.11.5])
AM_GNU_GETTEXT([external])





############################################################################
# Handle link type flag                                                    #
############################################################################

# If the user gave --enable-standalone then add -all-static and -ldl to the 
# linker arguments
# And this check has to be after the ajv_CXX_CHECK_LIB calls because it
# does not use libtool to compile so it fails as a valid flag to the compiler
AC_ARG_ENABLE(
	standalone,
	[  --enable-standalone     build ReZound linked only to static libraries so it can run nearly on its own],
	[
		# the 3rd arg would not be necessary if this section didn't have to come after ajv_CXX_CHECK_LIB's
		AC_CHECK_LIB(dl, dlopen,[LIBS="$LIBS -ldl"])
		LDFLAGS="-all-static $LDFLAGS"
		AC_MSG_NOTICE([*** NOTICE *** stand-alone build enabled -- now static versions of all the dependancy libraries will be required.  If final linking fails, then make sure a static version of the failed library is installed.])

		# warn about needing static libs if using FOX and statically linking
		if ! test -z "$FOX_LIBS"; then
			AC_MSG_NOTICE([*** NOTICE *** since static linking is enabled, FOX will require static versions of libjpeg, libpng, libtiff, XFree86-libs, and maybe others (Also, I have found it necessary to configure FOX as: "CPPFLAGS=-DNO_XIM configure --with-opengl=no" when statically linking)])
		fi

		
	]
	)







############################################################################
# Final                                                                    #
############################################################################

AC_CONFIG_FILES([
	Makefile
	src/Makefile
		src/PoolFile/Makefile
		src/misc/Makefile
			src/misc/CNestedDataFile/Makefile
		src/backend/Makefile
			src/backend/DSP/Makefile
			src/backend/Edits/Makefile
			src/backend/Effects/Makefile
			src/backend/Filters/Makefile
			src/backend/Looping/Makefile
			src/backend/Generate/Makefile
			src/backend/Remaster/Makefile
		src/frontend_fox/Makefile
	po/Makefile.in
])

AC_OUTPUT



# create a softlink to what should finally be the binary 
# in the frontend interface.. If there is ever a choice
# of frontend API, then this should become a conditional
# statement
rm -f ./rezound
ln -s src/frontend_fox/rezound ./rezound


rm -f a.out #autoconf  is leaving this around for some reason


echo
echo "Please consider joining the ReZound-users mailing list: 
	http://lists.sourceforge.net/lists/listinfo/rezound-users"
echo
echo "You may also want to click the 'Monitor' icon on the 'Latest File Releases'"
echo "section on http://sourceforge.net/projects/rezound/ to be informed when new"
echo "releases are made."
echo
