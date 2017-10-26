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

#ifndef __ModalDialog_H__
#define __ModalDialog_H__

#include "../../config/common.h"

#include <QDialog>
#include <QShowEvent>
#include <QHideEvent>

// used to place an invisible FXFrame in another widget to make sure it has a minimum height or width
//#define ASSURE_HEIGHT(parent,height) new FXFrame(parent,FRAME_NONE|LAYOUT_SIDE_LEFT | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,1,height);
//#define ASSURE_WIDTH(parent,width) new FXFrame(parent,FRAME_NONE|LAYOUT_SIDE_BOTTOM | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,width,1);

/*
 * This is meant to be a base class for a modal dialog
 *
 * It goes ahead and creates two panels with an okay and
 * cancel button in the lower panel... The derived class
 * should put whatever is needed in the upper panel
 */
#include "ui_ModalDialog.h"
class ModalDialog : public QDialog, private Ui::ModalDialog
{
	Q_OBJECT
public:
	enum FrameTypes { ftHorizontal,ftVertical };
	enum ShowTypes { stRememberSizeAndPosition,stNoChange,stShrinkWrap };

	ModalDialog(QWidget *parent,const QString title/*,int w,int h*/,FrameTypes frameType=ftHorizontal,ShowTypes showType=stRememberSizeAndPosition);
	virtual ~ModalDialog();

	QWidget *getFrame();
	QWidget *getButtonFrame();

	
	void setTitle(const QString title);
	const QString getOrigTitle() const;

protected:

	// a derived class can override this to validate stuff before the okay button closes the window
	virtual bool validateOnOkay() { return true; }

	virtual void showEvent(QShowEvent *event);
	virtual void hideEvent(QHideEvent *event);

private Q_SLOTS:

	void on_buttonBox_accepted();

private:
	QString m_origTitle;
	ShowTypes m_showType;
	bool m_firstShowing;

};

#endif
