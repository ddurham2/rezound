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

#include "parse_segment_cues.h"

CSaveAsMultipleFilesAction::CSaveAsMultipleFilesAction(const CActionSound &actionSound,ASoundFileManager *_soundFileManager,const string _directory,const string _filenamePrefix,const string _filenameSuffix,const string _extension,bool _openSavedSegments,unsigned _segmentNumberOffset,bool _selectionOnly) :
	AAction(actionSound),
	soundFileManager(_soundFileManager),
	directory(_directory),
	filenamePrefix(_filenamePrefix),
	filenameSuffix(_filenameSuffix),
	extension(_extension),
	openSavedSegments(_openSavedSegments),
	segmentNumberOffset(_segmentNumberOffset),
	selectionOnly(_selectionOnly)
{
}

CSaveAsMultipleFilesAction::~CSaveAsMultipleFilesAction()
{
}

bool CSaveAsMultipleFilesAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const CSound &sound=*(actionSound.sound);
	const sample_pos_t selectionStart= selectionOnly ? actionSound.start : 0;
	const sample_pos_t selectionLength= selectionOnly ? actionSound.selectionLength() : sound.getLength();

	class CBuildFilename : public FBuildFilename
	{
	public:
		const string prefix,suffix;
		CBuildFilename(const string _prefix,const string _suffix) :
			prefix(_prefix),
			suffix(_suffix)
		{
		}
		virtual ~CBuildFilename() {}

		const string operator()(const string str)
		{
			return prefix+str+suffix;
		}
	};

	const string prefix=directory+string(&CPath::dirDelim,1)+filenamePrefix;
	const string suffix=filenameSuffix+extension;
	CBuildFilename BuildFilename(prefix,suffix);

	segments_t segments=parse_segment_cues(sound,selectionStart,selectionLength,BuildFilename);

	if(segments.size()<=0)
		throw EUserMessage(_("No appropriately named cues found to define segments to save.  See the 'explain' button on the previous window for how to name the cues."));

	
	// translate the '#'s to the track numbers
	size_t track=segmentNumberOffset;
	size_t alignBy=(size_t)floor(log10((float)segments.size()))+1;
	for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
	{
		string::size_type p;
		while((p=i->first.find("#"))!=string::npos) // while we find a '#' in the string
			i->first=i->first.replace(p,1,istring(track,alignBy,true)); // replace it with the zero padded track number
		track++;
	}

#warning need to i18n this but it probably needs to be done better than just putting _() around each string literal
	// show the results and ask the user if they want to continue
	{
		string msg="These are the files about to be created...\n\n";
		for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
			msg+="from "+sound.getTimePosition(i->second.first)+" to "+sound.getTimePosition(i->second.second)+" ("+sound.getTimePosition(i->second.second-i->second.first)+") as '"+i->first+"'\n";
		msg+="\nDo you want to continue?";

		if(Question(msg,yesnoQues,false)!=yesAns)
			return false;
	}


	// proceed to save files
	for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
	{
		soundFileManager->savePartial(&sound,i->first,i->second.first,i->second.second-i->second.first);

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


const string CSaveAsMultipleFilesAction::getExplanation()
{
	return _("\n\
To save a large file (or just a selection from it) as several smaller segments you can create cues that define the segments and then click on \"Save As Multiple Files\" under the \"File\" menu.\n\
\n\
In general, cues can be named '(' and ')' to define the beginning and end of each segment to be saved.\n\
However, a ')' cue (closing a prior '(' cue) can be ommitted if a segment is to end at the beginning of the next segment.\n\
The very last ')' cue can also be ommitted if the last defined segment is to end at the end of the original audio file (or the end of the selection).\n\
Furthermore, the '(' cue can optionally be named '(xyz' if 'xyz' is to be included in the segment's filename.\n\
\n\
There are several parameters in the dialog that is displayed after selecting \"Save As Multiple Files\" under the \"File\" menu.\n\
The \"Save to Directory\" parameter is the directory to place each segment file into.\n\
The \"Filename Prefix\" will be placed before the optional 'xyz' from the '(xyz' cue name.\n\
The \"Filename Suffix\" will be placed after of the optional 'xyz' from the '(xyz' cue name.\n\
The \"Format\" parameter in specifies what audio format the segments should be saved as.\n\
After a segment's filename is formed by putting together, [directory]/[prefix][xyz][suffix].[format extension], if it contains '#' then the '#' will be replaced with a number based on the order that the segments are defined.\n\
    For instance: \"/home/john/sounds/track #.wav\" will be changed to \"/home/john/sounds/track 1.wav\" for the first segment that is saved, and all the subsequent segments will contain an increasing number.\n\
    Thus, you should use a '#' in either the save to directory, filename prefix, xyz, or the filename suffix to create unique filenames when saving the segments.\n\
The \"Segment Number Start\" parameter can be changed from '1' to start the '#' substitutions at something different.\n\
The \"Open Saved Segments\" can be selected simply if you want to open the segments after they have been saved.\n\
The \"Applies to\" parameter indicates if the action should regard only the current selection or the entire file.\n\
");
}


// ------------------------------

CSaveAsMultipleFilesActionFactory::CSaveAsMultipleFilesActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Save As Multiple Files"),"",NULL,dialog,false,false)
{
}

CSaveAsMultipleFilesActionFactory::~CSaveAsMultipleFilesActionFactory()
{
}

CSaveAsMultipleFilesAction *CSaveAsMultipleFilesActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
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
		actionParameters->getUnsignedParameter("Segment Number Start"),
		(actionParameters->getUnsignedParameter("Applies to")==1) // 0 -> "Entire File", 1 -> "Selection Only"
	);
}

