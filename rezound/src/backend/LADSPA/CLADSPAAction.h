/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#ifndef __CLADSPAAction_H__
#define __CLADSPAAction_H__

#include "../../../config/common.h"


class CLADSPAAction;
class CLADSPAActionFactory;
class ASoundFileManager;

#include "../AAction.h"
#include "../CActionParameters.h"

#include <vector>

#include "ladspa.h"

class CLADSPAAction : public AAction
{
public:
	CLADSPAAction(const LADSPA_Descriptor *desc,const CActionSound &actionSound,const CActionParameters &actionParameters);
	virtual ~CLADSPAAction();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const LADSPA_Descriptor *desc;
	CActionParameters actionParameters;
	const CPluginMapping channelMapping;
	ASoundFileManager *soundFileManager;

	vector<unsigned> inputAudioPorts;
	vector<unsigned> outputAudioPorts;
	vector<unsigned> inputControlPorts;
	vector<unsigned> outputControlPorts; // I don't use these, but I have to bind to them to prevent segfaults in some plugins that assume I've connected to them

	int restoreChannelsTempAudioPoolKey; // used if needing to restore removed channels
	CSound *origSound;
};

class CLADSPAActionFactory : public AActionFactory
{
public:
	CLADSPAActionFactory(const LADSPA_Descriptor *desc,AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CLADSPAActionFactory();

	CLADSPAAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;

	const LADSPA_Descriptor *getDescriptor() const { return desc; }

private:
	const LADSPA_Descriptor *desc;
};

#endif
