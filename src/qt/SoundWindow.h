#ifndef __SoundWindow_H__
#define __SoundWindow_H__

#include "../../config/common.h"

#include <QWidget>

#include "../backend/CSound_defs.h"
#include "settings.h"

class QCheckBox;
class CLoadedSound;
class SoundWindowScrollArea;
class SamplePosSpinBox;
class WaveRuler;
class CAddCueActionFactory;
class SoundWindowScrollArea;

// IWaveDrawing is an interface used by SoundWindow and GraphParamValue both borrowing needing a couple of methods in common for calling on drawWaveRuler()
class IWaveDrawing : public QWidget
{
public:
	virtual const int getCueScreenX(size_t cueIndex) const=0;
	virtual const sample_pos_t getSamplePosForScreenX(int X) const=0;
};

#define NIL_CUE_INDEX (0xffffffff)
#define CUE_Y(w) (((w)->height()-1)-(1+CUE_RADIUS))
extern void drawWaveRuler(IWaveDrawing *wd,QWidget *w,QPaintEvent *e,int offsetX,int totalWidth,const QFont &rulerFont,const QPalette &pal,CSound *sound,bool hasFocus,size_t focusedCueIndex,sample_pos_t focusedCueTime);

#include "ui_SoundWindow.h"
class SoundWindow : public IWaveDrawing, private Ui::SoundWindow
{
	Q_OBJECT

public:
    SoundWindow(QLayout * parent, CLoadedSound *loaded);
	virtual ~SoundWindow();

	// this brings this SoundWindow object to the front
	void setActiveState(bool isActive);
	bool getActiveState() const;

	void updateFromEdit(bool undoing=false);
	const string getLengthString(sample_pos_t length,TimeUnits units);
	void updateAllStatusInfo();
	void updateSelectionStatusInfo();

	enum LastChangedPositions { lcpNone,lcpStart,lcpStop };


	// horz
		enum HorzRecenterTypes { hrtNone,hrtAuto,hrtStart,hrtStop,hrtLeftEdge };
	void setHorzZoom(double v,HorzRecenterTypes horzRecenterType=hrtNone,bool forceUpdate=false);
	const double getHorzZoom() const;
	const sample_pos_t getHorzSize() const;

	void setHorzOffset(sample_pos_t v);
	const sample_pos_t getHorzOffset() const;
	const sample_pos_t getHorzOffsetToCenterTime(sample_pos_t time) const;
	const sample_pos_t getHorzOffsetToCenterStartPos() const;
	const sample_pos_t getHorzOffsetToCenterStopPos() const;

	void horzZoomInSome() { horzZoomSlider->triggerAction(QAbstractSlider::SliderPageStepAdd); }
	void horzZoomOutSome() { horzZoomSlider->triggerAction(QAbstractSlider::SliderPageStepSub); }
	void horzZoomInFull() { on_hZoomInFullButton_clicked(); }
	void horzZoomOutFull() { on_hZoomOutFullButton_clicked(); }
	void horzZoomSelectionFit();


	// vert 
	void setVertZoom(float v,bool forceUpdate=false);
	const float getVertZoom() const;
	const int getVertSize() const;

	void setVertOffset(int v);
	const int getVertOffset() const;



	void redraw();



	void showAmount(double seconds,sample_pos_t pos,int marginPixels=0);
	void centerStartPos();
	void centerStopPos();
	const bool isStartPosOnScreen() const;
	const bool isStopPosOnScreen() const;
	const sample_fpos_t getRenderedDrawSelectStart() const;
	const sample_fpos_t getRenderedDrawSelectStop() const;
	const sample_fpos_t getDrawSelectStart() const;
	const sample_fpos_t getDrawSelectStop() const;


	const sample_pos_t snapPositionToCue(sample_pos_t p) const;
	const int getCueScreenX(size_t cueIndex) const;


	const sample_pos_t getSamplePosForScreenX(int X) const;
	sample_pos_t getLeftEdgePosition() const { return getSamplePosForScreenX(0); }
	void setSelectStartFromScreen(sample_pos_t X);
	void setSelectStopFromScreen(sample_pos_t X);
	void updateFromSelectionChange(LastChangedPositions _lastChangedPosition=lcpNone);
	void drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible);


	void onAutoScroll();


	// called by SoundFileManager::same_method_name()
	const map<string,string> getPositionalInfo() const;
	void setPositionalInfo(const map<string,string> info);

    CLoadedSound * const loadedSound;
	string shuttleControlScalar;
	bool shuttleControlSpringBack;

protected:

Q_SIGNALS:
	void afterLayout();

public Q_SLOTS:
	void updatePlayPositionStatusInfo();

private Q_SLOTS:

	void onAfterLayout();

	void on_infoFrame_customContextMenuRequested(const QPoint &pos);

	void on_horzZoomSlider_valueChanged();
	void on_vertZoomSlider_valueChanged();

	void on_hZoomFitButton_clicked() { horzZoomSelectionFit(); }
	void on_hZoomOutFullButton_clicked() { horzZoomSlider->setValue(horzZoomSlider->minimum()); }
	void on_hZoomInFullButton_clicked() { horzZoomSlider->setValue(horzZoomSlider->maximum()); }
	void on_vZoomOutFullButton_clicked() { vertZoomSlider->setValue(vertZoomSlider->minimum()); }
	void on_vZoomInFullButton_clicked() { vertZoomSlider->setValue(vertZoomSlider->maximum()); }

	void on_zoomOutFullButton_clicked() { on_hZoomOutFullButton_clicked(); on_vZoomOutFullButton_clicked(); }

	void on_clearMutesButton_clicked();
	void on_invertMutesButton_clicked();
	void onMuteButtonClicked();

private:
    friend class SoundWindowScrollArea;
    friend class WaveRuler;

	int getZoomDecimalPlaces() const;
	void updateRuler();

	void updateZoomStatusInfo();

	bool isActive;
    WaveRuler *ruler;
    SoundWindowScrollArea *wvsa;

	sample_fpos_t horzZoomFactor;
	float vertZoomFactor;

	sample_pos_t horzOffset;
	sample_fpos_t prevHorzZoomFactor_horzOffset;

	int vertOffset;

	//int prevDrawPlayStatusX;
	sample_pos_t m_drawPlayPositionAt;
	sample_pos_t renderedStartPosition,renderedStopPosition;

	double lastHorzZoom;
	float lastVertZoom;

	LastChangedPositions lastChangedPosition;

	QLabel *totalLengthLabels[TIME_UNITS_COUNT];
	QLabel *selectionLengthLabels[TIME_UNITS_COUNT];
	QLabel *selectStartLabels[TIME_UNITS_COUNT];
	QLabel *selectStopLabels[TIME_UNITS_COUNT];

	//CSamplePosSpinBox *selectStartSpinBox;
	//CSamplePosSpinBox *selectStopSpinBox;

	void recreateMuteButtons();
	QCheckBox *muteButtons[MAX_CHANNELS];
	unsigned muteButtonCount;

	CAddCueActionFactory *addCueActionFactory;

	bool playingLEDOn;
	bool pausedLEDOn;
};

#endif
