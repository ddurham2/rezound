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

#ifndef __backend_initialize_H__
#define __backend_initialize_H__

#include "../../config/common.h"

class ASoundPlayer;
class ASoundFileManager;

// returns false if args were incorrect and initialization failed
bool initializeBackend(ASoundPlayer *&soundPlayer,int argc,char *argv[]);

// this should be called again after initializeBackend and the frontend has finished initializing
// so that the backend can handle more arguments (like loading files given on the command line)
// and it too returns false if the arguments were incorrect and the progam should halt
bool handleMoreBackendArgs(ASoundFileManager *fileManager,int argc,char *argv[]);

void deinitializeBackend();


#endif
