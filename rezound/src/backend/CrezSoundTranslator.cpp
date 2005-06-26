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
#include "AFrontendHooks.h"

#include <string.h> // for memcpy

#include <stdexcept>

#include <endian_util.h>

typedef TPoolAccesser<CSound::RCue::PackedChunk,CSound::PoolFile_t> CPackedCueAccesser;

CrezSoundTranslator::CrezSoundTranslator()
{
	if(sizeof(int24_t)!=3)
		Warning(string(__func__)+" -- sizeof(int24_t) is not 3!!! A new method will need to be devised (or somehow pack the struct");
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

// loads data from poolfile as type src_t and writes into dest as sample_t
template<typename src_t> inline bool CrezSoundTranslator::load_samples_from_X_to_native(unsigned i,CSound::PoolFile_t &loadFromFile,CSound *sound,const TStaticPoolAccesser<src_t,CSound::PoolFile_t> &src,const sample_pos_t size,CStatusBar &statusBar,Endians endian)
{
	const register sample_pos_t chunkSize=size/100;
	CSound::CInternalRezPoolAccesser dest=sound->getAudioInternal(i);
	sample_pos_t pos=0;
	
	// copy 100 chunks (where chunkSize is floor(size/100))
	for(unsigned int t=0;t<100 && chunkSize>0;t++)
	{
		if(endian==eLittleEndian)
		{ // need to convert from little endian to host endian
			for(sample_pos_t k=0;k<chunkSize;k++)
			{
				dest[pos]=convert_sample<src_t,sample_t>(lethe(src[pos]));
				pos++;
			}
		}
		else if(endian==eBigEndian)
		{ // need to convert from big endian to host endian
			for(sample_pos_t k=0;k<chunkSize;k++)
			{
				dest[pos]=convert_sample<src_t,sample_t>(bethe(src[pos]));;
				pos++;
			}
		}
		if(statusBar.update(t))
		{
			loadFromFile.closeFile(false,false);
			return false; // cancelled
		}
	}

	// copy remainder
	if(endian==eLittleEndian)
	{ // need to convert from little endian to host endian
		for(sample_pos_t k=0;k<size%100;k++)
		{
			dest[pos]=convert_sample<src_t,sample_t>(lethe(src[pos]));
			pos++;
		}
	}
	else if(endian==eBigEndian)
	{ // need to convert from big endian to host endian
		for(sample_pos_t k=0;k<size%100;k++)
		{
			dest[pos]=convert_sample<src_t,sample_t>(bethe(src[pos]));
			pos++;
		}
	}
	return true;
}

// loads data from poolfile as type sample_t and writes into dest as sample_t (and so uses copyData())
inline bool CrezSoundTranslator::load_samples__sample_t(unsigned i,CSound::PoolFile_t &loadFromFile,CSound *sound,const sample_pos_t size,CStatusBar &statusBar,Endians endian)
{
	CSound::CInternalRezPoolAccesser dest=sound->getAudioInternal(i);

	const register sample_pos_t chunkSize=size/100;

	for(unsigned int t=0;t<100 && chunkSize>0;t++)
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
#else // LITTLE ENDIAM
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

	// ??? perhaps handle this is the above for-loop on a last-iteration test
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

	return true;
}

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
				//loadFromFile.getPoolAccesser<RFormatInfo1::PackedChunk>("Format Info").read(&p,1);	doesn't work with gcc4 anymore
				loadFromFile.readPoolRaw("Format Info",&p,sizeof(p));

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
				//loadFromFile.getPoolAccesser<RFormatInfo2::PackedChunk>("Format Info").read(&p,1);	doesn't work with gcc4 anymore
				loadFromFile.readPoolRaw("Format Info",&p,sizeof(p));

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
				//loadFromFile.getPoolAccesser<RFormatInfo3::PackedChunk>("Format Info").read(&p,1);	doesn't work with gcc4 anymore
				loadFromFile.readPoolRaw("Format Info",&p,sizeof(p));

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

		// remember information for how to save the file if this class is also used to save it
		{
			AFrontendHooks::RezSaveParameters rsp;

			rsp.audioEncodingType=audioEncodingType;

			sound->getGeneralDataAccesser<AFrontendHooks::RezSaveParameters>("RezSaveParameters").write(&rsp,1);
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
			/* doesn't work with gcc4 anymore
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
			*/
			CSound::RCue::PackedChunk cueData;
			for(size_t t=0;t<loadFromFile.getPoolSize("Cues");t+=sizeof(cueData))
			{
				loadFromFile.readPoolRaw("Cues",&cueData,sizeof(cueData),t);

				CSound::RCue cue;
				cue.unpack(cueData);
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
		for(unsigned i=0;i<channelCount;i++)
		{
			CStatusBar statusBar(_("Loading Channel ")+istring(i+1)+"/"+istring(channelCount),0,99,true);

			if(audioEncodingType==aetPCMSigned8BitInteger)
			{
				TStaticPoolAccesser<int8_t,CSound::PoolFile_t> src=loadFromFile.getPoolAccesser<int8_t>("Channel "+istring(i+1));
				load_samples_from_X_to_native<int8_t>(i,loadFromFile,sound,src,size,statusBar,endian);
			}
			else if(audioEncodingType==aetPCMSigned16BitInteger)
			{
#if defined(SAMPLE_TYPE_S16)
				load_samples__sample_t(i,loadFromFile,sound,size,statusBar,endian);
#else
				TStaticPoolAccesser<int16_t,CSound::PoolFile_t> src=loadFromFile.getPoolAccesser<int16_t>("Channel "+istring(i+1));
				load_samples_from_X_to_native<int16_t>(i,loadFromFile,sound,src,size,statusBar,endian);
#endif
			}
			else if(audioEncodingType==aetPCMSigned24BitInteger)
			{
				TStaticPoolAccesser<int24_t,CSound::PoolFile_t> src=loadFromFile.getPoolAccesser<int24_t>("Channel "+istring(i+1));
				load_samples_from_X_to_native<int24_t>(i,loadFromFile,sound,src,size,statusBar,endian);
			}
			else if(audioEncodingType==aetPCMSigned32BitInteger)
			{
				TStaticPoolAccesser<int32_t,CSound::PoolFile_t> src=loadFromFile.getPoolAccesser<int32_t>("Channel "+istring(i+1));
				load_samples_from_X_to_native<int32_t>(i,loadFromFile,sound,src,size,statusBar,endian);
			}
			else if(audioEncodingType==aetPCM32BitFloat)
			{
#if defined(SAMPLE_TYPE_FLOAT)
				load_samples__sample_t(i,loadFromFile,sound,size,statusBar,endian);
#else
				TStaticPoolAccesser<float,CSound::PoolFile_t> src=loadFromFile.getPoolAccesser<float>("Channel "+istring(i+1));
				load_samples_from_X_to_native<float>(i,loadFromFile,sound,src,size,statusBar,endian);
#endif
			}
			else
				throw runtime_error(string(__func__)+" -- internal error -- unhandled audioEncodingType -- "+istring(audioEncodingType));
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

// ??? I would declare 'dest' in this function instead of passing it in except I get syntax errors trying to call TPoolFile::createPool<type>() for some reason
template<typename dest_t> inline bool CrezSoundTranslator::save_samples_from_native_as_X(unsigned i,CSound::PoolFile_t &saveToFile,const CSound *sound,TPoolAccesser<dest_t,CSound::PoolFile_t> dest,const sample_pos_t saveStart,const sample_pos_t saveLength,CStatusBar &statusBar)
{
	const CRezPoolAccesser src=sound->getAudio(i);
	dest.append(saveLength);

	const sample_pos_t chunkSize=saveLength/100;
	sample_pos_t pos=0;
	if(chunkSize>0)
	{
		for(unsigned int t=0;t<100;t++)
		{
			for(sample_pos_t k=0;k<chunkSize;k++)
			{
				dest[pos]=convert_sample<sample_t,dest_t>(src[pos+saveStart]);
				pos++;
			}

			if(statusBar.update(t))
			{
				// cancelled
				saveToFile.closeFile(false,true);
				return false;
			}
		}
	}

	for(sample_pos_t k=0;k<saveLength%100;k++)
	{
		dest[pos]=convert_sample<sample_t,dest_t>(src[pos+saveStart]);
		pos++;
	}

	return true;
}

inline bool CrezSoundTranslator::save_samples__sample_t(unsigned i,CSound::PoolFile_t &saveToFile,const CSound *sound,TPoolAccesser<sample_t,CSound::PoolFile_t> dest,const sample_pos_t saveStart,const sample_pos_t saveLength,CStatusBar &statusBar)
{
	const CRezPoolAccesser src=sound->getAudio(i);

	// sample_t is the same as what we're saving as
	const sample_pos_t chunkSize=saveLength/100;
	if(chunkSize>0)
	{
		for(unsigned int t=0;t<100;t++)
		{
			dest.copyData(t*chunkSize,src,t*chunkSize+saveStart,chunkSize,true);

			if(statusBar.update(t))
			{ // cancelled
				saveToFile.closeFile(false,true);
				return false;
			}
		}
	}
	dest.copyData(100*chunkSize,src,100*chunkSize+saveStart,saveLength%100,true);
	return true;
}

bool CrezSoundTranslator::onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs) const
{
	// get user preferences for saving the rez
	static bool parametersGotten=false;
	static AFrontendHooks::RezSaveParameters parameters;
	useLastUserPrefs&=parametersGotten;
	if(!useLastUserPrefs)
	{
		parameters=
			sound->containsGeneralDataPool("RezSaveParameters") 
			? 
				(sound->getGeneralDataAccesser<AFrontendHooks::RezSaveParameters>("RezSaveParameters")[0])
			: 
				(AFrontendHooks::RezSaveParameters){aetPCM32BitFloat}
			;

		if(!gFrontendHooks->promptForRezSaveParameters(parameters))
			return false;
		parametersGotten=true;
	}

	remove(filename.c_str()); // remove file in case it exists
	CSound::PoolFile_t saveToFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE);
	saveToFile.openFile(filename,true);
	saveToFile.clear();

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
		formatInfo3.audioEncodingType=parameters.audioEncodingType;


		RFormatInfo3::PackedChunk p;
		formatInfo3.pack(p);

		/* doesn't work with gcc4 anymore 
		CFormatInfo3PoolAccesser a=saveToFile.createPool<RFormatInfo3::PackedChunk>("Format Info");
		a.write(&p,1,true);
		*/
		TPoolAccesser<uint8_t,CSound::PoolFile_t> a(saveToFile.createPool<uint8_t>("Format Info"));
		a.write((const uint8_t *)&p,sizeof(p),true);
	}

	// write the output routing information
	// ??? need to do when I'm sure how it will be stored and there is actually a frontend to verify it
	// ??? perhaps write something to know if the output routing information is different on this machine when loaded than it was on the saved machine

	// write the cues
	{
		// unless we're converting sample rates here, the sample positions in sound's cues are valid for saving
		/* doesn't work with gcc4 anymore
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
		*/
		TPoolAccesser<uint8_t,CSound::PoolFile_t> destCues=saveToFile.createPool<uint8_t>("Cues");
		for(size_t t=0;t<sound->getCueCount();t++)
		{
			if(sound->getCueTime(t)>=saveStart && sound->getCueTime(t)<(saveStart+saveLength))
			{
				CSound::RCue::PackedChunk r;
				(*(sound->cueAccesser))[t].pack(r);
				destCues.write((const uint8_t *)&r,sizeof(r),true);
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
		// ??? need to make sure it's going to fit before I start writing... a conversion in sample format could as much as double the size
	// need to prompt the user about what format to save it in (also make the current format the default on the interface ???
	for(unsigned i=0;i<sound->getChannelCount();i++)
	{
		CStatusBar statusBar(_("Saving Channel ")+istring(i+1)+"/"+istring(sound->getChannelCount()),0,99,true);

		if(parameters.audioEncodingType==aetPCMSigned8BitInteger)
		{
			TPoolAccesser<int8_t,CSound::PoolFile_t> dest=saveToFile.createPool<int8_t>("Channel "+istring(i+1));
			if(!save_samples_from_native_as_X<int8_t>(i,saveToFile,sound,dest,saveStart,saveLength,statusBar))
				return false;
		}
		else if(parameters.audioEncodingType==aetPCMSigned16BitInteger)
		{
			TPoolAccesser<int16_t,CSound::PoolFile_t> dest=saveToFile.createPool<int16_t>("Channel "+istring(i+1));
#if defined(SAMPLE_TYPE_S16)
			if(!save_samples__sample_t(i,saveToFile,sound,dest,saveStart,saveLength,statusBar))
#else
			if(!save_samples_from_native_as_X<int16_t>(i,saveToFile,sound,dest,saveStart,saveLength,statusBar))
#endif
				return false;
		}
		else if(parameters.audioEncodingType==aetPCMSigned24BitInteger)
		{
			TPoolAccesser<int24_t,CSound::PoolFile_t> dest=saveToFile.createPool<int24_t>("Channel "+istring(i+1));
			if(!save_samples_from_native_as_X<int24_t>(i,saveToFile,sound,dest,saveStart,saveLength,statusBar))
				return false;
		}
		else if(parameters.audioEncodingType==aetPCMSigned32BitInteger)
		{
			TPoolAccesser<int32_t,CSound::PoolFile_t> dest=saveToFile.createPool<int32_t>("Channel "+istring(i+1));
			if(!save_samples_from_native_as_X<int32_t>(i,saveToFile,sound,dest,saveStart,saveLength,statusBar))
				return false;
		}
		else if(parameters.audioEncodingType==aetPCM32BitFloat)
		{
			TPoolAccesser<float,CSound::PoolFile_t> dest=saveToFile.createPool<float>("Channel "+istring(i+1));
#if defined(SAMPLE_TYPE_FLOAT)
			if(!save_samples__sample_t(i,saveToFile,sound,dest,saveStart,saveLength,statusBar))
#else
			if(!save_samples_from_native_as_X<float>(i,saveToFile,sound,dest,saveStart,saveLength,statusBar))
#endif
				return false;
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

