# ReZound backend/Edits/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2002 - David W. Durham
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

## EXTRA_DIST is used to add files that aren't automatically included 
## in make dist
EXTRA_DIST=CCopyCutDeleteEdit.h \
	   CCropEdit.h \
	   CCueAction.h \
	   CInsertSilenceEdit.h \
	   CMuteEdit.h \
	   CPasteEdit.h \
	   CRotateEdit.h \
	   CSelectionEdit.h \
	   EditActions.h

## see PoolFile Makefile.am for explanation here out
## 
noinst_LTLIBRARIES=libEdits.la
libEdits_la_SOURCES= CCopyCutDeleteEdit.cpp \
		     CCropEdit.cpp \
		     CCueAction.cpp \
		     CInsertSilenceEdit.cpp \
		     CMuteEdit.cpp \
		     CPasteEdit.cpp \
		     CRotateEdit.cpp \
		     CSelectionEdit.cpp 

CCopyCutDeleteEdit.o: CCopyCutDeleteEdit.cpp \
	CCopyCutDeleteEdit.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CCropEdit.o: CCropEdit.cpp \
	CCropEdit.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CCueAction.o: CCueAction.cpp \
	CCueAction.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.cpp \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h


CMuteEdit.o: CMuteEdit.cpp \
	CMuteEdit.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CPasteEdit.o: CPasteEdit.cpp \
	CPasteEdit.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h

CRotateEdit.o: CRotateEdit.cpp \
	CRotateEdit.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CSelectionEdit.o: CSelectionEdit.cpp \
	CSelectionEdit.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CInsertSilenceEdit.o: CInsertSilenceEdit.cpp \
	CInsertSilenceEdit.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/backend/AAction.h

