#ifndef ProgressDialog_H__
#define ProgressDialog_H__

#include "../../config/common.h"

#include <string>
using namespace std;

#include <QDialog>

#include "ui_ProgressDialog.h"
class ProgressDialog : public QDialog, private Ui::ProgressDialog
{
	Q_OBJECT

public:
    ProgressDialog(QWidget * parent, Qt::WindowFlags()) :
		QDialog(parent)
	{
		setupUi(this);

//		setWindowTitle(title.c_str());

//		cancelButton->setVisible(showCancelButton);
	}

    virtual ~ProgressDialog()
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
