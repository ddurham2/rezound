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

#ifndef __ClibaudiofileSoundTranslator_H__
#define __ClibaudiofileSoundTranslator_H__

#include "../../config/common.h"

#include "ASoundTranslator.h"

#include <audiofile.h>

class ClibaudiofileSoundTranslator : public ASoundTranslator
{
public:
	ClibaudiofileSoundTranslator();
	virtual ~ClibaudiofileSoundTranslator();

	bool handlesExtension(const string extension) const;
	bool supportsFormat(const string filename) const;

protected:

	void onLoadSound(const string filename,CSound *sound) const;
	void onSaveSound(const string filename,CSound *sound) const;

	void loadSoundGivenSetup(const string filename,CSound *sound,AFfilesetup initialSetup) const;
	void saveSoundGivenSetup(const string filename,CSound *sound,AFfilesetup initialSetup) const;

private:

};

#endif
