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
#include "../AStatusComm.h"

#include <istring>
#include <CPath.h>

#include <map>
#include <vector>
#include <utility>

#include <unistd.h> /* for unlink() and getuid()*/
#include <stdlib.h> /* for system() */
#include <sys/wait.h> /* for WEXITSTATUS */

#include "parse_segment_cues.h"

CBurnToCDAction::CBurnToCDAction(const AActionFactory *factory,const CActionSound *_actionSound,const string _tempSpaceDir,const string _pathTo_cdrdao,const unsigned _burnSpeed,const unsigned _gapBetweenTracks,const string _device,const string _extra_cdrdao_options,const bool _selectionOnly,const bool _testOnly) :
	AAction(factory,_actionSound),
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

bool CBurnToCDAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const CSound &sound=*(actionSound->sound);
	const sample_pos_t selectionStart= selectionOnly ? actionSound->start : 0;
	const sample_pos_t selectionLength= selectionOnly ? actionSound->selectionLength() : sound.getLength();
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
#warning cues on top of each other cause this to get confused
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

				// write out as stereo, 16bit, 44100
				if(channelCount==1)
				{	/* write 1 channel as stereo */
					const CRezPoolAccesser t_src=sound.getAudio(0);
					TSoundStretcher<const CRezPoolAccesser> src(t_src,srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);

					for(sample_pos_t t=0;t<destLength;t++)
					{
						int16_t s=convert_sample<sample_t,int16_t>(src.getSample());
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
					TSoundStretcher<const CRezPoolAccesser> srcL(t_srcL,srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);
					TSoundStretcher<const CRezPoolAccesser> srcR(t_srcR,srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);

					for(sample_pos_t t=0;t<destLength;t++)
					{
						int16_t left=convert_sample<sample_t,int16_t>(srcL.getSample());
						int16_t right=convert_sample<sample_t,int16_t>(srcR.getSample());

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

					TSoundStretcher<const CRezPoolAccesser> *src[MAX_CHANNELS]={0};
					for(unsigned t=0;t<channelCount;t++)
						src[t]=new TSoundStretcher<const CRezPoolAccesser>(*(t_src[t]),srcStart,srcLength,fDestLength,1,0,paddedForInterpolation);

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
		
							int16_t left=convert_sample<sample_t,int16_t>(tleft);
							int16_t right=convert_sample<sample_t,int16_t>(tright);
		
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

	int CDCount=0;
	do {

		// burn the files
		const string command="'"+pathTo_cdrdao+"' "+(testOnly ? "simulate " : "write ")+" -n "+endianSwap+"--speed "+istring(burnSpeed)+(istring(device).trim()=="" ? string("") : (" --device "+device))+" "+extra_cdrdao_options+" '"+TOCFilename+"' 2>&1"; // send err to out so we can capture it
		printf("about to run command: %s\n",command.c_str());
		/*
		int status=system(command.c_str());
		if(WEXITSTATUS(status)!=0)
			Warning(_("cdrdao returned non-zero exit status.  Consult standard output/error for problems."));
		else
			CDCount++;
		*/

		FILE *p=popen(command.c_str(),"r");
		showStatus(p);

		// finish eating the output of the command (otherwise it causes errors)
		for(char c; (fread(&c,1,1,p)==1); )
			{ fwrite(&c,1,1,stdout); fflush(stdout); }

		int ret=pclose(p);
		if(WEXITSTATUS(ret)!=0)
			Warning(_("cdrdao returned non-zero exit status.  Consult standard output/error for problems."));
		else
			CDCount++;

	} while(Question(_("Successful Burn Count: ")+istring(CDCount)+"\n"+_("Would you like to burn another?")+"\n"+_("(Insert a new blank, and then press 'Yes'.)"),yesnoQues)==yesAns);

	// cleanup
	unlink(TOCFilename.c_str());
	for(segments_t::iterator i=segments.begin();i!=segments.end();i++)
		unlink(i->first.c_str());

	return true;
}

AAction::CanUndoResults CBurnToCDAction::canUndo(const CActionSound *actionSound) const
{
	return curNA;
}

void CBurnToCDAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// not applicable
}

bool CBurnToCDAction::doesWarrantSaving() const
{
	return false;
}


const string CBurnToCDAction::getExplanation()
{
	return _("\n\
This action can be used to burn the loaded audio file onto a CD using the cdrdao (CD-R Disk-At-Once) tool.\n\
\n\
Tracks are defined by cues named in a '('... [')'] fashion.  See the 'Explain' button on the 'File->Save As Multiple Files' action on how this is done.  Some parts of that explaination should be obviously non-applicable.\n\
\n\
cdrdao must be installed on this system.  $PATH with be searched for 'cdrdao' when ReZound starts, but if it is not found you will need to supply the path to it on the action dialog.\n\
	");
}

#include <stdio.h>
const string CBurnToCDAction::detectDevice(const string pathTo_cdrdao)
{
	const string cmd="'"+pathTo_cdrdao+"' scanbus 2>&1 | grep '[0-9],[0-9],[0-9]:' | head -1 | cut -f1 -d':'";
	for(int t=0;t<2;t++)
	{
		string cmd;
		
		if(t==0)
			// try this first
			cmd="'"+pathTo_cdrdao+"' scanbus 2>&1 | grep '.\\{1,10\\}:[0-9],[0-9],[0-9]' | head -1 | cut -f1 -d' '";
		else if(t==1)
			// try this second
			cmd="'"+pathTo_cdrdao+"' scanbus 2>&1 | grep '[0-9],[0-9],[0-9]:' | head -1 | cut -f1 -d':'";

		printf("attemping to run command: %s\n",cmd.c_str());
		FILE *f=popen(cmd.c_str(),"r");
		if(f!=NULL)
		{
			char buffer[1024+1];
			if(fgets(buffer,1024,f))
			{
				if(strlen(buffer)>0)
					buffer[strlen(buffer)-1]=0; // remove trailing \n
				pclose(f);
				return buffer;
			}
	
			pclose(f);
		}
	}
	return "";
}

#include <ctype.h>
void CBurnToCDAction::showStatus(FILE *p)
{
	CStatusBar s(_("Burning..."),0,100,false); // ??? can I know pid to send a kill signal to for cancel?

	// state machine looks for " [0-9]+ of [0-9]+ " and bases the progress upon the two parsed numbers
	char c;
	int state=0;
	string str;
	while(fread(&c,1,1,p)==1)
	{
		// output everything we read
		fwrite(&c,1,1,stdout); fflush(stdout);

		switch(state)
		{
		case 0:
			str="";
			if(c==' ')
			{
				str+=c;
				state=1;
			}
			else
				state=0;
			break;

		case 1:
			if(isdigit(c))
			{
				str+=c;
				state=2;
			}
			else
				state=0;
			break;

		case 2:
			if(isdigit(c))
			{
				str+=c;
				state=2;
			}
			else if(c==' ')
			{
				str+=c;
				state=3;
			}
			else
				state=0;
			break;

		case 3:
			if(c=='o')
			{
				str+=c;
				state=4;
			}
			else
				state=0;
			break;

		case 4:
			if(c=='f')
			{
				str+=c;
				state=5;
			}
			else
				state=0;
			break;

		case 5:
			if(c==' ')
			{
				str+=c;
				state=6;
			}
			else
				state=0;
			break;

		case 6:
			if(isdigit(c))
			{
				str+=c;
				state=7;
			}
			else
				state=0;
			break;

		case 7:
			if(isdigit(c))
			{
				str+=c;
				state=7;
			}
			else if(c==' ')
			{
				str+=c;

				// now we have our parsed string " [0-9]+ of [0-9] ", so update the progress
				int part=1;
				int whole=1;
				sscanf(str.c_str()," %d of %d ",&part,&whole);

				s.update(part*100/whole);

				if(part>=whole)
					return; // we're done

				state=0;
			}
			else
				state=0;
			break;

		default:
			printf("%s: the state machine is messed up\n",__func__);
			state=0; // messed up state machine
		}
	}
}

// ------------------------------

CBurnToCDActionFactory::CBurnToCDActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Burn to CD"),"",NULL,dialog,false,false)
{
}

CBurnToCDActionFactory::~CBurnToCDActionFactory()
{
}

CBurnToCDAction *CBurnToCDActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CBurnToCDAction(
		this,
		actionSound,
		actionParameters->getValue<string>("Temp Space Directory"),
		actionParameters->getValue<string>("Path to cdrdao"),
		actionParameters->getValue<unsigned>("Burn Speed")+1, /* +1 because it's the zero-based index into a list of numbers */
		actionParameters->getValue<unsigned>("Gap Between Tracks"),
		actionParameters->getValue<string>("Device"),
		actionParameters->getValue<string>("Extra cdrdao Options"),
		(actionParameters->getValue<unsigned>("Applies to")==1), // 0 -> "Entire File", 1 -> "Selection Only"
		actionParameters->getValue<bool>("Simulate Burn Only")
	);
}

