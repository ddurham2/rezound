dnl Process this file with autoconf to build a configure script
dnl
dnl ReZound configure.ac, by David W. Durham based on a previous
dnl configure.in written by Anthony Ventimiglia and heavily 
dnl modified by David W. Durham
dnl
dnl Copyright (C) 2002 - David W. Durham
dnl 
dnl This file is part of ReZound, an audio editing application, 
dnl 
dnl ReZound is free software; you can redistribute it and/or modify it under the
dnl terms of the GNU General Public License as published by the Free Software
dnl Foundation; either version 2 of the License, or (at your option) any later
dnl version.
dnl 
dnl ReZound is distributed in the hope that it will be useful, but WITHOUT ANY
dnl WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
dnl FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
dnl details.
dnl 
dnl You should have received a copy of the GNU General Public License along with
dnl this program; if not, write to the Free Software Foundation, Inc., 59 Temple
dnl Place - Suite 330, Boston, MA  02111-1307, USA
# Process this file with autoconf to produce a configure script.

AC_INIT(rezound, 0.7.0beta, [http://sourceforge.net/tracker/?group_id=5056&atid=105056])
AC_CONFIG_AUX_DIR(config)
dnl ??? AC_CONFIG_SRCDIR([config/config.h])
AM_CONFIG_HEADER([config/config.h])
AM_INIT_AUTOMAKE
AC_DISABLE_SHARED   dnl This makes libtool only build static libs 

AC_LANG(C++)




dnl ############################################################################
dnl # Checks for programs                                                      #
dnl ############################################################################
CXXFLAGS= dnl AC_PROG_CXX automatically sets CXXFLAGS to -g -O2 if g++ is the C++ compiler and CXXFLAGS hasn't been set, so by setting CXXFLAGS, we can avoid this.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_LEX 
	expr z"$LEX" : z".*flex.*" >/dev/null || AC_MSG_ERROR([flex not found (and I have not tested if lex will work)])
AC_PROG_YACC 
	expr z"$YACC" : z".*bison.*" >/dev/null || AC_MSG_ERROR([bison not found (and yacc will not work)])

AM_PROG_LIBTOOL dnl turn on using libtool

AC_PATH_PROG(LAME, lame)
	test -z "$LAME" && AC_MSG_WARN([lame not found on \$PATH -- this is necessary for loading and saving MP3s -- it is called 'notlame' on some distroes -- the source can be downloaded from http://lame.sourceforge.net])






dnl ############################################################################
dnl # Set some compilter flags if they're available                            #
dnl ############################################################################
ajv_CXX_FLAG(-g)
ajv_CXX_FLAG(-Wall)
ajv_CXX_FLAG(-Wno-unused-function)
ajv_CXX_FLAG(-Wno-unused-variable)
ajv_CXX_FLAG(-Wno-unused)







dnl ############################################################################
dnl # Checks for header files                                                  #
dnl ############################################################################
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdlib.h string.h sys/ioctl.h sys/vfs.h unistd.h])






dnl ############################################################################
dnl # Checks for typedefs, structures, and compiler characteristics            $
dnl ############################################################################
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

dnl define WORDS_BIGENDIAN if this is a big endian platform
AH_TEMPLATE(WORDS_BIGENDIAN)
AC_C_BIGENDIAN([
	AC_DEFINE(WORDS_BIGENDIAN)
	AC_MSG_WARN([ReZound has not be made to work on big endian platforms yet.  However, you are welcome to see what goes wrong.])
])








dnl ############################################################################
dnl # Checks for library functions/classes                                     #
dnl ############################################################################
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT

dnl make -lm get added to the LIBS
AC_CHECK_LIB(m, sqrt,,AC_MSG_ERROR([compatible libc math library not found])) 

	dnl ??? realpath is failing now that I added AC_LANG(C++) but it's because stdlib.h is being included in the conftest.cc which defines this function
AC_CHECK_FUNCS([floor ftruncate memmove memset mkdir modf pow realpath sqrt strchr strdup strerror strrchr strstr strtol])

dnl  ??? on FreeBSD pthread isn't the right lib name... it should be libc_r ("-lc_r").. so I think on the action-if-not-found parameter to call AC_CHECK_LIB(c_r,pthread_create)  and same for the other 2 I need to ask Anthony about it
AC_CHECK_LIB(pthread, pthread_create,,AC_MSG_ERROR([compatible posix thread library not found]))
AC_CHECK_LIB(pthread, pthread_mutex_init,,AC_MSG_ERROR([compatible posix thread library not found]))
AC_CHECK_LIB(pthread, pthread_rwlock_init,,AC_MSG_ERROR([compatible posix thread library not found]))







dnl ############################################################################
dnl # Handle the flags that choose which audio I/O system to use               #
dnl ############################################################################
audioIOMethod=""

dnl handle the --enable-portaudio flag
AH_TEMPLATE(ENABLE_PORTAUDIO)
AC_ARG_ENABLE(
	portaudio,
	[
  --enable-portaudio      build ReZound to use PortAudio for audio I/O ],
	[
		if [[ "$enableval" = "yes" ]]
		then 
			dnl --enable-portaudio was given
			dnl Require that the portaudio library can be found
			AC_CHECK_LIB(
				portaudio,
				Pa_Initialize,

				origLIBS="$LIBS"
				LIBS="-lportaudio $LIBS"
				
				dnl libportaudio found, now check for portaudio header file
				[AC_CHECK_HEADER(
					portaudio.h,
					[
						AC_DEFINE(ENABLE_PORTAUDIO)
						AC_MSG_RESULT([*** using libportaudio for audio I/O])
						audioIOMethod="portaudio"
					],
					[
						LIBS="$origLIBS"
						AC_MSG_ERROR([***** portaudio.h header file not found (http://www.portaudio.com) *****])
					]
				)], 

				AC_MSG_ERROR([***** libportaudio.a not found (http://www.portaudio.com) *****])
			)
		fi
	]
)

dnl handle the --enable-jack flag
AH_TEMPLATE(ENABLE_JACK)
if [[ "$audioIOMethod" = "" ]]
then
	AC_ARG_ENABLE(
		jack,
		[  --enable-jack           build ReZound to use JACK for audio I/O ],
		[ dnl action if either enable or disable flag is given
			if [[ "$enableval" = "yes" ]]
			then 
				PKG_CHECK_MODULES(
					JACK,
					jack,
					[
						AC_DEFINE(ENABLE_JACK)
						AC_MSG_RESULT([*** using JACK for audio I/O])
						audioIOMethod="jack"
					],
					AC_MSG_ERROR([***** libjack.a or jack/jack.h not found (http://jackit.sourceforge.net) *****])
				)
				
				AC_MSG_RESULT([*** See http://sourceforge.net/docman/display_doc.php?docid=15095&group_id=5056 for more help on using ReZound with JACK])
			fi
		]
	)
fi


dnl ...
dnl handle other possible audio I/O system flags here
dnl ...

dnl handle the --enable-oss flag
dnl the behavior of this is that if no other audio I/O method was specified
dnl then OSS will be the default and the only one compiled.  If another method
dnl was specified, then ReZound will fall back on using OSS if that method
dnl fails to initialize at run-time.  There may be a better way to do this
dnl such as, enable as let them enable as many methods as they want, then
dnl choose at run-time which method to use.  Ask the first time and use that
dnl until they choose another one through the preferences interface or the 
dnl chosen one cannot be initialized.  I should probably do this when a 
dnl settings interface is created ???
AH_TEMPLATE(ENABLE_OSS)
AC_ARG_ENABLE(
	oss,
	[  --enable-oss (default)  build ReZound to use OSS for audio I/O ],
	[ dnl action if either enable or disable flag is given
		if [[ "$enableval" = "yes" ]]
		then 
			AC_DEFINE(ENABLE_OSS)
			if [[ "$audioIOMethod" = "" ]]
			then # no other audio I/O system specified
				AC_MSG_RESULT([*** using OSS for audio I/O])
				audioIOMethod="oss"
			else
				AC_MSG_RESULT([*** using OSS for audio I/O if $audioIOMethod fails to initialize])
			fi
		else
			AC_MSG_RESULT([*** disabling OSS for audio I/O])
		fi
	],
	[ dnl action if neither enable or disable flag is given (default to OSS)
			AC_DEFINE(ENABLE_OSS)
			if [[ "$audioIOMethod" = "" ]]
			then # no other audio I/O system specified
				AC_MSG_RESULT([*** using OSS for audio I/O])
				audioIOMethod="oss"
			else
				AC_MSG_RESULT([*** using OSS for audio I/O if $audioIOMethod fails to initialize])
			fi
	]
)







dnl ############################################################################
dnl # Check for Audio File Format I/O Libraries                                #
dnl ############################################################################

dnl ### Audiofile ##############################################################

dnl used to conditionally activate code in ClibaudiofileSoundTranslator.*
AH_TEMPLATE(HAVE_LIBAUDIOFILE)
dnl this macro detects libaudiofile and does an AC_SUBST on AUDIOFILE_CFLAGS and AUDIOFILE_LIBS, so I put these variables in src/backend/Makefile.am for substitution (also audiofile comes with with a pkg-config file, I could use that)
AM_PATH_AUDIOFILE(0.2.2,AC_DEFINE(HAVE_LIBAUDIOFILE),AC_MSG_NOTICE([audiofile website: http://oss.sgi.com/projects/audiofile]))



dnl ### Ogg Vorbis Libraries ##################################################

AC_LANG_PUSH(C) dnl xiph needs to include vorbisenc.h as well if C++ is going to compile the code

dnl used to conditionally activate code in ClibvorbisSoundTranslator.*
AH_TEMPLATE(HAVE_LIBOGG)
dnl this macro detects libogg and does an AC_SUBST on OGG_CFLAGS and OGG_LIBS, so I put these variables in src/backend/Makefile.am for substitution
XIPH_PATH_OGG(AC_DEFINE(HAVE_LIBOGG),AC_MSG_NOTICE([Ogg Vorbis website: http://www.xiph.org]))

dnl used to conditionally activate code in ClibvorbisSoundTranslator
AH_TEMPLATE(HAVE_LIBVORBIS)
dnl this macro detects libvorbis and does an AC_SUBST on VORBIS_CFLAGS, VORBIS_LIBS, VORBISFILE_LIBS and VORBISENC_LIBS, so I put these variables in src/backend/Makefile.am for substitution
XIPH_PATH_VORBIS(AC_DEFINE(HAVE_LIBVORBIS),AC_MSG_NOTICE([Ogg Vorbis website: http://www.xiph.org]))

AC_LANG_POP(C)







dnl ############################################################################
dnl # Handle the flags for the FFTW library                                    *
dnl ############################################################################

dnl check for the rfftw library installed.  Eventually this macro call
dnl should define HAVE_FFTW and enable or disable the FFT based actions
dnl if it's not found, then I should print a noticable warning that FFT
dnl actions will be disabled and why
dnl  ??? email about an official way to test for fftw
		dnl ??? actually I don't think I need fftw, just rfftw.. remove fftw and see if that's so
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







dnl ############################################################################
dnl # Check for the Frontend GUI Library                                       #
dnl ############################################################################

dnl This is a check for a class in a C++ library. To pinpoint the specific version we need we could change what class is linked.
dnl it sets $1_CXXFLAGS and $1_LIBS to either something useful or nothing
ajv_CXX_CHECK_LIB(FOX,FXApp,fox/fx.h,[http://fox-toolkit.org])
AC_SUBST(FOX_CXXFLAGS)
AC_SUBST(FOX_LIBS)

dnl Check for FOX's reswrap program. 
AC_PATH_PROG(RESWRAP, reswrap)
	test -z "$RESWRAP" && AC_MSG_ERROR([reswrap not found (it comes with fox) -- perhaps you need to add reswrap's location to your PATH])






dnl ############################################################################
dnl # Supply header files if their missing (and we have an implementation)     #
dnl ############################################################################

dnl this is used by the sections below AND ajv_CHECK_HEADER_SSTREAM
builddir=`pwd`  dnl I'm assuming that the pwd will always be what $(top_builddir) will be set to in AC_OUTPUT

dnl check for stdint.h
AC_CHECK_HEADER(stdint.h,
	[rm -f $builddir/src/misc/missing/generated/stdint.h],
	[
		mkdir -p $builddir/src/misc/missing/generated 2>/dev/null
		cp $srcdir/src/misc/missing/stdint.h-missing $builddir/src/misc/missing/generated/stdint.h
	]
)

dnl check for sstream (I would like to do it just like stdint.h except sstream is only found if c++/g++ is used to try to compile, and AC_CHECK_HEADER disregards that I used AC_LANG(C++) )
dnl ajv_CHECK_HEADER_SSTREAM
AC_CHECK_HEADER(sstream,
	[rm -f $builddir/src/misc/missing/generated/sstream],
	[
		mkdir -p $builddir/src/misc/missing/generated 2>/dev/null
		cp $srcdir/src/misc/missing/sstream-missing $builddir/src/misc/missing/generated/sstream
	]
)

dnl check for FXBackBufferedCanvas.h and use my implementation if not found
if ! test -z "$FOX_LIBS"; then
	AC_CHECK_HEADER(fox/FXBackBufferedCanvas.h,
		[rm -f $builddir/src/misc/missing/generated/fox/FXBackBufferedCanvas.h],
		[
			mkdir -p $builddir/src/misc/missing/generated/fox 2>/dev/null
			cp $srcdir/src/misc/missing/FXBackBufferedCanvas.h-missing $builddir/src/misc/missing/generated/fox/FXBackBufferedCanvas.h
		]
	)
fi







dnl ############################################################################
dnl # Handle link type flag                                                    #
dnl ############################################################################

dnl If the user gave --enable-standalone then add -all-static and -ldl to the 
dnl linker arguments
dnl And this check has to be after the ajv_CXX_CHECK_LIB calls because it
dnl does not use libtool to compile so it fails as a valid flag to the compiler
AC_ARG_ENABLE(
	standalone,
	[  --enable-standalone     build ReZound linked only to static libraries so it can run nearly on its own],
	[
		dnl the 3rd arg would not be necessary if this section didn't have to come after ajv_CXX_CHECK_LIB's
		AC_CHECK_LIB(dl, dlopen,[LIBS="$LIBS -ldl"])
		LDFLAGS="-all-static $LDFLAGS"
	]
	)








dnl ############################################################################
dnl # $define some install paths to be hard coded into the binary              #
dnl ############################################################################

AC_DEFINE_DIR(INSTALL_PREFIX,prefix,[ This is the prefix passed to configure])

tmp=`tmp1=\`pwd\` ; cd $srcdir ; pwd ; cd $tmp1`
AC_DEFINE_UNQUOTED(SOURCE_DIR,"$tmp",[ This is the top source directory for pre-installed testing])

AC_DEFINE_DIR(DATA_DIR,datadir,[ This the --datadir (share) set by configure ])







dnl ############################################################################
dnl # Internationalization and Localization                                    #
dnl ############################################################################

dnl LATER ??? AM_GNU_GETTEXT([external])






dnl ############################################################################
dnl # Final                                                                    #
dnl ############################################################################

AC_CONFIG_FILES([
	Makefile
	docs/Makefile
	share/Makefile
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
			src/backend/Remaster/Makefile
		src/frontend_fox/Makefile
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
