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

/*
 * This SoundTranslator class handles mp3 I/O by interfacing with the 
 * lame executable.  They do have a lame library, but the API is not
 * documented and the library itself does not deal with mp3 files only
 * with mp3 chunks.  Plus, doing it this way should avoid any possible
 * patent issues even though there's not supposed to be any with lame.
 * 
 * This class may be generalized later as a base class to support
 * other applications where the derived classes would handle the 
 * specifics of each application.
 */

#include "ClameSoundTranslator.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "mypopen.h"

#include <signal.h>

#include <typeinfo>
#include <stdexcept>

#include <CPath.h>
#include <TAutoBuffer.h>

#include "CSound.h"
#include "AStatusComm.h"
#include "AFrontendHooks.h"

sighandler_t prevSIGPIPE_Handler;
bool gAbortSave;
static void SIGPIPE_Handler(int sig)
{
	gAbortSave=true;
}


static string gPathToLame="";

ClameSoundTranslator::ClameSoundTranslator()
{
}

ClameSoundTranslator::~ClameSoundTranslator()
{
}

bool ClameSoundTranslator::checkForLame()
{
	// ??? this should eventually use a registry setting which comes from user preferences and would be checked first

	FILE *p=mypopen("which lame","r");
	if(p==NULL)
	{
		gPathToLame="";
		return(false);
	}

	char buffer[4096+1]={0};
	fgets(buffer,4096,p);
	mypclose(p);

	// remove trailing \n if it's there
	const size_t l=strlen(buffer);
	if(l>0 && buffer[l-1]=='\n')
		buffer[l-1]=0;

	gPathToLame=buffer;

	if(gPathToLame=="")
		fprintf(stderr,"'lame' executable not found in $PATH -- mp3 support will be disabled\n");

	return(gPathToLame!="");
}

/* translate \ to \\ and " to \" in the given filename */
// ??? This would probably need to behave a little differently on WIN32 unless it can use " and \ in pathnames
static const string fixEscapes(const string _filename)
{
	string filename=_filename;
	for(size_t t=0;t<filename.size();t++)
	{
		if(filename[t]=='\\' || filename[t]=='"')
		{
			filename.insert(t,"\\");
			t++;
		}
	}
	return(filename);
}

	// ??? could just return a CSound object an have used the one constructor that takes the meta info
	// ??? but, then how would I be able to have createWorkingPoolFileIfExists
bool ClameSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	bool ret=true;

	if(gPathToLame=="")
		throw(runtime_error(string(__func__)+" -- path to 'lame' not set"));

	if(!CPath(filename).exists())
		throw(runtime_error(string(__func__)+" -- file not found, '"+filename+"'"));

	const string cmdLine=gPathToLame+" --decode \""+fixEscapes(filename)+"\" -";

	fprintf(stderr,"lame command line: '%s'\n",cmdLine.c_str());

	FILE *errStream=NULL;
	FILE *p=mypopen(cmdLine.c_str(),"r",&errStream);
	if(p==NULL)
	{
		int err=errno;
		throw(runtime_error(string(__func__)+" -- error creating pipe to command: '"+cmdLine+"' -- "+strerror(err)));
	}

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		// for now lame always outputs a fixed size wave header as little 
		// endian just specifying the format so I'll just fread this struct
		// and later I will have to contend with endian-ness
		struct 
		{
			char RIFF_ID[4];
			uint32_t fileSize; // bogus coming from lame
			char WAVE_ID[4];
			char fmt_ID[4];
			uint32_t fmtSize;
			uint16_t dataType; // 1 => PCM
			uint16_t channelCount;
			uint32_t sampleRate;
			uint32_t bytesPerSec;
			uint16_t bytesPerSample;
			uint16_t bitsPerSample;
			char data_ID[4];
			uint32_t dataLength; // bogus coming from lame
		} waveHeader; 
		memset(&waveHeader,0,sizeof(waveHeader));


		fread(&waveHeader,1,sizeof(waveHeader),p);

		// verify some stuff about the output of lame
		if(waveHeader.fmtSize!=16)
			throw(runtime_error(string(__func__)+" -- it looks as if either there is an error in the input file -- or lame was not compiled with decoding support (get latest at http://mp3dev.org) -- or an error has occuring executing lame -- or your version of lame has started to output a different wave file header when decoding MPEG Layer-1,2,3 files to wave files.  Changes will have to be made to this source to handle the new wave file output -- check stderr for more information"));
		if(strncmp(waveHeader.RIFF_ID,"RIFF",4)!=0)
			throw(runtime_error(string(__func__)+" -- internal error -- 'RIFF' expected in lame output"));
		if(strncmp(waveHeader.WAVE_ID,"WAVE",4)!=0)
			throw(runtime_error(string(__func__)+" -- internal error -- 'WAVE' expected in lame output"));
		if(strncmp(waveHeader.fmt_ID,"fmt ",4)!=0)
			throw(runtime_error(string(__func__)+" -- internal error -- 'fmt ' expected in lame output"));
		if(strncmp(waveHeader.data_ID,"data",4)!=0)
			throw(runtime_error(string(__func__)+" -- internal error -- 'data' expected in lame output"));

		if(waveHeader.dataType!=1)
			throw(runtime_error(string(__func__)+" -- internal error -- it looks as if your version of lame has started to output non-PCM data when decoding mp3 files to wave files.  Changes will have to be made to this source to handle the new wave file output"));

		unsigned channelCount=waveHeader.channelCount;
		if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could just ignore the extra channels
			throw(runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in CSound.h"));

		unsigned sampleRate=waveHeader.sampleRate;
		if(sampleRate<4000 || sampleRate>96000)
			throw(runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate)));

		unsigned bits=waveHeader.bitsPerSample;
		if(bits!=16 && bits!=8)
			throw(runtime_error(string(__func__)+" -- an unlikely/unhandled bit rate of "+istring(bits)));

		#define REALLOC_FILE_SIZE (1024*1024/4)
		
		sound->createWorkingPoolFile(filename,sampleRate,channelCount,REALLOC_FILE_SIZE);
		
		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));

		#define BUFFER_SIZE 4096

		// print initial stderr from lame
		char errBuffer[BUFFER_SIZE+1];
		while(fgets(errBuffer,BUFFER_SIZE,errStream)!=NULL) // non-blocking i/o set by mypopen on this stream
			printf("%s",errBuffer);

		if(bits==16 && typeid(sample_t)==typeid(int16_t))
		{

			TAutoBuffer<sample_t> buffer(BUFFER_SIZE*channelCount);
			sample_pos_t pos=0;

			CStatusBar statusBar("Loading Sound",0,100,true);
			for(;;)
			{
				size_t chunkSize=fread((void *)((sample_t *)buffer),sizeof(sample_t)*channelCount,BUFFER_SIZE,p);
				if(chunkSize<=0)
					break;

				if((pos+REALLOC_FILE_SIZE)>sound->getLength())
					sound->addSpace(sound->getLength(),REALLOC_FILE_SIZE);

				for(unsigned c=0;c<channelCount;c++)
				{
					for(unsigned i=0;i<chunkSize;i++)
						(*(accessers[c]))[pos+i]=buffer[i*channelCount+c];
				}
				pos+=chunkSize;

				// read and parse the stderr of 'lame' to determine the progress of the load
				while(fgets(errBuffer,BUFFER_SIZE,errStream)!=NULL) // non-blocking i/o set by mypopen on this stream
				{
					int frameNumber,totalFrames;
					sscanf(errBuffer,"%*s %d%*c%d ",&frameNumber,&totalFrames);
					printf("%s",errBuffer);
					if(statusBar.update(frameNumber*100/totalFrames))
					{ // cancelled
						ret=false;
						goto cancelled;
					}
				}

			}
			printf("\n"); // after lame stderr output

			// remove any extra allocated space
			if(sound->getLength()>pos)
				sound->removeSpace(pos,sound->getLength()-pos);
		}
		else
			throw(runtime_error(string(__func__)+" -- an unhandled bit rate of "+istring(bits)));

		cancelled:

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		mypclose(p);
	}
	catch(...)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		mypclose(p);
	
		throw;
	}

	return ret;
}

bool ClameSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	bool ret=true;

	if(gPathToLame=="")
		throw(runtime_error(string(__func__)+" -- path to 'lame' not set"));

	if(CPath(filename).extension()!="mp3")
		throw(runtime_error(string(__func__)+" -- can only encode in MPEG Layer-3"));

	AFrontendHooks::Mp3CompressionParameters parameters;
	if(!gFrontendHooks->promptForMp3CompressionParameters(parameters))
		return(false);
	

	if(CPath(filename).exists())
	{
		if(unlink(filename.c_str())!=0)
		{
			int err=errno;
			throw(runtime_error(string(__func__)+" -- error removing file, '"+filename+"' -- "+strerror(err)));
		}
	}

	if(sound->getCueCount()>0 || sound->getUserNotes()!="")
	{
		if(Question("MPEG Layer-3 does not support saving user notes or cues\nDo you wish to continue?",yesnoQues)!=yesAns)
			return(false);
	}

	
	string cmdLine=gPathToLame+" ";

	if(!parameters.useFlagsOnly)
	{
		if(parameters.method==AFrontendHooks::Mp3CompressionParameters::brCBR)
		{
			cmdLine+=" -b "+istring(parameters.constantBitRate/1000)+" ";
		}
		else if(parameters.method==AFrontendHooks::Mp3CompressionParameters::brABR)
		{
			cmdLine+=" --abr "+istring(parameters.normBitRate/1000)+" -b "+istring(parameters.minBitRate/1000)+" -B "+istring(parameters.maxBitRate/1000)+" ";
		}
		else if(parameters.method==AFrontendHooks::Mp3CompressionParameters::brQuality)
		{
			cmdLine+=" -V "+istring(parameters.quality)+" ";
		}
		else
			throw(runtime_error(string(__func__)+" -- internal error -- unhandle bit rate method "+istring(parameters.method)));
	}

	cmdLine+=" "+parameters.additionalFlags+" ";

	cmdLine+=" - \""+fixEscapes(filename)+"\"";

	fprintf(stderr,"lame command line: '%s'\n",cmdLine.c_str());

	// setup a signal handler to handle a SIGPIPE in case lame crashes or doesn't like something
	gAbortSave=false;
	prevSIGPIPE_Handler=signal(SIGPIPE,SIGPIPE_Handler);

	FILE *p=mypopen(cmdLine.c_str(),"w");
	if(p==NULL)
	{
		int err=errno;
		throw(runtime_error(string(__func__)+" -- error creating pipe to command: '"+cmdLine+"' -- "+strerror(err)));
	}

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		const unsigned channelCount=sound->getChannelCount();
		const sample_pos_t size=sound->getLength();

		// for now lame always outputs a fixed size wave header as little 
		// endian just specifying the format so I'll just fread this struct
		// and later I will have to contend with endian-ness
		struct 
		{
			char RIFF_ID[4];
			uint32_t fileSize;
			char WAVE_ID[4];
			char fmt_ID[4];
			uint32_t fmtSize;
			uint16_t dataType; // 1 => PCM
			uint16_t channelCount;
			uint32_t sampleRate;
			uint32_t bytesPerSec;
			uint16_t bytesPerSample;
			uint16_t bitsPerSample;
			char data_ID[4];
			uint32_t dataLength;
		} waveHeader; 

		#define BITS 16 // has to go along with how we're writting it to the pipe below

		if(size>((0x7fffffff-4096)/((BITS/2)*channelCount)))
			throw(runtime_error(string(__func__)+" -- audio data is too large to be converted to mp3 (more than 2gigs of "+istring(BITS)+"bit/"+istring(channelCount)+"channels"));

		strncpy(waveHeader.RIFF_ID,"RIFF",4);
		waveHeader.fileSize=36+(size*(channelCount*(BITS/8)));
		strncpy(waveHeader.WAVE_ID,"WAVE",4);
		strncpy(waveHeader.fmt_ID,"fmt ",4);
		waveHeader.fmtSize=16;
		waveHeader.dataType=1;
		waveHeader.channelCount=channelCount;
		waveHeader.sampleRate=sound->getSampleRate();
		waveHeader.bytesPerSec=sound->getSampleRate()*channelCount*(BITS/8);
		waveHeader.bitsPerSample=BITS;
		strncpy(waveHeader.data_ID,"data",4);
		waveHeader.dataLength=size*(channelCount*(BITS/8));

		if(gAbortSave)
			throw(runtime_error(string(__func__)+" -- lame aborted -- check stderr for more information"));
		fwrite(&waveHeader,1,sizeof(waveHeader),p);


		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));

		#define BUFFER_SIZE 4096
		if(typeid(sample_t)==typeid(int16_t))
		{
			TAutoBuffer<sample_t> buffer(BUFFER_SIZE*channelCount);
			sample_pos_t pos=0;

			CStatusBar statusBar("Saving Sound",0,size,true);
			while(pos<size)
			{
				size_t chunkSize=BUFFER_SIZE;
				if(pos+chunkSize>size)
					chunkSize=size-pos;

				for(unsigned c=0;c<channelCount;c++)
				{
					for(unsigned i=0;i<chunkSize;i++)
						buffer[i*channelCount+c]=(*(accessers[c]))[pos+i];
				}
				pos+=chunkSize;

				if(gAbortSave)
					throw(runtime_error(string(__func__)+" -- lame aborted -- check stderr for more information"));
				if(fwrite((void *)((sample_t *)buffer),sizeof(sample_t)*channelCount,chunkSize,p)!=chunkSize)
					fprintf(stderr,"%s -- dropped some data while writing\n",__func__);

				if(statusBar.update(pos))
				{ // cancelled
					ret=false;
					goto cancelled;
				}
			}
		}
		else
			throw(runtime_error(string(__func__)+" -- internal error -- an unhandled sample_t type"));

		cancelled:

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		mypclose(p);

		if(prevSIGPIPE_Handler!=NULL && prevSIGPIPE_Handler!=SIG_ERR)
			signal(SIGPIPE,prevSIGPIPE_Handler);
	}
	catch(...)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		mypclose(p);
	
		if(prevSIGPIPE_Handler!=NULL && prevSIGPIPE_Handler!=SIG_ERR)
			signal(SIGPIPE,prevSIGPIPE_Handler);

		throw;
	}

	if(!ret)
		unlink(filename.c_str()); // remove the cancelled file

	return ret;
}


bool ClameSoundTranslator::handlesExtension(const string extension) const
{
	return(extension=="mp3" || extension=="mp2" || extension=="mp1");
}

bool ClameSoundTranslator::supportsFormat(const string filename) const
{
	// I've tried and only can really get lame to know the format
	// from the extension unless I can do some analysis on it myself
	return(false);
}

const vector<string> ClameSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("MPEG Layer-3,2,1");

	return(names);
}

const vector<vector<string> > ClameSoundTranslator::getFormatExtensions() const
{
	vector<vector<string> > list;
	vector<string> extensions;

	extensions.clear();
	extensions.push_back("mp3");
	extensions.push_back("mp2");
	extensions.push_back("mp1");
	list.push_back(extensions);

	return(list);
}

