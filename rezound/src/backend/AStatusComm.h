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

#ifndef __AStatusComm_H__
#define __AStatusComm_H__


#include "../../config/common.h"

/*
 * This class is used to communicate status to and from the 
 * user the platform specific frontend should implement these
 * pure virtual methods to display error messages and ask
 * them question via popup windows
 */


/******************************************
- Error Message Philosophy:
   If an error code is retrived from a
   function, and that function was written
   SPECIFICALLY for this application, then
   that function should have displayed the
   error message.  However if the error
   occured within a function that was
   written to be used by other applications
   (i.e. DirectSound Class, other libraries)
   then an exception should have been raised
   and the exception caught by the caller
   and then the code specific to this
   application should display the error
   message with respect to the exception.
******************************************/

#include <string>

enum VSeverity {none,light,medium,hard,fatal};
enum VQuestion {yesnoQues=1,cancelQues=2};
enum VAnswer {yesAns,noAns,cancelAns,defaultAns};

class AStatusComm
{
public:
	virtual void error(const string &message,VSeverity severity=none)=0;
	virtual void warning(const string &message)=0;
	virtual void message(const string &message)=0;
	virtual VAnswer question(const string &message,VQuestion options)=0;

	virtual void beep()=0;

	virtual int beginProgressBar(const string &title)=0;
	virtual void updateProgressBar(int handle,int progress)=0; // progress is 0 to 100
	virtual void endProgressBar(int handle)=0;
	virtual void endAllProgressBars()=0;

	// breaks "msg -- msg -- msg" into "msg\n-- msg\n-- msg"
	static const string breakIntoLines(const string s);
};

extern AStatusComm *gStatusComm;

// Easy Coders
void Error(const string &message,VSeverity severity=none);
void Warning(const string &message);
void Message(const string &message);
VAnswer Question(const string &message,VQuestion options);

int beginProgressBar(const string &title);
inline static void updateProgressBar(int handle,int progress) { gStatusComm->updateProgressBar(handle,progress); }
void endProgressBar(int handle);
void endAllProgressBars();


/*
	I made these macros to make adding status bars to actions relatively
	painless. 
        Just invoke BEGIN_PROGRESS_BAR() at the beginning of the action giving it
	a very short description of what is going on and the first and last value
	of the counter described below.  Then in the action's loop, invoke 
	UPDATE_PROGRESS_BAR() giving it a counter value which goes from firstValue 
	to lastValue.  Finally, when the action is finished, invoke END_PROGRESS_BAR()

	I don't think it's a terrible thing that two progress bars are created when
	an action is applied to two channels, and it greatly simplifies the logic 
	involved in handling the progress bar.

	??? Perhaps later I will have the ability to cancel an action in the middle,
	but for now, I'm not going to do this.  It would probably be likely that 
	some actions could be cancellable and some not, so a flag for this would 
	also be given to the BEGIN_PROGRESS_BAR() macro
 */

#define BEGIN_PROGRESS_BAR(title,firstValue,lastValue)				\
	const int __progressHandle=beginProgressBar(title);			\
	const sample_pos_t __progressSub= firstValue;				\
	const sample_pos_t __valueDiff= ((lastValue)-(firstValue));		\
	const sample_pos_t __progressDiv= __valueDiff<100 ? 1 : ((__valueDiff+100-1)/100); \
	const sample_pos_t __progressMul= __valueDiff<100 ? ((100+__valueDiff-1)/__valueDiff) : 1; \
	sample_pos_t __lastProgress=0;

#define RESET_PROGRESS_BAR()							\
	__lastProgress=0;							\
	gStatusComm->updateProgressBar(__progressHandle,0);

#define UPDATE_PROGRESS_BAR(value)						\
	const sample_pos_t __progress=((value)-__progressSub)/__progressDiv;	\
	if(__progress!=__lastProgress)						\
		gStatusComm->updateProgressBar(__progressHandle,(__lastProgress=__progress)*__progressMul);

#define END_PROGRESS_BAR()							\
	endProgressBar(__progressHandle);


#endif
