#include "ModalDialog.h"

#include "settings.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"

#include "ui_UserNotesDialogContent.h"
class UserNotesDialog : public ModalDialog, private Ui::UserNotesDialogContent
{
	Q_OBJECT
public:
    UserNotesDialog(QWidget *parent) :
		ModalDialog(parent,_("User Notes"))
	{
		Ui::UserNotesDialogContent::setupUi(getFrame());
		QMetaObject::connectSlotsByName(this);
	}

    virtual ~UserNotesDialog()
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

extern UserNotesDialog *gUserNotesDialog;
