# ReZound frontend_fox/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2002 - Anthony Ventimiglia
##   
##   This file is part of ReZound, an audio editing application, but
##   could be used for other applications which could use the PoolFile
##   class's functionality.
##   
##   PoolFile is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published
##   by the Free Software Foundation; either version 2 of the License,
##   or (at your option) any later version.
##   
##   PoolFile is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##   
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
################################################################################

include $(top_srcdir)/config/am_include.mk

noinst_LTLIBRARIES=libfrontend.la
libfrontend_la_SOURCES= CActionButton.cpp \
		 CChannelSelectDialog.cpp \
		 CConstantParamActionDialog.cpp \
		 CCueDialog.cpp \
		 CCueListDialog.cpp \
		 CEditToolbar.cpp \
		 CGraphParamActionDialog.cpp \
		 CMainWindow.cpp \
		 CNewSoundDialog.cpp \
		 CPasteChannelsDialog.cpp \
		 CProgressDialog.cpp \
		 CSoundFileManager.cpp \
		 CSoundListWindow.cpp \
		 CSoundWindow.cpp \
		 CStatusComm.cpp \
		 CUserNotesDialog.cpp \
		 drawPortion.cpp \
		 EditActionDialogs.cpp \
		 EffectActionDialogs.cpp \
		 FXConstantParamValue.cpp \
		 FXGraphParamValue.cpp \
		 FXModalDialogBox.cpp \
		 FXRezWaveView.cpp \
		 initialize.cpp \
		 RemasterActionDialogs.cpp \
		 rememberShow.cpp \
		 settings.cpp 

## bin_PROGRAMS is the macro that tells automake the name of the programs to
## install in the bin directory (/usr/local/bin) by default. By setting
## --prefix= at configure time the user can change this (eg: ./configure
## --prefix=/usr will install ReZound under /usr/bin/ReZound )
bin_PROGRAMS=ReZound

## for every name listed under bin_PROGRAMS, you have a prog_SOURCES. This lists
## all the sources in the current directory that are used to build ReZound.
ReZound_SOURCES= main.cpp 

## ReZound_LDADD is a list of extras to pass at link time. All the objects
## created by the above ReZound_SOURCES are automatically linked in, so here I
## list object files from other directories as well as flags passed to the
## linker. By using convienience libraries, it makes this part much easier. 
## It's up to each subdirectory's Makefile to build it's .la library.
## notice we use $(top_builddir) for the modules. This allows building in a 
## separate directory
ReZound_LDADD= libfrontend.la\
	$(top_builddir)/src/misc/cc++/libmiscCC.la \
	$(top_builddir)/src/misc/CNestedDataFile/libmiscCN.la \
	$(top_builddir)/src/PoolFile/libPoolFile.la \
	$(top_builddir)/src/backend/Edits/libEdits.la \
	$(top_builddir)/src/backend/Effects/libEffects.la \
	$(top_builddir)/src/backend/Remaster/libRemaster.la \
	$(top_builddir)/src/backend/libbackend.la

EXTRA_DIST=\
	CActionButton.h \
	CChannelSelectDialog.h \
	CConstantParamActionDialog.h \
	CCueDialog.h \
	CCueListDialog.h \
	CEditToolbar.h \
	CGraphParamActionDialog.h \
	CMainWindow.h \
	CNewSoundDialog.h \
	CPasteChannelsDialog.h \
	CProgressDialog.h \
	CSoundFileManager.h \
	CSoundListWindow.h \
	CSoundWindow.h \
	CStatusComm.h \
	CUserNotesDialog.h \
	EditActionDialogs.h \
	EffectActionDialogs.h \
	FXConstantParamValue.h \
	FXGraphParamValue.h \
	FXModalDialogBox.h \
	FXRezWaveView.h \
	RemasterActionDialogs.h \
	drawPortion.h \
	initialize.h \
	rememberShow.h \
	settings.h 

#
## All of the above is really all we need to build the package, but if we add
## dependancies below It makes a developers life easier.  The nice thing is we
## don't have to give any Compilation commands, that's all taken care of by
## autotools
##
CActionButton.o: CActionButton.cpp \
	CActionButton.h \
	CSoundFileManager.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/ASoundFileManager.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h 

ChannelSelectDialog.o: CChannelSelectDialog.cpp \
	CChannelSelectDialog.h \
	FXModalDialogBox.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h 

CConstantParamActionDialog.o: CConstantParamActionDialog.cpp \
	CConstantParamActionDialog.h \
	FXConstantParamValue.h \
	FXModalDialogBox.h \
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

CCueDialog.o: CCueDialog.cpp \
	CCueDialog.h \
	FXModalDialogBox.h \
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

CCueListDialog.o: CCueListDialog.cpp \
	CCueListDialog.h \
	CCueDialog.h \
	FXModalDialogBox.h \
	rememberShow.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/Edits/CCueAction.h \
	$(top_srcdir)/src/images/images.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CEditToolbar.o: CEditToolbar.cpp \
	CEditToolbar.h \
	rememberShow.h \
	CActionButton.h \
	CChannelSelectDialog.h \
	CConstantParamActionDialog.h \
	CGraphParamActionDialog.h \
	CPasteChannelsDialog.h \
	EditActionDialogs.h \
	FXConstantParamValue.h \
	FXGraphParamValue.h \
	FXModalDialogBox.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/Edits/CCopyCutDeleteEdit.h \
	$(top_srcdir)/src/backend/Edits/CCropEdit.h \
	$(top_srcdir)/src/backend/Edits/CInsertSilenceEdit.h \
	$(top_srcdir)/src/backend/Edits/CMuteEdit.h \
	$(top_srcdir)/src/backend/Edits/CPasteEdit.h \
	$(top_srcdir)/src/backend/Edits/CRotateEdit.h \
	$(top_srcdir)/src/backend/Edits/CSelectionEdit.h \
	$(top_srcdir)/src/backend/Edits/EditActions.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h  

CGraphParamActionDialog.o: CGraphParamActionDialog.cpp \
	CGraphParamActionDialog.h \
	FXGraphParamValue.h \
	FXModalDialogBox.h \
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

CMainWindow.o: CMainWindow.cpp \
	CMainWindow.h \
	CActionButton.h \
	CChannelSelectDialog.h \
	CConstantParamActionDialog.h \
	CEditToolbar.h \
	CGraphParamActionDialog.h \
	CSoundFileManager.h \
	CUserNotesDialog.h \
	EffectActionDialogs.h \
	FXConstantParamValue.h \
	FXGraphParamValue.h \
	FXModalDialogBox.h \
	RemasterActionDialogs.h \
	rememberShow.h \
	settings.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASoundFileManager.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/Effects/CChangeAmplitudeEffect.h \
	$(top_srcdir)/src/backend/Effects/CChangeRateEffect.h \
	$(top_srcdir)/src/backend/Effects/CDelayEffect.h \
	$(top_srcdir)/src/backend/Effects/CFlangeEffect.h \
	$(top_srcdir)/src/backend/Effects/CReverseEffect.h \
	$(top_srcdir)/src/backend/Effects/CStaticReverbEffect.h \
	$(top_srcdir)/src/backend/Effects/CTestEffect.h \
	$(top_srcdir)/src/backend/Effects/EffectActions.h \
	$(top_srcdir)/src/backend/file.h \
	$(top_srcdir)/src/backend/playcontrols.h \
	$(top_srcdir)/src/backend/Remaster/CNoiseGateAction.h \
	$(top_srcdir)/src/backend/Remaster/CRemoveDCAction.h \
	$(top_srcdir)/src/backend/Remaster/CUnclipAction.h \
	$(top_srcdir)/src/backend/Remaster/RemasterActions.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h  

CNewSoundDialog.o: CNewSoundDialog.cpp \
	CNewSoundDialog.h \
	CMainWindow.h \
	CStatusComm.h \
	FXModalDialogBox.h \
	settings.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/misc/cc++/path.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CPasteChannelsDialog.o: CPasteChannelsDialog.cpp \
	CPasteChannelsDialog.h \
	FXModalDialogBox.h \
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

CProgressDialog.o: CProgressDialog.cpp \
	CProgressDialog.h 

CSoundFileManager.o: CSoundFileManager.cpp \
	CSoundFileManager.h \
	CMainWindow.h \
	CNewSoundDialog.h \
	CSoundListWindow.h \
	CSoundWindow.h \
	FXModalDialogBox.h \
	FXRezWaveView.h \
	settings.h \
	$(top_srcdir)/src/backend/ASoundFileManager.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/path.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

CSoundListWindow.o: CSoundListWindow.cpp \
	CSoundListWindow.h \
	CSoundWindow.h \
	FXRezWaveView.h \
	rememberShow.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h

CSoundWindow.o: CSoundWindow.cpp \
	CSoundWindow.h \
	CCueDialog.h \
	CSoundFileManager.h \
	FXModalDialogBox.h \
	CCueListDialog.h \
	settings.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASoundFileManager.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/Edits/CCueAction.h \
	$(top_srcdir)/src/backend/file.h \
	$(top_srcdir)/src/images/images.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h  

CStatusComm.o: CStatusComm.cpp \
	CStatusComm.h \
	CMainWindow.h \
	CProgressDialog.h \
	$(top_srcdir)/src/backend/AStatusComm.h 

CUserNotesDialog.o: CUserNotesDialog.cpp \
	CUserNotesDialog.h \
	FXModalDialogBox.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

drawPortion.o: drawPortion.cpp \
	drawPortion.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

EditActionDialogs.o: EditActionDialogs.cpp \
	EditActionDialogs.h \
	CConstantParamActionDialog.h \
	CGraphParamActionDialog.h \
	FXConstantParamValue.h \
	FXGraphParamValue.h \
	FXModalDialogBox.h \
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

EffectActionDialogs.o: EffectActionDialogs.cpp \
	EffectActionDialogs.h \
	CConstantParamActionDialog.h \
	CGraphParamActionDialog.h \
	FXConstantParamValue.h \
	FXGraphParamValue.h \
	FXModalDialogBox.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/unit_conv.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

FXConstantParamValue.o: FXConstantParamValue.cpp \
	FXConstantParamValue.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

FXGraphParamValue.o: FXGraphParamValue.cpp \
	FXGraphParamValue.h \
	drawPortion.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

FXModalDialogBox.o: FXModalDialogBox.cpp \
	FXModalDialogBox.h \
	rememberShow.h \
	$(top_srcdir)/src/images/images.h 

FXRezWaveView.o: FXRezWaveView.cpp \
	FXRezWaveView.h \
	drawPortion.h \
	settings.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

initialize.o: initialize.cpp \
	initialize.h \
	CSoundFileManager.h \
	CStatusComm.h \
	settings.h \
	$(top_srcdir)/src/backend/file.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASoundFileManager.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/COSSSoundPlayer.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

main.o: main.cpp \
	CChannelSelectDialog.h \
	CCueDialog.h \
	CCueListDialog.h \
	CEditToolbar.h \
	CMainWindow.h \
	CNewSoundDialog.h \
	CPasteChannelsDialog.h \
	CSoundFileManager.h \
	CSoundListWindow.h \
	CStatusComm.h \
	CUserNotesDialog.h \
	FXModalDialogBox.h \
	initialize.h \
	settings.h \
	$(top_srcdir)/src/backend/ASoundFileManager.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CLoadedSound.h \
	$(top_srcdir)/src/backend/CSoundManagerClient.h \
	$(top_srcdir)/src/backend/CSoundManager.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/images/images.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

RemasterActionDialogs.o: RemasterActionDialogs.cpp \
	RemasterActionDialogs.h \
	CConstantParamActionDialog.h \
	FXModalDialogBox.h \
	FXConstantParamValue.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/unit_conv.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

rememberShow.o: rememberShow.cpp \
	rememberShow.h \
	settings.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/DiskTable/CStringDiskTable.h \
	$(top_srcdir)/src/PoolFile/DiskTable/TDiskTable.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h 

settings.o: settings.cpp \
	settings.h


