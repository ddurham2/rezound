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

#include "CBurnToCDAction.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"
#include "../DSP/TSoundStretcher.h"

#include <istring>
#include <CPath.h>

#include <map>
#include <vector>
#include <utility>

#include <unistd.h> /* for unlink() and getuid()*/
#include <stdlib.h> /* for system() */
#include <sys/wait.h> /* for WEXITSTATUS */

#include "parse_segment_cues.h"

CBurnToCDAction::CBurnToCDAction(const CActionSound &_actionSound,const string _tempSpaceDir,const string _pathTo_cdrdao,const unsigned _burnSpeed,const unsigned _gapBetweenTracks,const string _device,const string _extra_cdrdao_options,const bool _selectionOnly,const bool _testOnly) :
	AAction(_actionSound),
	tempSpaceDir(_tempSpaceDir),
	pathTo_cdrdao(_pathTo_cdrdao),
	burnSpeed(_burnSpeed),
	gapBetweenTracks(_gapBetweenTracks),
	device(_device),
	extra_cdrdao_options(_extra_cdrdao_options),
	selectionOnly(_selectionOnly),
	testOnly(_testOnly)
{
}

CBurnToCDAction::~CBurnToCDAction()
{
}

bool CBurnToCDAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const CSound &sound=*(actionSound.sound);
	const sample_pos_t selectionStart= selectionOnly ? actionSound.start : 0;
	const sample_pos_t selectionLength= selectionOnly ? actionSound.selectionLength() : sound.getLength();
	const sample_pos_t fullLength=sound.getLength();
	const unsigned channelCount=sound.getChannelCount();
	const unsigned sampleRate=sound.getSampleRate();

	if(!CPath(pathTo_cdrdao).exists())
		throw EUserMessage(_("cdrdao not found in the given path: ")+pathTo_cdrdao);

	class CBuildFilename : public FBuildFilename
	{
	public:
		const string prefix,suffix;
		int counter;
		CBuildFilename(const string _prefix,const string _suffix) :
			prefix(_prefix),
			suffix(_suffix),
			counter(0)
		{
		}
		virtual ~CBuildFilename() {}

		const string operator()(const string str)
		{
			return prefix+istring(counter++)+suffix;
		}
	};

	const string prefix=tempSpaceDir+"/rezound_temp_burning-"+istring((int)getuid())+"-";
	const string suffix=".raw";
	CBuildFilename BuildFilename(prefix,suffix);

	segments_t segments=parse_segment_cues(sound,selectionStart,selectionLength,BuildFilename);

	if(segments.size()>99)
		throw EUserMessage(_("Too many tracks defined by cues.  99 is the maximum."));
	else if(segments.size()<=0)
	{ // no '('... cues were found ask to create one long track
		if(Question(_("No appropriately named cues found to define tracks to burn.  See the 'explain' button on the previous window for how to name the cues.\nWould you like to continue burning the audio as one track?"),yesnoQues)!=yesAns)
			return false;
		else
			segments.push_back(make_pair(prefix+"0"+suffix,make_pair(selectionStart,selectionStart+selectionLength)));
	}
	
	// show the results and ask the user if they want to continue
	{
		string msg="These are the track(s) about to be created...\n\n";
		int counter=1;
		sample_fpos_t totalBurnLength=0;
		for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
		{
			const sample_pos_t start=i->second.first;
			const sample_pos_t stop=i->second.second;
			const sample_pos_t length=stop-start;

			const string trackStr="from "+sound.getTimePosition(start)+" to "+sound.getTimePosition(stop)+" ("+sound.getTimePosition(length)+") as track "+istring(counter++)+"\n";

			// if track length is less than 4 seconds complain
			if( (((sample_fpos_t)length)/sampleRate*44100) < (4*44100))
				throw EUserMessage(trackStr+"\n"+_("Track is less than 4 seconds long."));

			msg+=trackStr;
			totalBurnLength+=length;
		}

		// convert from current sample rate to what it will be
		totalBurnLength=totalBurnLength/sampleRate*44100;

		// add in silence between tracks
		totalBurnLength+=(gapBetweenTracks*44100)*(segments.size()-1);

		// convert from samples to minutes
		totalBurnLength=totalBurnLength/44100/60;

		msg+="\n";
		msg+=_("Gap between tracks: ")+istring(gapBetweenTracks)+"s\n";
		msg+=_("Total Burn Length: ")+istring(totalBurnLength,3,1)+" "+_("minutes")+"\n";
		msg+="\n";
		msg+=_("Do you want to continue?");

		if(totalBurnLength>74)
		{
			if(totalBurnLength<=80)
			{
				if(Question(_("Total data to be burned is ")+istring(totalBurnLength)+" "+_("minutes")+".  "+_("You will need an 80 minute CD\n")+_("Do you want to continue?"),yesnoQues)!=yesAns)
					return false;
			}
			else
			{
				Error(_("Total data to be burned is too large: ")+istring(totalBurnLength,3,1)+" "+_("minutes"));
				return false;
			}
		}

		if(Question(msg,yesnoQues,false)!=yesAns)
			return false;
	}


	if(Question(_("About to write data to burn.  Afterwards burning will begin.  Watch standard output/error for progress information, or just wait for cdrdao to finish.")+string("\n")+_("Do you want to continue?"),yesnoQues)!=yesAns)
		return false;


	// proceed to save files
	{
		CStatusBar statusBar(_("Writing Data to Burn"),0,(sample_pos_t)((sample_fpos_t)selectionLength/sampleRate*44100),true);
		sample_pos_t statusTotal=0;
		for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
		{
			try
			{
				const string filename=i->first;
				const sample_pos_t srcStart=i->second.first;
				const sample_pos_t srcStop=i->second.second;
				const sample_pos_t srcLength=srcStop-srcStart;
				const sample_fpos_t fDestLength=(sample_fpos_t)srcLength/sampleRate*44100;
				const sample_pos_t destLength=(sample_pos_t)fDestLength;
				const bool paddedForInterpolation=srcStop<(fullLength-8);
		
				FILE *f=fopen(filename.c_str(),"w");
				if(f==NULL)
					throw runtime_error(string(__func__)+" -- error creating file: "+filename);

		/* converts a smaple_t into a range-clipped 16bit value */
#define make_16(s) (int16_t)(max(-32767,min(32767,(int)((((float)(s))/MAX_SAMPLE)*32767))));

				// write out as stereo, 16bit, 44100
				if(channelCount==1)
				{	/* write 1 channel as stereo */
					const CRezPoolAccesser t_src=sound.getAudio(0);
					TSoundStretcher<CRezPoolAccesser> src(t_src,srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);

					for(sample_pos_t t=0;t<destLength;t++)
					{
						int16_t s=make_16(src.getSample());
						fwrite(&s,2,1,f);
						fwrite(&s,2,1,f);
						if(ferror(f))
						{
							const int errNO=errno;
							throw runtime_error(string(__func__)+" -- error writing to file: "+filename+" -- "+strerror(errNO));
						}

						if(statusBar.update(statusTotal++))
							throw EUserMessage(""); /* won't cause a popup */
					}
				}
				else if(channelCount==2)
				{
					const CRezPoolAccesser t_srcL=sound.getAudio(0);
					const CRezPoolAccesser t_srcR=sound.getAudio(1);
					TSoundStretcher<CRezPoolAccesser> srcL(t_srcL,srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);
					TSoundStretcher<CRezPoolAccesser> srcR(t_srcR,srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);

					for(sample_pos_t t=0;t<destLength;t++)
					{
						int16_t left=make_16(srcL.getSample());
						int16_t right=make_16(srcR.getSample());

						fwrite(&left,2,1,f);
						fwrite(&right,2,1,f);
						if(ferror(f))
						{
							const int errNO=errno;
							throw runtime_error(string(__func__)+" -- error writing to file: "+filename+" -- "+strerror(errNO));
						}

						if(statusBar.update(statusTotal++))
							throw EUserMessage(""); /* won't cause a popup */
					}
				}
				else
				{	/* write N number of channels as stereo where N>2 */
					const CRezPoolAccesser *t_src[MAX_CHANNELS]={0};
					for(unsigned t=0;t<channelCount;t++)
						t_src[t]=new CRezPoolAccesser(sound.getAudio(t));

					TSoundStretcher<CRezPoolAccesser> *src[MAX_CHANNELS]={0};
					for(unsigned t=0;t<channelCount;t++)
						src[t]=new TSoundStretcher<CRezPoolAccesser>(*(t_src[t]),srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);

					try /* ??? really need two separate try/catches .. or a master one around the whole thing and deallocate these objects there (declaring src[] and t_src[] outside that try (or figure out how to do it with a smart pointer */
					{
						for(sample_pos_t t=0;t<destLength;t++)
						{
							mix_sample_t tleft=0;
							mix_sample_t tright=0;
							for(unsigned i=0;i<channelCount;i++)
							{
								if(i%2)
									tright+=src[i]->getSample();
								else
									tleft+=src[i]->getSample();
							}
		
							int16_t left=make_16(tleft);
							int16_t right=make_16(tright);
		
							fwrite(&left,2,1,f);
							fwrite(&right,2,1,f);
							if(ferror(f))
							{
								const int errNO=errno;
								throw runtime_error(string(__func__)+" -- error writing to file: "+filename+" -- "+strerror(errNO));
							}

							if(statusBar.update(statusTotal++))
								throw EUserMessage(""); /* won't cause a popup */
						}
		
						for(unsigned t=0;t<channelCount;t++)
						{
							delete src[t];
							delete t_src[t];
						}
					}
					catch(...)
					{
						fclose(f);
						for(unsigned t=0;t<channelCount;t++)
						{
							delete src[t];
							delete t_src[t];
						}
						throw;
					}
				}
				fclose(f);
			}
			catch(...)
			{
				// remove files that were created
				for(segments_t::iterator i2=segments.begin();i2!=(i+1);i2++)
					unlink(i2->first.c_str());
				throw;
			}

		}
	}

	// build TOC file for cdrdao
	const string TOCFilename=prefix+".toc";
	FILE *f=fopen(TOCFilename.c_str(),"w");
	try
	{
		if(f==NULL)
		{
			const int errNO=errno;
			throw runtime_error(string(__func__)+" -- error creating TOC file -- "+strerror(errNO));
		}

		fprintf(f,"CD_DA\n");
		fprintf(f,"\n");
		for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
		{
			fprintf(f,"TRACK AUDIO\n");
			fprintf(f,"TWO_CHANNEL_AUDIO\n");
			if(gapBetweenTracks>0 && i!=segments.begin())
			{
				fprintf(f,"SILENCE 00:%02d:00\n",gapBetweenTracks);
				fprintf(f,"START\n");
			}
			fprintf(f,"FILE \"%s\" 0\n",i->first.c_str());
			fprintf(f,"\n");
		}

		if(ferror(f))
		{
			const int errNO=errno;
			fclose(f);
			throw runtime_error(string(__func__)+" -- error writing to file while building TOC -- "+strerror(errNO));
		}

		fclose(f);
	}
	catch(...)
	{
		unlink(TOCFilename.c_str());

		// cleanup files
		for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
			unlink(i->first.c_str());
		throw;
	}

#ifdef WORDS_BIGENDIAN
	const string endianSwap="";
#else
	const string endianSwap="--swap ";
#endif

	// burn the files
	const string command="'"+pathTo_cdrdao+"' "+(testOnly ? "simulate " : "write ")+endianSwap+"--speed "+istring(burnSpeed)+" --device "+device+" '"+TOCFilename+"'";
	printf("about to run command: %s\n",command.c_str());
	int status=system(command.c_str());
	if(WEXITSTATUS(status)!=0)
		Warning(_("cdrdao returned non-zero exit status.  Consult standard output/error for problems."));

	// cleanup
	unlink(TOCFilename.c_str());
	for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
		unlink(i->first.c_str());

	return true;
}

AAction::CanUndoResults CBurnToCDAction::canUndo(const CActionSound &actionSound) const
{
	return curNA;
}

void CBurnToCDAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	// not applicable
}

bool CBurnToCDAction::doesWarrantSaving() const
{
	return false;
}


const string CBurnToCDAction::getExplanation()
{
	return "\n\
This action can be used to burn the loaded audio file onto a CD using the cdrdao (CD-R Disk-At-Once) tool.\n\
\n\
Tracks are defined by cues named in a '('... [')'] fashion.  See the 'Explain' button on the 'File->Save As Multiple Files' action on how this is done.  Some parts of that explaination should be obviously non-applicable.\n\
\n\
cdrdao must be installed on this system.  $PATH with be searched for 'cdrdao' when ReZound starts, but if it is not found you will need to supply the path to it on the action dialog.\n\
	";
}

#include <stdio.h>
const string CBurnToCDAction::detectDevice(const string pathTo_cdrdao)
{
	string ret;
	const string cmd="'"+pathTo_cdrdao+"' scanbus 2>&1 | grep '[0-9],[0-9],[0-9]:' | head -1 | cut -f1 -d':'";
	printf("attemping to run command: %s\n",cmd.c_str());
	FILE *f=popen(cmd.c_str(),"r");
	if(f!=NULL)
	{
		char buffer[1024+1];
		if(fgets(buffer,1024,f))
		{
			if(strlen(buffer)>0)
				buffer[strlen(buffer)-1]=0; // remove trailing \n
			ret=buffer;
		}

		pclose(f);
	}

	return ret;
}


// ------------------------------

CBurnToCDActionFactory::CBurnToCDActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Burn to CD"),"",NULL,dialog,false,false)
{
}

CBurnToCDActionFactory::~CBurnToCDActionFactory()
{
}

CBurnToCDAction *CBurnToCDActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CBurnToCDAction(
		actionSound,
		actionParameters->getStringParameter("Temp Space Directory"),
		actionParameters->getStringParameter("Path to cdrdao"),
		actionParameters->getUnsignedParameter("Burn Speed")+1, /* +1 because it's the zero-based index into a list of numbers */
		actionParameters->getUnsignedParameter("Gap Between Tracks"),
		actionParameters->getStringParameter("Device"),
		actionParameters->getStringParameter("Extra cdrdao Options"),
		(actionParameters->getUnsignedParameter("Applies to")==1), // 0 -> "Entire File", 1 -> "Selection Only"
		actionParameters->getBoolParameter("Simulate Burn Only")
	);
}

