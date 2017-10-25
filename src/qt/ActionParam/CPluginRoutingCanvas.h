#ifndef __CPluginRoutingCanvas_H__
#define __CPluginRoutingCanvas_H__

#include <QWidget>

class CPluginRoutingParamValue;

class CPluginRoutingCanvas : public QWidget
{
public:
	CPluginRoutingParamValue *prpv;
	CPluginRoutingCanvas(QWidget *parent) : QWidget(parent) {}

protected:
	void paintEvent(QPaintEvent *ev);
};

#endif
