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

#ifndef __COSSSoundPlayer_H__
#define __COSSSoundPlayer_H__

#include "../../config/common.h"


#include "ASoundPlayer.h"

#include <cc++/thread.h>

class COSSSoundPlayer : public ASoundPlayer
{
public:

	COSSSoundPlayer();
	virtual ~COSSSoundPlayer();

	void initialize();
	void deinitialize();
	bool isInitialized() const;

private:

	bool initialized;
	int audio_fd;

	class CPlayThread : public ost::Thread
	{
	public:
		CPlayThread(COSSSoundPlayer *parent);
		virtual ~CPlayThread();

		void wait() { Terminate(); }

		bool kill;

	protected:

		COSSSoundPlayer *parent;

		void Run();

	};

	CPlayThread playThread;
	ost::Semaphore threadFinishedSem;

	friend class CPlayThread;
};


#endif
