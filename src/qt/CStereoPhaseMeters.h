#ifndef __CStereoPhaseMeters_H__
#define __CStereoPhaseMeters_H__

#include <vector>
using namespace std;

#include <QWidget>
#include <QEvent>
#include <QSlider>

#include "../backend/CSound_defs.h"

class ASoundPlayer;
class CStereoPhaseMeter;

class CStereoPhaseMeters : public QWidget
{
	Q_OBJECT
public:
	CStereoPhaseMeters(QWidget *parent);
	virtual ~CStereoPhaseMeters();

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
	vector<CStereoPhaseMeter *> m_stereoPhaseMeters;
	QSlider *m_zoomDial;
};

#endif
