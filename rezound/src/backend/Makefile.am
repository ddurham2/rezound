# ReZound backend/Makefile.am Written by Anthony Ventimiglia
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

## first we'll dive into the subs to make everything in there
##
SUBDIRS=Edits  \
	Effects \
	Remaster

## EXTRA_DIST these files don't make it into make dist by default but
## they are required for the build
EXTRA_DIST= \
	   file.h \
	   playcontrols.h \
	   unit_conv.h \
	   AAction.h \
	   ALFO.h \
	   ASound.h \
	   ASoundPlayer.h \
	   ASoundRecorder.h \
	   ASoundFileManager.h \
	   AStatusComm.h \
	   CEnvelope.h \
	   CGraphParamValueNode.h \
	   CLoadedSound.h \
	   COSSSoundPlayer.h \
	   CSoundManager.h \
	   CSoundManagerClient.h \
	   CSoundPlayerChannel.h \
	   CTrigger.h \
	   ClibaudiofileSound.h \
	   Cold_rezSound.h \
	   CrezSound.h \
	   CrezSoundRecorder.h \
	   DSPBlocks.h 
	   

## see PoolFile Makefile.am for explanation here out
## 
noinst_LTLIBRARIES=libbackend.la
libbackend_la_SOURCES= AAction.cpp  \
		       ASound.cpp  \
		       ASoundFileManager.cpp \
		       ASoundPlayer.cpp  \
		       ASoundRecorder.cpp \
		       AStatusComm.cpp \
		       CEnvelope.cpp \
		       CGraphParamValueNode.cpp \
		       CLoadedSound.cpp  \
		       COSSSoundPlayer.cpp \
		       CSoundManager.cpp \
		       CSoundManagerClient.cpp \
		       CSoundPlayerChannel.cpp \
		       CTrigger.cpp \
		       ClibaudiofileSound.cpp \
		       Cold_rezSound.cpp \
		       CrezSound.cpp \
		       CrezSoundRecorder.cpp \
		       file.cpp  \
		       playcontrols.cpp 


## The above macros are enough to build the package, but putting in the
## dependancies makes the developer's job easier.

AAction.o: AAction.cpp \
	AAction.h \
	ASound.h \
	ASoundPlayer.h \
	AStatusComm.h \
	CEnvelope.h \
	CGraphParamValueNode.h \
	CLoadedSound.h \
	CSoundManagerClient.h \
	CSoundManager.h \
	CSoundPlayerChannel.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

ASound.o: ASound.cpp \
	ASound.h \
	AStatusComm.h \
	$(top_srcdir)/src/misc/cc++/path.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

ASoundFileManager.o: ASoundFileManager.cpp \
	ASoundFileManager.h \
	AAction.h \
	ASound.h \
	ASoundPlayer.h \
	AStatusComm.h \
	CEnvelope.h \
	CGraphParamValueNode.h \
	CLoadedSound.h \
	CSoundManagerClient.h \
	CSoundManager.h \
	CSoundPlayerChannel.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/path.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

ASoundPlayer.o: ASoundPlayer.cpp \
	ASoundPlayer.h \
	ASound.h \
	CSoundPlayerChannel.h \
	CEnvelope.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

ASoundRecorder.o: ASoundRecorder.cpp  \
	ASoundRecorder.h \
	ASound.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

AStatusComm.o: AStatusComm.cpp \
	AStatusComm.h  

CauSound.o: CauSound.cpp \
	CauSound.h

ClibaudiofileSound.o: ClibaudiofileSound.cpp \
	ClibaudiofileSound.h \
	AStatusComm.h \
	ASound.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/misc/cc++/path.h 

CEnvelope.o: CEnvelope.cpp  \
	CEnvelope.h

CLoadedSound.o: CLoadedSound.cpp \
	CLoadedSound.h \
	AAction.h \
	ASound.h \
	ASoundPlayer.h \
	AStatusComm.h \
	CEnvelope.h \
	CGraphParamValueNode.h \
	CSoundManagerClient.h \
	CSoundManager.h \
	CSoundPlayerChannel.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CGraphParamValueNode.o: CGraphParamValueNode.cpp \
	CGraphParamValueNode.h \
	ASound.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h 

COSSSoundPlayer.o: COSSSoundPlayer.cpp \
	COSSSoundPlayer.h \
	ASound.h \
	ASoundPlayer.h \
	CEnvelope.h \
	CSoundPlayerChannel.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CrawSound.o: CrawSound.cpp \
	CrawSound.h

CrezSound.o: CrezSound.cpp \
	CrezSound.h \
	ASound.h \
	CrezSound.cpp \
	CrezSound.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CrezSoundRecorder.o: CrezSoundRecorder.cpp \
	CrezSoundRecorder.h \
	ASound.h \
	ASoundRecorder.h  \
	CrezSound.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h  \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CSoundManagerClient.o: CSoundManagerClient.cpp \
	CSoundManagerClient.h \
	ASound.h \
	CSoundManager.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h  \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CSoundManager.o: CSoundManager.cpp \
	CSoundManager.h \
	ASound.h \
	ClibaudiofileSound.h \
	Cold_rezSound.h \
	CrezSound.h \
	CSoundManagerClient.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CSoundPlayerChannel.o: CSoundPlayerChannel.cpp \
	CSoundPlayerChannel.h \
	ASound.h \
	ASoundPlayer.h \
	CEnvelope.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CTrigger.o: CTrigger.cpp \
	CTrigger.h

Cold_rezSound.o: Cold_rezSound.cpp \
	Cold_rezSound.h \
	ASound.h \
	Cold_rezSound.cpp \
	Cold_rezSound.h \
	$(top_srcdir)/src/misc/cc++/path.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 


CStubAction.o: CStubAction.cpp \
	CStubAction.h \
	AAction.h \
	ASound.h \
	ASoundPlayer.h \
	AStatusComm.h \
	CEnvelope.h \
	CGraphParamValueNode.h \
	CSoundPlayerChannel.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

file.o: file.cpp \
	file.h \
	AAction.h \
	ASoundFileManager.h \
	ASound.h \
	ASoundPlayer.h \
	AStatusComm.h \
	CEnvelope.h \
	CGraphParamValueNode.h \
	CLoadedSound.h \
	CSoundManagerClient.h \
	CSoundManager.h \
	CSoundPlayerChannel.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

playcontrols.o: playcontrols.cpp \
	playcontrols.h \
	AAction.h \
	ASound.h \
	ASoundFileManager.h \
	ASoundPlayer.h \
	AStatusComm.h \
	CEnvelope.h \
	CGraphParamValueNode.h \
	CLoadedSound.h \
	CSoundManager.h \
	CSoundManagerClient.h \
	CSoundPlayerChannel.h \
	CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h

##
## End backend/Makefile.am

