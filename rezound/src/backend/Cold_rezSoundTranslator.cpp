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

#include "Cold_rezSoundTranslator.h"
#include "CSound.h"

#include <stdio.h>

#include <stdexcept>

#include <CPath.h>


#define CURRENT_FORMAT_VERSION 1

Cold_rezSoundTranslator::Cold_rezSoundTranslator()
{
}

Cold_rezSoundTranslator::~Cold_rezSoundTranslator()
{
}

bool Cold_rezSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	FILE *f=NULL;
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

		f=fopen(filename.c_str(),"rb");
		if(f==NULL)
			throw(runtime_error(string(__func__)+" -- error opening '"+filename+"' -- "+strerror(errno)));

		fread(Header,1,4,f);
		if(strncmp(Header,"sNdF",4)!=0)
			throw(runtime_error(string(__func__)+" -- not a valid sound file or corrupt data"));

		int32_t tmp;

		fread(&tmp,sizeof(tmp),1,f);
		if(tmp>CURRENT_FORMAT_VERSION)
			throw(runtime_error(string(__func__)+" -- version of format is too great"));
		
		fread(&tmp,sizeof(tmp),1,f);
		unsigned channelCount=tmp;

		fread(&tmp,sizeof(tmp),1,f);
		unsigned sampleRate=tmp;

		fread(&tmp,sizeof(tmp),1,f);
		//unsigned bits=tmp;

		fread(&tmp,sizeof(tmp),1,f);
		Data_Style=(RezDataStyles)tmp;

		unsigned size=(CPath(filename).getSize()-ftell(f))/(channelCount*sizeof(sample_t));


		sound->createWorkingPoolFile(filename,sampleRate,channelCount,size);

		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));


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
		
		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}

		fclose(f);f=NULL;
	}
	catch(...)
	{
		if(f!=NULL)
			fclose(f);

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];
		throw;
	}
	return true;
}

bool Cold_rezSoundTranslator::onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength) const
{
	throw(runtime_error(string(__func__)+" -- unimplemented"));
}


bool Cold_rezSoundTranslator::handlesExtension(const string extension) const
{
	return(false);
}

bool Cold_rezSoundTranslator::supportsFormat(const string filename) const
{
	bool oldFormat=false;

	FILE *f=fopen(filename.c_str(),"rb");
	if(f!=NULL)
	{
		char buffer[5]={0};
		fread(buffer,1,4,f);
		if(strncmp(buffer,"sNdF",4)==0)
			oldFormat=true;
		fclose(f);
	}

	return(oldFormat);
}

const vector<string> Cold_rezSoundTranslator::getFormatNames() const
{
	vector<string> names;
	names.push_back("Old ReZound Format");

	return(names);
}

const vector<vector<string> > Cold_rezSoundTranslator::getFormatExtensions() const
{
	vector<vector<string> > list;

	vector<string> extensions;
	extensions.push_back("rez");
	list.push_back(extensions);

	return(list);
}

