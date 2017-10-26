#ifndef StereoPhaseMeters_H__
#define StereoPhaseMeters_H__

#include <vector>
using namespace std;

#include <QWidget>
#include <QEvent>
#include <QSlider>

#include "../backend/CSound_defs.h"

class ASoundPlayer;
class StereoPhaseMeter;

class StereoPhaseMeters : public QWidget
{
	Q_OBJECT
public:
    StereoPhaseMeters(QWidget *parent);
    virtual ~StereoPhaseMeters();

	void setSoundPlayer(ASoundPlayer *soundPlayer);

protected:

	bool eventFilter(QObject *watched, QEvent *ev);

	void timerEvent(QTimerEvent *e);

	//void resizeEvent()

private Q_SLOTS:

	void onZoomDial(int val);

private:
	int m_timerId;
	ASoundPlayer *m_soundPlayer;
	vector<sample_t> m_samplingForStereoPhaseMeters;
    vector<StereoPhaseMeter *> m_stereoPhaseMeters;
	QSlider *m_zoomDial;
};

#endif
