#include "CLevelMeters.h"

#include <vector>
using namespace std;

#include <QFrame>
#include <QPainter>
#include <QPaintEvent>
#include <QGridLayout>

#include "MeterColors.h"

#include "../backend/CSound_defs.h"
#include "../backend/unit_conv.h"
#include "../backend/ASoundPlayer.h"
#include "../backend/ASoundRecorder.h"
#include "settings.h"


/*
 * This class provides a reusable level meter widget.
 * It has to be given the number of channels to measure and must be supplied with the new information on some regular interval.
 */


//#define NUM_LEVEL_METER_TICKS 17
#define NUM_BALANCE_METER_TICKS 9

// ??? move to settings.h eventually
/*
static bool gLevelMetersEnabled=true;
static int gMaxPeakFallRate=1;
static int gMaxPeakFallDelayTime=1;
static int gMeterUpdateTime=1;
*/

class CLevelMeter : public QWidget
{
public:
	QLabel *grandMaxPeakLevelLabel;

	CLevelMeter(QWidget *parent,int *_pLevelMeterTickCount) :
		QWidget(parent),

		grandMaxPeakLevelLabel(NULL),

		RMSLevel(-1),
		peakLevel(-1),
		maxPeakLevel(-1),
		grandMaxPeakLevel(-1),
		maxPeakFallTimer(0),

		pLevelMeterTickCount(_pLevelMeterTickCount)
	{
		setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
		setFixedHeight(sizeHint().height());
	}

	virtual ~CLevelMeter()
	{
	}

	void setLevel(sample_t _RMSLevel, sample_t _peakLevel)
	{
		RMSLevel=_RMSLevel;
		peakLevel=_peakLevel;

                // start decreasing the max peak level after maxPeakFallTimer falls below zero
		if((--maxPeakFallTimer)<0)
		{
			maxPeakLevel=maxPeakLevel-(sample_t)(MAX_SAMPLE*gMaxPeakFallRate);
			maxPeakLevel=maxPeakLevel<0 ? 0 : maxPeakLevel;
			maxPeakFallTimer=0;
		}

		// if the peak level is >= the maxPeakLevel then set a new maxPeakLevel and reset the peak file timer
		if(peakLevel>=maxPeakLevel)
		{
			maxPeakLevel=peakLevel;
			maxPeakFallTimer=gMaxPeakFallDelayTime/gMeterUpdateTime;
		}

		if(peakLevel>grandMaxPeakLevel)
			setGrandMaxPeakLevel(peakLevel); // sets the label and everything

		update();
	}

	sample_t getRMSLevel() const
	{
		return RMSLevel;
	}

	sample_t getPeakLevel() const
	{
		return peakLevel;
	}

	const string getGrandMaxPeakLevelString(const sample_t maxPeakLevel) const
	{
		return (istring(amp_to_dBFS(maxPeakLevel),3,1,false)+" dBFS");
	}

        void setGrandMaxPeakLevel(const sample_t maxPeakLevel)
	{
		grandMaxPeakLevel=maxPeakLevel;
		if(grandMaxPeakLevelLabel)
			grandMaxPeakLevelLabel->setText(getGrandMaxPeakLevelString(maxPeakLevel).c_str());
	}
										

protected:

	/*override*/ QSize sizeHint() const
	{
		return QSize(0,20);
	}

	/*override*/ void paintEvent(QPaintEvent *e)
	{
		const int width=this->width();
		const int height=this->height();

		QPainter p;
		p.begin(this);

		// draw *pLevelMeterTickCount tick marks above level indication
		p.fillRect(0,0,width,2,M_BACKGROUND);
		p.setPen(M_TEXT_COLOR);
		for(int t=0;t<(*pLevelMeterTickCount);t++)
		{
			const int x=(width-1)*t/((*pLevelMeterTickCount)-1);
			p.drawLine(x,0,x,1);
		}

		// draw horz line below level indication
		p.setPen(M_TEXT_COLOR);
		p.drawLine(0,height-1,width,height-1);

		// draw gray background underneath the stippled level indication
		p.fillRect(0,2,width,height-3,M_METER_OFF);


		// if the global setting is disabled, stop drawing right here
		if(!gLevelMetersEnabled)
			return ;

		// draw RMS level indication
		int x=(int)(RMSLevel*width/MAX_SAMPLE);
		QBrush b;
		b.setStyle(Qt::Dense4Pattern);
		if(x>(width*3/4))
		{
		// ??? this really ought to change colors at certain dB
			b.setColor(M_GREEN); // green
			p.fillRect(0,2,width/2,height-3,b);
			b.setColor(M_YELLOW); // yellow
			p.fillRect(width/2,2,width/4,height-3,b);
			b.setColor(M_RED); // red
			p.fillRect(width*3/4,2,x-(width*3/4),height-3,b);
		}
		else if(x>(width/2))
		{
			b.setColor(M_GREEN); // green
			p.fillRect(0,2,width/2,height-3,b);
			b.setColor(M_YELLOW); // yellow
			p.fillRect(width/2,2,x-width/2,height-3,b);
		}
		else
		{
			b.setColor(M_GREEN); // green
			p.fillRect(0,2,x,height-3,b);
		}

		// draw peak indication
		int y=height/2;
		x=(int)(peakLevel*width/MAX_SAMPLE);
		if(x>(width*3/4))
		{
			p.fillRect(0,y,width/2,2,QColor(M_GREEN));
			p.fillRect(width/2,y,width/4,2,QColor(M_YELLOW));
			p.fillRect(width*3/4,y,x-(width*3/4),2,QColor(M_RED));
		}
		else if(x>(width/2))
		{
			p.fillRect(0,y,width/2,2,QColor(M_GREEN));
			p.fillRect(width/2,y,x-width/2,2,QColor(M_YELLOW));
		}
		else
		{
			p.fillRect(0,y,x,2,QColor(M_GREEN));
		}

		// draw the max peak indicator
		x=(int)(maxPeakLevel*width/MAX_SAMPLE);
		QRgb maxPeakColor;
		if(x>(width*3/4))
			maxPeakColor=M_BRT_RED;
		else if(x>(width/2))
			maxPeakColor=M_BRT_YELLOW;
		else
			maxPeakColor=M_BRT_GREEN;
		p.fillRect(x-1,2,2,height-3,QColor(maxPeakColor));

		p.end();
	}

	mix_sample_t RMSLevel,peakLevel,maxPeakLevel,grandMaxPeakLevel;
	int maxPeakFallTimer;
	
	int *pLevelMeterTickCount;
};

class CBalanceMeter : public QWidget
{
public:
	CBalanceMeter(QWidget *parent,CLevelMeter *_left,CLevelMeter *_right) :
		QWidget(parent),
		left(_left),
		right(_right)//,
		//m_RMSBalance(0),
		//m_PeakLevel(0),
	{
		setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
		setFixedHeight(sizeHint().height());
	}

	virtual ~CBalanceMeter()
	{
	}

	/*
	void setBalance(sample_t leftRMSLevel,sample_t rightRMSLevel,sample_t leftPeakLevel,sample_t rightPeakLevel)
	{
		m_RMSBalance=((float)rightRMSLevel-(float)leftRMSLevel)/MAX_SAMPLE;
		m_peakBalance=((float)rightPeakLevel-(float)leftPeakLevel)/MAX_SAMPLE;
		update(); // flag for repainting
	}
	*/

protected:

	/*override*/ void paintEvent(QPaintEvent *e)
	{
		const int width=this->width();
		const int height=this->height();

		QPainter p;
		p.begin(this);

		// draw NUM_BALANCE_METER_TICKS tick marks above level indication
		p.fillRect(0,0,width,2,M_BACKGROUND);
		p.setPen(M_TEXT_COLOR);
		for(int t=0;t<NUM_BALANCE_METER_TICKS;t++)
		{
			if(t==4)
				continue; // skip middle tick that doesn't always line up with middle line:w

			const int x=(width-1)*t/(NUM_BALANCE_METER_TICKS-1);
			p.drawLine(x,0,x,1);
		}

		// draw horz line below level indication
		p.setPen(M_TEXT_COLOR);
		p.drawLine(0,height-1,width,height-1);

		// draw gray background underneath the stippled level indication
		p.fillRect(0,2,width,height-3,M_METER_OFF);


		// if the global setting is disabled, stop drawing right here
		if(!gLevelMetersEnabled)
			return;

		const float RMSBalance=((float)right->getRMSLevel()-(float)left->getRMSLevel())/MAX_SAMPLE;
		const float peakBalance=((float)right->getPeakLevel()-(float)left->getPeakLevel())/MAX_SAMPLE;

		// draw RMS balance indication
		int x=(int)((RMSBalance*width)/2);
		QBrush b;
		b.setStyle(Qt::Dense4Pattern);
		b.setColor(M_RMS_BALANCE_COLOR);
		if(x>0)
			p.fillRect(width/2,2,x,height-3,QColor(M_RMS_BALANCE_COLOR));
		else // drawing has to be done a little differently when x is negative
			p.fillRect(width/2+x,2,-x,height-3,QColor(M_RMS_BALANCE_COLOR));


		// draw the peak balance indicator
		int y=height/2;
		x=(int)((peakBalance*width)/2);
		if(x>0)
			p.fillRect(width/2,y,x,2,QColor(M_PEAK_BALANCE_COLOR));
		else // drawing has to be done a little differently when x is negative
			p.fillRect(width/2+x,y,-x,2,QColor(M_PEAK_BALANCE_COLOR));

		// draw line underneath
		p.setPen(QColor(M_TEXT_COLOR));
		p.drawLine(width/2,2,width/2,height-1);

		p.end();
	}

	/*override*/ QSize sizeHint() const
	{
		return QSize(0,20);
	}

private:
	CLevelMeter *left,*right;
	/*
	sample_t m_RMSBalance;
	sample_t m_PeakLevel;
	*/
};

class CLabelLayout : public QLayout
{
public:
	CLabelLayout(QWidget *parent)
		: QLayout(parent)
	{
		setContentsMargins(0,0,0,0);
		setSpacing(0);
	}

	
	~CLabelLayout()
	{
		QLayoutItem *item;
		while ((item = takeAt(0)))
			delete item;
	}

	void addItem(QLayoutItem *item)
	{
		itemList.append(item);
	}

	int count() const
	{
		return itemList.size();
	}

	QLayoutItem *itemAt(int index) const
	{
		return itemList.value(index);
	}

	QLayoutItem *takeAt(int index)
	{
		if (index >= 0 && index < itemList.size())
			return itemList.takeAt(index);
		else
			return 0;
	}

	void setGeometry(const QRect &rect)
	{
//printf("hello.. %d,%d\n",contentsRect().width(),contentsRect().height());
		QLayout::setGeometry(rect);

		// re-layout labels
		for(int t=0;t<itemList.size();t++)
		{
			const QRect cr=contentsRect();

			QLayoutItem *child=itemList[t];
			const QSize ms=child->minimumSize();

			const int x=(cr.width()-1)*t/(itemList.size()-1);

			int w= t==0 ? 0 : ms.width();
			if(t!=itemList.size()-1)
				w/=2;

			child->setGeometry(QRect(
				x-w,
				cr.height()-ms.height(),
				ms.width(),
				ms.height()
			));

//printf("child moved to: %d,%d  %d,%d\n",child->geometry().x(),child->geometry().y(), child->geometry().width(),child->geometry().height());
		}
	}

	QSize sizeHint() const
	{
		return minimumSize();
	}

	QSize minimumSize() const
	{
		if(itemList.count()>0)
			return itemList[0]->minimumSize();
		else
			return QSize(0,0);
	}

private:
	int doLayout(const QRect &rect) const;
	QList<QLayoutItem *> itemList;
};

#if 0
#warning try creating a QLayout subclass instead of a QWidget subclass.. see http://doc.trolltech.com/4.3/layouts-flowlayout-flowlayout-cpp.html for example
class QLabelContainer : public QWidget
{
public:
	QLabelContainer(QWidget *parent) :
		QWidget(parent)
	{
		setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
	}

	virtual ~QLabelContainer()
	{
	}

	/*override*/ QSize sizeHint() const
	{
		//printf("%s: %d\n",__func__,((QWidget *)children()[0])->minimumSizeHint().height());
		return ((QWidget *)children()[0])->minimumSizeHint();
	}


//protected:
	//void resizeEvent(QResizeEvent *)
	void setGeometry(const QRect &r)
	{
printf("heloo\n");
		QWidget::setGeometry(r);

		// re-layout labels
		for(int t=0;t<children().size();t++)
		{
			QWidget *child=(QWidget *)children()[t];

			const int x=(width()-1)*t/(children().size()-1);

			int w= t==0 ? 0 : child->minimumSizeHint().width();
			if(t!=children().size()-1)
				w/=2;

			child->move(x-w,height()-child->minimumSizeHint().height());
		}
	}
};
#endif

CLevelMeters::CLevelMeters(QWidget *parent) :
	QWidget(parent),

	levelMeterTickCount(0),
	labelContainer(NULL),

	m_timerId(-1)
{
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	QPalette pal(palette());
	pal.setColor(QPalette::Active,QPalette::Window,M_BACKGROUND);
	pal.setColor(QPalette::Active,QPalette::WindowText,M_TEXT_COLOR);
	pal.setColor(QPalette::Inactive,QPalette::Window,M_BACKGROUND);
	pal.setColor(QPalette::Inactive,QPalette::WindowText,M_TEXT_COLOR);
	setPalette(pal);

	// this should be set so that the whole background is set to M_BACKGROUND before child widgets begin to draw
	setAutoFillBackground(true);
}

CLevelMeters::~CLevelMeters()
{
}

bool CLevelMeters::eventFilter(QObject *watched, QEvent *event)
{
	QMouseEvent *me=dynamic_cast<QMouseEvent *>(event);
	if(watched==grandMaxPeakLevelLabel && me && me->type()==QEvent::MouseButtonRelease)
		resetGrandMaxPeakLevels();

	if(watched==labelContainer && event->type()==QEvent::Resize)
		recreateLevelLabels(((QResizeEvent *)event)->size().width());

	return false;
}

void CLevelMeters::resetGrandMaxPeakLevels()
{
	for(size_t t=0;t<m_levelMeters.size();t++)
		m_levelMeters[t]->setGrandMaxPeakLevel(0);
}

void CLevelMeters::initialize(unsigned channelCount,bool justCleanup)
{
	// delete all meters
	qDeleteAll(children());
	m_levelMeters.clear();
	m_balanceMeters.clear();

	// stop timer
	if(m_timerId>=0)
		killTimer(m_timerId);
	m_timerId=-1;

	if(justCleanup)
		return;

	setLayout(new QHBoxLayout);
	layout()->setMargin(0);

	// add frame around phase meter
	QFrame *frame=new QFrame(this);
	frame->setFrameShape(QFrame::Panel);
	frame->setFrameShadow(QFrame::Sunken);
	frame->setLineWidth(2);

	layout()->addWidget(frame);

	int row=0;
	QGridLayout *gridLayout=new QGridLayout;
	frame->setLayout(gridLayout);
	gridLayout->setMargin(4);
	gridLayout->setSpacing(2);

	// create labels to db units
	{
		// create the horizontal set of labels
		labelContainer=new QWidget(this);//QLabelContainer(this);
		gridLayout->addWidget(labelContainer,row,0);
		labelContainer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
		labelContainer->setAutoFillBackground(false);
		labelContainer->installEventFilter(this);
		//recreateLevelLabels();

		// create the clickable "max" heading
		QLabel *l=new QLabel(frame);
		l->setPalette(palette());
		l->setText("max");
		l->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		//l->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
		gridLayout->addWidget(l,row,1);
		l->installEventFilter(this);
		grandMaxPeakLevelLabel=l;
	}
	row++;

	// create level meters and balance meters
	for(unsigned t=0;t<channelCount;t++)
	{
		// create the level meter
		CLevelMeter *lm=new CLevelMeter(frame,&levelMeterTickCount);
		m_levelMeters.push_back(lm);

		gridLayout->addWidget(lm,row,0);
		
		// create the grand max level indicator
		QLabel *l=new QLabel(frame);
		l->setPalette(palette());
		// set a fixed width which reduces CPU since it doesn't have to layout every time the text changes
		l->setFixedWidth(l->fontMetrics().width(lm->getGrandMaxPeakLevelString(dBFS_to_amp(-100)).c_str()));
		l->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		gridLayout->addWidget(l,row,1);
		lm->grandMaxPeakLevelLabel=l;

		lm->setLevel(0,0);

		row++;

		// add a balance meter for every 2 level meters
		if((t%2)==1)
		{
			QLabel *l;
			QWidget *c=new QWidget(frame);
			c->setLayout(new QHBoxLayout);
			c->layout()->setMargin(0);
			c->layout()->setSpacing(2);

			l=new QLabel(c);
			l->setPalette(palette());
			l->setText("-1.0");
			c->layout()->addWidget(l);

			CBalanceMeter *bm=new CBalanceMeter(c,m_levelMeters[t-1],m_levelMeters[t]);
			m_balanceMeters.push_back(bm);
			c->layout()->addWidget(bm);

			l=new QLabel(c);
			l->setPalette(palette());
			l->setText("+1.0");
			c->layout()->addWidget(l);

			gridLayout->addWidget(c,row,0);

			row++;
		}

	}

	gridLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Fixed,QSizePolicy::Expanding),row,0);
	row++;

	m_timerId=startTimer(gMeterUpdateTime);
}


void CLevelMeters::timerEvent(QTimerEvent *e)
{
	if(e->timerId()==m_timerId && m_levelMeters.size()>0)
		updateMeters();
}


/*
void CLevelMeters::resizeEvent(QResizeEvent *e)
{
	QWidget::resizeEvent(e);
	recreateLevelLabels();
}
*/
/*
void CLevelMeters::setGeometry(const QRect &r)
{
printf("hello from CLevelMeters::setGeometry\n");
	QWidget::setGeometry(r);
	recreateLevelLabels();
}
*/

bool CLevelMeters::recreateLevelLabels(int containerWidth)
{
	QFontMetrics m(labelContainer->font());
	int labelWidth=qMax(1,m.width("-100.0"));
	int nLabels=qMax(2,containerWidth/labelWidth*7/8); // calculate how many labels need to be shown

printf("nLabels: %d  width: %d\n",nLabels,containerWidth);
	if(nLabels==levelMeterTickCount)
		return false; // no need to do anything
	levelMeterTickCount=nLabels;

	qDeleteAll(labelContainer->children());
	//labelContainer->setLayout(new CLabelLayout(labelContainer));
	labelContainer->setLayout(new QHBoxLayout(labelContainer));

	#define MAKE_DB_LABEL(text) { QLabel *l=new QLabel; l->setAutoFillBackground(false); l->setText(text); /*should NOT be necessary, but for some reason is necessary */l->setPalette(palette()); l->show(); labelContainer->layout()->addWidget(l); }
	MAKE_DB_LABEL("dBFS") // create the -infinity label as just the units label
	for(int t=1;t<levelMeterTickCount;t++)
	{
		istring s;
		//if(t>levelMeterTickCount/2)
			s=istring(round(scalar_to_dB((double)t/(levelMeterTickCount-1))*10)/10,3,1); // round to nearest tenth
		//else
		//	s=istring(round(scalar_to_dB((double)t/(levelMeterTickCount-1))),3,1); // round to nearest int

		if(s.rfind(".0")!=istring::npos) // remove .0's from the right side
			s.erase(s.length()-2,2);
		MAKE_DB_LABEL(s.c_str())
	}

	//labelContainer->resizeEvent(NULL);
	//labelContainer->layout()->update();
	return true;
}



CPlaybackLevelMeters::CPlaybackLevelMeters(QWidget *parent) :
	CLevelMeters(parent),
	m_soundPlayer(0)
{
}

CPlaybackLevelMeters::~CPlaybackLevelMeters()
{
}

void CPlaybackLevelMeters::setSoundPlayer(ASoundPlayer *soundPlayer)
{
	m_soundPlayer=soundPlayer;
	if(m_soundPlayer)
		initialize(m_soundPlayer->devices[0].channelCount,false);
	else
		initialize(0,true);
}

void CPlaybackLevelMeters::updateMeters()
{
	if(
		m_soundPlayer && 
		m_soundPlayer->isInitialized()
	)
	{
		if(gLevelMetersEnabled)
		{
			// set the level for all the level meters
			for(size_t t=0;t<m_levelMeters.size();t++)
				m_levelMeters[t]->setLevel(m_soundPlayer->getRMSLevel(t),m_soundPlayer->getPeakLevel(t));

			// make the balance meters redraw
			for(size_t t=0;t<m_balanceMeters.size();t++)
				m_balanceMeters[t]->update();
		}

		/* handled elsewhere now
		if(gStereoPhaseMetersEnabled)
		{
			m_soundPlayer->getSamplingForStereoPhaseMeters(samplingForStereoPhaseMeters,samplingForStereoPhaseMeters.getSize());
			for(size_t t=0;t<m_stereoPhaseMeters.size();t++)
				m_stereoPhaseMeters[t]->updateCanvas();
		}
		*/

		/* handled elsewhere now
		if(gFrequencyAnalyzerEnabled)
		{
			if(m_analyzer->setAnalysis(m_soundPlayer->getFrequencyAnalysis(),m_soundPlayer->getFrequencyAnalysisOctaveStride()))
			{
				const vector<float> a=m_soundPlayer->getFrequencyAnalysis();
				vector<size_t> frequencies;
				for(size_t t=0;t<a.size();t++)
					frequencies.push_back(m_soundPlayer->getFrequency(t));
				m_analyzer->rebuildLabels(frequencies,m_soundPlayer->getFrequencyAnalysisOctaveStride());
			}
		}
		*/
	}
}


CRecordLevelMeters::CRecordLevelMeters(QWidget *parent) :
	CLevelMeters(parent),
	m_soundRecorder(0)
{
}

CRecordLevelMeters::~CRecordLevelMeters()
{
}

void CRecordLevelMeters::setSoundRecorder(ASoundRecorder *soundRecorder)
{
	m_soundRecorder=soundRecorder;
	if(m_soundRecorder)
		initialize(m_soundRecorder->getChannelCount(),false);
	else
		initialize(0,true);
}

void CRecordLevelMeters::updateMeters()
{
	if(
		m_soundRecorder /*&& 
		m_soundRecorder->isInitialized()*/
	)
	{
		if(gLevelMetersEnabled)
		{
			// set the level for all the level meters
			for(size_t t=0;t<m_levelMeters.size();t++)
				m_levelMeters[t]->setLevel(m_soundRecorder->getAndResetLastPeakValue(t),0);

			// make the balance meters redraw
			for(size_t t=0;t<m_balanceMeters.size();t++)
				m_balanceMeters[t]->update();
		}
	}
}

