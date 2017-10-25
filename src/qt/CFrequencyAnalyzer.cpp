#include "CFrequencyAnalyzer.h"

#include <math.h>

#include <QFrame>
#include <QPainter>
#include <QPaintEvent>
#include <QCursor>
#include <QFont>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "MeterColors.h"

#include "../backend/CSound_defs.h"
#include "../backend/ASoundPlayer.h"
#include "../misc/istring"
#include "settings.h"

#define ANALYZER_BAR_WIDTH 3


/*
 * This class provides a reusable frequency analyzer widget.
 */


class CFrequencyAnalyzerCanvas : public QWidget
{
public:
	CFrequencyAnalyzerCanvas(QWidget *parent) :
		QWidget(parent),
		zoom(1.0),
		drawBarFreq(false)
	{
		// NOTE: the actual fixed width is set when setAnalysis() is called
		setSizePolicy(QSizePolicy::Fixed,QSizePolicy::MinimumExpanding);
		setAutoFillBackground(false);
		setMouseTracking(true);
	}

	virtual ~CFrequencyAnalyzerCanvas()
	{
	}

protected:

	/*override*/ void paintEvent(QPaintEvent *ev)
	{
		QPainter p;
		p.begin(this);

		p.fillRect(0,0,width(),height(),QColor(M_BACKGROUND));

		// if the global setting is disabled, stop drawing right here
		if(!gFrequencyAnalyzerEnabled)
			return;

		// the w and h that we're going to render to (minus some borders and tick marks)
		const size_t canvasWidth=width(); 
		const size_t canvasHeight=height();
		
		const size_t barWidth=analysis.size()>0 ? canvasWidth/analysis.size() : 1;

		// draw vertical octave separator lines
		p.setPen(QColor(M_METER_OFF));
		if(octaveStride>0)
		{
			for(size_t x=0;x<canvasWidth;x+=(barWidth*octaveStride))
				p.drawLine(x,0,x,canvasHeight);
		}

		// ??? also probably want some dB labels 
		// draw 5 horz lines up the panel
		p.setPen(QColor(M_TEXT_COLOR));
		for(size_t t=0;t<4;t++)
		{
			size_t y=((canvasHeight-1)*t/(4-1));
			p.drawLine(0,y,canvasWidth,y);
		}

			
		// draw frequency analysis bars  (or render text if fftw wasn't installed)
		if(analysis.size()>0)
		{
			size_t x=0;
			const size_t drawBarWidth=max(1,(int)barWidth-1);
			for(size_t t=0;t<analysis.size();t++)
			{
				const size_t barHeight=(size_t)floor(analysis[t]*canvasHeight);
				p.fillRect(x+1,canvasHeight-barHeight,drawBarWidth,barHeight,QColor(M_GREEN));
				x+=barWidth;
			}
		}
#ifndef HAVE_LIBRFFTW
		else
		{
			p.setPen(QColor(M_GREEN));
			p.drawText(3,3+12,_("Configure with FFTW"));
			p.drawText(3,20+12,_("for Freq. Analysis"));
		}
#endif


		// draw the peaks
		p.setPen(QColor(M_BRT_YELLOW));
		size_t x=0;
		for(size_t t=0;t<peaks.size();t++)
		{
			const size_t peakHeight=(size_t)floor(peaks[t]*canvasHeight);
			const int y=canvasHeight-peakHeight-1;
			p.drawLine(x+1,y,x+barWidth-1,y);
			x+=barWidth;
		}

		// if mouse is over the canvas, then report what frequency is to be displaye
		if(analysis.size()>0 && drawBarFreq && barFreqIndex<frequencies.size())
		{
			const string f=istring(frequencies[barFreqIndex])+"Hz";
			const int w=QFontMetrics(font()).width(f.c_str(),f.length());
			p.fillRect(0,0,w+3,QFontMetrics(font()).height()+2,QColor(M_METER_OFF));
			p.setPen(QColor(M_TEXT_COLOR));
			p.drawText(1,QFontMetrics(font()).height()-2,f.c_str());
		}

		p.end();
	}


	void enterEvent(QEvent *ev)
	{
		drawBarFreq=true;
		mouseMoveEvent(NULL);
	}

	void leaveEvent(QEvent *ev)
	{
		drawBarFreq=false;
		mouseMoveEvent(NULL);
	}

	void mouseMoveEvent(QMouseEvent *ev)
	{
		barFreqIndex=mapFromGlobal(QCursor::pos()).x()/ANALYZER_BAR_WIDTH;
		update();
	}

private:
	friend class CFrequencyAnalyzer;

	vector<float> analysis;
	vector<float> peaks;
	vector<int> peakFallDelayTimers;
	size_t octaveStride;
	float zoom;

	// used when the mouse is over the canvas to draw what the frequency for the pointed-to bar is
	vector<size_t> frequencies;
	bool drawBarFreq;
	size_t barFreqIndex;

	bool setAnalysis(const vector<float> &_analysis,size_t _octaveStride)
	{
		analysis=_analysis;
		for(size_t t=0;t<analysis.size();t++)
			analysis[t]*=zoom;

		octaveStride=_octaveStride;

		// resize the canvas frame if needed
		int desiredWidth;
		if(analysis.size()>0)
			desiredWidth=(analysis.size()*ANALYZER_BAR_WIDTH);
		else
			desiredWidth=150;  // big enough to render a message about installing fftw

		bool rebuildLabels=false;

		if(width()!=desiredWidth)
		{
			setFixedWidth(desiredWidth);
			rebuildLabels=true;
		}

		
		// rebuild peaks if the number of elements in analysis changed from the last time this was called  (should really only do anything the first time this is called)
		if(peaks.size()!=analysis.size())
		{
			peaks.clear();
			for(size_t t=0;t<analysis.size();t++)
			{
				peaks.push_back(0.0f);
				peakFallDelayTimers.push_back(0);
			}
			rebuildLabels=true;
		}

		// make peaks fall
		for(size_t t=0;t<peaks.size();t++)
		{
			peakFallDelayTimers[t]=max(0,peakFallDelayTimers[t]-1);
			if(peakFallDelayTimers[t]==0) // only decrease the peak when the fall timer has reached zero
				peaks[t]=max(0.0f,(float)(peaks[t]-gAnalyzerPeakFallRate));
		}


		// update peaks if there is a new max
		for(size_t t=0;t<analysis.size();t++)
		{
			if(peaks[t]<analysis[t])
			{
				peaks[t]=analysis[t];
				peakFallDelayTimers[t]=gAnalyzerPeakFallDelayTime/gMeterUpdateTime;
			}
		}
		
		update();
		return rebuildLabels;
	}

};


CFrequencyAnalyzer::CFrequencyAnalyzer(QWidget *parent) :
	QWidget(parent),

	m_timerId(-1),
	m_soundPlayer(NULL),

	canvas(NULL),
	zoomDial(NULL)
{
	setSizePolicy(QSizePolicy::Fixed,QSizePolicy::MinimumExpanding);

	// adjust the font to use for labels
	QFont statusFont(font());
	statusFont.setPointSizeF(6.1);
	statusFont.setBold(false);
	setFont(statusFont);

	setLayout(new QHBoxLayout);
	layout()->setMargin(2);
	layout()->setSpacing(2);

	// create zoom dial
	layout()->addWidget(zoomDial=new QSlider(this));
	zoomDial->setOrientation(Qt::Vertical);
	zoomDial->setRange(1,200);
	zoomDial->setValue(25);
	zoomDial->setToolTip(_("Adjust Zoom Factor for Analyzer\nRight-click for Default"));
	zoomDial->installEventFilter(this);
	connect(zoomDial,SIGNAL(valueChanged(int)),this,SLOT(onZoomDial(int)));

	QWidget *right=new QWidget(this);

	// set palette so text labels will be same colors as canvas uses
	QPalette p(right->palette());
	p.setColor(QPalette::Window,QColor(M_BACKGROUND));
	p.setColor(QPalette::WindowText,QColor(M_TEXT_COLOR));
	right->setPalette(p); // to be inherited by children
	right->setAutoFillBackground(true);

	right->setLayout(new QVBoxLayout);
	right->layout()->setMargin(0);
	right->layout()->setSpacing(0);
	right->layout()->addWidget(canvas=new CFrequencyAnalyzerCanvas(this));
	right->layout()->addWidget(labelsFrame=new QWidget(this));

		// add frame around freq analyzer
		QFrame *frame=new QFrame;
		frame->setFrameShape(QFrame::Panel);
		frame->setFrameShadow(QFrame::Sunken);
		frame->setLineWidth(2);
		frame->setLayout(new QHBoxLayout);
		frame->layout()->setMargin(0);
		frame->layout()->addWidget(right);
		//labels aren't added yet frame->setFixedWidth(frame->minimumSizeHint().width());

	layout()->addWidget(frame);

	canvas->zoom=zoomDial->value();
}

CFrequencyAnalyzer::~CFrequencyAnalyzer()
{
	delete canvas;
}

void CFrequencyAnalyzer::setSoundPlayer(ASoundPlayer *soundPlayer)
{
	m_soundPlayer=soundPlayer;

	// stop timer
	if(m_timerId>=0)
		killTimer(m_timerId);
	m_timerId=-1;

	if(!m_soundPlayer)
		return;

	m_timerId=startTimer(gMeterUpdateTime);
}

bool CFrequencyAnalyzer::eventFilter(QObject *watched, QEvent *ev)
{
	if(watched==zoomDial && ev->type()==QEvent::MouseButtonRelease && ((QMouseEvent *)ev)->button()==Qt::RightButton)
		zoomDial->setValue(25);
	return false;
}

void CFrequencyAnalyzer::timerEvent(QTimerEvent *e)
{
	if(
		e->timerId()==m_timerId && 
		m_soundPlayer!=NULL && 
		m_soundPlayer->isInitialized()
	)
	{
		if(gFrequencyAnalyzerEnabled)
		{
			setFixedWidth(minimumSizeHint().width()); // ??? is this killing CPU because layout is happening all the tme?
			if(canvas->setAnalysis(m_soundPlayer->getFrequencyAnalysis(),m_soundPlayer->getFrequencyAnalysisOctaveStride()))
			{
				const vector<float> a=m_soundPlayer->getFrequencyAnalysis();
				vector<size_t> frequencies;
				for(size_t t=0;t<a.size();t++)
					frequencies.push_back(m_soundPlayer->getFrequency(t));
				rebuildLabels(frequencies,m_soundPlayer->getFrequencyAnalysisOctaveStride());
			}
		}
	}
}

void CFrequencyAnalyzer::onZoomDial(int val)
{
	canvas->zoom=val;
	canvas->update();
}

void CFrequencyAnalyzer::rebuildLabels(const vector<size_t> _frequencies,size_t octaveStride)
{
	canvas->frequencies=_frequencies;

	// delete all meters
	while(labelsFrame->children().size()>0)
		delete labelsFrame->children()[0];
	labelsFrame->setLayout(new QHBoxLayout);
	labelsFrame->layout()->setMargin(0);
	labelsFrame->layout()->setSpacing(0);

	labelsFrame->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);

	for(size_t t=0;t<canvas->analysis.size()/octaveStride;t++)
	{
		string text;
		if(canvas->frequencies[t*octaveStride]>=1000)
			text=istring(canvas->frequencies[t*octaveStride]/1000.0,1,1)+"k";
		else
			text=istring(canvas->frequencies[t*octaveStride]);

		QLabel *l=new QLabel(this);

		// ??? should NOT be necessary, but for some reason it is.. bug? (also in CLevelMeters.h)
		QPalette p(l->palette());
		p.setColor(QPalette::Window,QColor(M_BACKGROUND));
		p.setColor(QPalette::WindowText,QColor(M_TEXT_COLOR));
		l->setPalette(p);

		l->setText(text.c_str());
		if(t!=((canvas->analysis.size()/octaveStride)-1))
			l->setFixedWidth(ANALYZER_BAR_WIDTH*octaveStride);
		else
			l->setFixedWidth((ANALYZER_BAR_WIDTH*octaveStride)+(ANALYZER_BAR_WIDTH*(canvas->analysis.size()%octaveStride))); // make the last one the remaining width
		labelsFrame->layout()->addWidget(l);
	}
}

