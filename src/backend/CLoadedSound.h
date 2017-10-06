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

#ifndef __CLoadedSound_H__
#define __CLoadedSound_H__

#include "../../config/common.h"

#include <string>
#include <stack>

class AAction;
class CSound;
class CSoundPlayerChannel;
class ASoundTranslator;

class CLoadedSound
{
public:
											// translator can be NULL
	CLoadedSound(const string filename,CSoundPlayerChannel *channel,bool isReadOnly,const ASoundTranslator *translator);
	CLoadedSound(const CLoadedSound &src);
	virtual ~CLoadedSound();

	void clearUndoHistory();

	const string getFilename() const;
	void changeFilename(const string newFilename);

	bool isReadOnly() const;

	CSoundPlayerChannel * const channel;
	CSound * const sound;
	const ASoundTranslator *translator; // can be NULL, is used to save (in contrast with save-as) the sound after an open

	// this stack is used for undoing actions (later a stack could be used for redoing actions)
	stack<AAction *> actions;

private:
	string filename;
 	bool _isReadOnly;

};



#endif
