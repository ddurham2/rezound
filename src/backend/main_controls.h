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

#ifndef __file_H__
#define __file_H__


#include "../../config/common.h"

// this file is used just for the frontend to simply invoke a function and not worry about it succeeding or not... 
// all exceptions will be popped up as error messages

#include <string>

#include "CSound_defs.h"
#include "CSoundPlayerChannel.h" // for LoopTypes enum

class ASoundFileManager;


// --- file operations ---
 	// returns false if a prompt for a filename was cancelled or there was an error loading
bool openSound(ASoundFileManager *soundFileManager,const string filename="");
void newSound(ASoundFileManager *soundFileManager);
void closeSound(ASoundFileManager *soundFileManager);
void saveSound(ASoundFileManager *soundFileManager);
void saveAsSound(ASoundFileManager *soundFileManager);
void revertSound(ASoundFileManager *soundFileManager);
void recordSound(ASoundFileManager *soundFileManager);

void recordMacro();

const bool exitReZound(ASoundFileManager *soundFileManager);



// --- play control operations ---
void play(ASoundFileManager *soundFileManager,CSoundPlayerChannel::LoopTypes loopType,bool selectionOnly);
void play(ASoundFileManager *soundFileManager,sample_pos_t position);
void pause(ASoundFileManager *soundFileManager);
void stop(ASoundFileManager *soundFileManager);

void jumpToBeginning(ASoundFileManager *soundFileManager);
void jumpToStartPosition(ASoundFileManager *soundFileManager);

void jumpToPreviousCue(ASoundFileManager *soundFileManager);
void jumpToNextCue(ASoundFileManager *soundFileManager);


// --- undo operations ---

void undo(ASoundFileManager *soundFileManager);
void clearUndoHistory(ASoundFileManager *soundFileManager);


#endif
