#include "StereoPhaseMeters.h"

#include <math.h>

#include <QFrame>
#include <QPainter>
#include <QPaintEvent>
#include <QHBoxLayout>
#include <QImage>

#include "MeterColors.h"

#include "settings.h"

#include "../backend/ASoundPlayer.h"

/*
 * This class provides a reusable stereo phase meter widget.
 * It has to be given the number of channels to measure and must be supplied with the new information on some regular interval.
 */


// ??? move to settings.h eventually
/*
static bool gStereoPhaseMetersEnabled=true;
*/

class StereoPhaseMeter : public QWidget
{
public:
	StereoPhaseMeter(sample_t *_samplingBuffer,size_t _samplingNFrames,unsigned _samplingNChannels,unsigned _samplingLeftChannel,unsigned _samplingRightChannel) :
		QWidget(),
		qimage(NULL),
		m_samplingBuffer(_samplingBuffer),
		m_samplingNFrames(_samplingNFrames),
		m_samplingNChannels(_samplingNChannels),
		m_samplingLeftChannel(_samplingLeftChannel),
		m_samplingRightChannel(_samplingRightChannel),

		m_clear(true),

		m_zoom(1)
	{
		setFixedSize(QSize(100,100));

		m_pixelBuffer.resize(width()*height()*4);
		qimage=new QImage(m_pixelBuffer.data(),width(),height(),QImage::Format_RGB32);
	}

	virtual ~StereoPhaseMeter()
	{
		delete qimage;
	}

	void clearCanvas()
	{
		m_clear=true;
	}

	void tick() // this should be called once per timer tick
	{
		const int width=this->width();
		const int height=this->height();

		QRgb *data=(QRgb *)m_pixelBuffer.data();

		if(m_clear)
		{
			memset(data,0,m_pixelBuffer.size());
			m_clear=false;
		}

		// if the global setting is disabled, stop drawing right here
		if(!gStereoPhaseMetersEnabled)
			return;

		// fade previous frame (so we get a ghosting history)
		for(int t=0;t<width*height;t++)
		{
			const QRgb c=data[t];
			data[t]= c==0 ? 0 : qRgb( qRed(c)*7/8, qGreen(c)*7/8, qBlue(c)*7/8 );
		}

		// draw the axies
		for(int t=0;t<width;t++)
			data[t+(width*width/2)]=M_PHASE_METER_AXIS; // horz
		for(int t=0;t<height;t++)
			data[(t*width)+width/2]=M_PHASE_METER_AXIS; // vert

		// draw the points
		for(size_t t=0;t<m_samplingNFrames;t++)
		{
			// let x and y be the normalized (1.0) sample values (x:right y:left) then scaled up to the canvas width/height and centered in the square
			const int x=(int)( (m_zoom*m_samplingBuffer[t*m_samplingNChannels+m_samplingRightChannel])*width/2/MAX_SAMPLE) + width/2;
			const int y=(int)(-(m_zoom*m_samplingBuffer[t*m_samplingNChannels+m_samplingLeftChannel ])*height/2/MAX_SAMPLE) + height/2; // negation because increasing values go down on the screen which is up-side-down from the Cartesian plane

			if(x>=0 && x<width && y>=0 && y<height)
			{
				/*
				if(gStereoPhaseMeterUnrotate)
					data[unrotateMapping[y*width+x]]=M_BRT_GREEN;
				else
				*/
					data[y*width+x]=M_BRT_GREEN;
			}
		}
	}

	void setZoom(float zoom)
	{
		m_zoom=zoom;
	}


protected:
	/*override*/ void paintEvent(QPaintEvent *e)
	{
		QPainter p;
		p.begin(this);

		p.drawImage(0,0,*qimage);

		p.end();
	}

private:
	QImage *qimage;
	vector<uchar> m_pixelBuffer;
	const sample_t *m_samplingBuffer;
	const size_t m_samplingNFrames;
	const unsigned m_samplingNChannels;
	const unsigned m_samplingLeftChannel;
	const unsigned m_samplingRightChannel;

	bool m_clear;

	float m_zoom;

	#if 0  // I think this could be defined in the StereoPhaseMeters window instead of in each stereo meter
	vector<int> unrotateMapping; // width*height number of pixels mapping

	void recalcRotateLookup()
	{
		const int width=width();
		const int height=height();

		unrotateMapping.resize(width*height);

		const double ang=-M_PI_4; // -45 degrees

		for(int sx=0;sx<width;sx++)
		for(int sy=0;sy<height;sy++)
		{
			double wx=sx-width/2;
			double wy=sy-height/2;

			// shrink square so that the corners aren't cut off when rotated 
			// (this also fixes the gaps between pixels if I don't shrink it)
			wx*=(sqrt(2.0)/2.0); 
			wy*=(sqrt(2.0)/2.0);

			double rot_wx=wx*cos(ang)-wy*sin(ang);
			double rot_wy=wx*sin(ang)+wy*cos(ang);

			rot_wx+=width/2;
			rot_wy+=height/2;

			rot_wx=round(rot_wx);
			rot_wy=round(rot_wy);
			int offset=(int)(rot_wy*width+rot_wx);
			if((rot_wx>=0 && rot_wx<width && rot_wy>=0 && rot_wy<height) && (offset>=0 && offset<(width*height)))
				unrotateMapping[sy*width+sx]=offset;
			else
				unrotateMapping[sy*width+sx]=0;
		}
		
	}
	#endif
};

StereoPhaseMeters::StereoPhaseMeters(QWidget *parent) :
	QWidget(parent),
	m_timerId(-1),
	m_soundPlayer(NULL)
{
	//setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
	setAutoFillBackground(false);
}

StereoPhaseMeters::~StereoPhaseMeters()
{
}

void StereoPhaseMeters::setSoundPlayer(ASoundPlayer *soundPlayer)
{
	m_soundPlayer=soundPlayer;

	// delete all meters
	while(children().size()>0)
		delete children()[0];
	m_stereoPhaseMeters.clear();

	// stop timer
	if(m_timerId>=0)
		killTimer(m_timerId);
	m_timerId=-1;

	if(!m_soundPlayer)
		return;

	int row=0;
	setLayout(new QHBoxLayout); // could re-orient depending on where it's docked top/bottom or left/right ???
	layout()->setContentsMargins(2,2,2,2);
	layout()->setSpacing(2);

	// create zoom dial
	layout()->addWidget(m_zoomDial=new QSlider(this));
	m_zoomDial->setOrientation(Qt::Vertical);
	m_zoomDial->setRange(1,400);
	m_zoomDial->setValue(100);
	m_zoomDial->setToolTip(_("Adjust Zoom Factor for Stereo Phase Meter\nAll the way down means no zooming"));
	m_zoomDial->installEventFilter(this);
	connect(m_zoomDial,SIGNAL(valueChanged(int)),this,SLOT(onZoomDial(int)));

	// create stereo phase meters
	m_samplingForStereoPhaseMeters.resize(gStereoPhaseMeterPointCount*soundPlayer->devices[0].channelCount);
	for(unsigned t=0;t<soundPlayer->devices[0].channelCount/2;t++)
	{
		// create the level meter
		StereoPhaseMeter *spm=new StereoPhaseMeter(
			m_samplingForStereoPhaseMeters.data(),
			gStereoPhaseMeterPointCount,
			soundPlayer->devices[0].channelCount,
			t*2+0,
			t*2+1
		);
		spm->setZoom(((float)m_zoomDial->value())/100.0f);
		m_stereoPhaseMeters.push_back(spm);

			// add frame around phase meter
			QFrame *frame=new QFrame;
			frame->setFrameShape(QFrame::Panel);
			frame->setFrameShadow(QFrame::Sunken);
			frame->setLineWidth(2);
			frame->setLayout(new QHBoxLayout);
			frame->layout()->setContentsMargins(0,0,0,0);
			frame->layout()->addWidget(spm);
			frame->setFixedSize(frame->minimumSizeHint());

		layout()->addWidget(frame);
	}


	m_timerId=startTimer(gMeterUpdateTime);

	setFixedWidth(minimumSizeHint().width());
}

bool StereoPhaseMeters::eventFilter(QObject *watched, QEvent *ev)
{
	if(watched==m_zoomDial && ev->type()==QEvent::MouseButtonRelease && ((QMouseEvent *)ev)->button()==Qt::RightButton)
		m_zoomDial->setValue(100);
	return false;
}

void StereoPhaseMeters::timerEvent(QTimerEvent *e)
{
	if(
		e->timerId()==m_timerId && 
		m_soundPlayer!=NULL && 
		m_stereoPhaseMeters.size()>0 && 
		m_soundPlayer->isInitialized()
	)
	{
		if(gStereoPhaseMetersEnabled)
		{
			m_soundPlayer->getSamplingForStereoPhaseMeters(m_samplingForStereoPhaseMeters.data(),m_samplingForStereoPhaseMeters.size());
			for(size_t t=0;t<m_stereoPhaseMeters.size();t++)
			{
				m_stereoPhaseMeters[t]->tick();
				m_stereoPhaseMeters[t]->update();
			}
		}
	}
}

//void StereoPhaseMeters::resizeEvent()

void StereoPhaseMeters::onZoomDial(int val)
{
	for(size_t t=0;t<m_stereoPhaseMeters.size();t++)
		m_stereoPhaseMeters[t]->setZoom(((float)m_zoomDial->value())/100.0f);
}

