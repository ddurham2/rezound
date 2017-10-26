#ifndef LevelMeters_H__
#define LevelMeters_H__

#include <vector>
using namespace std;

#include <QWidget>
#include <QLabel>
#include <QEvent>

class ASoundPlayer;
class ASoundRecorder;
class LevelMeter;
class BalanceMeter;

class LevelMeters : public QWidget
{
	//Q_OBJECT
public:
	LevelMeters(QWidget *parent);
	virtual ~LevelMeters();

	bool eventFilter(QObject *watched, QEvent *event);

//public Q_SLOTS:

	void resetGrandMaxPeakLevels();

protected:

	void initialize(unsigned channelCount,bool justCleanup);

	void timerEvent(QTimerEvent *e);

/*
	void resizeEvent(QResizeEvent *e);
*/
/*
	void setGeometry(const QRect &r);
*/

protected:
	virtual void updateMeters()=0;

	vector<LevelMeter *> m_levelMeters;
	vector<BalanceMeter *> m_balanceMeters;

private:
	int levelMeterTickCount;
	QWidget *labelContainer;

	QLabel *grandMaxPeakLevelLabel;
	//QLabel *balanceMetersRightMargin;

	int m_timerId;


	bool recreateLevelLabels(int containerWidth);
};

class PlaybackLevelMeters : public LevelMeters
{
public:
	PlaybackLevelMeters(QWidget *parent);
	virtual ~PlaybackLevelMeters();

	void setSoundPlayer(ASoundPlayer *soundPlayer);

protected:
	void updateMeters();

private:
	ASoundPlayer *m_soundPlayer;
};

class RecordLevelMeters : public LevelMeters
{
public:
	RecordLevelMeters(QWidget *parent);
	virtual ~RecordLevelMeters();

	void setSoundRecorder(ASoundRecorder *soundRecorder);

protected:
	void updateMeters();

private:
	ASoundRecorder *m_soundRecorder;
};

#endif
