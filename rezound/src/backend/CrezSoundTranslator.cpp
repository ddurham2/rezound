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

#include <stdexcept>

CrezSoundTranslator::CrezSoundTranslator()
{
}

CrezSoundTranslator::~CrezSoundTranslator()
{
}

void CrezSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	CSound::PoolFile_t originalPoolFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE);

	originalPoolFile.openFile(filename,false);
	// ??? needs a status bar ??? perhaps I could pass a function object to it
	originalPoolFile.copyToFile(sound->getWorkingFilename(filename));
	originalPoolFile.closeFile(false,false);

	if(!sound->createFromWorkingPoolFileIfExists(filename,false))
		throw(runtime_error(string(__func__)+" -- internal error -- bad data in file or TPoolFile::copyToFile failed"));

	// undo the side-effect that createFromWrokingPoolFile set isModified to true
	sound->setIsModified(false);
}

void CrezSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	// this is not what I need to do.. I need to write the specific pools so I don't write temp pools and undo pools
	sound->poolFile.flushData();
	sound->poolFile.copyToFile(filename);
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

