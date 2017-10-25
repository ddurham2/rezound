#ifndef __CDurationSpinBox_H__
#define __CDurationSpinBox_H__

#include <QSpinBox>
#include "../backend/unit_conv.h"

class CDurationSpinBox : public QSpinBox
{
	Q_OBJECT
public:
	CDurationSpinBox(QWidget *parent) :
		QSpinBox(parent)
	{
	}

	virtual ~CDurationSpinBox()
	{
	}

protected:
	// 1000ths of a second to text
	QString textFromValue(int value) const
	{
		return seconds_to_string((sample_fpos_t)value/1000.0, 3,true).c_str();
	}

	// text to 1000ths of a second
	int valueFromText(const QString &text) const
	{
		return (int)(string_to_seconds(text.toStdString())*1000);
	}
};

#endif
