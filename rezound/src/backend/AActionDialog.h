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

#ifndef __AActionDialog_H__
#define __AActionDialog_H__

#include "../../config/common.h"

class AActionDialog;

#include <stddef.h>

class CActionParameters;
class CActionSound;

/*
 * Any dialog shown to the user for an action to be performed should derived from 
 * this class so that the backend can show the dialog independant of the frontend
 * implementation.
 *
 * The show method should return true if the action is to be performed or false if 
 * the users press say a cancel button, it should also fill actionParameters with
 * the values for the action to use.  The order and type of those parameters should 
 * be agreed upon by the action implementation and the dialog implementation.
 */
class AActionDialog
{
public:
	AActionDialog();
	virtual ~AActionDialog();

	virtual bool show(CActionSound *actionSound,CActionParameters *actionParameters)=0;

	// can be implemented to get data from the dialog which does not fit the normal model of passing parameters to actions
	// 	namely, right now, the matrix of bools that indicates how to paste data is obtained through this method
	// 	I wouldn't need to necessarily use this method if actionParameters had perhaps a 2dim bool array type or something
	virtual void *getUserData() { return(NULL); }

	// performAction sets this to true if a dialog was shown else false
	bool wasShown;
};

#endif
