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

#include "Cold_rezSound.h"

/* ???
 * VERY Quick and dirty implementation to read the old rezound format so I can load all my old sounds... only loads if the internal format is set to CD quality
 */

#include <stdio.h>
#include <string.h>

#include <stdexcept>
#include <typeinfo>
#include <memory>
#include <string>

#include <cc++/path.h>

#include <istring>

#define CURRENT_FORMAT_VERSION 1

Cold_rezSound::Cold_rezSound() :
	ASound()
{
}

void Cold_rezSound::loadSound(const string _filename)
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
		enum RezDataStyles
		{
			NON_INTERLACED_DATA=0,
			INTERLACED_DATA=1
		} Data_Style;
		
		char Header[5];
		Header[0]=0;

		FILE *f=fopen(filename.c_str(),"rb");
		if(f==NULL)
			throw(runtime_error(string(__func__)+" -- error opening '"+filename+"' -- "+strerror(errno)));

		fread(Header,1,4,f);
		if(strncmp(Header,"sNdF",4)!=0)
		{
			// Error
			fclose(f);
			throw(runtime_error(string(__func__)+" -- not a valid sound file or corrupt data"));
		}

		int32_t tmp;

		fread(&tmp,sizeof(tmp),1,f);
		if(tmp>CURRENT_FORMAT_VERSION)
		{
			// Error
			fclose(f);
			throw(runtime_error(string(__func__)+" -- version of format is too great"));
		}
		
		fread(&tmp,sizeof(tmp),1,f);
		channelCount=tmp;

		fread(&tmp,sizeof(tmp),1,f);
		sampleRate=tmp;

		fread(&tmp,sizeof(tmp),1,f);
		//bits=tmp;

		fread(&tmp,sizeof(tmp),1,f);
		Data_Style=(RezDataStyles)tmp;

		size=(ost::Path(filename).getSize()-ftell(f))/(channelCount*sizeof(sample_t));


		createWorkingPoolFile(filename);

		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(getAudio(t));


		sample_t buffer[4096];
		size_t count=size/4096+1;
		for(unsigned c=0;c<channelCount;c++)
		{
			sample_pos_t pos=0;
			for(size_t t=0;t<count;t++)
			{
				const size_t chunkSize=  (t==count-1 ) ? size%4096 : 4096;
				if(chunkSize!=0)
				{
					if(fread(buffer,sizeof(sample_t),chunkSize,f)!=chunkSize)
						throw(runtime_error(string(__func__)+" -- error reading audio data -- "+strerror(errno)));

					for(size_t i=0;i<chunkSize;i++)
						(*(accessers[c]))[pos++]=buffer[i];
				}
			}
		}
		
		fclose(f);

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}
	}
	catch(...)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		throw;
	}

	matchUpChannelLengths(NIL_SAMPLE_POS);
}

void Cold_rezSound::saveSound(const string filename)
{
	throw(runtime_error(string(__func__)+" -- unimplemented"));
	/*
	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		errorMessage="";
		afSetErrorHandler(errorFunction);

		// ??? all the following parameters need to be passed in somehow as the export format
		AFfilesetup newFile=afNewFileSetup();
		afInitFileFormat(newFile,AF_FILE_WAVE); 
		afInitSampleFormat(newFile,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(int16_t)*8); // ??? int16_t matching AF_SAMPFMT_TWOSCOMP
		afInitChannels(newFile,AF_DEFAULT_TRACK,channelCount);
		afInitRate(newFile,AF_DEFAULT_TRACK,sampleRate);

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

		afFreeFileSetup(newFile);

		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(getAudio(t));


		TAutoBuffer<sample_t> buffer((size_t)(channelCount*4096));
		sample_pos_t pos=0;
		AFframecount count=size/4096+1;
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
		}

		
		afCloseFile(h);

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}
	}
	catch(...)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		throw;
	}
	*/

}


