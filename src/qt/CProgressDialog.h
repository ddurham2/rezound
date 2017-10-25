#ifndef __CProgressDialog_H__
#define __CProgressDialog_H__

#include "../../config/common.h"

#include <string>
using namespace std;

#include <QDialog>

#include "ui_CProgressDialog.h"
class CProgressDialog : public QDialog, private Ui::CProgressDialog
{
	Q_OBJECT

public:
	CProgressDialog(QWidget * parent , const string title,bool showCancelButton) :
		QDialog(parent,0),
		isCancelled(false)
	{
		setupUi(this);

		setWindowTitle(title.c_str());

		cancelButton->setVisible(showCancelButton);
	}

	virtual ~CProgressDialog()
	{
	}

	void setProgress(int progress,const string timeElapsed,const string timeRemaining)
	{
		progressBar->setValue(progress);
		timeElapsedLabel->setText(timeElapsed.c_str());
		timeRemainingLabel->setText(timeRemaining.c_str());
		QApplication::processEvents();
	}

	bool isCancelled;

private Q_SLOTS:

	void on_cancelButton_clicked()
	{
		isCancelled=true;
	}

private:

};

#endif
