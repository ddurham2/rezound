# ReZound src/misc/CNestedDataFile/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
## Copyright (C) 2002 - Anthony Ventimiglia
## 
## This file is part of ReZound, an audio editing application, 
## 
## ReZound is free software; you can redistribute it and/or modify it under the
## terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
## version.
## 
## ReZound is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
## A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place - Suite 330, Boston, MA  02111-1307, USA
include $(top_srcdir)/config/am_include.mk

noinst_HEADERS=CNestedDataFile.h

EXTRA_DIST= \
	cfg.l \
	cfg.y \
	anytype.h \
	test_CNestedDataFile.cpp

##bin_PROGRAMS=test_CNestedDataFile
##test_CNestedDataFile_SOURCES=test_CNestedDataFile.cpp CNestedDataFile.cpp cfg.tab.cc

noinst_LTLIBRARIES=libmiscCN.la 
libmiscCN_la_SOURCES= \
		CNestedDataFile.cpp \
		cfg.tab.cc

CFG_L=$(top_srcdir)/src/misc/CNestedDataFile/cfg.l
CFG_Y=$(top_srcdir)/src/misc/CNestedDataFile/cfg.y
		     
cfg.tab.o: cfg.tab.cc
	
cfg.tab.cc: cfg.lex.c $(CFG_Y) CNestedDataFile.h anytype.h
	$(YACC) -p cfg_ -b cfg $(CFG_Y)
	mv cfg.tab.c cfg.tab.cc
	
cfg.lex.c: $(CFG_L) CNestedDataFile.h
	$(LEX) -Pcfg_ -t $(CFG_L) > cfg.lex.c


# target to build the test app
test_CNestedDataFile: test_CNestedDataFile.cpp cfg.tab.o CNestedDataFile.o anytype.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o test_CNestedDataFile test_CNestedDataFile.cpp CNestedDataFile.o cfg.tab.o 


clean-local: 
	-rm -f cfg.tab.cc cfg.lex.c test_CNestedDataFile test.dat2

