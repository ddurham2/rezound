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


#include "CSoundManagerClient.h"
#include "CSoundPlayerChannel.h"
#include "AAction.h"

//#include <TStack.h>
#include <stack>

class CLoadedSound
{
public:
	CSoundManagerClient * const client;
	CSoundPlayerChannel * const channel;

	ASound *getSound() const; // has to get it thru channel

	// this stack is used for undoing actions (later a stack could be used for redoing actions)
	//TStack<AAction *> actions;
	stack<AAction *> actions;

	CLoadedSound(CSoundManagerClient *client,CSoundPlayerChannel *channel);
	CLoadedSound(const CLoadedSound &src);
	virtual ~CLoadedSound();

	void clearUndoHistory();

	bool isReadOnly() const;

	bool operator==(const CLoadedSound &rhs) const;

};



#endif
