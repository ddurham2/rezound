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

#include "CrezSoundTranslator.h"
#include "CSound.h"
#include "AStatusComm.h"

#include <stdio.h>

#include <stdexcept>
#include <typeinfo>

CrezSoundTranslator::CrezSoundTranslator()
{
}

CrezSoundTranslator::~CrezSoundTranslator()
{
}

// need to include this I use some of the template *methods* for types that are no where else
// so the explicit instantation at the bottom of CSound.cpp doesn't instantiate everything
#include <TPoolFile.cpp>

struct RFormatInfo1
{
	uint32_t version;
	uint64_t size;
	uint32_t sampleRate;
	uint32_t channelCount;

	void operator=(const RFormatInfo1 &src)
	{
		version=src.version;
		size=src.size;
		sampleRate=src.sampleRate;
		channelCount=src.channelCount;
	}
};
typedef TPoolAccesser<RFormatInfo1,CSound::PoolFile_t > CFormat1InfoPoolAccesser;


enum PCMTypes
{
	pcmSigned16BitInteger=1
};

struct RFormatInfo2
{
	uint32_t version;
	uint64_t size;
	uint32_t sampleRate;
	uint32_t channelCount;
	PCMTypes PCMType;

	void operator=(const RFormatInfo2 &src)
	{
		version=src.version;
		size=src.size;
		sampleRate=src.sampleRate;
		channelCount=src.channelCount;
		PCMType=src.PCMType;
	}
};
typedef TPoolAccesser<RFormatInfo2,CSound::PoolFile_t > CFormat2InfoPoolAccesser;



bool CrezSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	// after the "Format Info" pool is read, these will be populated with data from the file
	sample_pos_t size=0;
	unsigned sampleRate=0;
	unsigned channelCount=0;
	PCMTypes PCMType;

	CSound::PoolFile_t loadFromFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE); 
	loadFromFile.openFile(filename,false); // we could pass a read-only flag if there was such a thing???

	try
	{

		// read the meta data pool
		{
			// check version at the beginning of RFormat and perhaps handle things differently
			uint32_t version=0xffffffff;
			loadFromFile.readPoolRaw("Format Info",&version,sizeof(version));
			if(version==1)
			{
				const CFormat1InfoPoolAccesser a=loadFromFile.getPoolAccesser<RFormatInfo1>("Format Info");
				RFormatInfo1 r;
				r=a[0];

				if(r.size>MAX_LENGTH)
				{
					// ??? what should I do? truncate the sound or just error out?
				}

				size=r.size;
				sampleRate=r.sampleRate;
				channelCount=r.channelCount;
				PCMType=pcmSigned16BitInteger;
			}
			else if(version==2)
			{
				const CFormat2InfoPoolAccesser a=loadFromFile.getPoolAccesser<RFormatInfo2>("Format Info");
				RFormatInfo2 r;
				r=a[0];

				if(r.size>MAX_LENGTH)
				{
					// ??? what should I do? truncate the sound or just error out?
				}

				size=r.size;
				sampleRate=r.sampleRate;
				channelCount=r.channelCount;
				PCMType=r.PCMType;
			}
			else
				throw(runtime_error(string(__func__)+" -- unhandled format version: "+istring(version)));


			if(sampleRate<4000 || sampleRate>96000)
				throw(runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate)+" probably indicates a corrupt file"));
			if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could warn the user and just ignore the extra channels
				throw(runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in CSound.h"));

			sound->createWorkingPoolFile(filename,sampleRate,channelCount,size);
		}


		// read the output routing information
		// ??? need to do when I'm sure how it will be stored and there is actually a frontend to verify it
		// ??? perhaps know if the output routing information is different on this machine than it was on the saved machine
			// prompt the user to reset the information if it was different

#warning shouldnt I load/save the general data pools if perhaps I have flagged them to the persistant?

		// read the cues
		{
			const CSound::CCuePoolAccesser srcCues=loadFromFile.createPool<CSound::RCue>("Cues",false);
			sound->cueAccesser->clear();
			sound->cueAccesser->copyData(0,srcCues,0,srcCues.getSize(),true);
			sound->rebuildCueIndex();
		}


		// read the user notes
		{
			const TStaticPoolAccesser<int8_t,CSound::PoolFile_t> src=loadFromFile.createPool<int8_t>("UserNotes",false);

			string userNotes;

			char buffer[101];
			for(size_t t=0;t<src.getSize()/100;t++)
			{
				// ??? here we need to assert that char and int8_t are the same
				src.read((int8_t *)buffer,100);
				buffer[100]=0;
				userNotes+=buffer;
			}

			// ??? here we need to assert that char and int8_t are the same
			src.read((int8_t *)buffer,src.getSize()%100);
			buffer[src.getSize()%100]=0;
			userNotes+=buffer;
													
			sound->setUserNotes(userNotes);
		}
		

		// read the audio
		{
			// ??? need to do convertions to the native type
			if((PCMType==pcmSigned16BitInteger && typeid(sample_t)==typeid(int16_t)) )
			{
				for(unsigned i=0;i<channelCount;i++)
				{
					CStatusBar statusBar("Loading Channel "+istring(i+1)+"/"+istring(channelCount),0,99,true);

					CSound::CInternalRezPoolAccesser dest=sound->getAudioInternal(i);

					const sample_pos_t chunkSize=size/100;

					if(chunkSize>0)
					{
						for(unsigned int t=0;t<100;t++)
						{
							dest.copyData(t*chunkSize,loadFromFile.getPoolAccesser<sample_t>("Channel "+istring(i+1)),t*chunkSize,chunkSize);
							if(statusBar.update(t))
							{
								loadFromFile.closeFile(false,false);
								return false; // cancelled
							}
						}
					}
					dest.copyData(100*chunkSize,loadFromFile.getPoolAccesser<sample_t>("Channel "+istring(i+1)),100*chunkSize,size%100);
				}
			}
			else
				throw(runtime_error(string(__func__)+" -- unhandled format conversion while loading"));
		}

	}
	catch(...)
	{
		loadFromFile.closeFile(false,false);
		throw;
	}

	loadFromFile.closeFile(false,false);
	return true;
}

bool CrezSoundTranslator::onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength) const
{
	// ??? The user still needs to be able to choose what format the data will be saved in... whether 16bit float.. etc.. and the import/export process needs to convert to and from the working format

	remove(filename.c_str()); // remove file incase it exists
	CSound::PoolFile_t saveToFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE);
	saveToFile.openFile(filename,true);
	saveToFile.clear();

 	PCMTypes PCMType=pcmSigned16BitInteger; // ??? or what the user asked as export format 

	// write the meta data pool
	{
		RFormatInfo2 formatInfo2;

		formatInfo2.version=2;
		formatInfo2.size=saveLength;
		formatInfo2.sampleRate=sound->getSampleRate();
		formatInfo2.channelCount=sound->getChannelCount();
		formatInfo2.PCMType=PCMType;

		CFormat2InfoPoolAccesser formatInfoAccesser=saveToFile.createPool<RFormatInfo2>("Format Info");
		formatInfoAccesser.write(&formatInfo2,1,true);
	}

	// write the output routing information
	// ??? need to do when I'm sure how it will be stored and there is actually a frontend to verify it
	// ??? perhaps write something to know if the output routing information is different on this machine when loaded than it was on the saved machine

	// write the cues
	{
		// unless we're converting sample rates here, the sample positions in sound's cues are valid for saving
		CSound::CCuePoolAccesser destCues=saveToFile.createPool<CSound::RCue>("Cues");
		for(size_t t=0;t<sound->getCueCount();t++)
		{
			if(sound->getCueTime(t)>=saveStart && sound->getCueTime(t)<(saveStart+saveLength))
			{
				destCues.append(1);
				destCues[destCues.getSize()-1]=CSound::RCue(sound->getCueName(t).c_str(),sound->getCueTime(t)-saveStart,sound->isCueAnchored(t));
			}
		}
	}
	

	// write the user notes
	{
		TPoolAccesser<int8_t,CSound::PoolFile_t> dest=saveToFile.createPool<int8_t>("UserNotes");
		const string userNotes=sound->getUserNotes();
		dest.write((int8_t *)userNotes.c_str(),userNotes.length(),true);
	}


	// write the audio data
	{
		// ??? need to make sure it's going to fit before I start writing... a convertion in sample format could as much as double the size

		// need to do conversions from the native type ???
		if((PCMType==pcmSigned16BitInteger && typeid(sample_t)==typeid(int16_t)) )
		{
			for(unsigned i=0;i<sound->getChannelCount();i++)
			{
				CStatusBar statusBar("Saving Channel "+istring(i+1)+"/"+istring(sound->getChannelCount()),0,99,true);

				TPoolAccesser<sample_t,CSound::PoolFile_t> dest=saveToFile.createPool<sample_t>("Channel "+istring(i+1));

				const sample_pos_t chunkSize=saveLength/100;

				if(chunkSize>0)
				{
					for(unsigned int t=0;t<100;t++)
					{
						dest.copyData(t*chunkSize,sound->getAudio(i),t*chunkSize+saveStart,chunkSize,true);

						if(statusBar.update(t))
						{ // cancelled
							saveToFile.closeFile(false,true);
							return false;
						}
					}
				}
				dest.copyData(100*chunkSize,sound->getAudio(i),100*chunkSize+saveStart,saveLength%100,true);
			}
		}
		else
			throw(runtime_error(string(__func__)+" -- unhandled format conversion while saving"));
	}

	saveToFile.closeFile(false,false);

	return true;
}


bool CrezSoundTranslator::handlesExtension(const string extension) const
{
	return(extension=="rez");
}

bool CrezSoundTranslator::supportsFormat(const string filename) const
{
	// check for the 512th-xxx bytes containing REZOUND_POOLFILE_SIGNATURE
	// and must have at least 1024 bytes to read
	FILE *f=fopen(filename.c_str(),"rb");
	if(f==NULL)
		return(false);

	char buffer[1024];
	if(fread(buffer,1,1024,f)!=1024)
	{
		fclose(f);
		return(false);
	}
	if(strncmp(buffer+512,REZOUND_POOLFILE_SIGNATURE,strlen(REZOUND_POOLFILE_SIGNATURE))==0)
	{
		fclose(f);
		return(true);
	}

	return(false);
}

const vector<string> CrezSoundTranslator::getFormatNames() const
{
	vector<string> names;
	names.push_back("Native ReZound");

	return(names);
}

const vector<vector<string> > CrezSoundTranslator::getFormatExtensions() const
{
	vector<vector<string> > list;

	vector<string> extensions;
	extensions.push_back("rez");
	list.push_back(extensions);

	return(list);
}

