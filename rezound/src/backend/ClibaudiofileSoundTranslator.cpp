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

#include "ClibaudiofileSoundTranslator.h"

#include <unistd.h> // for unlink

#include <stdexcept>
#include <typeinfo>

#include <istring>
#include <TAutoBuffer.h>

#include <cc++/path.h>

#include "CSound.h"
#include "AStatusComm.h"


ClibaudiofileSoundTranslator::ClibaudiofileSoundTranslator()
{
}

ClibaudiofileSoundTranslator::~ClibaudiofileSoundTranslator()
{
}

static string errorMessage;
static void errorFunction(long code,const char *msg)
{
	errorMessage=msg;
}

	// ??? could just return a CSound object an have used the one constructor that takes the meta info
	// ??? but, then how would I be able to have createWorkingPoolFileIfExists
void ClibaudiofileSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	// passing AF_NULL_FILESETUP makes it detect the parameters from the file itself
	loadSoundGivenSetup(filename,sound,AF_NULL_FILESETUP);
}

void ClibaudiofileSoundTranslator::loadSoundGivenSetup(const string filename,CSound *sound,AFfilesetup initialSetup) const
{
	errorMessage="";
	afSetErrorHandler(errorFunction);

	AFfilehandle h=afOpenFile(filename.c_str(),"r",initialSetup);
	if(h==AF_NULL_FILEHANDLE)
		throw(runtime_error(string(__func__)+" -- error opening '"+filename+"' -- "+errorMessage));

	// ??? this if set may not completly handle all possilbilities
	int ret;
	if(typeid(sample_t)==typeid(int16_t))
		ret=afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(sample_t)*8);
	else if(typeid(sample_t)==typeid(float))
		ret=afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_FLOAT,sizeof(sample_t)*8);
	else
	{
		afCloseFile(h);
		throw(runtime_error(string(__func__)+" -- unhandled sample_t format"));
	}

	if(ret!=0)
		throw(runtime_error(string(__func__)+" -- error setting virtual format -- "+errorMessage));
		
	if(afSetVirtualByteOrder(h,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN))
		throw(runtime_error(string(__func__)+" -- error setting virtual byte order -- "+errorMessage));

		// ??? I'm not sure about the 3to4 parameter, because won't it give it to me in whatever bit size I said in afSetVirtualSampleFormat ?
	//unsigned channelCount=(unsigned)(afGetVirtualFrameSize(h,AF_DEFAULT_TRACK,1)/sizeof(sample_t));
	unsigned channelCount=afGetChannels(h,AF_DEFAULT_TRACK);
	if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could just ignore the extra channels
		throw(runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in ASound.h"));
	unsigned sampleRate=(unsigned)afGetRate(h,AF_DEFAULT_TRACK);

	if(sampleRate<4000 || sampleRate>96000)
		throw(runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate)+" indicates probably a corrupt file and SGI's libaudiofile has been known to miss these"));

	
	// ??? make sure it's not more than MAX_LENGTH
		// ??? just truncate the length
	sample_pos_t size=afGetFrameCount(h,AF_DEFAULT_TRACK);
	if(size<0)
		throw(runtime_error(string(__func__)+" -- libaudiofile reports the data length as "+istring(size)));

	const sample_pos_t fileSize=ost::Path(filename).getSize(false)/(channelCount*sizeof(sample_t));
	if(fileSize<(size/25)) // ??? possibly 1/25th compression... really just trying to check for a sane value
	{
		Warning("libaudiofile reports that "+filename+" contains "+istring(size)+" samples yet the file is most likely not large enough to contain that many samples.\nLoading what can be loaded.");
		//size=fileSize; not doing this because I once ran across a situation where you could read more from the file that stat said it was... even ls showed it smaller than I could actually read
		//		??? however ^^^ on the other hand, I don't want the length to be 8 gigs worth of space...   Perhaps I should always ignore the given size, and add space in large units until I run out of file... I should apply cues last then
		//throw(runtime_error(string(__func__)+" -- libaudiofile is not seeing this as a corrupt file -- it thinks the data length is "+istring(size)+" yet when the file is only "+istring(fileSize)+" bytes"));
	}

	sound->createWorkingPoolFile(filename,sampleRate,channelCount,size);

	// load the cues
	const size_t cueCount=afGetMarkIDs(h,AF_DEFAULT_TRACK,NULL);
	if(cueCount<4096) // check for a sane amount
	{
		sound->clearCues();

		ost::TAutoBuffer<int> markIDs(cueCount);
		afGetMarkIDs(h,AF_DEFAULT_TRACK,markIDs);
		for(size_t t=0;t<cueCount;t++)
		{
			string name=afGetMarkName(h,AF_DEFAULT_TRACK,markIDs[t]);
			if(name=="")
				name=sound->getAvailableCueName("cue"); // give it a unique name

			const sample_pos_t time=afGetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t]);

			// to determine if the cue is anchored or not, name will be:
			// "asdf|+" if anchored, or "asdf|-" or "asdf" if not anchored
			const size_t delimPos=name.rfind("|");
			bool isAnchored=false;
			if(delimPos!=string::npos)
			{
				isAnchored=name.substr(delimPos+1)=="+";
				name.erase(delimPos);
			}

			try
			{
				sound->addCue(name,time,isAnchored);
			}
			catch(exception &e)
			{ // don't make an error adding a cue cause the whole file not to load
				Warning("file: '"+filename+"' -- "+e.what());
			}
		}
	}
	else
		Warning("Insane amount of markers so they were ignored: "+istring(cueCount)+" -- "+filename);
	

	// load the user notes
		// ???
	

	// load the audio data

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));


		ost::TAutoBuffer<sample_t> buffer((size_t)(afGetVirtualFrameSize(h,AF_DEFAULT_TRACK,1)*4096/sizeof(sample_t)));
		sample_pos_t pos=0;
		AFframecount count=size/4096+1;
		BEGIN_PROGRESS_BAR("Loading Sound",0,count);
		for(AFframecount t=0;t<count;t++)
		{
			const int chunkSize=  (t==count-1 ) ? size%4096 : 4096;
			if(chunkSize!=0)
			{
				if(afReadFrames(h,AF_DEFAULT_TRACK,(void *)buffer,chunkSize)!=chunkSize)
				{
					//throw(runtime_error(string(__func__)+" -- error reading audio data -- "+errorMessage));
					Error("error reading audio data from "+filename+" -- "+errorMessage+" -- keeping what was read");
					break;
				}

				for(unsigned c=0;c<channelCount;c++)
				{
					for(int i=0;i<chunkSize;i++)
						(*(accessers[c]))[pos+i]=buffer[i*channelCount+c];
				}
				pos+=chunkSize;
			}

			UPDATE_PROGRESS_BAR(t);
		}

		END_PROGRESS_BAR();

		afCloseFile(h);

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}
	}
	catch(...)
	{
		endAllProgressBars();

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];
		throw;
	}

	sound->setIsModified(false);
}

void ClibaudiofileSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	int fileType;

	const string extension=istring(ost::Path(filename).Extension()).lower();
	if(extension=="aiff")
		fileType=AF_FILE_AIFF;
	else if(extension=="wav")
		fileType=AF_FILE_WAVE;
	else if(extension=="snd" || extension=="au")
		fileType=AF_FILE_NEXTSND;
	else if(extension=="sf")
		fileType=AF_FILE_BICSF;
	else
		throw(runtime_error(string(__func__)+" -- unhandled extension for filename: "+filename));

	AFfilesetup setup=afNewFileSetup();
	try
	{
		// ??? all the following parameters need to be passed in somehow as the export format
		// 	??? can easily do it with AFrontendHooks
		afInitFileFormat(setup,fileType); 
		afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN); 			// ??? I would actually want to pass how the user wants to export the data...
		afInitChannels(setup,AF_DEFAULT_TRACK,sound->getChannelCount());
		afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(int16_t)*8); 	// ??? I would actually want to pass how the user wants to export the data... int16_t matching AF_SAMPFMT_TWOSCOMP
		afInitRate(setup,AF_DEFAULT_TRACK,sound->getSampleRate()); 				// ??? I would actually want to pass how the user wants to export the data... doesn't actual do any conversion right now (perhaps I could patch it for them)
		afInitCompression(setup,AF_DEFAULT_TRACK,AF_COMPRESSION_NONE);
			//afInitInitCompressionParams(setup,AF_DEFAULT_TRACK, ... );
		afInitFrameCount(setup,AF_DEFAULT_TRACK,sound->getLength());				// ??? I suppose I could allow the user to specify something shorter on the dialog

		saveSoundGivenSetup(filename,sound,setup);

		afFreeFileSetup(setup);
	}
	catch(...)
	{
		afFreeFileSetup(setup);
		throw;
	}

	
}


void ClibaudiofileSoundTranslator::saveSoundGivenSetup(const string filename,CSound *sound,AFfilesetup initialSetup) const
{
	errorMessage="";
	afSetErrorHandler(errorFunction);

	const unsigned channelCount=sound->getChannelCount();
	const unsigned sampleRate=sound->getSampleRate();
	const sample_pos_t size=sound->getLength();

	// setup for saving the cues (except for positions)
	ost::TAutoBuffer<int> markIDs(sound->getCueCount());
	if(sound->getCueCount()>0)
	{
		for(size_t t=0;t<sound->getCueCount();t++)
			markIDs[t]=t;

		afInitMarkIDs(initialSetup,AF_DEFAULT_TRACK,markIDs,(int)sound->getCueCount());
		for(size_t t=0;t<sound->getCueCount();t++)
		{
			// to indicate if the cue is anchored we append to the name:
			// "|+" or "|-" whether it is or isn't
			const string name=sound->getCueName(t)+"|"+(sound->isCueAnchored(t) ? "+" : "-");
			afInitMarkName(initialSetup,AF_DEFAULT_TRACK,markIDs[t],name.c_str());

			// can't save position yet because that function requires a file handle, but
			// we can't move this code after the afOpenFile, or it won't save cues at all
			//afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t],sound->getCueTime(t));
		}
	}

	// save the user notes
		// ???

	unlink(filename.c_str());
	AFfilehandle h=afOpenFile(filename.c_str(),"w",initialSetup);
	if(h==AF_NULL_FILEHANDLE)
		throw(runtime_error(string(__func__)+" -- error opening '"+filename+"' for writing -- "+errorMessage));

	// ??? this if set may not completly handle all possilbilities
	int ret;
	if(typeid(sample_t)==typeid(int16_t))
		ret=afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(sample_t)*8);
	else if(typeid(sample_t)==typeid(float))
		ret=afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_FLOAT,sizeof(sample_t)*8);
	else
	{
		afCloseFile(h);
		throw(runtime_error(string(__func__)+" -- unhandled sample_t format"));
	}

	if(ret!=0)
		throw(runtime_error(string(__func__)+" -- error setting virtual format -- "+errorMessage));

	if(afSetVirtualByteOrder(h,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN))
		throw(runtime_error(string(__func__)+" -- error setting virtual byte order -- "+errorMessage));
	afSetVirtualChannels(h,AF_DEFAULT_TRACK,channelCount);

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{

		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));

		
		// save the audio data
		ost::TAutoBuffer<sample_t> buffer((size_t)(channelCount*4096));
		sample_pos_t pos=0;
		AFframecount count=size/4096+1;
		BEGIN_PROGRESS_BAR("Saving Sound",0,count);
		for(AFframecount t=0;t<count;t++)
		{
			const int chunkSize=  (t==count-1 ) ? size%4096 : 4096;
			if(chunkSize!=0)
			{
				for(unsigned c=0;c<channelCount;c++)
				{
					for(int i=0;i<chunkSize;i++)
						buffer[i*channelCount+c]=(*(accessers[c]))[pos+i];
				}
				if(afWriteFrames(h,AF_DEFAULT_TRACK,(void *)buffer,chunkSize)!=chunkSize)
					throw(runtime_error(string(__func__)+" -- error writing audio data -- "+errorMessage));
				pos+=chunkSize;
			}

			UPDATE_PROGRESS_BAR(t);
		}

		END_PROGRESS_BAR();
		
		// write the cue's positions
		for(size_t t=0;t<sound->getCueCount();t++)
			afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t],sound->getCueTime(t));


		afCloseFile(h);

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}
	}
	catch(...)
	{
		// attempt to close the file ??? and free the setup???

		endAllProgressBars();

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		throw;
	}

}


bool ClibaudiofileSoundTranslator::handlesExtension(const string extension) const
{
	return(extension=="wav" || extension=="aiff" || extension=="au" || extension=="snd" || extension=="sf");
}

bool ClibaudiofileSoundTranslator::supportsFormat(const string filename) const
{
	int fd=open(filename.c_str(),O_RDONLY);
	int _e=errno;
	if(fd==-1)
		throw(runtime_error(string(__func__)+" -- error opening file '"+filename+"' -- "+strerror(_e)));

	int implemented=0;
	int id=afIdentifyNamedFD(fd,filename.c_str(),&implemented);
	close(fd);

	return(implemented);
}

const vector<string> ClibaudiofileSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("AIFF");
	names.push_back("Wave/RIFF");
	names.push_back("NeXT/Sun");
	names.push_back("Berkeley/IRCAM/CARL");

	return(names);
}

const vector<vector<string> > ClibaudiofileSoundTranslator::getFormatExtensions() const
{
	vector<vector<string> > list;
	vector<string> extensions;

	extensions.clear();
	extensions.push_back("aiff");
	list.push_back(extensions);

	extensions.clear();
	extensions.push_back("wav");
	list.push_back(extensions);

	extensions.clear();
	extensions.push_back("snd");
	extensions.push_back("au");
	list.push_back(extensions);

	extensions.clear();
	extensions.push_back("sf");
	list.push_back(extensions);

	return(list);
}


