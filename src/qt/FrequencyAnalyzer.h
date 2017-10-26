#ifndef FrequencyAnalyzer_H__
#define FrequencyAnalyzer_H__

#include <vector>
using namespace std;

#include <QWidget>
#include <QEvent>
#include <QSlider>

class ASoundPlayer;
class FrequencyAnalyzerCanvas;

class FrequencyAnalyzer : public QWidget
{
	Q_OBJECT
public:
    FrequencyAnalyzer(QWidget *parent);
    virtual ~FrequencyAnalyzer();

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

	FrequencyAnalyzerCanvas *canvas;
	QWidget *labelsFrame;
	QSlider *zoomDial;

	void rebuildLabels(const vector<size_t> _frequencies,size_t octaveStride);
};

#endif
