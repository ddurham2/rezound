#ifndef PluginRoutingCanvas_H__
#define PluginRoutingCanvas_H__

#include <QWidget>

class PluginRoutingParamValue;

class PluginRoutingCanvas : public QWidget
{
public:
    PluginRoutingParamValue *prpv;
    PluginRoutingCanvas(QWidget *parent) : QWidget(parent) {}

protected:
	void paintEvent(QPaintEvent *ev);
};

#endif
