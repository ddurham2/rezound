#include "CModalDialog.h"

#include "settings.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"

#include "ui_CUserNotesDialogContent.h"
class CUserNotesDialog : public CModalDialog, private Ui::CUserNotesDialogContent
{
	Q_OBJECT
public:
	CUserNotesDialog(QWidget *parent) :
		CModalDialog(parent,_("User Notes"))
	{
		Ui::CUserNotesDialogContent::setupUi(getFrame());
		QMetaObject::connectSlotsByName(this);
	}

	virtual ~CUserNotesDialog()
	{
	}

	int exec(CLoadedSound *loadedSound)
	{
		textEdit->setPlainText(loadedSound->sound->getUserNotes().c_str());
		textEdit->setFocus();
		int ret=QDialog::exec();
		if(ret)
		{
			loadedSound->sound->setIsModified(true);
			loadedSound->sound->setUserNotes(textEdit->toPlainText().toStdString());
		}
		return ret;
	}
};

extern CUserNotesDialog *gUserNotesDialog;
