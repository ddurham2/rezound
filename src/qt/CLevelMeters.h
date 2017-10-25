#ifndef __CLevelMeters_H__
#define __CLevelMeters_H__

#include <vector>
using namespace std;

#include <QWidget>
#include <QLabel>
#include <QEvent>

class ASoundPlayer;
class ASoundRecorder;
class CLevelMeter;
class CBalanceMeter;

class CLevelMeters : public QWidget
{
	//Q_OBJECT
public:
	CLevelMeters(QWidget *parent);
	virtual ~CLevelMeters();

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

	vector<CLevelMeter *> m_levelMeters;
	vector<CBalanceMeter *> m_balanceMeters;

private:
	int levelMeterTickCount;
	QWidget *labelContainer;

	QLabel *grandMaxPeakLevelLabel;
	//QLabel *balanceMetersRightMargin;

	int m_timerId;


	bool recreateLevelLabels(int containerWidth);
};

class CPlaybackLevelMeters : public CLevelMeters
{
public:
	CPlaybackLevelMeters(QWidget *parent);
	virtual ~CPlaybackLevelMeters();

	void setSoundPlayer(ASoundPlayer *soundPlayer);

protected:
	void updateMeters();

private:
	ASoundPlayer *m_soundPlayer;
};

class CRecordLevelMeters : public CLevelMeters
{
public:
	CRecordLevelMeters(QWidget *parent);
	virtual ~CRecordLevelMeters();

	void setSoundRecorder(ASoundRecorder *soundRecorder);

protected:
	void updateMeters();

private:
	ASoundRecorder *m_soundRecorder;
};

#endif
