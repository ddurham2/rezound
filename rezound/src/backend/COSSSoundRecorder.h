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

#ifndef __COSSSoundRecorder_H__
#define __COSSSoundRecorder_H__

#include "../../config/common.h"


#include "ASoundRecorder.h"

#include <cc++/thread.h>

class COSSSoundRecorder : public ASoundRecorder
{
public:

	COSSSoundRecorder();
	virtual ~COSSSoundRecorder();

	void initialize(ASound *sound);
	void deinitialize();

	void redo();

	//????This was changed from it's private status
	//????To make gcc 2.95.2 happy -AV
	int audio_fd;
private:

	//????audio_fd was moved from here to public
	bool initialized;

	class CRecordThread : public ost::Thread
	{
	public:
		CRecordThread(COSSSoundRecorder *parent);
		virtual ~CRecordThread();

		void wait() { Terminate(); }

		bool kill;

	protected:

		COSSSoundRecorder *parent;

		void Run();

	};

	CRecordThread recordThread;

};


#endif
