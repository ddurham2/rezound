#ifndef __CFrequencyAnalyzer_H__
#define __CFrequencyAnalyzer_H__

#include <vector>
using namespace std;

#include <QWidget>
#include <QEvent>
#include <QSlider>

class ASoundPlayer;
class CFrequencyAnalyzerCanvas;

class CFrequencyAnalyzer : public QWidget
{
	Q_OBJECT
public:
	CFrequencyAnalyzer(QWidget *parent);
	virtual ~CFrequencyAnalyzer();

	void setSoundPlayer(ASoundPlayer *soundPlayer);

protected:

	bool eventFilter(QObject *watched, QEvent *ev);

	void timerEvent(QTimerEvent *e);

	//void resizeEvent();

private Q_SLOTS:

	void onZoomDial(int val);

private:
	int m_timerId;
	ASoundPlayer *m_soundPlayer;

	CFrequencyAnalyzerCanvas *canvas;
	QWidget *labelsFrame;
	QSlider *zoomDial;

	void rebuildLabels(const vector<size_t> _frequencies,size_t octaveStride);
};

#endif
