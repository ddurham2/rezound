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
 * This SoundTranslator class handles vox format I/O by interfacing with 
 * the vox and devox executables.
 * Information on the format and the vox and devox applications cab be
 * found at: http://www.cis.ksu.edu/~tim/vox/
 */

#include "CvoxSoundTranslator.h"

#include <typeinfo>
#include <stdexcept>

#include <CPath.h>
#include <TAutoBuffer.h>

#include "CSound.h"
#include "AFrontendHooks.h"
#include "AStatusComm.h"

#include "settings.h"

static string gPathToVox="";
static string gPathToDevox="";

CvoxSoundTranslator::CvoxSoundTranslator() :
	ApipedSoundTranslator()
{
}

CvoxSoundTranslator::~CvoxSoundTranslator()
{
}

bool CvoxSoundTranslator::checkForApp()
{
	gPathToVox=findAppOnPath("vox");
	if(gPathToVox=="")
		fprintf(stderr,"'vox' executable not found in $PATH -- vox format support will be disabled\n");

	gPathToDevox=findAppOnPath("devox");
	if(gPathToDevox=="")
		fprintf(stderr,"'devox' executable not found in $PATH -- vox format support will be disabled\n");

	return gPathToVox!="" && gPathToDevox!="";
}

	// ??? could just return a CSound object an have used the one constructor that takes the meta info
	// ??? but, then how would I be able to have createWorkingPoolFileIfExists
bool CvoxSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	bool ret=true;

	if(gPathToDevox=="")
		throw(runtime_error(string(__func__)+" -- path to 'devox' not set"));

	if(!checkThatFileExists(filename))
		throw(runtime_error(string(__func__)+" -- file not found, '"+filename+"'"));

	// remove the temp file; devox the input file to the temp file; cat the temp file for reading stdout; remove temp file
	const string tempFilename=gFallbackWorkDir+"/rez_devox_temp_output";
	const string cmdLine="rm -f "+tempFilename+" ; "+gPathToDevox+" -b 16 "+escapeFilename(filename)+" "+tempFilename+" ; cat "+tempFilename+" ; rm -f "+tempFilename+"";

	fprintf(stderr,"devox command line: '%s'\n",cmdLine.c_str());

	FILE *p=popen(cmdLine,"r",NULL);

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		AFrontendHooks::VoxParameters parameters;
		if(!gFrontendHooks->promptForVoxParameters(parameters))
			return false;

		// prompt for channels and sampleRate
		unsigned channelCount=parameters.channelCount;
		unsigned sampleRate=parameters.sampleRate;

		unsigned bits=16;

		#define REALLOC_FILE_SIZE (1024*1024/4)
		
		sound->createWorkingPoolFile(filename,sampleRate,channelCount,REALLOC_FILE_SIZE);
		
		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));

		#define BUFFER_SIZE 4096

		if(bits==16 && typeid(sample_t)==typeid(int16_t))
		{
			TAutoBuffer<sample_t> buffer(BUFFER_SIZE*channelCount);
			sample_pos_t pos=0;

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
			}

			// remove any extra allocated space
			if(sound->getLength()>pos)
				sound->removeSpace(pos,sound->getLength()-pos);
		}
		else
			throw(runtime_error(string(__func__)+" -- an unhandled bit rate of "+istring(bits)));

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		pclose(p);
	}
	catch(...)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		pclose(p);
	
		throw;
	}

	return ret;
}

bool CvoxSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	bool ret=true;

	if(gPathToVox=="")
		throw(runtime_error(string(__func__)+" -- path to 'vox' not set"));

	if(sound->getCueCount()>0 || sound->getUserNotes()!="")
	{
		if(Question("VOX Format does not support saving user notes or cues\nDo you wish to continue?",yesnoQues)!=yesAns)
			return false;
	}

	removeExistingFile(filename);

	const string tempFilename=gFallbackWorkDir+"/rez_devox_temp_input";
	string cmdLine="rm -f "+tempFilename+" ; cat > "+tempFilename+" ; vox -b 16 "+tempFilename+" "+escapeFilename(filename)+" ; rm -f "+tempFilename;

	fprintf(stderr,"vox command line: '%s'\n",cmdLine.c_str());

	setupSIGPIPEHandler();

	FILE *p=popen(cmdLine,"w",NULL);

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		const unsigned channelCount=sound->getChannelCount();
		const sample_pos_t size=sound->getLength();

		#define BITS 16 // has to go along with how we're writing it to the pipe below

		if(size>((0x7fffffff-4096)/((BITS/2)*channelCount)))
			throw(runtime_error(string(__func__)+" -- audio data is too large to be converted to vox (more than 2gigs of "+istring(BITS)+"bit/"+istring(channelCount)+"channels"));

		if(SIGPIPECaught)
			throw(runtime_error(string(__func__)+" -- vox aborted -- check stderr for more information"));

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

				if(SIGPIPECaught)
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

		pclose(p);

		restoreOrigSIGPIPEHandler();
	}
	catch(...)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		pclose(p);
	
		restoreOrigSIGPIPEHandler();

		throw;
	}

	if(!ret)
		unlink(filename.c_str()); // remove the cancelled file

	return ret;
}


bool CvoxSoundTranslator::handlesExtension(const string extension) const
{
	return extension=="vox";
}

bool CvoxSoundTranslator::supportsFormat(const string filename) const
{
	// can't tell
	return false;
}

const vector<string> CvoxSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("VOX Format - OKI ADPCM");

	return names;
}

const vector<vector<string> > CvoxSoundTranslator::getFormatExtensions() const
{
	vector<vector<string> > list;
	vector<string> extensions;

	extensions.clear();
	extensions.push_back("vox");
	list.push_back(extensions);

	return list;
}

