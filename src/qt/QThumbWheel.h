/****************************************************************************
** $Id: qt/qthumbwheel.h   3.2.1   edited May 13 09:08 $
**
** Definition of QThumbWheel class
**
** Created : 010205
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
** -------------------------------------------------------------------
** David Durham -- 2/6/2006 -- ported to Qt 4.1
**
**********************************************************************/

#ifndef QTHUMBWHEEL_H
#define QTHUMBWHEEL_H

#include <QAbstractSlider>

class QThumbWheel : public QAbstractSlider
{
    Q_OBJECT

public:
    QThumbWheel( QWidget *parent=0);
    virtual ~QThumbWheel();

    //virtual void        setOrientation( Orientation );
    //Orientation         orientation() const;
    //virtual void        setTracking( bool enable );
    //bool                tracking() const;
    virtual void        setTransmissionRatio( double r );
    double              transmissionRatio() const;

/*
public Q_SLOTS:
    virtual void setValue( int );

Q_SIGNALS:
    void        valueChanged( int value );
    */

protected:
    //void        valueChange();
    //void        rangeChange();
    //void        stepChange();

    void        keyPressEvent( QKeyEvent * );
    void        mousePressEvent( QMouseEvent * );
    void        mouseReleaseEvent( QMouseEvent * );
    void        mouseMoveEvent( QMouseEvent * );
    void        wheelEvent( QWheelEvent * );
    //void        focusInEvent( QFocusEvent *e );
    //void        focusOutEvent( QFocusEvent *e );

    void        paintEvent( QPaintEvent *e );

private:
    void        init();
    int         valueFromPosition( const QPoint & );


    double      rat;
    int         pressedAt;
    bool        mousePressed;
};

/*
inline QThumbWheel::Orientation QThumbWheel::orientation() const
{
    return orient;
}

inline bool QThumbWheel::tracking() const
{
    return (bool)track;
}
*/

inline double QThumbWheel::transmissionRatio() const
{
    return rat;
}

#endif // QWHEEL_H
