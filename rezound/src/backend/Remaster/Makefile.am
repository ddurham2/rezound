# ReZound backend/Remaster/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2002 - Anthony Ventimiglia
##   
##  This file is part of ReZound, an audio editing application.
##  
##  ReZound is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published
##  by the Free Software Foundation; either version 2 of the License,
##  or (at your option) any later version.
##  
##  ReZound is distributed in the hope that it will be useful, but
##  WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##  
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
##  



include $(top_srcdir)/config/am_include.mk

## see PoolFile Makefile.am for explanation here out
## 
noinst_HEADERS= \
		CBalanceAction.h \
		CMonoizeAction.h \
		CNoiseGateAction.h \
		CRemoveDCAction.h \
		CInvertPhaseAction.h \
		CUnclipAction.h \
		CCompressorAction.h \
		CNormalizeAction.h \
		CAdaptiveNormalizeAction.h \
		CResampleAction.h \
		CChangePitchAction.h \
		CChangeTempoAction.h \
		CMarkQuietAreasAction.h \
		RemasterActions.h

noinst_LTLIBRARIES=libRemaster.la
libRemaster_la_SOURCES= \
			CBalanceAction.cpp \
			CMonoizeAction.cpp \
			CNoiseGateAction.cpp \
			CRemoveDCAction.cpp \
			CInvertPhaseAction.cpp \
			CUnclipAction.cpp \
			CCompressorAction.cpp \
			CNormalizeAction.cpp \
			CAdaptiveNormalizeAction.cpp \
			CResampleAction.cpp \
			CChangePitchAction.cpp \
			CChangeTempoAction.cpp \
			CMarkQuietAreasAction.cpp 


AM_CXXFLAGS=@SOUNDTOUCH_CXXFLAGS@
# had to put this in here or it wouldn't link because of lib order
libRemaster_la_LIBADD=@SOUNDTOUCH_LIBS@ 
