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

#include <unistd.h>
#include "COSSSoundRecorder.h"

#include "CSoundManager.h"


#include <ctype.h>

#include <stdexcept>

#include <istring>

#include "CrezSound.h"
#include "Cold_rezSound.h"
#include "ClibaudiofileSound.h"

#include <cc++/path.h>

CSoundManager::CSoundManager()
{
}

CSoundManager::~CSoundManager()
{
	for(size_t t=0;t<clients.size();t++)
		delete clients[t];
}

// used to check the first 4 chars of the file for 'sNdF' to know if it's the old format
bool checkForOldRezFormat(const string &filename)
{
	bool oldFormat=false;
	{
		FILE *f=fopen(filename.c_str(),"rb");
		if(f!=NULL)
		{
			char buffer[5]={0};
			fread(buffer,1,4,f);
			if(strncmp(buffer,"sNdF",4)==0)
				oldFormat=true;
			fclose(f);
		}
	}
	return(oldFormat);
}

CSoundManagerClient CSoundManager::openSound(const string filename,const bool readOnly,const bool raw)
{
	string extension;
	if(!raw)
	{
		extension=istring(ost::Path(filename).Extension()).lower();
		if(extension=="")
			throw(runtime_error(string(__func__)+" -- cannot determine the extension on the filename: "+filename));
	}
	else
		extension="raw";

	ASound *sound;
	if(extension=="rez")
		sound=checkForOldRezFormat(filename) ? static_cast<ASound *>(new Cold_rezSound()) : static_cast<ASound *>(new CrezSound());
	else if(ClibaudiofileSound::supportsFormat(filename))
		sound=new ClibaudiofileSound();
	// else sox
	else
		throw(runtime_error(string(__func__)+" -- unhandled file type with extension: "+extension));

	try
	{
		sound->loadSound(filename);
	}
	catch(...)
	{
		delete sound;
		throw;
	}

	sounds.push_back(sound);
	closeSoundLater.push_back(false);
	soundReferenceCounts.push_back(0);

	return(CSoundManagerClient(sound,this,readOnly));
}

CSoundManagerClient CSoundManager::newSound(const string &filename,const unsigned sampleRate,const unsigned channels,const sample_pos_t size)
{
	string extension=istring(ost::Path(filename).Extension()).lower();
	if(extension=="")
		throw(runtime_error(string(__func__)+" -- cannot determine the extension on the filename: "+filename));

	ASound *sound;
	if(extension=="rez")
		sound=new CrezSound(filename,sampleRate,channels,size);
	else if(ClibaudiofileSound::handlesExtension(extension))
		sound=new ClibaudiofileSound(filename,sampleRate,channels,size);
	// else sox
	else
		throw(runtime_error(string(__func__)+" -- unhandled extension on the filename: "+filename));

	sounds.push_back(sound);
	closeSoundLater.push_back(false);
	soundReferenceCounts.push_back(0);

	COSSSoundRecorder r;
	r.initialize(sound,channels,sampleRate);
	r.start();
	system("sleep 3");
	r.deinitialize();

	return(CSoundManagerClient(sound,this,false));
}

void CSoundManager::closeSound(CSoundManagerClient &client)
{
	size_t index=findSoundIndex(client.sound);
	closeSoundLater[index]=true;
}


void CSoundManager::addClient(CSoundManagerClient *client)
{
	clients.push_back(client);
	addSoundReference(client->sound);
}

void CSoundManager::removeClient(CSoundManagerClient *client)
{
	for(size_t t=0;t<clients.size();t++)
	{
		if(clients[t]==client)
		{
			clients.erase(clients.begin()+t);
			removeSoundReference(client->sound);
			return;
		}
	}
	throw(runtime_error(string(__func__)+" -- client not found in list"));
}

void CSoundManager::addSoundReference(ASound *sound)
{
	soundReferenceCounts[findSoundIndex(sound)]++;
}

void CSoundManager::removeSoundReference(ASound *sound)
{
	size_t index=findSoundIndex(sound);
	if((--soundReferenceCounts[index])<=0)
	{
		if(closeSoundLater[index])
			sounds[index]->closeSound();
		delete sounds[index];

		sounds.erase(sounds.begin()+index);
		closeSoundLater.erase(closeSoundLater.begin()+index);
		soundReferenceCounts.erase(soundReferenceCounts.begin()+index);
	}
}

const size_t CSoundManager::findSoundIndex(ASound *sound)
{
	for(size_t t=0;t<sounds.size();t++)
	{
		if(sounds[t]==sound)
			return(t);
	}
	throw(runtime_error(string(__func__)+" -- sound object not found in list"));
}

