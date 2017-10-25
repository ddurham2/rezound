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

#include "CModalDialog.h"

#include "rememberShow.h"

CModalDialog::CModalDialog(QWidget *parent,const QString title/*,int w,int h*/,FrameTypes frameType,ShowTypes showType) :
	QDialog(parent,0),
	m_showType(showType),
	m_firstShowing(true)
{
	setupUi(this);
	setTitle(title);

	connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
}

CModalDialog::~CModalDialog()
{
}

void CModalDialog::on_buttonBox_accepted()
{
	if(validateOnOkay())
		accept();
}

void CModalDialog::showEvent(QShowEvent *event)
{
	if(m_firstShowing)
	{
		QSize hint=sizeHint();
		if(m_showType==stRememberSizeAndPosition)
		{
			rememberShow(this,m_origTitle.toStdString());
			resize(max(width(),hint.width()),max(height(),hint.height()));
		}
		else if(m_showType==stShrinkWrap)
		{
			resize(hint);
		}
		// else if(m_showType==stNoChange) .. do nothing
	}
	m_firstShowing=false;
	QDialog::showEvent(event);
}

void CModalDialog::hideEvent(QHideEvent * event)
{
	QDialog::hideEvent(event);
	if(m_showType==stRememberSizeAndPosition)
	{
		rememberHide(this,m_origTitle.toStdString());
	}
}

QWidget *CModalDialog::getFrame()
{
	return contentFrame;
}

QWidget *CModalDialog::getButtonFrame()
{
	return buttonFrame;
}

void CModalDialog::setTitle(const QString title)
{
	m_origTitle=title;
	QWidget::setWindowTitle(gettext(title.toStdString().c_str()));
}

const QString CModalDialog::getOrigTitle() const
{
	return m_origTitle;
}
