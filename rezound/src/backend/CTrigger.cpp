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

#include "CTrigger.h"

#include <algorithm>

void CTrigger::set(TriggerFunc triggerFunc,void *data)
{
	mutex.EnterMutex();
	try
	{
		REvent e(triggerFunc,data);
		//events.append(e);
		events.push_back(e);
		mutex.LeaveMutex();
	}
	catch(...)
	{
		mutex.LeaveMutex();
		throw;
	}
}

void CTrigger::unset(TriggerFunc triggerFunc,void *data)
{
	mutex.EnterMutex();
	try
	{
		//events.removeValue(e);
		vector<REvent>::iterator i=find(events.begin(),events.end(),REvent(triggerFunc,data));
		if(i!=events.end())
			events.erase(i);

		mutex.LeaveMutex();
	}
	catch(...)
	{
		mutex.LeaveMutex();
		throw;
	}
}

void CTrigger::trip()
{
	mutex.EnterMutex();
	try
	{
		for(size_t t=0;t<events.size();t++)
			events[t].triggerFunc(events[t].data);
		mutex.LeaveMutex();
	}
	catch(...)
	{
		mutex.LeaveMutex();
		throw;
	}
}



