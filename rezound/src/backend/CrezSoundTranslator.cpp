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
#include <string.h> // for memcpy

#include <stdexcept>
#include <typeinfo>

#include <endian_util.h>

enum Endians
{
	eLittleEndian=0,
	eBigEndian=1
};

typedef TPoolAccesser<CSound::RCue::PackedChunk,CSound::PoolFile_t> CPackedCueAccesser;

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

	typedef uint8_t PackedChunk[
		4+ //sizeof(version)+
		8+ //sizeof(size)+
		4+ //sizeof(sampleRate)+
		4  //sizeof(channelCount)
	];

	void unpack(const PackedChunk &r)
	{
		// unpack the values from r into the data members
	
		register unsigned offset=0;
		
		memcpy(&version,r+offset,sizeof(version));
		lethe(&version);
		offset+=sizeof(version);
		
		memcpy(&size,r+offset,sizeof(size));
		lethe(&size);
		offset+=sizeof(size);
		
		memcpy(&sampleRate,r+offset,sizeof(sampleRate));
		lethe(&sampleRate);
		offset+=sizeof(sampleRate);
		
		memcpy(&channelCount,r+offset,sizeof(channelCount));
		lethe(&channelCount);
		offset+=sizeof(channelCount);
	}
};


enum AudioEncodingTypes
{
	aetPCMSigned16BitInteger=1
};

struct RFormatInfo2
{
	uint32_t version;
	uint64_t size;
	uint32_t sampleRate;
	uint32_t channelCount;
	uint32_t audioEncodingType;

	typedef uint8_t PackedChunk[
		4+ //sizeof(version)+
		8+ //sizeof(size)+
		4+ //sizeof(sampleRate)+
		4+ //sizeof(channelCount)+
		4  //sizeof(audioEncodingType)
	];

	void unpack(const PackedChunk &r)
	{
		// unpack the values from r into the data members
	
		register unsigned offset=0;
		
		memcpy(&version,r+offset,sizeof(version));
		lethe(&version);
		offset+=sizeof(version);
		
		memcpy(&size,r+offset,sizeof(size));
		lethe(&size);
		offset+=sizeof(size);
		
		memcpy(&sampleRate,r+offset,sizeof(sampleRate));
		lethe(&sampleRate);
		offset+=sizeof(sampleRate);
		
		memcpy(&channelCount,r+offset,sizeof(channelCount));
		lethe(&channelCount);
		offset+=sizeof(channelCount);
		
		memcpy(&audioEncodingType,r+offset,sizeof(audioEncodingType));
		lethe(&audioEncodingType);
		offset+=sizeof(audioEncodingType);
	}
};

struct RFormatInfo3
{
	uint32_t version; /* always written little endian */
	uint8_t  endian; /* 0 little, 1 big - indicates the endian that the subsequent data was written in */
	uint64_t size;
	uint32_t sampleRate;
	uint32_t channelCount;
	uint32_t audioEncodingType;

	typedef uint8_t PackedChunk[
		4+ //sizeof(version)+
		1+ //sizeof(endian)+
		8+ //sizeof(size)+
		4+ //sizeof(sampleRate)+
		4+ //sizeof(channelCount)+
		4  //sizeof(audioEncodingType)
	];

	void pack(PackedChunk &r) const
	{
		// pack the values of the data members into r
		
		register unsigned offset=0;

		typeof(version) tVersion=hetle(version);
		memcpy(r+offset,&tVersion,sizeof(version));
		offset+=sizeof(version);

		memcpy(r+offset,&endian,sizeof(endian));
		offset+=sizeof(endian);

		memcpy(r+offset,&size,sizeof(size));
		offset+=sizeof(size);

		memcpy(r+offset,&sampleRate,sizeof(sampleRate));
		offset+=sizeof(sampleRate);

		memcpy(r+offset,&channelCount,sizeof(channelCount));
		offset+=sizeof(channelCount);

		memcpy(r+offset,&audioEncodingType,sizeof(audioEncodingType));
		offset+=sizeof(audioEncodingType);
	}

	void unpack(const PackedChunk &r)
	{
		// unpack the values from r into the data members

		register unsigned offset=0;

		memcpy(&version,r+offset,sizeof(version));
		lethe(&version);
		offset+=sizeof(version);
	
		memcpy(&endian,r+offset,sizeof(endian));
		offset+=sizeof(endian);

		memcpy(&size,r+offset,sizeof(size));
		offset+=sizeof(size);
		
		memcpy(&sampleRate,r+offset,sizeof(sampleRate));
		offset+=sizeof(sampleRate);
		
		memcpy(&channelCount,r+offset,sizeof(channelCount));
		offset+=sizeof(channelCount);
		
		memcpy(&audioEncodingType,r+offset,sizeof(audioEncodingType));
		offset+=sizeof(audioEncodingType);
		
		// now convert endian if necessary
#ifdef WORDS_BIGENDIAN
		if(endian==eLittleEndian)
		{ // little-endian data read on a big-endian machine
			swap_endian(&size);
			swap_endian(&sampleRate);
			swap_endian(&channelCount);
			swap_endian(&audioEncodingType);
		}
#else
		if(endian==eBigEndian)
		{ // big-endian data read on a little-endian machine
			swap_endian(&size);
			swap_endian(&sampleRate);
			swap_endian(&channelCount);
			swap_endian(&audioEncodingType);
		}
#endif
	}
};
typedef TPoolAccesser<RFormatInfo3::PackedChunk,CSound::PoolFile_t > CFormatInfo3PoolAccesser;

bool CrezSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	// after the "Format Info" pool is read, these will be populated with data from the file
	sample_pos_t size=0;
	Endians endian=eLittleEndian;
	unsigned sampleRate=0;
	unsigned channelCount=0;
	AudioEncodingTypes audioEncodingType;

	CSound::PoolFile_t loadFromFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE); 
	loadFromFile.openFile(filename,false); // we could pass a read-only flag if there was such a thing???

	try
	{

		// read the meta data pool
		{
			// check version at the beginning of RFormat and perhaps handle things differently
			uint32_t version=0xffffffff;
			loadFromFile.readPoolRaw("Format Info",&version,sizeof(version));
			lethe(&version);
			if(version==1)
			{
				// read packed version into p
				RFormatInfo1::PackedChunk p;
				loadFromFile.getPoolAccesser<RFormatInfo1::PackedChunk>("Format Info").read(&p,1);

				// unpack from p into r
				RFormatInfo1 r;
				r.unpack(p);

				// use values now in r
				if(r.size>MAX_LENGTH)
				{
					// ??? what should I do? truncate the sound or just error out?
				}

				size=r.size;
				endian=eLittleEndian;
				sampleRate=r.sampleRate;
				channelCount=r.channelCount;
				audioEncodingType=aetPCMSigned16BitInteger;
			}
			else if(version==2)
			{
				// read packed version into p
				RFormatInfo2::PackedChunk p;
				loadFromFile.getPoolAccesser<RFormatInfo2::PackedChunk>("Format Info").read(&p,1);

				// unpack from p into r
				RFormatInfo2 r;
				r.unpack(p);

				// use values now in r
				if(r.size>MAX_LENGTH)
				{
					// ??? what should I do? truncate the sound or just error out?
				}

				size=r.size;
				endian=eLittleEndian;
				sampleRate=r.sampleRate;
				channelCount=r.channelCount;
				audioEncodingType=(AudioEncodingTypes)r.audioEncodingType;
			}
			else if(version==3)
			{
				// read packed version into p
				RFormatInfo3::PackedChunk p;
				loadFromFile.getPoolAccesser<RFormatInfo3::PackedChunk>("Format Info").read(&p,1);

				// unpack values from p into r
				RFormatInfo3 r;
				r.unpack(p);

				// use values now in r
				if(r.size>MAX_LENGTH)
				{
					// ??? what should I do? truncate the sound or just error out?
				}

				size=r.size;
				endian=(Endians)r.endian;
				sampleRate=r.sampleRate;
				channelCount=r.channelCount;
				audioEncodingType=(AudioEncodingTypes)r.audioEncodingType;
			}
			else
				throw runtime_error(string(__func__)+" -- unhandled format version: "+istring(version));


			if(sampleRate<4000 || sampleRate>96000)
				throw runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate)+" probably indicates a corrupt file");
			if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could warn the user and just ignore the extra channels
				throw runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in CSound.h");

			sound->createWorkingPoolFile(filename,sampleRate,channelCount,size);
		}


		// read the output routing information
		// ??? need to do when I'm sure how it will be stored and there is actually a frontend to verify it
		// ??? perhaps know if the output routing information is different on this machine than it was on the saved machine
			// prompt the user to reset the information if it was different

#warning shouldnt I load/save the general data pools if perhaps I have flagged them to the persistant?


		// read the cues
		if(loadFromFile.containsPool("Cues"))
		{
			// have to write the cues into a packed byte array because 
			// simply reading/writing a struct on different platforms can 
			// yield different word alignments and padding  (the easiest thing to do would have been to use gcc __attribute__ ((packed)) on the structs, but I would only do that if I knew this extension existed on all compilers (maybe ANSI C will adopted this one day) ???)
			const CPackedCueAccesser srcCues=loadFromFile.getPoolAccesser<CSound::RCue::PackedChunk>("Cues");
			sound->cueAccesser->clear();
			sound->cueAccesser->seek(0);
			for(size_t t=0;t<srcCues.getSize();t++)
			{
				CSound::RCue::PackedChunk r;
				srcCues.read(&r,1);

				CSound::RCue cue;
				cue.unpack(r);
				sound->cueAccesser->write(&cue,1,true);
			}
			sound->rebuildCueIndex();
		}


		// read the user notes
		{
			const TStaticPoolAccesser<char,CSound::PoolFile_t> src=loadFromFile.createPool<char>("UserNotes",false);

			string userNotes;

			char buffer[101];
			for(size_t t=0;t<src.getSize()/100;t++)
			{
				src.read((char *)buffer,100);
				buffer[100]=0;
				userNotes+=buffer;
			}

			src.read((char *)buffer,src.getSize()%100);
			buffer[src.getSize()%100]=0;
			userNotes+=buffer;
													
			sound->setUserNotes(userNotes);
		}
		

		// read the audio
		{
			// ??? need to do convertions to the native type
			if((audioEncodingType==aetPCMSigned16BitInteger && typeid(sample_t)==typeid(int16_t)) )
			{
				for(unsigned i=0;i<channelCount;i++)
				{
					CStatusBar statusBar(_("Loading Channel ")+istring(i+1)+"/"+istring(channelCount),0,99,true);

					CSound::CInternalRezPoolAccesser dest=sound->getAudioInternal(i);

					const register sample_pos_t chunkSize=size/100;

					if(chunkSize>0)
					{
						for(unsigned int t=0;t<100;t++)
						{
							dest.copyData(t*chunkSize,loadFromFile.getPoolAccesser<sample_t>("Channel "+istring(i+1)),t*chunkSize,chunkSize);
#ifdef WORDS_BIGENDIAN
							if(endian==eLittleEndian)
							{ // need to convert from little endian to big
								const sample_pos_t start=t*chunkSize;
								const sample_pos_t end=start+chunkSize;
								for(unsigned k=start;k<end;k++)
									dest[k]=swap_endian(dest[k]);
							}
#else // LITTLE ENDIAN
							if(endian==eBigEndian)
							{ // need to convert from big endian to little
								const sample_pos_t start=t*chunkSize;
								const sample_pos_t end=start+chunkSize;
								for(unsigned k=start;k<end;k++)
									dest[k]=swap_endian(dest[k]);
							}
#endif
							if(statusBar.update(t))
							{
								loadFromFile.closeFile(false,false);
								return false; // cancelled
							}
						}
					}
					dest.copyData(100*chunkSize,loadFromFile.getPoolAccesser<sample_t>("Channel "+istring(i+1)),100*chunkSize,size%100);
#ifdef WORDS_BIGENDIAN
					if(endian==eLittleEndian)
					{ // need to convert from little endian to big
						const sample_pos_t start=100*chunkSize;
						const sample_pos_t end=start+(size%100);
						for(unsigned k=start;k<end;k++)
							dest[k]=swap_endian(dest[k]);
					}
#else // LITTLE ENDIAN
					if(endian==eBigEndian)
					{ // need to convert from big endian to little
						const sample_pos_t start=100*chunkSize;
						const sample_pos_t end=start+(size%100);
						for(unsigned k=start;k<end;k++)
							dest[k]=swap_endian(dest[k]);
					}
#endif
				}
			}
			else
				throw runtime_error(string(__func__)+" -- unhandled format conversion while loading");
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

bool CrezSoundTranslator::onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs) const
{
	// ??? The user still needs to be able to choose what format the data will be saved in... whether 16bit float.. etc.. and the import/export process needs to convert to and from the working format

	remove(filename.c_str()); // remove file incase it exists
	CSound::PoolFile_t saveToFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE);
	saveToFile.openFile(filename,true);
	saveToFile.clear();

 	AudioEncodingTypes audioEncodingType=aetPCMSigned16BitInteger; // ??? or what the user asked as export format 

	// write the meta data pool
	{
		RFormatInfo3 formatInfo3;

		formatInfo3.version=3;
#ifdef WORDS_BIGENDIAN
		formatInfo3.endian=eBigEndian;
#else
		formatInfo3.endian=eLittleEndian;
#endif
		formatInfo3.size=saveLength;
		formatInfo3.sampleRate=sound->getSampleRate();
		formatInfo3.channelCount=sound->getChannelCount();
		formatInfo3.audioEncodingType=audioEncodingType;


		RFormatInfo3::PackedChunk p;
		formatInfo3.pack(p);

		CFormatInfo3PoolAccesser a=saveToFile.createPool<RFormatInfo3::PackedChunk>("Format Info");
		a.write(&p,1,true);
	}

	// write the output routing information
	// ??? need to do when I'm sure how it will be stored and there is actually a frontend to verify it
	// ??? perhaps write something to know if the output routing information is different on this machine when loaded than it was on the saved machine

	// write the cues
	{
		// unless we're converting sample rates here, the sample positions in sound's cues are valid for saving
		CPackedCueAccesser destCues=saveToFile.createPool<CSound::RCue::PackedChunk>("Cues");
		for(size_t t=0;t<sound->getCueCount();t++)
		{
			if(sound->getCueTime(t)>=saveStart && sound->getCueTime(t)<(saveStart+saveLength))
			{
				CSound::RCue::PackedChunk r;
				(*(sound->cueAccesser))[t].pack(r);
				destCues.write(&r,1,true);
			}
		}
	}
	

	// write the user notes
	{
		TPoolAccesser<char,CSound::PoolFile_t> dest=saveToFile.createPool<char>("UserNotes");
		const string userNotes=sound->getUserNotes();
		dest.write(userNotes.c_str(),userNotes.length(),true);
	}


	// write the audio data
	{
		// ??? need to make sure it's going to fit before I start writing... a convertion in sample format could as much as double the size

		// need to do conversions from the native type ???
		if((audioEncodingType==aetPCMSigned16BitInteger && typeid(sample_t)==typeid(int16_t)) )
		{
			for(unsigned i=0;i<sound->getChannelCount();i++)
			{
				CStatusBar statusBar(_("Saving Channel ")+istring(i+1)+"/"+istring(sound->getChannelCount()),0,99,true);

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
			throw runtime_error(string(__func__)+" -- unhandled format conversion while saving");
	}

	saveToFile.closeFile(false,false);

	return true;
}


bool CrezSoundTranslator::handlesExtension(const string extension,const string filename) const
{
	return extension=="rez";
}

bool CrezSoundTranslator::supportsFormat(const string filename) const
{
	// check for the 512th-xxx bytes containing REZOUND_POOLFILE_SIGNATURE
	// and must have at least 1024 bytes to read
	FILE *f=fopen(filename.c_str(),"rb");
	if(f==NULL)
		return false;

	char buffer[1024];
	if(fread(buffer,1,1024,f)!=1024)
	{
		fclose(f);
		return false;
	}
	if(strncmp(buffer+512,REZOUND_POOLFILE_SIGNATURE,strlen(REZOUND_POOLFILE_SIGNATURE))==0)
	{
		fclose(f);
		return true;
	}

	return false;
}

const vector<string> CrezSoundTranslator::getFormatNames() const
{
	vector<string> names;
	names.push_back(_("Native ReZound"));

	return names;
}

const vector<vector<string> > CrezSoundTranslator::getFormatFileMasks() const
{
	vector<vector<string> > list;

	vector<string> fileMasks;
	fileMasks.push_back("*.rez");
	list.push_back(fileMasks);

	return list;
}

