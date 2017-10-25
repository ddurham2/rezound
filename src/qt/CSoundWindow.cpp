#include "CSoundWindow.h"

#include <QAbstractScrollArea>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPainter>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QColor>
#include <QPaintEvent>
#include <QMenu>
#include <QAction>
#include <QFont>
#include <QStyleOptionFocusRect>
#include <QToolTip>
#include <QTimer>
#include <QMetaObject>
//#include "QThumbWheel.h"

#include "CSoundFileManager.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"
#include "../backend/CSoundPlayerChannel.h"
#include "../backend/CActionParameters.h"
#include "../backend/Edits/CCueAction.h"
#include "../backend/Edits/CSelectionEdit.h"
#include "../backend/main_controls.h"
#include "settings.h"

#include "CMainWindow.h"
#include "CSoundWindow.h"
#include "drawPortion.h"

#include <math.h>
#include <algorithm>

#include <map>
#include <string>
using namespace std;


#define DRAW_PLAY_POSITION_TIME 75	// 75 ms
#define ZOOM_MUL 7

static double zoomSliderToZoomFactor(int zoomSlider)
{
	//return pow(zoomSlider/(100.0*ZOOM_MUL),1.0/4.0);
	return zoomSlider/(100.0*ZOOM_MUL);
}

static int zoomFactorToZoomSlider(sample_fpos_t zoomFactor)
{
	//return (int)(pow(zoomFactor,4.0)*(100.0*ZOOM_MUL));
	return (int)(zoomFactor*(100.0*ZOOM_MUL));
}

#warning many of the methods need to affect the range and position of the scroll areas scrollbars

/* TODO:
 *
 * - I need to load a sound that is a square wave with complete 32767 to -32767 range to verify that it 
 *   renders properly AND that the scroll bars can fully extend to the length and not further
 *
 * - I thought there was a problem with the way select stop is drawn, but now I don't think there is
 *   	- example: load a sound of say length 5.  Well, when start = 0 and stop = 4 (fully selected)
 *   	 	   it doesn't draw as if the 5th sample is selected, but that's because, when start is 0
 *   	 	   it draw from X=0 and so when stop is 0 it too draws from X=0... hence when position 4
 *   	 	   or the 5th sample is selected, it does fill past the beginning of the selected sample
 *   	 	   - perhaps a floor on the start calc and a ceil on the stop calc would fix this.. but
 *   	 	     right now, I'm not going to worry about it
 *
 * - test that all samples are actually visible when zoomed all the way in
 * 	- this does have a little bit of a problem, but I noted it in the docs/devel/TODO and I 
 * 	  have more important problems to fix.
 */

static QColor playStatusColor(255,0,0);

#define RIGHT_MARGIN 10

class CSoundWindowScrollArea : public QAbstractScrollArea
{
	Q_OBJECT 

public:
	CSoundWindowScrollArea(CSoundWindow *_wv) :
		QAbstractScrollArea(),
		wv(_wv),
		first(true),
		lastDrawWasUnsuccessful(false),
		
		draggingSelectStart(false),
		draggingSelectStop(false),
		momentaryPlaying(false),

		autoScrollTimerID(-1)
	{
		setFrameStyle(QFrame::NoFrame);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

		connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onHorzScroll()));
		connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onVertScroll()));
	}

	virtual ~CSoundWindowScrollArea()
	{
	}

public Q_SLOTS:

	void onHorzScroll()
	{
		wv->horzOffset=horizontalScrollBar()->value();
		wv->updateRuler();
	}

	void onVertScroll()
	{
		wv->vertOffset=verticalScrollBar()->value();
	}

protected:

	void drawPortion(int left,int width,QPainter *dc)
	{
		if(!isVisible())
			return;

		// Here is where while an action may be processing that it has the sound locked 
		// for resize so if a redraw occurs that this would deadlock if used a waiting 
		// lock instead of a try-lock.
		// so, I just draw an empty background on the whole thing until a real redraw 
		// can succeed.  
		//
		// ??? One better solution would be to arrange for a back buffer to be saved 
		// before the action started, and I could blit from that for updates
		// ??? Qt offers a back-buffer itself, so we should make use of that
		CSoundLocker sl(wv->loadedSound->sound, false, true);
		if(!sl.isLocked())
		{ // can't lock.. just paint with background.. whole thing first time the lock fails.. just do updated part on not-the-first time
			// no reason to erase.. just let Qt's backbuffer redraw.. perhaps alter what's there to look invalid. ???
			if(lastDrawWasUnsuccessful)
				dc->fillRect(left,0,width,viewport()->height(),backGroundColor); 
			else
				viewport()->update();
			lastDrawWasUnsuccessful=true;
			return;
		}
		else
			lastDrawWasUnsuccessful=false;

		wv->renderedStartPosition=wv->loadedSound->channel->getStartPosition();
		wv->renderedStopPosition=wv->loadedSound->channel->getStopPosition();

		const int vOffset=((wv->getVertSize()-viewport()->height())/2)-wv->vertOffset;
		::drawPortion(left,width,dc,wv->loadedSound->sound,viewport()->width(),viewport()->height(),(int)wv->getDrawSelectStart(),(int)wv->getDrawSelectStop(),wv->horzZoomFactor,wv->horzOffset,wv->vertZoomFactor,vOffset);
		//printf("hoffset: %d hzoom: %f   voffset: %d vzoom: %f\n",wv->horzOffset,wv->horzZoomFactor,vOffset,wv->vertZoomFactor);

		if(gDrawVerticalCuePositions)
		{	// draw cue positions as inverted colors

			// calculate the min and max times based on the left and right boundries of the drawing
			const sample_pos_t minTime=wv->getSamplePosForScreenX(max(0,left-1));
			const sample_pos_t maxTime=wv->getSamplePosForScreenX(left+width+1);

			/* ??? if I iterated over the cues by increasing time, then I could be more efficient by finding the neared cue to minTime and stop when a cue is greater than maxTime */
			const size_t cueCount=wv->loadedSound->sound->getCueCount();
			for(size_t t=0;t<cueCount;t++)
			{
				const sample_pos_t cueTime=wv->loadedSound->sound->getCueTime(t);
				if(cueTime>=minTime && cueTime<=maxTime)
				{
					const int x=wv->getCueScreenX(t);
					::drawPortion(x,1,dc,wv->loadedSound->sound,viewport()->width(),viewport()->height(),(int)wv->getDrawSelectStart(),(int)wv->getDrawSelectStop(),wv->horzZoomFactor,wv->horzOffset,wv->vertZoomFactor,vOffset,false,true);
				}
			}
		}

		if(wv->m_drawPlayPositionAt<MAX_LENGTH)
		{
			int x=(int)((sample_fpos_t)wv->m_drawPlayPositionAt/wv->horzZoomFactor-wv->horzOffset);
			dc->setPen(playStatusColor);
			dc->drawLine(x,0,x,height());
		}
	}

	void paintEvent(QPaintEvent *e)
	{
		printf("paintEvent width: %d\n",e->rect().width());
		QPainter p(viewport());

#if 0
		// ??? why?
		if(first)
		{ // draw black while data is drawing the first time
			p.fillRect(e->rect(),backGroundColor);
			first=false;
		}
#endif

		drawPortion(e->rect().left(),e->rect().width(),&p);
	}

	void wheelEvent(QWheelEvent *e)
	{
		int delta=9;
		// scroll by 5 pages if control is pressed
		if(e->modifiers()&Qt::ControlModifier)
			delta=viewport()->width()*5;
		// scroll by just a little bit if control is pressed
		else if(e->modifiers()&Qt::ShiftModifier)
			delta=10;
		// scroll by 1/3-pages with no modifiers
		else
			delta=viewport()->width()/3;

		// handle direction
		if(e->delta()<0)
			delta=delta;
		else
			delta=-delta;

		horizontalScrollBar()->setValue(horizontalScrollBar()->value()+delta);
		e->accept();
	}

	void resizeEvent(QResizeEvent *e)
	{
		//printf("%s: resizeEvent: old: %dx%d  new: %dx%d\n",__func__,e->oldSize().width(),e->oldSize().height(),e->size().width(),e->size().height());
		// recalc horz zoom since the width has changed
		wv->setHorzZoom(wv->getHorzZoom(),CSoundWindow::hrtLeftEdge,true);
		wv->setVertZoom(wv->getVertZoom(),true);
	}

	void scrollContentsBy(int dx,int dy)
	{
		if(dy==0)
			viewport()->scroll(dx,dy);
		else
			viewport()->update();
	}

	void mousePressEvent(QMouseEvent *e)
	{
		e->ignore();
		const sample_pos_t X=e->x();

		if(e->button()==Qt::LeftButton && e->modifiers()&Qt::ControlModifier) // left button pressed while holding control
		{
			const sample_pos_t position=wv->getSamplePosForScreenX(X);
			play(gSoundFileManager,position);
			e->accept();
			return;
		}
		else if(e->button()==Qt::RightButton && e->modifiers()&Qt::ControlModifier) // right button pressed while holding control
		{
			const sample_pos_t position=wv->getSamplePosForScreenX(X);
			play(gSoundFileManager,position);
			momentaryPlaying=true;
			e->accept();
			return;
		}
		else if(e->button()==Qt::LeftButton && e->modifiers()&Qt::ShiftModifier) // left button pressed while holding alternate
		{
			const sample_pos_t position=wv->getSamplePosForScreenX(X);

			CActionParameters actionParameters(NULL);

			actionParameters.setValue<string>("name",gAddCueAtClick_CueName);
			actionParameters.setValue<sample_pos_t>("position",position);
			actionParameters.setValue<bool>("isAnchored",gAddCueAtClick_Anchored);

			wv->addCueActionFactory->performAction(wv->loadedSound,&actionParameters,false);
			wv->updateFromEdit();

			e->accept();
			return;
		}

		if(!draggingSelectStop && !draggingSelectStart)
		{
			if(e->button()==Qt::LeftButton)
			{
				if(X<=wv->getDrawSelectStop())
				{
					draggingSelectStart=true;
					wv->setSelectStartFromScreen(X);
				}
				else
				{
					draggingSelectStop=true;
					const sample_pos_t origSelectStop=wv->loadedSound->channel->getStopPosition();
					wv->setSelectStopFromScreen(X);
					wv->loadedSound->channel->setStartPosition(origSelectStop);
				}
				e->accept();
			}
			else if(e->button()==Qt::RightButton)
			{
				if(X>=wv->getDrawSelectStart())
				{
					draggingSelectStop=true;
					wv->setSelectStopFromScreen(X);
				}
				else
				{
					draggingSelectStart=true;
					const sample_pos_t origSelectStart=wv->loadedSound->channel->getStartPosition();
					wv->setSelectStartFromScreen(X);
					wv->loadedSound->channel->setStopPosition(origSelectStart);
				}
				e->accept();
			}
		}
		wv->updateFromSelectionChange();
	}

	void mouseReleaseEvent(QMouseEvent *e)
	{
		e->ignore();
		if(e->button()==Qt::RightButton && momentaryPlaying)
		{
			momentaryPlaying=false;
			stop(gSoundFileManager);
			e->accept();
			return;
		}

		if((e->button()==Qt::LeftButton || e->button()==Qt::RightButton) && (draggingSelectStart || draggingSelectStop))
		{
			stopAutoScroll();
			draggingSelectStart=draggingSelectStop=false;
			e->accept();
		}
	}

	void handleMouseMoveSelectChange(int X)
	{
		// I put it here so that when shift is held to stop the autoscrolling that it doesn't select anything off screen even if the mouse is outside the wave view
		X=max(0,min(X,(viewport()->width()-1)));

		if(draggingSelectStart)
		{
			if(X>wv->getDrawSelectStop())
			{
				draggingSelectStart=false;
				draggingSelectStop=true;
				const sample_pos_t origSelectStop=wv->loadedSound->channel->getStopPosition();
				wv->setSelectStopFromScreen(X);
				wv->loadedSound->channel->setStartPosition(origSelectStop);
			}
			else
				wv->setSelectStartFromScreen(X);

			wv->updateFromSelectionChange();
		}
		else if(draggingSelectStop)
		{
			if(X<=wv->getDrawSelectStart())
			{
				draggingSelectStop=false;
				draggingSelectStart=true;
				const sample_pos_t origSelectStart=wv->loadedSound->channel->getStartPosition();
				wv->setSelectStartFromScreen(X);
				wv->loadedSound->channel->setStopPosition(origSelectStart);
			}
			else
				wv->setSelectStopFromScreen(X);

			wv->updateFromSelectionChange();
		}
	}

	void mouseMoveEvent(QMouseEvent *e)
	{
		e->ignore();
		if(draggingSelectStart || draggingSelectStop)
			e->accept();

		autoScroll_win_x=e->x();
		autoScroll_win_y=e->y();

		// Here we detect if the mouse is against the side walls and scroll that way if the mouse is down
		if(draggingSelectStart || draggingSelectStop)
		{
			if(!(e->modifiers()&Qt::ShiftModifier))
			{
				if(startAutoScroll(e->x(),e->y()))
					return;
			}
		}

		handleMouseMoveSelectChange(e->x());
	}

private:
	friend class CWaveRuler;

	CSoundWindow *wv;
	bool first;
	bool lastDrawWasUnsuccessful;
	bool draggingSelectStart,draggingSelectStop;
	bool momentaryPlaying;

	#define AUTOSCROLL_FUDGE 11
	int autoScrollTimerID;
	int autoScroll_win_x,autoScroll_win_y;


	bool startAutoScroll(int win_x,int win_y)
	{
		bool autoscrolling=false;

		// not worrying about vertical scrolling
		if(horizontalScrollBar()->pageStep()<(horizontalScrollBar()->maximum()-horizontalScrollBar()->minimum()))
		{
			if((win_x<AUTOSCROLL_FUDGE) && (0<horizontalScrollBar()->value())) 
				autoscrolling=true;
			else if( (viewport()->width()-AUTOSCROLL_FUDGE<=win_x) && (horizontalScrollBar()->value()<((horizontalScrollBar()->maximum()-horizontalScrollBar()->minimum())-horizontalScrollBar()->pageStep())) )
				autoscrolling=true;
		}

		if(autoscrolling)
		{
			if(autoScrollTimerID==-1)
			{
				autoScrollTimerID=startTimer(80); // ??? 80ms good enough?
				// and fire now too
				onAutoScroll();
			}
		}
		else
		{
			if(autoScrollTimerID!=-1)
			{
				killTimer(autoScrollTimerID);
				autoScrollTimerID=-1;
			}
		}

		return autoscrolling;
	}

	void stopAutoScroll()
	{
		if(autoScrollTimerID!=-1)
		{
			killTimer(autoScrollTimerID);
			autoScrollTimerID=-1;
		}
	}

	/*override*/ void timerEvent(QTimerEvent *e)
	{
		if(e->timerId()==autoScrollTimerID)
			onAutoScroll();
	}

	void onAutoScroll(){
		// do widget maintenance stuff n autoscroll
		printf("onAutoScroll\n");
		int dx=0;
		int dy=0;

		if(autoScroll_win_x<AUTOSCROLL_FUDGE)
			dx=AUTOSCROLL_FUDGE-autoScroll_win_x;
		else if(viewport()->width()-AUTOSCROLL_FUDGE<=autoScroll_win_x)
			dx=viewport()->width()-AUTOSCROLL_FUDGE-autoScroll_win_x;
		dx=-dx;

		if(autoScroll_win_y<AUTOSCROLL_FUDGE)
			dy=AUTOSCROLL_FUDGE-autoScroll_win_y;
		else if(viewport()->height()-AUTOSCROLL_FUDGE<=autoScroll_win_y)
			dy=viewport()->height()-AUTOSCROLL_FUDGE-autoScroll_win_y;
		dy=-dy;

		if(dx || dy){
			int oldposx=horizontalScrollBar()->value();
			int oldposy=verticalScrollBar()->value();

			horizontalScrollBar()->setValue(horizontalScrollBar()->value()+dx);
			verticalScrollBar()->setValue(verticalScrollBar()->value()+dy);

			// stop autoscrolling if we can't scroll anymore
			if((horizontalScrollBar()->value()==oldposx) || (verticalScrollBar()->value()!=oldposy))
				stopAutoScroll();
		}


		handleMouseMoveSelectChange(autoScroll_win_x);
		wv->onAutoScroll();
		// ruler gets updated because onHorzScroll() slot is invoked
	}
};

// ------------------------------

class CWaveRuler : public QWidget
{
	Q_OBJECT


public:
	CWaveRuler(CSoundWindow *_wv) :
		QWidget(),
		wv(_wv)
	{
		rulerFont=font();
		rulerFont.setPixelSize(10);

		cueClicked=NIL_CUE_INDEX;

		draggingCue=false;

		setFocusPolicy((Qt::FocusPolicy)(Qt::TabFocus | Qt::ClickFocus));
		focusedCueIndex=NIL_CUE_INDEX;
		focusNextCue(); // to make it focus the first one if there is one

		selectionEditPositionFactory=new CSelectionEditPositionFactory;
		moveCueActionFactory=new CMoveCueActionFactory;
		removeCueActionFactory=new CRemoveCueActionFactory;
		replaceCueActionFactory=new CReplaceCueActionFactory(/*???FIXME gCueDialog*/NULL);

		setAutoFillBackground(true);
		setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

		setContextMenuPolicy(Qt::CustomContextMenu);
		connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenuRequested(const QPoint &)));
	}

	virtual ~CWaveRuler()
	{
		delete selectionEditPositionFactory;
		delete moveCueActionFactory;
		delete removeCueActionFactory;
		delete replaceCueActionFactory;
	}

	QSize sizeHint() const
	{
		return QSize(0,19+9); // ??? might should base this on the font height.. but the fontmetrics::height()*2+5 was way too tall
	}

	size_t getClickedCue(int x,int y)
	{
		const size_t cueCount=wv->loadedSound->sound->getCueCount();
		size_t cueClicked=NIL_CUE_INDEX;
		if(cueCount>0)
		{
			// go in reverse order so that the more recent one created can be removed first
			// I mean, if you accidently create one, then want to remove it, and it's on top
			// of another one then it needs to find that more recent one first
			for(size_t t=cueCount;t>=1;t--)
			{
				int X=wv->getCueScreenX(t-1);
				if(X!=CUE_OFF_SCREEN)
				{
					int Y=CUE_Y(this);

					// check distance from clicked point to the cue's position
					if( ((x-X)*(x-X))+((y-Y)*(y-Y)) <= ((CUE_RADIUS+1)*(CUE_RADIUS+1)) )
					{
						cueClickedOffset=x-X;
						cueClicked=t-1;
						break;
					}
				}
			}
		}
		return cueClicked;
	}

	void focusNextCue()
	{
		if(focusedCueIndex==NIL_CUE_INDEX)
		{
			sample_pos_t dummy;
			if(!wv->loadedSound->sound->findNearestCue(0,focusedCueIndex,dummy))
				focusedCueIndex=NIL_CUE_INDEX;
			else
				focusedCueTime=wv->loadedSound->sound->getCueTime(focusedCueIndex);
		}
		else
		{
			if(wv->loadedSound->sound->findNextCue(focusedCueTime,focusedCueIndex))
				focusedCueTime=wv->loadedSound->sound->getCueTime(focusedCueIndex);
		}
	}

	void focusPrevCue()
	{
		if(focusedCueIndex==NIL_CUE_INDEX)
			focusNextCue(); // would implement the same thing here, so just call it .. just finds some cue
		else
		{
			if(wv->loadedSound->sound->findPrevCue(focusedCueTime,focusedCueIndex))
				focusedCueTime=wv->loadedSound->sound->getCueTime(focusedCueIndex);
		}
	}

	void focusFirstCue()
	{
		sample_pos_t dummy;
		if(!wv->loadedSound->sound->findNearestCue(0,focusedCueIndex,dummy))
			focusedCueIndex=NIL_CUE_INDEX;
		else
			focusedCueTime=wv->loadedSound->sound->getCueTime(focusedCueIndex);
	}

	void focusLastCue()
	{
		sample_pos_t dummy;
		if(!wv->loadedSound->sound->findNearestCue(wv->loadedSound->sound->getLength()-1,focusedCueIndex,dummy))
			focusedCueIndex=NIL_CUE_INDEX;
		else
			focusedCueTime=wv->loadedSound->sound->getCueTime(focusedCueIndex);
	}

	void onAutoScroll()
	{
		if(draggingCue)
		{
			QMouseEvent e(QMouseEvent::MouseMove,QPoint(),QCursor::pos(),Qt::LeftButton,Qt::LeftButton,QApplication::keyboardModifiers());
			mouseMoveEvent(&e);
		}
	}


protected:
#if 0 // before replacing with a call to drawWaveRuler
	void paintEvent(QPaintEvent *e)
	{
		QPainter dc;
		dc.begin(this);

		#define LABEL_TICK_FREQ 80
		/*
		 * Here, we draw the ruler
		 * 	We want to always put 1 labeled position every LABELED_TICK_FREQ pixels 
		 * 	And draw ruler ticks also
		 */

		int lastX=wv->wvsa->viewport()->width();

		/*
		const int left=0;
		const int right=left+lastX;
		*/

		int start=e->rect().left();
		start-=LABEL_TICK_FREQ-1;

		int end=start+e->rect().width();
		end+=LABEL_TICK_FREQ-1;

		if(end>lastX)
			end=lastX;

		dc.setFont(rulerFont);
		for(int x=start;x<=end;x++)
		{
			dc.setPen(palette().color(QPalette::Text));
			if((x%LABEL_TICK_FREQ)==0)
			{
				dc.drawLine(x,0,x,10);

				const string time=wv->loadedSound->sound->getTimePosition(wv->getSamplePosForScreenX(x));

				dc.drawText(x+2,14,time.c_str());

				// draw ligher ticks around main long tickmark
				dc.setPen(palette().color(QPalette::Mid));
				dc.drawLine(x-1,0,x-1,7);
				dc.drawLine(x+1,0,x+1,7);
			}
			else if(((x+(LABEL_TICK_FREQ/2))%(LABEL_TICK_FREQ))==0)
				dc.drawLine(x,0,x,4); // half way tick between labeled ticks
			else if((x%(LABEL_TICK_FREQ/10))==0)
				dc.drawLine(x,0,x,2); // small tenth ticks
		}

		// render cues
		const size_t cueCount=wv->loadedSound->sound->getCueCount();
		for(size_t t=0;t<cueCount;t++)
		{
			// ??? I could figure out the min and max screen-visible time and make sure that is in range before testing every cue
			int cueXPosition=wv->getCueScreenX(t);

			if(cueXPosition!=CUE_OFF_SCREEN)
			{
				const string cueName=wv->loadedSound->sound->getCueName(t);

				if(hasFocus() && focusedCueIndex!=NIL_CUE_INDEX && focusedCueTime==wv->loadedSound->sound->getCueTime(t))
				{ // draw focus rectangle (before drawing everything else)

					// rect around cue symbol
					QRect cueRect(cueXPosition-CUE_RADIUS,CUE_Y(this)-CUE_RADIUS, CUE_RADIUS*2,CUE_RADIUS*2);

					// rect around text
					QRect textRect=fontMetrics().boundingRect(cueName.c_str());
					textRect.moveTo(cueXPosition+CUE_RADIUS+2, height()-2-textRect.height());

					{ // I don't know why, but this hack is here because boundingRect() returns something that is almost twice as tall as it needs to be
						textRect.adjust(0,textRect.height()/2-1,0,0); 
					}

					// unite the two rects
					QRect focusRect=cueRect.united(textRect);

					/*
					// draw the focus rect
					focusRect.adjust(-1,-1,1,1);
					QPen p(palette().color(QPalette::Dark));
					p.setStyle(Qt::DotLine);
					dc.setPen(p);
					dc.drawRect(focusRect);
					*/
					
					// draw the focus rect using the current style
					QStyleOptionFocusRect option;
					option.initFrom(this);
					option.rect=focusRect;
					option.rect.adjust(-2,-2,2,2);
					option.backgroundColor = palette().color(QPalette::Background);
					style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &dc, this);
				}

				dc.setPen(palette().color(QPalette::Text));
				dc.drawText(cueXPosition+CUE_RADIUS+2,height()-2,cueName.c_str());

				//#warning drawArc sux at drawing small circles, report this to Trolltech
				//dc.drawArc(cueXPosition-CUE_RADIUS,CUE_Y(this)-CUE_RADIUS,CUE_RADIUS*2,CUE_RADIUS*2,0*16,360*16);
				dc.drawEllipse(QRect(cueXPosition-CUE_RADIUS,CUE_Y(this)-CUE_RADIUS,CUE_RADIUS*2,CUE_RADIUS*2));

				dc.setPen(wv->loadedSound->sound->isCueAnchored(t) ? QColor(0,0,255) : QColor(255,0,0));
				dc.drawLine(cueXPosition-1,height()-4,cueXPosition-1,CUE_Y(this)-1);
				dc.drawLine(cueXPosition,height()-1,cueXPosition,CUE_Y(this));
				dc.drawLine(cueXPosition+1,height()-4,cueXPosition+1,CUE_Y(this)-1);
			}
		}

		dc.end();
	}
#endif

	void paintEvent(QPaintEvent *e)
	{
		drawWaveRuler(wv,this,e,0,wv->wvsa->viewport()->width(),rulerFont,palette(),wv->loadedSound->sound,hasFocus(),focusedCueIndex,focusedCueTime);
	}

	/* ??? when I have keyboard event handling for doing something with the focused cue,  make it so that if you press esc while dragging a cue that it moves back to the original location */
	void mousePressEvent(QMouseEvent *e)
	{
		if(e->button()!=Qt::LeftButton)
			return;

		//??? handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);

		cueClicked=getClickedCue(e->x(),e->y()); // setting data member
		if(cueClicked<wv->loadedSound->sound->getCueCount())
		{
			origCueClickedTime=wv->loadedSound->sound->getCueTime(cueClicked);

			origStartPosition=wv->loadedSound->channel->getStartPosition();
			origStopPosition=wv->loadedSound->channel->getStopPosition();

			draggingSelectionToo=
				!(e->modifiers()&Qt::ShiftModifier) && 
				(origCueClickedTime==wv->loadedSound->channel->getStartPosition() || 
				origCueClickedTime==wv->loadedSound->channel->getStopPosition());

			// show tip text showing position of cue
			QToolTip::showText(e->globalPos(),wv->loadedSound->sound->getTimePosition(origCueClickedTime,wv->getZoomDecimalPlaces()).c_str());
			
			// set focused cue
			focusedCueIndex=cueClicked;
			focusedCueTime=wv->loadedSound->sound->getCueTime(focusedCueIndex);
			update();
			draggingCue=true;
		}

		e->accept();
	}

	void mouseMoveEvent(QMouseEvent *e)
	{
		if(e->buttons()&Qt::LeftButton && cueClicked<wv->loadedSound->sound->getCueCount())
		{	// dragging a cue around

			const string cueName=wv->loadedSound->sound->getCueName(cueClicked);
			const sample_pos_t cueTime=wv->loadedSound->sound->getCueTime(cueClicked);
			const int cueTextWidth=QFontMetrics(rulerFont).width(cueName.c_str());

			// last_x is where the cue is on the screen now (possibly after autoscrolling)
			const int last_x=wv->getCueScreenX(cueClicked);

			update();

			/* nah.. just update() (above)
			// erase cue at old position
			update(last_x-CUE_RADIUS-1,0,CUE_RADIUS*2+1+cueTextWidth+2,height());	
			*/

			// erase vertical cue line at old position
			if(gDrawVerticalCuePositions) 
				wv->wvsa->viewport()->update(last_x-5,0,10,wv->wvsa->height()); 

#if 0 // not sure what this is for exactly
			if(object==NULL) // simulated event from auto-scrolling
			{
				// get where the dragCueHint appears on the wave view canvas
				FXint hint_win_x,hint_win_y;
				dragCueHint->getParent()->translateCoordinatesTo(hint_win_x,hint_win_y,this,dragCueHint->getX(),dragCueHint->getY());
		
				// redraw what was under the dragCueHint (necessary when auto-scrolling)
				wv->wvsa->viewport()->update(hint_win_x+(last_x-event->win_x),0,dragCueHint->width()+2,wv->wvsa->height()); /* +2 I guess because of some border? I dunno exactly, but it fixed the prob; except I did still see the problem once when dragging a cue way off to the side and wiggling it around sometimes going back over the window letting it autoscroll ??? */
			}
#endif


			sample_pos_t newTime=wv->getSamplePosForScreenX(e->x()-cueClickedOffset);
			
			if(draggingSelectionToo)
			{
				const sample_pos_t start=wv->loadedSound->channel->getStartPosition();
				const sample_pos_t stop=wv->loadedSound->channel->getStopPosition();

				if(cueTime==start)
				{	// move start position with cue drag
					if(newTime>stop)
					{ // dragged cue past stop position 
						wv->loadedSound->channel->setStopPosition(newTime);
						wv->loadedSound->channel->setStartPosition(stop);
						wv->updateFromSelectionChange(CSoundWindow::lcpStop);
					}
					else
					{
						wv->loadedSound->channel->setStartPosition(newTime);
						wv->updateFromSelectionChange(CSoundWindow::lcpStart);
					}

				}
				else if(cueTime==stop)
				{	// move stop position with cue drag
					if(newTime<start)
					{ // dragged cue before start position 
						wv->loadedSound->channel->setStartPosition(newTime);
						wv->loadedSound->channel->setStopPosition(start);
						wv->updateFromSelectionChange(CSoundWindow::lcpStart);
					}
					else
					{
						wv->loadedSound->channel->setStopPosition(newTime);
						wv->updateFromSelectionChange(CSoundWindow::lcpStop);
					}

				}
			}

			// update cue position
			wv->loadedSound->sound->setCueTime(cueClicked,newTime);
			if(cueClicked==focusedCueIndex)
				focusedCueTime=newTime;

			/* nah.. just update() (above)
			// draw cue at new position
			update((e->x()-cueClickedOffset)-CUE_RADIUS-1,0,CUE_RADIUS*2+1+cueTextWidth+2,height());
			*/

			// draw vertical cue line at new position
			if(gDrawVerticalCuePositions) 
				wv->wvsa->viewport()->update((e->x()-cueClickedOffset)-5,0,10,wv->wvsa->viewport()->height()); 

			// show tip text showing position of cue
			QToolTip::showText(e->globalPos(),wv->loadedSound->sound->getTimePosition(newTime,wv->getZoomDecimalPlaces()).c_str());

			// have to call canvas->repaint() on real mouse moves because if while autoscrolling a real (that is, object!=NULL) mouse move event occurs, then the window may or may not have been blitted leftward or rightward yet and we will erase the vertical cue position at the wrong position (or something like that, I really had a hard time figuring out the problem that shows up if you omit this call to repaint() )
		//	if(/*?object &&*/ gDrawVerticalCuePositions)
				//wv->wvsa->viewport()->update();

			if(e->x()<0 || e->x()>=width())
			{ // scroll parent window leftwards or rightwards if mouse is beyond the window edges
				// ??? some horrible flicker.. not sure of the cause
				wv->wvsa->autoScroll_win_x=e->x();
				wv->wvsa->autoScroll_win_y=e->y();
				wv->wvsa->startAutoScroll(e->x(),e->y());
			}
			else if(e->x()>0 && e->x()<(width()-1)) // just inside the view
				// ??? hmm
				wv->wvsa->stopAutoScroll();

		}

		e->accept();
	}

	void mouseReleaseEvent(QMouseEvent *e)
	{
		if(cueClicked<wv->loadedSound->sound->getCueCount() && wv->loadedSound->sound->getCueTime(cueClicked)!=origCueClickedTime)
		{	// was dragging a cue around
			const sample_pos_t newCueTime=wv->loadedSound->sound->getCueTime(cueClicked);

			// set back to the orig position
			wv->loadedSound->sound->setCueTime(cueClicked,origCueClickedTime);

			// set cue to new position except use an AAction object so it goes on the undo stack
			CActionParameters actionParameters(NULL);
			actionParameters.setValue<unsigned>("index",cueClicked);
			actionParameters.setValue<sample_pos_t>("position",newCueTime);
			actionParameters.setValue<sample_pos_t>("restoreStartPosition",origStartPosition);
			actionParameters.setValue<sample_pos_t>("restoreStopPosition",origStopPosition);
			moveCueActionFactory->performAction(wv->loadedSound,&actionParameters,false);
		}

		QToolTip::showText(QPoint(),""); // hide tooltip
		wv->wvsa->stopAutoScroll();

		cueClicked=NIL_CUE_INDEX;
		draggingCue=false;
		e->accept();
	}

	void mouseDoubleClickEvent(QMouseEvent *e)
	{
		e->ignore();
		if(e->button()!=Qt::LeftButton)
			return;

		cueClicked=getClickedCue(e->x(),e->y()); // setting data member
		if(cueClicked<wv->loadedSound->sound->getCueCount())
			onEditCue();
		else
		{ // clicking not on a cue.. change selection to be that which is between the two nearest cues (or to start or end if there isn't any near on that side)
			size_t index;
			sample_pos_t startPos=0;
			sample_pos_t stopPos=wv->loadedSound->sound->getLength()-1;
			sample_pos_t clickedPos=wv->getSamplePosForScreenX(e->x());

			if(wv->loadedSound->sound->findPrevCueInTime(clickedPos,index))
				startPos=wv->loadedSound->sound->getCueTime(index);

			if(wv->loadedSound->sound->findNextCueInTime(clickedPos,index))
				stopPos=wv->loadedSound->sound->getCueTime(index)-1; // select from the position AT the start cue to one less sample prior to the next cue

			if(stopPos<startPos) // safety
				stopPos=startPos;

			CActionParameters actionParameters(NULL);
			selectionEditPositionFactory->selectStart=startPos;
			selectionEditPositionFactory->selectStop=stopPos;
			selectionEditPositionFactory->performAction(wv->loadedSound,&actionParameters,false);
			gSoundFileManager->getActiveWindow()->updateFromEdit(); // would prefer to call this for the very window that owns us, but I suppose the user couldn't have clicked it if it wasn't active
		}

		cueClicked=NIL_CUE_INDEX;
		e->accept();
	}

#if 0 // unneccessary now

	long onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
		FXComposite::onFocusIn(sender,sel,ptr);
		update();
		return 1;
	}

	long onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
		FXComposite::onFocusOut(sender,sel,ptr);
		update();
		return 1;
	}
#endif

	/* ???
	 * If possible, make plus and minus keys nudge the cue to the left and right.. add acceleration 
	 * if possible and make the undo action pertain the to last time the key was release for 
	 * some given time I guess (because I dont want every little nudge to be undoable)
	 */

	void keyPressEvent(QKeyEvent *ev)
	{
		ev->ignore();
		if(ev->key()==Qt::Key_Left)
		{
			ev->accept();
			focusPrevCue();
		}
		else if(ev->key()==Qt::Key_Right)
		{
			ev->accept();
			focusNextCue();
		}
		else if(ev->key()==Qt::Key_Home)
		{
			ev->accept();
			focusFirstCue();
		}
		else if(ev->key()==Qt::Key_End)
		{
			ev->accept();
			focusLastCue();
		}
		else if(ev->key()==Qt::Key_Delete)
		{ // delete a cue
			ev->accept();
			if(focusedCueIndex>=wv->loadedSound->sound->getCueCount())
				return;

			const size_t removeCueIndex=focusedCueIndex;

			focusNextCue();

			CActionParameters actionParameters(NULL);
			actionParameters.setValue<unsigned>("index",removeCueIndex);
			removeCueActionFactory->performAction(wv->loadedSound,&actionParameters,false);
			if(removeCueIndex<focusedCueIndex)
				focusedCueIndex--; // decrement since we just remove one below it

			if(focusedCueIndex>=wv->loadedSound->sound->getCueCount())
				focusLastCue(); // find last cue if we just deleted what was the last cue

			/* I don't think I like this behavior
			if(focusedCueIndex!=NIL_CUE_INDEX)
				wv->wvsa->centerTime(focusedCueTime);
			*/
			wv->wvsa->viewport()->update();
			update();
			return;
		}
		else if(ev->key()==Qt::Key_Return || ev->key()==Qt::Key_Enter)
		{ // edit a cue
			ev->accept();
			if(focusedCueIndex>=wv->loadedSound->sound->getCueCount())
				return;
			else
			{
				cueClicked=focusedCueIndex;
				onEditCue();
			}
			return;
		}
		else
		{
			if(ev->key()==Qt::Key_Escape && QApplication::mouseButtons()&Qt::LeftButton && cueClicked<wv->loadedSound->sound->getCueCount())
			{ // ESC pressed for cancelling a drag
				ev->accept();
				wv->loadedSound->sound->setCueTime(cueClicked,origCueClickedTime);
				QToolTip::showText(QPoint(),""); // hide tooltip
				wv->wvsa->stopAutoScroll();
				cueClicked=NIL_CUE_INDEX;
				update();
				wv->wvsa->viewport()->update();
			}

			return;
		}

		if(focusedCueIndex!=NIL_CUE_INDEX)
		{
			wv->setHorzOffset(wv->getHorzOffsetToCenterTime(focusedCueTime));
			update();
		}
	}

private Q_SLOTS:

	void onCustomContextMenuRequested(const QPoint &pos)
	{
		QMenu menu;

		// see if any cue was clicked on
		cueClicked=getClickedCue(pos.x(),pos.y()); // setting data member for onSetPositionToCue's sake

		if(/*always true  cueClicked>=0 && */cueClicked<wv->loadedSound->sound->getCueCount())
		{
			const sample_fpos_t cueTime=wv->loadedSound->sound->getCueTime(cueClicked);

			if(cueTime<wv->loadedSound->channel->getStartPosition())
				menu.addAction(_("Set Start Position to Cue"),this,SLOT(onSetStartPositionToCue()));
			else if(cueTime>wv->loadedSound->channel->getStopPosition())
				menu.addAction(_("Set Stop Position to Cue"),this,SLOT(onSetStopPositionToCue()));
			else
			{
				menu.addAction(_("Set Start Position to Cue"),this,SLOT(onSetStartPositionToCue()));
				menu.addAction(_("Set Stop Position to Cue"),this,SLOT(onSetStopPositionToCue()));
			}
			menu.addSeparator();
			menu.addAction(_("&Edit Cue"),this,SLOT(onEditCue()));
			menu.addAction(_("&Remove Cue"),this,SLOT(onRemoveCue()));
			menu.addSeparator();
			menu.addAction(_("Show Cues &List"),this,SLOT(onShowCueList()));
		}
		else
		{
			addCueTime=  wv->getSamplePosForScreenX(pos.x());

			menu.addAction(_("Center Start Position"),this,SLOT(onCenterStartPosition()));
			menu.addAction(_("Center Stop Position"),this,SLOT(onCenterStopPosition()));
			menu.addSeparator();
			menu.addAction(_("Add Cue at This Position"),this,SLOT(onAddCueAtThisPosition()));
			menu.addAction(_("Add Cue at Start Position"),this,SLOT(onAddCueAtStartPosition()));
			menu.addAction(_("Add Cue at Stop Position"),this,SLOT(onAddCueAtStopPosition()));
			menu.addSeparator();
			menu.addAction(_("Show Cues &List"),this,SLOT(onShowCueList()));
		}

		menu.exec(mapToGlobal(pos));
		cueClicked=NIL_CUE_INDEX;
	}

	void onCenterStartPosition()
	{ 
		wv->centerStartPos();
	}

	void onCenterStopPosition()
	{
		wv->centerStopPos();
	}

	void addCue(sample_pos_t position)
	{
		CActionParameters actionParameters(NULL);

		actionParameters.setValue<string>("name",gAddCueAtClick_CueName);
		actionParameters.setValue<sample_pos_t>("position",position);
		actionParameters.setValue<bool>("isAnchored",gAddCueAtClick_Anchored);

		wv->addCueActionFactory->performAction(wv->loadedSound,&actionParameters,false);
		wv->updateFromEdit();
		update();
	}

	void onAddCueAtThisPosition()
	{
		addCue(addCueTime);
	}

	void onAddCueAtStartPosition()
	{
		addCue(wv->loadedSound->channel->getStartPosition());
	}

	void onAddCueAtStopPosition()
	{
		addCue(wv->loadedSound->channel->getStopPosition());
	}

	void onShowCueList()
	{
		/* ??? FIXME
		gCueListDialog->setLoadedSound(loadedSound);
		gCueListDialog->execute(PLACEMENT_CURSOR);
		*/
	}


	void onSetStartPositionToCue()
	{
		wv->loadedSound->channel->setStartPosition((sample_pos_t)wv->loadedSound->sound->getCueTime(cueClicked));
		wv->wvsa->viewport()->update();
	}

	void onSetStopPositionToCue()
	{
		wv->loadedSound->channel->setStopPosition((sample_pos_t)wv->loadedSound->sound->getCueTime(cueClicked));
		wv->wvsa->viewport()->update();
	}

	void onEditCue()
	{
		CActionParameters actionParameters(NULL);

		// add the parameters for the dialog to display initially
		actionParameters.setValue<unsigned>("index",cueClicked);
		actionParameters.setValue<string>("name",wv->loadedSound->sound->getCueName(cueClicked));
		actionParameters.setValue<sample_pos_t>("position",wv->loadedSound->sound->getCueTime(cueClicked));
		actionParameters.setValue<bool>("isAnchored",wv->loadedSound->sound->isCueAnchored(cueClicked));

		replaceCueActionFactory->performAction(wv->loadedSound,&actionParameters,false);

		wv->wvsa->viewport()->update();
		update();
	}

	void onRemoveCue()
	{
		CActionParameters actionParameters(NULL);
		actionParameters.setValue<unsigned>("index",cueClicked);
		removeCueActionFactory->performAction(wv->loadedSound,&actionParameters,false);

		wv->wvsa->viewport()->update();
		update();
	}



private:
	CSoundWindow *wv;

	QFont rulerFont;

	#define NIL_CUE_INDEX (0xffffffff)
	size_t cueClicked; // the index of the cue clicked on holding the value between the click event and the menu item event
	int cueClickedOffset; // used when dragging cues to know how far from the middle a cue was clicked
	sample_pos_t origCueClickedTime;
	sample_pos_t origStartPosition; // start position before drag cue start (for undo purposes)
	sample_pos_t origStopPosition; // stop position before drag cue start (for undo purposes)
	sample_pos_t addCueTime; // the time in the audio where the mouse was clicked to add a cue if that's what they choose

	bool draggingCue;

	size_t focusedCueIndex; // NIL_CUE_INDEX if none focused
	sample_pos_t focusedCueTime;

	bool draggingSelectionToo; // true when a cue to be dragged was on the start or stop position

	CSelectionEditPositionFactory *selectionEditPositionFactory;
	CMoveCueActionFactory *moveCueActionFactory;
	CRemoveCueActionFactory *removeCueActionFactory;
	CReplaceCueActionFactory *replaceCueActionFactory;
};
#warning search FXRezWaveView for all places that rulerPanel was updated

void drawWaveRuler(IWaveDrawing *wd,QWidget *w,QPaintEvent *e,int offsetX,int totalWidth,const QFont &rulerFont,const QPalette &pal,CSound *sound,bool hasFocus,size_t focusedCueIndex,sample_pos_t focusedCueTime)
{
	QPainter dc;
	dc.begin(w);
	dc.setFont(rulerFont);

	#define LABEL_TICK_FREQ 80
	/*
	 * Here, we draw the ruler
	 * 	We want to always put 1 labeled position every LABELED_TICK_FREQ pixels 
	 * 	And draw ruler ticks also
	 */

	int lastX=totalWidth-offsetX;

	int start=e->rect().left();
	start-=LABEL_TICK_FREQ-1;

	int end=start+e->rect().width();
	end+=LABEL_TICK_FREQ-1;

	if(end>lastX)
		end=lastX;

	for(int _x=max(0,start);_x<=end;_x++)
	{
		int x=_x+offsetX;
		dc.setPen(pal.color(QPalette::Text));
		if((_x%LABEL_TICK_FREQ)==0)
		{
			dc.drawLine(x,0,x,10);

			const string time=sound->getTimePosition(wd->getSamplePosForScreenX(_x));

			dc.drawText(x+2,14,time.c_str());

			// draw ligher ticks around main long tickmark
			dc.setPen(pal.color(QPalette::Mid));
			dc.drawLine(x-1,0,x-1,7);
			dc.drawLine(x+1,0,x+1,7);
		}
		else if(((_x+(LABEL_TICK_FREQ/2))%(LABEL_TICK_FREQ))==0)
			dc.drawLine(x,0,x,4); // half way tick between labeled ticks
		else if((_x%(LABEL_TICK_FREQ/10))==0)
			dc.drawLine(x,0,x,2); // small tenth ticks
	}

	// render cues
	const size_t cueCount=sound->getCueCount();
	for(size_t t=0;t<cueCount;t++)
	{
		// ??? I could figure out the min and max screen-visible time and make sure that is in range before testing every cue
		int cueXPosition=wd->getCueScreenX(t);

		if(cueXPosition!=CUE_OFF_SCREEN)
		{
			cueXPosition+=offsetX;
			const string cueName=sound->getCueName(t);

			if(hasFocus && focusedCueIndex!=NIL_CUE_INDEX && focusedCueTime==sound->getCueTime(t))
			{ // draw focus rectangle (before drawing everything else)

				// rect around cue symbol
				QRect cueRect(cueXPosition-CUE_RADIUS,CUE_Y(w)-CUE_RADIUS, CUE_RADIUS*2,CUE_RADIUS*2);

				// rect around text
				QRect textRect=QFontMetrics(rulerFont).boundingRect(cueName.c_str());
				textRect.moveTo(cueXPosition+CUE_RADIUS+2, w->height()-2-textRect.height());

				{ // I don't know why, but this hack is here because boundingRect() returns something that is almost twice as tall as it needs to be
					textRect.adjust(0,textRect.height()/2-1,0,0); 
				}

				// unite the two rects
				QRect focusRect=cueRect.united(textRect);

				/*
				// draw the focus rect
				focusRect.adjust(-1,-1,1,1);
				QPen p(pal.color(QPalette::Dark));
				p.setStyle(Qt::DotLine);
				dc.setPen(p);
				dc.drawRect(focusRect);
				*/
				
				// draw the focus rect using the current style
				QStyleOptionFocusRect option;
				option.initFrom(wd);
				option.rect=focusRect;
				option.rect.adjust(-2,-2,2,2);
				option.backgroundColor = pal.color(QPalette::Background);
				w->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &dc, wd);
			}

			dc.setPen(pal.color(QPalette::Text));
			dc.drawText(cueXPosition+CUE_RADIUS+2,w->height()-2,cueName.c_str());

			//#warning drawArc sux at drawing small circles, report this to Trolltech
			//dc.drawArc(cueXPosition-CUE_RADIUS,CUE_Y(w)-CUE_RADIUS,CUE_RADIUS*2,CUE_RADIUS*2,0*16,360*16);
			dc.drawEllipse(QRect(cueXPosition-CUE_RADIUS,CUE_Y(w)-CUE_RADIUS,CUE_RADIUS*2,CUE_RADIUS*2));

			dc.setPen(sound->isCueAnchored(t) ? QColor(0,0,255) : QColor(255,0,0));
			dc.drawLine(cueXPosition-1,w->height()-4,cueXPosition-1,CUE_Y(w)-1);
			dc.drawLine(cueXPosition,w->height()-1,cueXPosition,CUE_Y(w));
			dc.drawLine(cueXPosition+1,w->height()-4,cueXPosition+1,CUE_Y(w)-1);
		}
	}

	dc.end();
}


// ------------------------------

#if 0
class CSamplePosSpinBox : public QAbstractSpinBox
{
	Q_OBJECT

public:
	CSamplePosSpinBox(CSoundWindow *_sw) :
		sw(_sw)
	{
		setButtonSymbols(QAbstractSpinBox::PlusMinus);
	}

	virtual ~CSamplePosSpinBox()
	{
	}

	void setValue(sample_pos_t v) { _value=v; }
	sample_pos_t value() const { return _value; }

	/*override*/ void stepBy(int steps)
	{
		_value+=steps;
		sw->updateSelectionStatusInfo();
	}

private:
	CSoundWindow *sw;

	sample_pos_t _value;
};
#endif

#if 0 
class CSamplePosSpinBoxEdit : public QLineEdit
{
public:
	CSamplePosSpinBoxEdit() :
		QLineEdit()
	{
	}

	virtual ~CSamplePosSpinBoxEdit()
	{
	}

	QSize sizeHint() const
	{
		//return QSize(fontMetrics()->width(text()),fontMetrics->height());
		//return fontMetrics().size(Qt::TextSingleLine,text());
		return QSize(500,20);
	}

	QSize minimumSizeHint() const
	{
		return sizeHint();
	}
};

class CSamplePosSpinBox : public QSpinBox
{
	Q_OBJECT
public:
	CSamplePosSpinBox(CSoundWindow *_sw) :
		sw(_sw)
	{
		setReadOnly(true);
		setFocusPolicy(Qt::NoFocus);
		setLineEdit(new CSamplePosSpinBoxEdit);
		setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
		lineEdit()->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
	}

	virtual ~CSamplePosSpinBox()
	{
	}

	/*override*/ QString textFromValue(int v) const
	{
		QString s;
		for(size_t t=(TimeUnits)0;t<TIME_UNITS_COUNT;t++)
		{
			if(gShowTimeUnits[t])
				s+=(sw->getLengthString(sw->loadedSound->channel->getStartPosition(),(TimeUnits)t)+"  ").c_str();
		}
		return s;
	}

	CSoundWindow *sw;
};
#endif

// ------------------------------

#include "CSoundWindow.moc"

static void playTrigger(void *Pthis);
CSoundWindow::CSoundWindow(QLayout * parent,CLoadedSound *_loadedSound) :
	loadedSound(_loadedSound),
	isActive(false),


	horzZoomFactor(1.0),
	vertZoomFactor(1.0),

	horzOffset(0),
	vertOffset(0),

	m_drawPlayPositionAt(MAX_LENGTH),
	//prevDrawPlayStatusX(0xffffffff),
	renderedStartPosition(0),renderedStopPosition(0),

	lastHorzZoom(-1.0),lastVertZoom(-1.0),

	lastChangedPosition(lcpStart),

	muteButtonCount(0),

	addCueActionFactory(new CAddCueActionFactory(NULL)),

	playingLEDOn(false),
	pausedLEDOn(false)
{
	setupUi(this);

	// set a fixed width on the playPositionLabel which reduces CPU since it doesn't have to layout every time the text changes
	playPositionLabel->setFixedWidth(
		playPositionLabel->fontMetrics().width(
			(string(_("Playing: "))+"000:00:00.00000").c_str()
		)
	);
		
	parent->addWidget(this);

#if 0
	if I can't figure out the spinner stuff... I might just leave it off.. implementing it will likely prove just to be an exercize more than it is useful to users
	// setup spinners and such
	{
		selectStartSpinBox=new CSamplePosSpinBox(this);
		((QBoxLayout *)selectStartFrame->layout())->insertWidget(0,selectStartSpinBox);

		selectStopSpinBox=new CSamplePosSpinBox(this);
		((QBoxLayout *)selectStopFrame->layout())->insertWidget(0,selectStopSpinBox);
	}
#endif

	// add scroll area and ruler
	{
		contentsFrame->setLayout(new QVBoxLayout);
		contentsFrame->layout()->setMargin(0);
		contentsFrame->layout()->setSpacing(0);

		ruler=new CWaveRuler(this);
		contentsFrame->layout()->addWidget(ruler);

		wvsa=new CSoundWindowScrollArea(this);
		contentsFrame->layout()->addWidget(wvsa);
	}

	// build arrays of labels for showing multiple units
	{
		totalLengthLabels[tuSeconds]=totalLengthLabel_seconds;
		totalLengthLabels[tuFrames]=totalLengthLabel_frames;
		totalLengthLabels[tuBytes]=totalLengthLabel_bytes;

		selectionLengthLabels[tuSeconds]=selectionLengthLabel_seconds;
		selectionLengthLabels[tuFrames]=selectionLengthLabel_frames;
		selectionLengthLabels[tuBytes]=selectionLengthLabel_bytes;

		selectStartLabels[tuSeconds]=selectStartLabel_seconds;
		selectStartLabels[tuFrames]=selectStartLabel_frames;
		selectStartLabels[tuBytes]=selectStartLabel_bytes;

		selectStopLabels[tuSeconds]=selectStopLabel_seconds;
		selectStopLabels[tuFrames]=selectStopLabel_frames;
		selectStopLabels[tuBytes]=selectStopLabel_bytes;
	}

	recreateMuteButtons();

	// adjust ranges on sliders for more accuracy
	horzZoomSlider->setMaximum(horzZoomSlider->maximum()*ZOOM_MUL);
	horzZoomSlider->setTickInterval(horzZoomSlider->tickInterval()*ZOOM_MUL);
	horzZoomSlider->setPageStep(horzZoomSlider->maximum()/25);

	vertZoomSlider->setMaximum(vertZoomSlider->maximum()*ZOOM_MUL);
	vertZoomSlider->setTickInterval(vertZoomSlider->tickInterval()*ZOOM_MUL);
	vertZoomSlider->setPageStep(vertZoomSlider->maximum()/25);
	
	// create a delayed call to showAmount() since we need to be laid out first
	connect(this,SIGNAL(afterLayout()),this,SLOT(onAfterLayout()),Qt::QueuedConnection);
	Q_EMIT afterLayout();
	hide();

	// register to know when the sound start/stops so we can know when it is necessary
	// to draw the play position and when to change the state of the playing/paused LEDs
	loadedSound->channel->addOnPlayTrigger(playTrigger,this);
}

CSoundWindow::~CSoundWindow()
{
	loadedSound->channel->removeOnPlayTrigger(playTrigger,this);
	delete addCueActionFactory;
}

void CSoundWindow::onAfterLayout()
{
	setVertZoom(0.0,true);
	showAmount(gInitialLengthToShow,0);
}

void CSoundWindow::setActiveState(bool _isActive)
{
	isActive=_isActive;
	if(isActive)
	{
		static_cast<CMainWindow *>(parent())->positionShuttleGivenSpeed(loadedSound->channel->getSeekSpeed(),shuttleControlScalar,shuttleControlSpringBack);
		gSoundFileManager->untoggleActiveForAllSoundWindows(this);
		show();
	}
	else
		hide();
}

bool CSoundWindow::getActiveState() const
{
	return isActive;
}

void CSoundWindow::updateFromEdit(bool undoing)
{
	if(undoing)
		setHorzZoom(getHorzZoom(),hrtNone,true);
	else
		setHorzZoom(getHorzZoom(),hrtLeftEdge,true);

	if(loadedSound->sound->getChannelCount()!=muteButtonCount)
		recreateMuteButtons();

	const float origVertZoomFactor=vertZoomFactor;
	const float vZoom=getVertZoom();
	setVertZoom(vZoom,true);
	if(origVertZoomFactor!=vertZoomFactor) // if the vertical zoom factor had to change, then re-center the vertical offset
		setVertOffset(getVertSize()/2-wvsa->viewport()->height()/2);

	updateAllStatusInfo();
	wvsa->viewport()->update();
	ruler->update();
}

void CSoundWindow::recreateMuteButtons()
{
	Qt::CheckState state[MAX_CHANNELS];
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		state[t]=Qt::Unchecked;

	for(unsigned t=0;t<muteButtonCount;t++)
		state[t]=muteButtons[t]->checkState();

	// clear all children
	delete muteButtonFrame->layout();
	qDeleteAll(muteButtonFrame->children());
	muteButtonFrame->setLayout(new QVBoxLayout);
	((QVBoxLayout*)muteButtonFrame->layout())->setSpacing(0);
	((QVBoxLayout*)muteButtonFrame->layout())->setMargin(1);

	// set the sizing for mute buttons to look right
	{
		int p;
		p=style()->pixelMetric(QStyle::PM_ScrollBarExtent);
		invertMutesButton->setFixedSize(QSize(p,p));
		clearMutesButton->setFixedSize(QSize(p,p));
		muteButtonsLabel->setFixedHeight(ruler->sizeHint().height()+p);

		p=style()->pixelMetric(QStyle::PM_IndicatorWidth);
		leftFrame->setMaximumWidth(p+4);
	}

	muteButtonCount=loadedSound->sound->getChannelCount();
	for(unsigned t=0;t<muteButtonCount;t++)
	{
		muteButtons[t]=new QCheckBox();
		muteButtons[t]->setCheckState(state[t]); // restore the checked state
		connect(muteButtons[t],SIGNAL(clicked()),this,SLOT(onMuteButtonClicked()));

		muteButtonFrame->layout()->addItem(new QSpacerItem(1,1,QSizePolicy::Fixed,QSizePolicy::Expanding));
		muteButtonFrame->layout()->addWidget(muteButtons[t]);
		muteButtonFrame->layout()->addItem(new QSpacerItem(1,1,QSizePolicy::Fixed,QSizePolicy::Expanding));
	}
}

void CSoundWindow::onMuteButtonClicked()
{
	for(unsigned t=0;t<loadedSound->sound->getChannelCount();t++)
		loadedSound->channel->setMute(t,muteButtons[t]->checkState() ? true : false);
}

void CSoundWindow::on_clearMutesButton_clicked()
{
	for(unsigned t=0;t<muteButtonCount;t++)
		muteButtons[t]->setCheckState(Qt::Unchecked);
	onMuteButtonClicked();
}

void CSoundWindow::on_invertMutesButton_clicked()
{
	for(unsigned t=0;t<muteButtonCount;t++)
		muteButtons[t]->setCheckState(muteButtons[t]->checkState()==Qt::Checked ? Qt::Unchecked : Qt::Checked);
	onMuteButtonClicked();
}

void CSoundWindow::updateRuler()
{
	ruler->update();
}

static const string commifyNumber(const string s)
{
	string ret;
	ret.reserve(s.length());
	for(size_t t=0;t<s.length();t++)
	{
		if(t>0 && (t%3)==0)
			ret=","+ret;
		ret=s[s.length()-t-1]+ret;
	}

	return ret;
}

int CSoundWindow::getZoomDecimalPlaces() const
{
	// ??? this should depend on the actual horzZoomFactor value because the QSlider doesn't represent how many samples a pixel represents
	int places;
	if(horzZoomSlider->value()>75*ZOOM_MUL)
		places=5;
	else if(horzZoomSlider->value()>60*ZOOM_MUL)
		places=4;
	else if(horzZoomSlider->value()>40*ZOOM_MUL)
		places=3;
	else
		places=2;

	return places;
}

const string CSoundWindow::getLengthString(sample_pos_t length,TimeUnits units)
{
	if(units==tuSeconds)
		return loadedSound->sound->getTimePosition(length,getZoomDecimalPlaces());
	else if(units==tuFrames)
		return commifyNumber(istring(length))+"f";
	else if(units==tuBytes)
		return commifyNumber(istring(length*sizeof(sample_t)*loadedSound->sound->getChannelCount()))+"b";
	throw runtime_error(string(__func__)+" -- unhandled units enum value: "+istring(units));
}

void CSoundWindow::updateAllStatusInfo()
{
	updateZoomStatusInfo(); // called first because some later logic depends on zoom slider values

	sampleRateLabel->setText((_("Rate: ")+istring(loadedSound->sound->getSampleRate())).c_str());
	channelCountLabel->setText((_("Channels: ")+istring(loadedSound->sound->getChannelCount())).c_str());

	audioSizeLabel->setText(loadedSound->sound->getAudioDataSize().c_str());
	workingFileSizeLabel->setText(loadedSound->sound->getPoolFileSize().c_str());

	for(size_t t=0;t<TIME_UNITS_COUNT;t++)
	{
		if(gShowTimeUnits[t])
		{
			totalLengthLabels[t]->show();
			selectionLengthLabels[t]->show();
			selectStartLabels[t]->show();
			selectStopLabels[t]->show();
		}
		else
		{
			totalLengthLabels[t]->hide();
			selectionLengthLabels[t]->hide();
			selectStartLabels[t]->hide();
			selectStopLabels[t]->hide();
		}
	}

	for(size_t t=0;t<TIME_UNITS_COUNT;t++)
		totalLengthLabels[t]->setText(getLengthString(loadedSound->sound->getLength(),(TimeUnits)t).c_str());

	updateSelectionStatusInfo();
	updatePlayPositionStatusInfo();
}

void CSoundWindow::updateSelectionStatusInfo()
{
	for(size_t t=(TimeUnits)0;t<TIME_UNITS_COUNT;t++)
	{
		selectionLengthLabels[t]->setText(getLengthString(loadedSound->channel->getStopPosition()-loadedSound->channel->getStartPosition()+1,(TimeUnits)t).c_str());
		selectStartLabels[t]->setText(getLengthString(loadedSound->channel->getStartPosition(),(TimeUnits)t).c_str());
		selectStopLabels[t]->setText(getLengthString(loadedSound->channel->getStopPosition(),(TimeUnits)t).c_str());
	}
}

void CSoundWindow::updateZoomStatusInfo()
{
	// horz
	horzZoomSlider->blockSignals(true);
		horzZoomSlider->setValue(zoomFactorToZoomSlider(lastHorzZoom));
	horzZoomSlider->blockSignals(false);
	horzZoomValueLabel->setText(("  "+istring(horzZoomSlider->value()/(double)ZOOM_MUL,3,1,true)+"%").c_str());
	horzZoomValueLabel->repaint(); // make it update now

	// adjust horz scroll position and range for new horz zoom factor
	// 	(would be a problem for files with more than 2billion sample frames)
	// and block signals here because sometimes the current scrollbar value is out of the new range and that calls us back and messes thing up
	wvsa->horizontalScrollBar()->blockSignals(true); 
	wvsa->horizontalScrollBar()->setRange(0,getHorzSize()-wvsa->viewport()->width());
	wvsa->horizontalScrollBar()->blockSignals(false);
	wvsa->horizontalScrollBar()->setValue(horzOffset);

	// ---------
	
	// vert
	vertZoomSlider->blockSignals(true);
		vertZoomSlider->setValue(zoomFactorToZoomSlider(lastVertZoom));
	vertZoomSlider->blockSignals(false);

	// adjust vert scroll position and range for new vert zoom factor
	// and block signals here because sometimes the current scrollbar value is out of the new range and that calls us back and messes thing up
	wvsa->verticalScrollBar()->blockSignals(true); 
	wvsa->verticalScrollBar()->setRange(0,getVertSize()-wvsa->viewport()->height());
	wvsa->verticalScrollBar()->blockSignals(false);
	wvsa->verticalScrollBar()->setValue(vertOffset);

}

void playTrigger(void *Pthis)
{
	CSoundWindow *that=(CSoundWindow *)Pthis;

	// call updatePlayPisitionStatusInfo() now, within the main thread (which will also schedule a timer for it to be called again if playing)
	bool b=QMetaObject::invokeMethod(that,"updatePlayPositionStatusInfo");
}

void CSoundWindow::updatePlayPositionStatusInfo()
{
	/*
	FXDCWindow dc(playPositionLabel);

	if(prevPlayPositionLabelWidth>0)
	{ // erase previous text
		dc.setForeground(playPositionLabel->getBackColor());
		dc.fillRectangle(2,0,prevPlayPositionLabelWidth,playPositionLabel->getHeight());
		prevPlayPositionLabelWidth=0;
	}

	if(loadedSound->channel->isPlaying())
	{ // draw new play position
		const string label=_("Playing: ")+getLengthString(loadedSound->channel->getPosition(),tuSeconds);

		dc.compat_setFont(statusFont);
		dc.setForeground(FXRGB(0,0,0));
		dc.drawText(2,(playPositionLabel->getHeight()/2)+(statusFont->getFontHeight()/2),label.c_str(),label.size());

		prevPlayPositionLabelWidth=statusFont->getTextWidth(label.c_str(),label.size())+1;
	}
	*/
	// ^^ I think I did it that way because laying out the text was so CPU intensive.. I'll need to see if the same is true for Qt
	if(loadedSound->channel->isPlaying())
		playPositionLabel->setText((_("Playing: ")+getLengthString(loadedSound->channel->getPosition(),tuSeconds)).c_str());
	else
		playPositionLabel->setText(_("Playing:"));

	if(loadedSound->channel->isPlaying() && !playingLEDOn)
	{
		playingLED->setCurrentIndex(1);
		playingLEDOn=true;
	}
	else if(!loadedSound->channel->isPlaying() && playingLEDOn)
	{
		playingLED->setCurrentIndex(0);
		playingLEDOn=false;
	}

	if(loadedSound->channel->isPaused() && !pausedLEDOn)
	{
		pausedLED->setCurrentIndex(1);
		pausedLEDOn=true;
	}
	else if(!loadedSound->channel->isPaused() && pausedLEDOn)
	{
		pausedLED->setCurrentIndex(0);
		pausedLEDOn=false;
	}


	if(loadedSound->channel->isPlaying())
	{
		drawPlayPosition(loadedSound->channel->getPosition(),false,gFollowPlayPosition);
		QTimer::singleShot(DRAW_PLAY_POSITION_TIME,this,SLOT(updatePlayPositionStatusInfo()));
	}
	else
	{
		// erase the play position line and don't register to get the timer again
		drawPlayPosition(loadedSound->channel->getPosition(),true,false);
	}
}

void CSoundWindow::on_infoFrame_customContextMenuRequested(const QPoint &pos)
{
	QMenu *menu=new QMenu;
	menu->addAction(_("Show Time Units in..."));
	menu->addSeparator();

		QAction *secondsAction=new QAction(_("Seconds"),menu);
		secondsAction->setCheckable(true);
		secondsAction->setChecked(gShowTimeUnits[tuSeconds]);
	menu->addAction(secondsAction);

		QAction *framesAction=new QAction(_("Frames"),menu);
		framesAction->setCheckable(true);
		framesAction->setChecked(gShowTimeUnits[tuFrames]);
	menu->addAction(framesAction);

		QAction *bytesAction=new QAction(_("Bytes"),menu);
		bytesAction->setCheckable(true);
		bytesAction->setChecked(gShowTimeUnits[tuBytes]);
	menu->addAction(bytesAction);

	QAction *action=menu->exec(infoFrame->mapToGlobal(pos));

	if(action==secondsAction)
		gShowTimeUnits[tuSeconds]=!gShowTimeUnits[tuSeconds];
	else if(action==framesAction)
		gShowTimeUnits[tuFrames]=!gShowTimeUnits[tuFrames];
	else if(action==bytesAction)
		gShowTimeUnits[tuBytes]=!gShowTimeUnits[tuBytes];

	delete menu;

	updateAllStatusInfo();
}


// -------------

// the given zoom factor should be in the range of 0 (fit in window) to 1 (zoomed all the way in)
void CSoundWindow::setHorzZoom(double v,HorzRecenterTypes horzRecenterType,bool force)
{
	// save the current zoom factor
	const double oldZoom=getHorzZoom();

	// recalculate horz offset for the new zoom factor
	v=max(0.0,min(1.0,v)); // make v in range: [0,1]
	if(force || v!=lastHorzZoom)
	{
		const sample_fpos_t old_horzZoomFactor=horzZoomFactor;
		const bool startPosIsOnScreen=isStartPosOnScreen();
		const bool stopPosIsOnScreen=isStopPosOnScreen();
		const sample_fpos_t startPositionScreenX=getDrawSelectStart();
		const sample_fpos_t stopPositionScreenX=getDrawSelectStop();

		// how many samples are represented by one pixel when fully zoomed out (accounting for a small right side margin of unused space)
		const sample_fpos_t maxZoomFactor=(sample_fpos_t)loadedSound->sound->getLength()/(sample_fpos_t)max(1,(int)(wvsa->viewport()->width()-RIGHT_MARGIN));

		// map v:0..1 --> horzZoomFactor:maxZoomFactor..1
		horzZoomFactor=maxZoomFactor+((1.0-maxZoomFactor)*v);

		// adjust the horz offset to try to keep something centered on screen while zooming
		sample_fpos_t tHorzOffset=horzOffset;
		switch(horzRecenterType)
		{
		case hrtNone:
			break;

		case hrtAuto:
			if(lastChangedPosition==lcpStart && startPosIsOnScreen)
				// start postion where it is
				tHorzOffset=loadedSound->channel->getStartPosition()/horzZoomFactor-startPositionScreenX;
			else if(lastChangedPosition==lcpStop && stopPosIsOnScreen)
				// stop postion where it is
				tHorzOffset=loadedSound->channel->getStopPosition()/horzZoomFactor-stopPositionScreenX;
			else if(startPosIsOnScreen)
				// start postion where it is
				tHorzOffset=loadedSound->channel->getStartPosition()/horzZoomFactor-startPositionScreenX;
			else if(stopPosIsOnScreen)
				// stop postion where it is
				tHorzOffset=loadedSound->channel->getStopPosition()/horzZoomFactor-stopPositionScreenX;
			else
				// leftEdge
				tHorzOffset=horzOffset/horzZoomFactor*old_horzZoomFactor;
			break;

		case hrtStart:
			tHorzOffset=getHorzOffsetToCenterStartPos();
			break;

		case hrtStop:
			tHorzOffset=getHorzOffsetToCenterStopPos();
			break;

		case hrtLeftEdge:
			tHorzOffset=(sample_pos_t)(sample_fpos_round(horzOffset/horzZoomFactor*old_horzZoomFactor));
			break;
		};
		// make sure it's in range and round
		horzOffset=(sample_pos_t)sample_fpos_round(max((sample_fpos_t)0.0,min((sample_fpos_t)getHorzSize(),tHorzOffset)));

		// if there are so few samples that less than one sample is represented by a pixel, then don't allow the offset because it causes problems
		if(horzZoomFactor<1.0)
			horzOffset=0;

		lastHorzZoom=v;
	}

	/*
	// NOTE, that calling setValue makes it's slot invoked which turns around and calls this method again, but it blocks the slider's signals before calling this method
	horzZoomSlider->setValue(zoomFactorToZoomSlider(lastHorzZoom));

	// adjust horz scroll position and range for new horz zoom factor
	// 	(would be a problem for files with more than 2billion sample frames)
	// and block signals here because sometimes the current scrollbar value is out of the new range and that calls us back and messes thing up
	wvsa->horizontalScrollBar()->blockSignals(true); 
	wvsa->horizontalScrollBar()->setRange(0,getHorzSize()-wvsa->viewport()->width());
	wvsa->horizontalScrollBar()->blockSignals(false);
	wvsa->horizontalScrollBar()->setValue(horzOffset);
	*/
	updateAllStatusInfo();

	// schedule a redraw
	//wvsa->viewport()->update();
	// actually.. draw now because it seems a bit slow just to schedule one
	redraw();
}

const sample_pos_t CSoundWindow::getHorzSize() const
{
	return (sample_pos_t)((sample_fpos_t)loadedSound->sound->getLength()/horzZoomFactor)+RIGHT_MARGIN; // accounting for the right margin of unused space
}

const double CSoundWindow::getHorzZoom() const
{
	return lastHorzZoom;
}

void CSoundWindow::horzZoomSelectionFit()
{
	showAmount((sample_fpos_t)(loadedSound->channel->getStopPosition()-loadedSound->channel->getStartPosition()+1)/(sample_fpos_t)loadedSound->sound->getSampleRate(),loadedSound->channel->getStartPosition(),10);
}

void CSoundWindow::redraw()
{
	wvsa->viewport()->repaint(); // ??? try using update
	ruler->update();
}

const float CSoundWindow::getVertZoom() const
{
	return lastVertZoom;
}

void CSoundWindow::setVertZoom(float v,bool force)
{
	v=max(0.0f,min(1.0f,v)); // make v in range: [0,1]

	if(force || v!=lastVertZoom)
	{
		// how many sample values are represented by one pixel when fully zoomed out (as if there were one channel)
		const float maxZoomFactor=MAX_WAVE_HEIGHT/(float)wvsa->viewport()->height();

		const int oldSize=getVertSize()/2;

		// map v:0..1 --> vertZoomFactor:maxZoomFactor..1
		vertZoomFactor=maxZoomFactor+((1.0f-maxZoomFactor)*v);

		// adjust the vertical offset to try to keep the same thing centered when changing the zoom factor
		const int newSize=getVertSize()/2;
		vertOffset+=(newSize-oldSize);
		vertOffset=max(0,vertOffset);

		lastVertZoom=v;
	}

	/*
	vertZoomSlider->setValue(zoomFactorToZoomSlider(lastVertZoom));

	// adjust vert scroll position and range for new vert zoom factor
	// and block signals here because sometimes the current scrollbar value is out of the new range and that calls us back and messes thing up
	wvsa->verticalScrollBar()->blockSignals(true); 
	wvsa->verticalScrollBar()->setRange(0,getVertSize()-wvsa->viewport()->height());
	wvsa->verticalScrollBar()->blockSignals(false);
	wvsa->verticalScrollBar()->setValue(vertOffset);
	*/
	updateZoomStatusInfo();

	// schedule a redraw
	//wvsa->viewport()->update();
	// actually.. draw now because it seems a bit slow just to schedule one
	wvsa->viewport()->repaint(); // redraw()??? update()???
}

const int CSoundWindow::getVertSize() const
{
	return (int)ceil(MAX_WAVE_HEIGHT/vertZoomFactor);
}


void CSoundWindow::setHorzOffset(sample_pos_t v)
{
	// ??? have a problem in that QScrollBars take 32bit ints.. 
	wvsa->horizontalScrollBar()->setValue(v); 
}

const sample_pos_t CSoundWindow::getHorzOffset() const
{
	return horzOffset;
}

void CSoundWindow::setVertOffset(int v)
{
	if(v!=vertOffset)
	{
		vertOffset=v;

		// ???  should update only what is necessary from the previous position if we were in a view mode where that would help.. when drawing just the waveform, drawing anything horizontal means drawing the whole vertical part too
		
		wvsa->viewport()->update();
	}
}

const int CSoundWindow::getVertOffset() const
{
	return vertOffset;
}

const sample_pos_t CSoundWindow::getHorzOffsetToCenterTime(sample_pos_t time) const
{
	if(time>=loadedSound->sound->getLength())
		time=loadedSound->sound->getLength()-1;
	return (sample_pos_t)max((sample_fpos_t)0.0,sample_fpos_round(time/horzZoomFactor)-wvsa->viewport()->width()/2);
}

const sample_pos_t CSoundWindow::getHorzOffsetToCenterStartPos() const
{
	return getHorzOffsetToCenterTime(loadedSound->channel->getStartPosition());
}

const sample_pos_t CSoundWindow::getHorzOffsetToCenterStopPos() const
{
	return getHorzOffsetToCenterTime(loadedSound->channel->getStopPosition());
}

void CSoundWindow::showAmount(double seconds,sample_pos_t pos,int marginPixels)
{
	//printf("%f < %u/%u\n",seconds ,loadedSound->sound->getLength(), loadedSound->sound->getSampleRate());
	if(seconds<(loadedSound->sound->getLength()/loadedSound->sound->getSampleRate()))
	{
		// ??? why wouldn't I just call setHorzZoom() and setHorzOffset() with the right values? .. maybe the math was too convoluted?
		// calculate the zoom percent that will show the right amount and setHorzZoomFactor which will update the GUI as well

		horzZoomFactor=max((sample_fpos_t)1.0,((sample_fpos_t)seconds*loadedSound->sound->getSampleRate())/(wvsa->viewport()->width()-(2*marginPixels)));
		horzOffset=(sample_pos_t)(max((sample_fpos_t)0,min((sample_fpos_t)loadedSound->sound->getLength()-wvsa->viewport()->width()+RIGHT_MARGIN,(sample_fpos_t)pos-(marginPixels*horzZoomFactor)))/horzZoomFactor);

		// recalc the percent value so that getHorzZoom() will return the correct value
		const sample_fpos_t maxZoomFactor=(sample_fpos_t)loadedSound->sound->getLength()/(sample_fpos_t)max(1,(int)(wvsa->viewport()->width()-RIGHT_MARGIN));
		lastHorzZoom=(horzZoomFactor-maxZoomFactor)/(1.0-maxZoomFactor);

		updateAllStatusInfo();
	}
	else
	{
		setHorzZoom(0.0,hrtNone,true);
		setHorzOffset(0);
	}

	wvsa->viewport()->update();
	updateRuler();
}        

void CSoundWindow::centerStartPos()
{
	setHorzOffset(getHorzOffsetToCenterStartPos());
}

void CSoundWindow::centerStopPos()
{
	setHorzOffset(getHorzOffsetToCenterStopPos());
}

const bool CSoundWindow::isStartPosOnScreen() const
{
	sample_fpos_t x=getDrawSelectStart();
	return (x>=0 && x<wvsa->viewport()->width());
}

const bool CSoundWindow::isStopPosOnScreen() const
{
	sample_fpos_t x=getDrawSelectStop();
	return (x>=0 && x<wvsa->viewport()->width());
}

const sample_fpos_t CSoundWindow::getRenderedDrawSelectStart() const
{
	return sample_fpos_round((sample_fpos_t)renderedStartPosition/horzZoomFactor-(sample_fpos_t)horzOffset); 
}

const sample_fpos_t CSoundWindow::getRenderedDrawSelectStop() const
{
	return sample_fpos_round((sample_fpos_t)renderedStopPosition/horzZoomFactor-(sample_fpos_t)horzOffset);
}

const sample_fpos_t CSoundWindow::getDrawSelectStart() const
{
	return sample_fpos_round((sample_fpos_t)loadedSound->channel->getStartPosition()/horzZoomFactor-(sample_fpos_t)horzOffset);
}

const sample_fpos_t CSoundWindow::getDrawSelectStop() const
{
	return sample_fpos_round((sample_fpos_t)loadedSound->channel->getStopPosition()/horzZoomFactor-(sample_fpos_t)horzOffset);
}


// --- cue methods -------------------------------------------------------

const sample_pos_t CSoundWindow::snapPositionToCue(sample_pos_t p) const
{
	const sample_pos_t snapToCueDistance=(sample_pos_t)(gSnapToCueDistance*horzZoomFactor+0.5);
	if(gSnapToCues)
	{
		if(!(QApplication::keyboardModifiers()&Qt::ShiftModifier))
		{ // and shift isn't being held down
			size_t cueIndex;
			sample_pos_t distance;

			const bool found=loadedSound->sound->findNearestCue(p,cueIndex,distance);
			if(found && distance<=snapToCueDistance)
				p=loadedSound->sound->getCueTime(cueIndex);
		}
	}
	return p;
}

const int CSoundWindow::getCueScreenX(size_t cueIndex) const
{
	sample_fpos_t X=((sample_fpos_t)loadedSound->sound->getCueTime(cueIndex)/horzZoomFactor-horzOffset);
	if(X>=(-CUE_RADIUS-1) && X<(wvsa->viewport()->width()+CUE_RADIUS+1)) // ??? not sure about this call to width() it was just "width" in the old code
		return (int)sample_fpos_round(X);
	else
		return CUE_OFF_SCREEN;
}

// --- setting of selection positions methods ------------------------------------------------

const sample_pos_t CSoundWindow::getSamplePosForScreenX(int X) const
{
	sample_fpos_t p=sample_fpos_floor((X+(sample_fpos_t)horzOffset)*horzZoomFactor);
	if(p<0)
		p=0.0;
	else if(p>=loadedSound->sound->getLength())
		p=loadedSound->sound->getLength()-1;

	return (sample_pos_t)p;
}

void CSoundWindow::setSelectStartFromScreen(sample_pos_t X)
{
	sample_pos_t newSelectStart=snapPositionToCue(getSamplePosForScreenX(X));

	// if we selected the right-most pixel on the screen
	// and--if there were a next pixel--and selecting that
	// would select >= sound's length, we make newSelectStart
	// be the sound's length - 1
	// 								??? why X+1?		??? may need to check >=len-1
	if((int)X>=(sample_pos_t)(wvsa->viewport()->width()-1) && (sample_pos_t)((X+(sample_fpos_t)horzOffset+1)*horzZoomFactor)>=loadedSound->sound->getLength())
		newSelectStart=loadedSound->sound->getLength()-1;


	/* not possible
	if(newSelectStart<0)
		newSelectStart=0;
	else */if(newSelectStart>=loadedSound->sound->getLength())
		newSelectStart=loadedSound->sound->getLength()-1;

	lastChangedPosition=lcpStart;
	loadedSound->channel->setStartPosition(newSelectStart);
}

void CSoundWindow::setSelectStopFromScreen(sample_pos_t X)
{
	sample_pos_t newSelectStop=snapPositionToCue(getSamplePosForScreenX(X));

	// if we selected the right-most pixel on the screen
	// and--if there were a next pixel--and selecting that
	// would select >= sound's length, we make newSelectStop
	// be the sound's length - 1
	// 											??? may beed to check >=len-1
	if((int)X>=(wvsa->viewport()->width()-1) && (sample_pos_t)((X+(sample_fpos_t)horzOffset+1)*horzZoomFactor)>=loadedSound->sound->getLength())
		newSelectStop=loadedSound->sound->getLength()-1;

	/* not possible
	if(newSelectStop<0)
		newSelectStop=0;
	else */if(newSelectStop>=loadedSound->sound->getLength())
		newSelectStop=loadedSound->sound->getLength()-1;

	lastChangedPosition=lcpStop;
	loadedSound->channel->setStopPosition(newSelectStop);
}

void CSoundWindow::updateFromSelectionChange(LastChangedPositions _lastChangedPosition)
{
	if(_lastChangedPosition!=lcpNone)
		lastChangedPosition=_lastChangedPosition;

	// ??? couldn't I get rid of the lastChangedPosition and the enum by detecting here which position moved?

	const sample_fpos_t oldDrawSelectStart=getRenderedDrawSelectStart();
	const sample_fpos_t drawSelectStart=getDrawSelectStart();
	const sample_fpos_t oldDrawSelectStop=getRenderedDrawSelectStop();
	const sample_fpos_t drawSelectStop=getDrawSelectStop();

	if(oldDrawSelectStart!=drawSelectStart)
	{
		if(drawSelectStart>oldDrawSelectStart)
			wvsa->viewport()->update((int)oldDrawSelectStart,0,(int)(drawSelectStart-oldDrawSelectStart),wvsa->viewport()->height()); // shortening
		else
			wvsa->viewport()->update((int)drawSelectStart,0,(int)(oldDrawSelectStart-drawSelectStart),wvsa->viewport()->height()); // lengthening
	}
	if(oldDrawSelectStop!=drawSelectStop)
	{
		if(drawSelectStop>oldDrawSelectStop)
			wvsa->viewport()->update((int)oldDrawSelectStop+1,0,(int)(drawSelectStop-oldDrawSelectStop),wvsa->viewport()->height()); // lengthening
		else
			wvsa->viewport()->update((int)drawSelectStop+1,0,(int)(oldDrawSelectStop-drawSelectStop),wvsa->viewport()->height()); // shortening
	}

	updateSelectionStatusInfo();
}

void CSoundWindow::onAutoScroll()
{
	ruler->onAutoScroll();
}

void CSoundWindow::drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible)
{
	int drawPlayStatusX;
	
	// update old position to erase
	drawPlayStatusX=(int)((sample_fpos_t)m_drawPlayPositionAt/horzZoomFactor-horzOffset);
	wvsa->viewport()->update(drawPlayStatusX-1,0,3,wvsa->viewport()->height());
	m_drawPlayPositionAt=MAX_LENGTH;
	
	if(!justErasing)
	{ // update the new position to draw
		m_drawPlayPositionAt=dataPosition;

		drawPlayStatusX=(int)((sample_fpos_t)m_drawPlayPositionAt/horzZoomFactor-horzOffset);
		wvsa->viewport()->update(drawPlayStatusX-1,0,3,wvsa->viewport()->height());


		if(scrollToMakeVisible && (!loadedSound->channel->isPaused() || loadedSound->channel->getSeekSpeed()!=1.0) && wvsa->width()>25)
		{ // scroll the wave view to make the play position visible on the left most side of the screen
			// if the play position is off the screen
			if((drawPlayStatusX<0 || drawPlayStatusX>=wvsa->viewport()->width()))
			{
				wvsa->horizontalScrollBar()->setValue((sample_pos_t)sample_fpos_round((sample_fpos_t)m_drawPlayPositionAt/horzZoomFactor-25));
				wvsa->onHorzScroll();
				drawPlayStatusX=25;
			}
		}

	}
}


// --- slot callbacks


// --- horz zoom stuff ---------

void CSoundWindow::on_horzZoomSlider_valueChanged()
{
	HorzRecenterTypes horzRecenterType=hrtAuto;

	// only regard the keyboard modifier keys if the mouse is bing used to drag the dial
	if(QApplication::mouseButtons()&Qt::LeftButton)
	{
		// depend on keyboard modifier change the way it recenters while zooming
		if(QApplication::keyboardModifiers()&Qt::ShiftModifier)
			horzRecenterType=CSoundWindow::hrtStart;
		else if(QApplication::keyboardModifiers()&Qt::ControlModifier)
			horzRecenterType=CSoundWindow::hrtStop;
	}
		
	setHorzZoom(zoomSliderToZoomFactor(horzZoomSlider->value()),horzRecenterType);

#if 0
	horzZoomSlider->blockSignals(true);
	// ??? for some reason, this does not behave quite like the original (before rewrite), but it is acceptable for now... perhaps look at it later
	setHorzZoom(zoomSliderToZoomFactor(horzZoomSlider->value()),horzRecenterType);
	horzZoomSlider->blockSignals(false);

	horzZoomValueLabel->setText(("  "+istring(horzZoomSlider->value()/(double)ZOOM_MUL,3,1,true)+"%").c_str());
	horzZoomValueLabel->repaint(); // make it update now

	updateAllStatusInfo(); // in case the number of drawn decimal places needs to change
#endif

}

#if 0
void CSoundWindow::updateHorzZoomDial()
{
	horzZoomSlider->setValue(zoomFactorToZoomDial(waveView->getHorzZoom()));
	/* should call slot above
	horzZoomValueLabel->setText(("  "+istring(horzZoomSlider->getValue()/(double)ZOOM_MUL,3,1,true)+"%").c_str());
	*/
}
#endif

// --- vert zoom stuff --------

void CSoundWindow::on_vertZoomSlider_valueChanged()
{
	setVertZoom(vertZoomSlider->value()/(100.0*ZOOM_MUL));
	/*
	vertZoomSlider->blockSignals(true);
	setVertZoom(vertZoomSlider->value()/(100.0*ZOOM_MUL));
	vertZoomSlider->blockSignals(false);
	*/
}

/*
void CSoundWindow::updateVertZoomDial()
{
				// inverse of expr in CSoundWindow::on_vertZoomSlider_valueChanged()
	vertZoomDial->setValue((FXint)(100*ZOOM_MUL*pow((double)getVertZoom(),2.0)));
}
*/

// ----------------------------------------------------------

const map<string,string> CSoundWindow::getPositionalInfo() const
{
	map<string,string> info;
	info["horzZoom"]=istring(getHorzZoom());
	info["vertZoom"]=istring(getVertZoom());
	info["horzOffset"]=istring(getHorzOffset());
	info["vertOffset"]=istring(getVertOffset());
	return info;
}

void CSoundWindow::setPositionalInfo(const map<string,string> _info)
{
	map<string,string> info=_info; /* making copy so ["asdf"] can be called on the map */
	if(!info.empty())
	{
		setHorzZoom(atof(info["horzZoom"].c_str()),hrtNone);
		setVertZoom(atof(info["vertZoom"].c_str()));
		setHorzOffset(atoll(info["horzOffset"].c_str()));
		setVertOffset(atoi(info["vertOffset"].c_str()));

		//updateVertZoomDial();
		//updateHorzZoomDial();
		//updateAllStatusInfo();
	}
}

