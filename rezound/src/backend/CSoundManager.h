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

#ifndef __CSoundManager_H__
#define __CSoundManager_H__


#include "../../config/common.h"

class CSoundManager;

#include <string>
#include <vector>

#include "CSoundManagerClient.h"


/*
    This class is used to load or create sounds

    The companion class is CSoundManagerClient which is used to manipulate the
    sound once it is created.
*/
class CSoundManager
{
public:

	CSoundManager();
	virtual ~CSoundManager();

	CSoundManagerClient openSound(const string filename,const bool readOnly,const bool raw=false);
	CSoundManagerClient newSound(const string &filename,const unsigned sampleRate,const unsigned channels,const sample_pos_t size);
	void closeSound(CSoundManagerClient &client);

private:

	friend CSoundManagerClient;

	// information to keep up with for each SoundManagerClient created
	//TBasicList <CSoundManagerClient *> clients;
	vector<CSoundManagerClient *> clients;



	// information to keep up with for each ASound object created
	
	//TBasicList <ASound *> sounds;
	//CboolBasicList closeSoundLater;
	//CintBasicList soundReferenceCounts;
	vector<ASound *> sounds;
	vector<bool> closeSoundLater;
	vector<int> soundReferenceCounts;


	void addClient(CSoundManagerClient *client);
	void removeClient(CSoundManagerClient *client);


	void addSoundReference(ASound *sound);
	void removeSoundReference(ASound *sound);
	const size_t findSoundIndex(ASound *sound);

};

#endif
