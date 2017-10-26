/* 
 * Copyright (C) 2008 - David W. Durham
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

#ifndef CrossfadeEdgesDialog_H_
#define CrossfadeEdgesDialog_H__

#include "../../config/common.h"
#include "qt_compat.h"

class CrossfadeEdgesDialog;

#include "ActionParam/ActionParamDialog.h"

extern CrossfadeEdgesDialog *gCrossfadeEdgesDialog;

/* 
 * I derive from ActionParamDialog even though this isn't a dialog for any action.
 * I derive from it so that the user can have preset crossfade settings
 */
class CrossfadeEdgesDialog : public ActionParamDialog
{
public:
    CrossfadeEdgesDialog(QWidget *mainWindow);
    virtual ~CrossfadeEdgesDialog();

	void showIt();
};

#endif
