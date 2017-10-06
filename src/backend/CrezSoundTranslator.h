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

#ifndef __CrezSoundTranslator_H__
#define __CrezSoundTranslator_H__

#include "../../config/common.h"

#include "ASoundTranslator.h"

#include <CSound.h>
class CStatusBar;

class CrezSoundTranslator : public ASoundTranslator
{
public:
	CrezSoundTranslator();
	virtual ~CrezSoundTranslator();

	bool handlesExtension(const string extension,const string filename) const;
	bool supportsFormat(const string filename) const;

	const vector<string> getFormatNames() const;
	const vector<vector<string> > getFormatFileMasks() const;


protected:

	bool onLoadSound(const string filename,CSound *sound) const;
	bool onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs) const;

private:
	template<typename src_t> static inline bool load_samples_from_X_to_native(unsigned i,CSound::PoolFile_t &loadFromFile,CSound *sound,const TStaticPoolAccesser<src_t,CSound::PoolFile_t> &src,const sample_pos_t size,CStatusBar &statusBar,Endians endian);
	static inline bool load_samples__sample_t(unsigned i,CSound::PoolFile_t &loadFromFile,CSound *sound,const sample_pos_t size,CStatusBar &statusBar,Endians endian);

	template<typename dest_t> static inline bool save_samples_from_native_as_X(unsigned i,CSound::PoolFile_t &saveToFile,const CSound *sound,TPoolAccesser<dest_t,CSound::PoolFile_t> dest,const sample_pos_t saveStart,const sample_pos_t saveLength,CStatusBar &statusBar);
	static inline bool save_samples__sample_t(unsigned i,CSound::PoolFile_t &saveToFile,const CSound *sound,TPoolAccesser<sample_t,CSound::PoolFile_t> dest,const sample_pos_t saveStart,const sample_pos_t saveLength,CStatusBar &statusBar);

};

#endif
