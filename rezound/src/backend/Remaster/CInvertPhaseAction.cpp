/* 
 * Copyright (C) 2003 - David W. Durham
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

#include "CInvertPhaseAction.h"

CInvertPhaseAction::CInvertPhaseAction(const CActionSound actionSound) :
	AAction(actionSound),
	allowCancel(true)
{
}

CInvertPhaseAction::~CInvertPhaseAction()
{
}

bool CInvertPhaseAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar(_("Inverting Phase -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),start,stop,allowCancel);

			CRezPoolAccesser audio=actionSound.sound->getAudio(i);

			for(sample_pos_t t=start;t<=stop;t++)
			{
				audio[t]=ClipSample(-audio[t]);

				if(statusBar.update(t))
				{ // cancelled
					statusBar.hide();

					// undo what we've done so-far for this channel
					{
						CStatusBar statusBar(_("Cancelling Invert Phase -- Channel ")+istring(channelsDoneCount),start,t);
						for(sample_pos_t j=start;j<t;j++)
						{
							audio[j]=ClipSample(-audio[j]);
							statusBar.update(j);
						}
					}

					// undo all previously processed channels
					for(unsigned k=0;k<i;k++)
					{
						if(actionSound.doChannel[k])
						{
							CRezPoolAccesser audio=actionSound.sound->getAudio(k);
							CStatusBar statusBar(_("Cancelling Invert Phase -- Channel ")+istring(--channelsDoneCount),start,stop);
							for(sample_pos_t t=start;t<=stop;t++)
							{
								audio[t]=ClipSample(-audio[t]);
								statusBar.update(t);
							}
						}
					}

					return false;
				}
			}
			actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	return true;
}

AAction::CanUndoResults CInvertPhaseAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}

void CInvertPhaseAction::undoActionSizeSafe(const CActionSound &_actionSound)
{
	CActionSound actionSound(_actionSound);
	allowCancel=false;
	try
	{
		doActionSizeSafe(actionSound,false);
		allowCancel=true;
	}
	catch(...)
	{
		allowCancel=true;
		throw;
	}
}



// ------------------------------

CInvertPhaseActionFactory::CInvertPhaseActionFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Invert Phase"),"",channelSelectDialog,NULL)
{
}

CInvertPhaseActionFactory::~CInvertPhaseActionFactory()
{
}

CInvertPhaseAction *CInvertPhaseActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CInvertPhaseAction(actionSound);
}

const string CInvertPhaseActionFactory::getExplanation() const
{
	return _("Invert Phase 180 degrees");
}
