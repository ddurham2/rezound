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

#include "ClibaudiofileSound.h"

#include <unistd.h> // for unlink

#include <stdexcept>
#include <typeinfo>
#include <memory>
#include <string>

#include <cc++/path.h>

#include <TAutoBuffer.h>
#include <istring>

#include "audiofile.h"

#include "AStatusComm.h"


// ??? libaudiofile doesn't seem to provide a way of preserving any other information within a file, if I could even get it, I could store it in the working pool file and if they export it to the same format, I could replace that information
// 	- yes it does... just found it.... lots'a'stuffes can be saved

static string errorMessage;
static void errorFunction(long code,const char *msg)
{
	errorMessage=msg;
}

ClibaudiofileSound::ClibaudiofileSound() :
	ASound()
{
}

ClibaudiofileSound::ClibaudiofileSound(const string &_filename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size) :
	ASound(_filename,_sampleRate,_channelCount,_size)
{
#if 0
	sampleRate=_sampleRate;
	channelCount=_channelCount;
	size=_size;

/* done in createWorkingPoolFile
	if(channelCount>=MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- invalid number of channels: "+istring(channelCount)));
*/
/*
	if(_size>MAX_LENGTH)
		throw(runtime_error(string(__func__)+" -- size is greater than MAX_LENGTH"));
*/

	const string tempFilename=_filename;

	try
	{
		// get a real temp file name
		remove(tempFilename.c_str());

		createWorkingPoolFile(tempFilename);
		filename=tempFilename;
	}
	catch(...)
	{
		try 
		{
			poolFile.closeFile(false,true);
		} catch(...) {}
		//remove(tempFilename);
		throw;
	}

	matchUpChannelLengths(NIL_SAMPLE_POS);
#endif
}

void ClibaudiofileSound::loadSound(const string _filename)
{
	if(createFromWorkingPoolFileIfExists(_filename))
	{
		filename=_filename;
		return;
	}

	filename=_filename;

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		errorMessage="";
		afSetErrorHandler(errorFunction);

		AFfilehandle h=afOpenFile(filename.c_str(),"r",AF_NULL_FILESETUP);
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
		//channelCount=(unsigned)(afGetVirtualFrameSize(h,AF_DEFAULT_TRACK,1)/sizeof(sample_t));
		channelCount=afGetChannels(h,AF_DEFAULT_TRACK);
		if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could just ignore the extra channels
			throw(runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in ASound.h"));
		sampleRate=(unsigned)afGetRate(h,AF_DEFAULT_TRACK);

		if(sampleRate<4000 || sampleRate>96000)
			throw(runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate)+" indicates probably a corrupt file and SGI's libaudiofile has been known to miss these"));

		
		// ??? make sure it's not more than MAX_LENGTH
			// ??? just truncate the length
		size=afGetFrameCount(h,AF_DEFAULT_TRACK);
		if(size<0)
			throw(runtime_error(string(__func__)+" -- libaudiofile reports the data length as "+istring(size)));

		const sample_pos_t fileSize=ost::Path(filename).getSize(false)/(channelCount*sizeof(sample_t));
		if(fileSize<size)
		{
			Warning("libaudiofile reports that "+filename+" contains "+istring(size)+" samples yet the file is not large enough to contain that many samples.\nLoading what can be loaded.");
			//size=fileSize; not doing this because I once ran across a situation where you could read more from the file that stat said it was... even ls showed it smaller than I could actually read
			//throw(runtime_error(string(__func__)+" -- libaudiofile is not seeing this as a corrupt file -- it thinks the data length is "+istring(size)+" yet when the file is only "+istring(fileSize)+" bytes"));
		}

		createWorkingPoolFile(filename);

		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(getAudio(t));


		// load the cues
		const size_t cueCount=afGetMarkIDs(h,AF_DEFAULT_TRACK,NULL);
		if(cueCount<4096) // check for a sane amount
		{
			clearCues();

			ost::TAutoBuffer<int> markIDs(cueCount);
			afGetMarkIDs(h,AF_DEFAULT_TRACK,markIDs);
			for(size_t t=0;t<cueCount;t++)
			{
				string name=afGetMarkName(h,AF_DEFAULT_TRACK,markIDs[t]);
				if(name=="")
					continue;

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
					addCue(name,time,isAnchored);
				}
				catch(exception &e)
				{ // don't make an error adding a cue cause the whole file not to load
					Warning("file: '"+_filename+"' -- "+e.what());
				}
			}
		}
		else
			Warning("Insane amount of markers so they were ignored: "+istring(cueCount)+" -- "+filename);
		

		// load the user notes
		

		// load the audio data
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

	matchUpChannelLengths(NIL_SAMPLE_POS);

	setIsModified(false);
}

void ClibaudiofileSound::saveSound(const string filename)
{
	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		int fileType;

		if(istring(ost::Path(filename).Extension()).lower()=="wav")
			fileType=AF_FILE_WAVE;
		else if(istring(ost::Path(filename).Extension()).lower()=="aiff")
			fileType=AF_FILE_AIFF;
		else if(istring(ost::Path(filename).Extension()).lower()=="snd" || istring(ost::Path(filename).Extension()).lower()=="au")
			fileType=AF_FILE_NEXTSND;
		else if(istring(ost::Path(filename).Extension()).lower()=="sf")
			fileType=AF_FILE_BICSF;
		else
			throw(runtime_error(string(__func__)+" -- unhandled extension for filename: "+filename));

		
		errorMessage="";
		afSetErrorHandler(errorFunction);

		// ??? all the following parameters need to be passed in somehow as the export format
		AFfilesetup newFile=afNewFileSetup();
		afInitFileFormat(newFile,fileType); 
		afInitSampleFormat(newFile,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(int16_t)*8); // ??? int16_t matching AF_SAMPFMT_TWOSCOMP
		afInitChannels(newFile,AF_DEFAULT_TRACK,channelCount);
		afInitRate(newFile,AF_DEFAULT_TRACK,sampleRate);

		// setup for saving the cues (except for positions)
		ost::TAutoBuffer<int> markIDs(getCueCount());
		if(getCueCount()>0)
		{
			for(size_t t=0;t<getCueCount();t++)
				markIDs[t]=t;

			afInitMarkIDs(newFile,AF_DEFAULT_TRACK,markIDs,(int)getCueCount());
			for(size_t t=0;t<getCueCount();t++)
			{
				// to indicate if the cue is anchored we append to the name:
				// "|+" or "|-" whether it is or isn't
				const string name=getCueName(t)+"|"+(isCueAnchored(t) ? "+" : "-");
				afInitMarkName(newFile,AF_DEFAULT_TRACK,markIDs[t],name.c_str());

				// can't save position yet because that function requires a file handle, but
				// we can't move this code after the afOpenFile, or it won't save cues at all
				//afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t],getCueTime(t));
			}
		}

		// save the user notes
		

		unlink(filename.c_str());
		AFfilehandle h=afOpenFile(filename.c_str(),"w",newFile);
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


		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(getAudio(t));

		
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
		for(size_t t=0;t<getCueCount();t++)
			afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t],getCueTime(t));


		afFreeFileSetup(newFile);
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

bool ClibaudiofileSound::supportsFormat(const string filename)
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

bool ClibaudiofileSound::handlesExtension(const string _extension)
{
	istring extension(_extension);
	extension.lower();

	return(extension=="wav" || extension=="aiff" || extension=="au" || extension=="snd" || extension=="sf");
}

