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

#ifndef __FXCheckBoxParamValue_H__
#define __FXCheckBoxParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <vector>
#include <string>

#include <fox/fx.h>

class CNestedDataFile;

class FXCheckBoxParamValue : public FXVerticalFrame
{
	FXDECLARE(FXCheckBoxParamValue);
public:
	FXCheckBoxParamValue(FXComposite *p,int opts,const char *title,bool checked);

	const bool getValue();
	void setValue(const bool checked);

	const string getTitle() const;

	void setHelpText(const FXString &text);
	FXString getHelpText() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

protected:
	FXCheckBoxParamValue() {}

private:
	FXCheckButton *checkBox;
};

#endif
