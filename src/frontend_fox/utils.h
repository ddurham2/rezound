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

#ifndef __utils_h__
#define __utils_h__

#include "fox_compat.h"

static void setFontOfAllChildren(FXComposite *w,FXFont *f)
{
	if(w)
	{
		for(FXint t=0;t<w->numChildren();t++)
		{
			FXObject *c=w->childAtIndex(t);
			if(dynamic_cast<FXComposite *>(c))
				setFontOfAllChildren(static_cast<FXComposite *>(c),f);
			else if(dynamic_cast<FXComboBox *>(c))
				static_cast<FXComboBox *>(c)->setFont(f);
			else if(dynamic_cast<FXGroupBox *>(c))
				static_cast<FXGroupBox *>(c)->setFont(f);
			else if(dynamic_cast<FXHeader *>(c))
				static_cast<FXHeader *>(c)->setFont(f);
			else if(dynamic_cast<FXIconList *>(c))
				static_cast<FXIconList *>(c)->setFont(f);
			else if(dynamic_cast<FXLabel *>(c))
				static_cast<FXLabel *>(c)->setFont(f);
			else if(dynamic_cast<FXListBox *>(c))
				static_cast<FXListBox *>(c)->setFont(f);
			else if(dynamic_cast<FXList *>(c))
				static_cast<FXList *>(c)->setFont(f);
			else if(dynamic_cast<FXMDIChild *>(c))
				static_cast<FXMDIChild *>(c)->setFont(f);
			else if(dynamic_cast<FXMenuCaption *>(c))
				static_cast<FXMenuCaption *>(c)->setFont(f);
			else if(dynamic_cast<FXProgressBar *>(c))
				static_cast<FXProgressBar *>(c)->setFont(f);
#if REZ_FOX_VERSION>=10113
			else if(dynamic_cast<FXRuler *>(c))
				static_cast<FXRuler *>(c)->setFont(f);
#endif
			else if(dynamic_cast<FXSpinner *>(c))
				static_cast<FXSpinner *>(c)->setFont(f);
			else if(dynamic_cast<FXStatusLine *>(c))
				static_cast<FXStatusLine *>(c)->setFont(f);
			else if(dynamic_cast<FXTable *>(c))
				static_cast<FXTable *>(c)->setFont(f);
			else if(dynamic_cast<FXTextField *>(c))
				static_cast<FXTextField *>(c)->setFont(f);
			else if(dynamic_cast<FXText *>(c))
				static_cast<FXText *>(c)->setFont(f);
			else if(dynamic_cast<FXToolTip *>(c))
				static_cast<FXToolTip *>(c)->setFont(f);
			else if(dynamic_cast<FXTreeListBox *>(c))
				static_cast<FXTreeListBox *>(c)->setFont(f);
			else if(dynamic_cast<FXTreeList *>(c))
				static_cast<FXTreeList *>(c)->setFont(f);

		}
	}
}


// recursively call enable for all descendants of a given window
static void enableAllChildren(FXWindow *w)
{
	for(int t=0;t<w->numChildren();t++)
	{
		w->childAtIndex(t)->enable();
		enableAllChildren(w->childAtIndex(t));
	}
}

// recursively call disable for all descendants of a given window
static void disableAllChildren(FXWindow *w)
{
	for(int t=0;t<w->numChildren();t++)
	{
		w->childAtIndex(t)->disable();
		disableAllChildren(w->childAtIndex(t));
	}
}


#endif
