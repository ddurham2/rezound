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

#include "CSaveAsMultipleFilesAction.h"

#include <map>
#include <CPath.h>

#include "../CActionParameters.h"
#include "../AStatusComm.h"
#include "../ASoundFileManager.h"
#include "../ASoundTranslator.h"

CSaveAsMultipleFilesAction::CSaveAsMultipleFilesAction(const CActionSound actionSound,ASoundFileManager *_soundFileManager,const string _directory,const string _filenamePrefix,const string _filenameSuffix,const string _extension,bool _openSavedSegments,unsigned _segmentNumberOffset) :
	AAction(actionSound),
	soundFileManager(_soundFileManager),
	directory(_directory),
	filenamePrefix(_filenamePrefix),
	filenameSuffix(_filenameSuffix),
	extension(_extension),
	openSavedSegments(_openSavedSegments),
	segmentNumberOffset(_segmentNumberOffset)
{
}

CSaveAsMultipleFilesAction::~CSaveAsMultipleFilesAction()
{
}

bool CSaveAsMultipleFilesAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	// built a list of a cues ordered by the time and keeping only the cues that start with '(' or ')'
	multimap<sample_pos_t,size_t> timeOrderedCueIndex;
	for(size_t t=0;t<actionSound.sound->getCueCount();t++)
	{
		const string cueName=actionSound.sound->getCueName(t);
		if(cueName.size()>0 && (cueName[0]=='(' || cueName[0]==')'))
			timeOrderedCueIndex.insert(pair<sample_pos_t,size_t>(actionSound.sound->getCueTime(t),t));
	}

	vector<pair<string,pair<sample_pos_t,sample_pos_t> > > segments;

	const string prefix=directory+string(&CPath::dirDelim,1)+filenamePrefix;
	const string suffix=filenameSuffix+extension;

	/*
		State Machine:
		                "("
		                --- 
		               |   |
		         "("   V   |
		----> 0 -----> 1 -- 
		      ^         
		      |        |
		      |________|
		         ")"
	*/

	string filename="";
	sample_pos_t start=0;
	int state=0;
	for(multimap<sample_pos_t,size_t>::iterator i=timeOrderedCueIndex.begin();i!=timeOrderedCueIndex.end();i++)
	{
		const string cueName=actionSound.sound->getCueName(i->second);
		const sample_pos_t time=actionSound.sound->getCueTime(i->second);
		const char c=cueName[0];
	
		switch(state)
		{
		case 0:
			if(c=='(')
			{
				filename=prefix+cueName.substr(1)+suffix;
				start=time;
		
				state=1;
			}
			else
				throw runtime_error(string(__func__)+" -- syntax error in cue names -- expected to find a cue beginning with '(' at time: "+actionSound.sound->getTimePosition(time));

			break;

		case 1:
			if(c=='(')
			{
				segments.push_back(pair<string,pair<sample_pos_t,sample_pos_t> >(filename,pair<sample_pos_t,sample_pos_t>(start,time>0 ? time-1 : time)));

				filename=prefix+cueName.substr(1)+suffix;
				start=time;

				state=1;
			}
			else if(c==')')
			{
				segments.push_back(pair<string,pair<sample_pos_t,sample_pos_t> >(filename,pair<sample_pos_t,sample_pos_t>(start,time)));
				// clear out for test after the loop
				filename="";
				start=0;

				state=0;
			}
			else
				throw runtime_error(string(__func__)+" -- syntax error in cue names -- expected to find a cue beginning with '(' or ')' at time: "+actionSound.sound->getTimePosition(time));

			break;
	
		default:
			throw runtime_error(string(__func__)+" -- internal error -- invalid state: "+istring(state));
		}
	}

	// if a (... was encourtered without a closing ) then we assume to save to the end of file
	if(filename!="" && start!=0)
		segments.push_back(pair<string,pair<sample_pos_t,sample_pos_t> >(filename,pair<sample_pos_t,sample_pos_t>(start,actionSound.sound->getLength()-1)));


	if(segments.size()<=0)
	{
		Message("No appropriately named cues found to define segments to save.  See the 'explain' button on the previous window for how to name the cues.");
		return false;
	}

	
	// translate the '#'s to the track numbers
	size_t track=segmentNumberOffset;
	size_t alignBy=(size_t)floor(log10((float)segments.size()))+1;
	for(vector<pair<string,pair<sample_pos_t,sample_pos_t> > >::iterator i=segments.begin();i!=segments.end();i++)
	{
		string::size_type p;
		while((p=i->first.find("#"))!=string::npos) // while we find a '#' in the string
			i->first=i->first.replace(p,1,istring(track,alignBy,true)); // replace it with the zero padded track number
		track++;
	}

	// show the results and ask the user if they want to continue
	string msg="These are the files about to be created...\n\n";
	for(vector<pair<string,pair<sample_pos_t,sample_pos_t> > >::iterator i=segments.begin();i!=segments.end();i++)
		msg+="from "+actionSound.sound->getTimePosition(i->second.first)+" to "+actionSound.sound->getTimePosition(i->second.second)+" ("+actionSound.sound->getTimePosition(i->second.second-i->second.first)+") as '"+i->first+"'\n";
	msg+="\nDo you want to continue?";

	if(Question(msg,yesnoQues,false)!=yesAns)
		return false;


	// proceed to save files
	for(vector<pair<string,pair<sample_pos_t,sample_pos_t> > >::iterator i=segments.begin();i!=segments.end();i++)
	{
		soundFileManager->savePartial(actionSound.sound,i->first,i->second.first,i->second.second-i->second.first);

		if(openSavedSegments)
			soundFileManager->open(i->first);
	}
	
	return true;
}

AAction::CanUndoResults CSaveAsMultipleFilesAction::canUndo(const CActionSound &actionSound) const
{
	return curNA;
}

void CSaveAsMultipleFilesAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	// not applicable
}

bool CSaveAsMultipleFilesAction::doesWarrantSaving() const
{
	return false;
}


// ------------------------------

CSaveAsMultipleFilesActionFactory::CSaveAsMultipleFilesActionFactory(AActionDialog *normalDialog) :
	AActionFactory("Save As Multiple Files","Save As Multiple Files",false,NULL,normalDialog,NULL,false,false)
{
}

CSaveAsMultipleFilesActionFactory::~CSaveAsMultipleFilesActionFactory()
{
}

CSaveAsMultipleFilesAction *CSaveAsMultipleFilesActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	const string formatName=ASoundTranslator::getFlatFormatList()[actionParameters->getUnsignedParameter("Format")];
	return new CSaveAsMultipleFilesAction(
		actionSound,
		actionParameters->getSoundFileManager(),
		actionParameters->getStringParameter("Save to Directory"),
		actionParameters->getStringParameter("Filename Prefix"),
		actionParameters->getStringParameter("Filename Suffix"),
		"."+formatName.substr(0,formatName.find(" ")), // cut out only the first few chars (which is the extension
		actionParameters->getBoolParameter("Open Saved Segments"),
		actionParameters->getUnsignedParameter("Segment Number Start")
	);
}

