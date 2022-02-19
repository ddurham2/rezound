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

#include "ModalDialog.h"

#include "rememberShow.h"

ModalDialog::ModalDialog(QWidget *parent,const QString title/*,int w,int h*/,FrameTypes frameType,ShowTypes showType) :
	QDialog(parent,Qt::Widget),
	m_showType(showType),
	m_firstShowing(true)
{
	setupUi(this);
	setTitle(title);

	connect(buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
}

ModalDialog::~ModalDialog()
{
}

void ModalDialog::on_buttonBox_accepted()
{
	if(validateOnOkay())
		accept();
}

void ModalDialog::showEvent(QShowEvent *event)
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

void ModalDialog::hideEvent(QHideEvent * event)
{
	QDialog::hideEvent(event);
	if(m_showType==stRememberSizeAndPosition)
	{
		rememberHide(this,m_origTitle.toStdString());
	}
}

QWidget *ModalDialog::getFrame()
{
	return contentFrame;
}

QWidget *ModalDialog::getButtonFrame()
{
	return buttonFrame;
}

void ModalDialog::setTitle(const QString title)
{
	m_origTitle=title;
	QWidget::setWindowTitle(gettext(title.toStdString().c_str()));
}

const QString ModalDialog::getOrigTitle() const
{
	return m_origTitle;
}
