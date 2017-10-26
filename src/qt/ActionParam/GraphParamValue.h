/* 
 * Copyright (C) 2007 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#ifndef __GraphParamValue_H__
#define __GraphParamValue_H__


#include "../../../config/common.h"
#include "../qt_compat.h"

#include <string>

#include "../../backend/CSound_defs.h"
#include "../../backend/CGraphParamValueNode.h"

#include "../../backend/AActionParamMapper.h"
#include "ui_GraphParamValue.h"

#include <QLinkedList>

class QImage;
class CGraphParamNode;
class CNestedDataFile;
class CHorzRuler;

/*
 * This can be constructed to be a widget for drawing a curve as a parameter
 * to some action to be performed on a sound.  Either setSound() should be 
 * called to display a waveform behind the curve and 'time' will be the 
 * horizontal unit displayed.  OR, setHorzParameters should be called to indicate
 * the horizontal units and the background will be a solid color.
 * The vertical units are specified through setVertParameters
 */

#include "SoundWindow.h" // for IWaveDrawing and drawWaveRuler()
class GraphParamValue : public IWaveDrawing, public Ui::GraphParamValue
{
	Q_OBJECT
public:
    GraphParamValue(const char *name);
    virtual ~GraphParamValue();

	//int getDefaultWidth();
	//int getDefaultHeight();
	void setMinSize(int minWidth,int minHeight) { setMinimumSize(minWidth,minHeight); }

	void setSound(CSound *sound,sample_pos_t start,sample_pos_t stop);
	void setHorzParameters(const string horzAxisLabel,const string horzUnits,AActionParamMapper *horzValueMapper);
	void setVertParameters(const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper);

	void clearNodes();

	void updateNumbers();

	const CGraphParamValueNodeList &getNodes() const;

    void copyFrom(const GraphParamValue *src);
    void swapWith(GraphParamValue *src);

	// ??? these necessary?
	const int getScalar() const;
	void setScalar(const int scalar);

	const int getMinScalar() const;
	const int getMaxScalar() const;
	// -------- ^^^

	const string getName() const;


	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f) const;


	const int getCueScreenX(size_t cueIndex) const;
	const sample_pos_t getSamplePosForScreenX(int X) const;

protected:
	bool eventFilter(QObject *w,QEvent *e);

	void onGraphCanvasPaint(QPaintEvent *e);
	void onGraphCanvasResize(QResizeEvent *e);

	void onCreateOrStartDragNode(QMouseEvent *e);
	void onDragNode(QMouseEvent *e);
	void onStopDragNode(QMouseEvent *e);
	void onDestroyNode(QMouseEvent *e);

private Q_SLOTS:
	void on_scalarSpinner_valueChanged(int v);

	void on_clearButton_clicked();
	void on_horzFlipButton_clicked();
	void on_vertFlipButton_clicked();
	void on_smoothButton_clicked();
	void on_undoButton_clicked();
	void on_redoButton_clicked();

	void on_vertDeformSlider_sliderReleased();
	void on_horzDeformSlider_sliderReleased();


private:
	friend class CGraphParamNode;
	friend class CVertRuler;
	friend class CHorzRuler;

	const string name;

	int draggingNode;
	int dragOffsetX,dragOffsetY;

	CGraphParamValueNodeList nodes;
	mutable CGraphParamValueNodeList retNodes; // a copy of nodes that is returned

	int getGraphPanelWidth() const; // always returns an even value
	int getGraphPanelHeight() const; // always returns an even value

	// returns the index where it was inserted
	int insertIntoNodes(const CGraphParamValueNode &node);

	int findNodeAt(int x,int y);

	double screenToNodeHorzValue(int x,bool undeform=true);
	double screenToNodeVertValue(int y,bool undeform=true);

	int nodeToScreenX(const CGraphParamValueNode &node);
	int nodeToScreenY(const CGraphParamValueNode &node);

	void updateStatus();
	void clearStatus();

	const string getHorzValueString(double horzValue) const; /* horzValue is [0..1] */
	const string getVertValueString(double vertValue) const; /* vertValue is [0..1] */

	double deformNodeX(double x,int sliderValue) const;
	double undeformNodeX(double x,int sliderValue) const;

	void drawVertRuler(QPaintEvent *e);
	void drawHorzRuler(QPaintEvent *e);

	struct RState
	{
        RState(GraphParamValue *v);

		CGraphParamValueNodeList nodes;
		int scalar;
		int horzDeform;
		int vertDeform;
	};
	QLinkedList<RState *> undoStack;
	QLinkedList<RState *> redoStack;
	void pushUndo();
	void popUndo();
	void popRedo();
	void clearUndo();
	void clearRedo();
	void applyState(RState *s);
	void setUndoButtonsState();

	CSound *sound;
	sample_pos_t start,stop;

	string horzAxisLabel;
	string horzUnits;
	AActionParamMapper *horzValueMapper;

	string vertAxisLabel;
	string vertUnits;
	AActionParamMapper *vertValueMapper;

	QImage *background;
	int prevScalar;
	double hScalar;
	sample_pos_t hOffset;

	QFont rulerFont;

};

#endif
