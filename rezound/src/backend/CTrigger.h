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

#ifndef __CTrigger_H__
#define __CTrigger_H__


#include "../../config/common.h"

#include <stddef.h>

#include <stdexcept>
#include <vector>

#include <cc++/thread.h>

typedef void (* TriggerFunc)(void *data);

class CTrigger
{
public:
	virtual ~CTrigger() { }

	virtual void set(TriggerFunc triggerFunc,void *data);
	virtual void unset(TriggerFunc triggerFunc,void *data);

	virtual void trip();

	virtual CTrigger &operator=(CTrigger &src)
	{
		throw(runtime_error(string(__func__)+" -- why assign?"));
	}

private:

	struct REvent
	{
		TriggerFunc triggerFunc;
		void *data;

		REvent(TriggerFunc _triggerFunc,void *_data)
		{
			triggerFunc=_triggerFunc;
			data=_data;
		}

		REvent(const REvent &src)
		{
			triggerFunc=src.triggerFunc;
			data=src.data;
		}

		bool operator==(const REvent &src)
		{
			if(triggerFunc==src.triggerFunc && data==src.data)
				return(true);
			return(false);
		}
	};

	vector<REvent> events;
	ost::Mutex mutex;


};

#endif
