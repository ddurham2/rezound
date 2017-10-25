/* 
 * Copyright (C) 2007 - David W. Durham
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

#include "CMIDISampleDumpParametersDialog.h"

#include <stdexcept>
#include <istring>

#include "../backend/CActionParameters.h"

CMIDISampleDumpParametersDialog::CMIDISampleDumpParametersDialog(QWidget *mainWindow) :
	CActionParamDialog(mainWindow)
{
	setTitle(N_("MIDI Sample Dump"));

	QWidget *p0=newVertPanel(NULL);

	m_waitForDump=addCheckBoxEntry(p0,N_("Wait for Dump"),true);
	connect(m_waitForDump,SIGNAL(changed()),this,SLOT(onWaitForDumpCheckBox_changed()));
	
	m_sampleID=addNumericTextEntry(p0,N_("Sample ID"),"",0,0,16384);
	m_sampleID->setStep(1.0);
	m_sampleID->setDecimals(0);

	m_sysexChannel=addNumericTextEntry(p0,N_("SysEx Channel"),"",0,0,127);
	m_sysexChannel->setStep(1.0);
	m_sysexChannel->setDecimals(0);

	m_loopType=addNumericTextEntry(p0,N_("Loop Type"),"",0,0,127);
	m_loopType->setStep(1.0);
	m_loopType->setDecimals(0);
}

bool CMIDISampleDumpParametersDialog::showForOpen(int &sysExChannel,int &waveformId)
{
	CActionParameters actionParameters(NULL);

	m_waitForDump->setValue(waveformId<0);
	m_waitForDump->show();

	if(waveformId>=0)
		m_sampleID->setValue(waveformId);

	if(sysExChannel>=0)
		m_sysexChannel->setValue(sysExChannel);

	m_loopType->hide();

	onWaitForDumpCheckBox_changed();

	if(CActionParamDialog::show(NULL,&actionParameters))
	{

		if(m_waitForDump->getValue())
		{
			waveformId=-1;
			sysExChannel=-1;
		}
		else
		{
			waveformId=(int)m_sampleID->getValue();
			sysExChannel=(int)m_sysexChannel->getValue();
		}
		return true;
	}
	return false;
}

bool CMIDISampleDumpParametersDialog::showForSave(int &sysExChannel,int &waveformId,int &loopType)
{
	CActionParameters actionParameters(NULL);

	m_waitForDump->setValue(false);
	m_waitForDump->hide();

	if(waveformId>=0)
		m_sampleID->setValue(waveformId);

	if(sysExChannel>=0)
		m_sysexChannel->setValue(sysExChannel);

	if(loopType>=0)
		m_loopType->setValue(loopType);
	m_loopType->show();

	onWaitForDumpCheckBox_changed();

	if(CActionParamDialog::show(NULL,&actionParameters))
	{
		waveformId=(int)m_sampleID->getValue();
		sysExChannel=(int)m_sysexChannel->getValue();
		loopType=(int)m_loopType->getValue();
		return true;
	}
	return false;
}

void CMIDISampleDumpParametersDialog::onWaitForDumpCheckBox_changed()
{
	m_sampleID->setEnabled(!m_waitForDump->getValue());
	m_sysexChannel->setEnabled(!m_waitForDump->getValue());
}

