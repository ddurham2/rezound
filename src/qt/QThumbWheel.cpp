/****************************************************************************
** $Id: qt/qthumbwheel.cpp   3.2.1   edited May 13 09:08 $
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

#include "QThumbWheel.h"

#include <QKeyEvent>
#include <QPainter>
#include <qdrawutil.h>
#include <math.h>

static const double m_pi = 3.14159265358979323846;
static const double rad_factor = 180.0 / m_pi;
QThumbWheel::QThumbWheel( QWidget *parent )
    : QAbstractSlider( parent )
{
    init();
}

QThumbWheel::~QThumbWheel()
{
}

void QThumbWheel::init()
{
    setAutoFillBackground(true);
    setTracking(true);
    mousePressed = false;
    pressedAt = -1;
    rat = 1.0;
    //setFrameStyle( WinPanel | Sunken );
    setFocusPolicy( Qt::WheelFocus );
    setGeometry(0,0,200,30);
    setMinimumWidth(200);
    setMinimumHeight(30);
    setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
}

void QThumbWheel::setTransmissionRatio( double r )
{
    rat = r;
}

/*
void QThumbWheel::setValue( int value )
{
    QRangeControl::setValue( value );
}

void QThumbWheel::valueChange()
{
    repaint( FALSE );
    Q_EMIT valueChanged(value());
}

void QThumbWheel::rangeChange()
{
}

void QThumbWheel::stepChange()
{
}
*/

void QThumbWheel::keyPressEvent( QKeyEvent *e )
{
    switch ( e->key() ) {
    case Qt::Key_Left:
        if ( orientation() == Qt::Horizontal )
		triggerAction(SliderSingleStepSub);
        break;
    case Qt::Key_Right:
        if ( orientation() == Qt::Horizontal )
		triggerAction(SliderSingleStepAdd);
        break;
    case Qt::Key_Up:
        if ( orientation() == Qt::Vertical )
		triggerAction(SliderSingleStepSub);
        break;
    case Qt::Key_Down:
        if ( orientation() == Qt::Vertical )
		triggerAction(SliderSingleStepAdd);
        break;
    case Qt::Key_PageUp:
	triggerAction(SliderPageStepSub);
        break;
    case Qt::Key_PageDown:
	triggerAction(SliderPageStepAdd);
        break;
    case Qt::Key_Home:
	triggerAction(SliderToMinimum);
        break;
    case Qt::Key_End:
	triggerAction(SliderToMaximum);
        break;
    default:
        e->ignore();
        return;
    };
}

void QThumbWheel::mousePressEvent( QMouseEvent *e )
{
    if ( e->button() == Qt::LeftButton ) {
        mousePressed = true;
        pressedAt = valueFromPosition( e->pos() );
    }
}

void QThumbWheel::mouseReleaseEvent( QMouseEvent *e )
{
    int movedTo = valueFromPosition( e->pos() );
    setValue( value() + movedTo - pressedAt );
    pressedAt = movedTo;
}

void QThumbWheel::mouseMoveEvent( QMouseEvent *e )
{
    if ( !mousePressed )
        return;
    if ( hasTracking() ) {
        int movedTo = valueFromPosition( e->pos() );
        setValue( value() + movedTo - pressedAt );
        pressedAt = movedTo;
    }
}

void QThumbWheel::wheelEvent( QWheelEvent *e )
{
    int step = ( e->modifiers() & Qt::ControlModifier ) ? singleStep() : pageStep();
    setValue( value() - e->delta()*step/120 );
    e->accept();
}

/*
void QThumbWheel::focusInEvent( QFocusEvent *e )
{
    QWidget::focusInEvent( e );
}

void QThumbWheel::focusOutEvent( QFocusEvent *e )
{
    QWidget::focusOutEvent( e );
}
*/

void QThumbWheel::paintEvent(QPaintEvent *e)
{
	printf("HELLO...............\n");
    QPainter pt(this);

    QRect cr = e->rect();

    const int n = 17;
    const double delta = m_pi / double(n);
    // ### use positionFromValue() with rad*16 or similar
    double alpha = 2*m_pi*double(value()-minimum())/ double(maximum()-minimum())*transmissionRatio();
    alpha = fmod(alpha, delta);
    QPen pen0( palette().midlight().color() );
    QPen pen1( palette().dark().color() );

    if ( orientation() == Qt::Horizontal ) {
        double r = 0.5*cr.width();
        int y0 = cr.y()+1;
        int y1 = cr.bottom()-1;
        for ( int i = 0; i < n; i++ ) {
            int x = cr.x() + int((1-cos(delta*double(i)+alpha))*r);
            pt.setPen( pen0 );
            pt.drawLine( x, y0, x, y1 );
            pt.setPen( pen1 );
            pt.drawLine( x+1, y0, x+1, y1 );
        }
    } else {
        // vertical orientation
        double r = 0.5*cr.height();
        int x0 = cr.x()+1;
        int x1 = cr.right()-1;
        for ( int i = 0; i < n; i++ ) {
            int y = cr.y() + int((1-cos(delta*double(i)+alpha))*r);
            pt.setPen( pen0 );
            pt.drawLine( x0, y, x1, y );
            pt.setPen( pen1 );
            pt.drawLine( x0, y+1, x1, y+1 );
        }
    }
    qDrawShadePanel( &pt, cr, palette());
}

int QThumbWheel::valueFromPosition( const QPoint &p )
{
    QRect wrec = contentsRect();
    int pos, min, max;
    if ( orientation() == Qt::Horizontal ) {
        pos = p.x();
        min = wrec.left();
        max = wrec.right();
    } else {
        pos = p.y();
        min = wrec.top();
        max = wrec.bottom();
    }
    double alpha;
    if ( pos < min )
        alpha = 0;
    else if ( pos > max )
        alpha = m_pi;
    else
        alpha = acos( 1.0 - 2.0*double(pos-min)/double(max-min) );// ### taylor
    double deg = alpha*rad_factor/transmissionRatio();
    // ### use valueFromPosition()
    return minimum() + int((maximum()-minimum())*deg/360.0);
}

