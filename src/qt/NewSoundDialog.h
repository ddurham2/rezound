#include "ModalDialog.h"

#include "settings.h"
#include "../backend/CSound_defs.h"
#include "../backend/AStatusComm.h"
#include "../backend/AFrontendHooks.h"
#include "../misc/istring"
#include "../misc/CPath.h"
#include "../misc/CNestedDataFile/CNestedDataFile.h"

#include <QFileDialog>


#include "ui_NewSoundDialogContent.h"
class NewSoundDialog : public ModalDialog, private Ui::NewSoundDialogContent
{
	Q_OBJECT
public:
	NewSoundDialog(QWidget *parent) :
		ModalDialog(parent,_("New Sound"))
	{
		Ui::NewSoundDialogContent::setupUi(getFrame());
		QMetaObject::connectSlotsByName(this);
	}

	virtual ~NewSoundDialog()
	{
	}

	int exec()
	{
		restoreDefault();
		return QDialog::exec();
	}

	const string getFilename() const { return filenameEdit->text().toStdString(); }
	const bool getRawFormat() const { return rawFormatCheckBox->isChecked(); }
	void setFilename(const string f,const bool _rawFormat=false) { filenameEdit->setText(f.c_str()); rawFormatCheckBox->setChecked(_rawFormat); }
	const unsigned getChannelCount() const { return channelCountComboBox->currentText().toUInt(); }
	const unsigned getSampleRate() const { return sampleRateComboBox->currentText().toUInt(); }
	const sample_pos_t getLength() const { return (sample_pos_t)(lengthComboBox->currentText().toUInt())*(sample_pos_t)getSampleRate(); }

	void hideFilename(bool hide) { filenameLabel->setVisible(!hide); filenameEdit->setVisible(!hide); browseButton->setVisible(!hide); }
	void hideChannelCount(bool hide) { channelCountLabel->setVisible(!hide); channelCountComboBox->setVisible(!hide); }
	void hideSampleRate(bool hide) { sampleRateLabel->setVisible(!hide); sampleRateComboBox->setVisible(!hide); }
	void hideLength(bool hide) { lengthLabel->setVisible(!hide); lengthLabel2->setVisible(!hide); lengthComboBox->setVisible(!hide); }

protected:

	/*override*/ bool validateOnOkay()
	{
		if(!filenameEdit->isHidden())
		{
			istring filename=filenameEdit->text().toStdString();
			filename.trim();

			if(filename=="")
			{
				Warning(_("Please supply a filename"));
				return false;
			}


			// make sure filename has some extension at the end
			if(CPath(filename).extension()=="")
				filename.append(".rez");

			if(CPath(filename).exists())
			{
				if(Question(_("Are you sure you want to overwrite the existing file:\n   ")+filename,yesnoQues)!=yesAns)
					return false;
			}
		}
		else
			filenameEdit->setText("");

		int channelCount=channelCountComboBox->currentText().toInt();
		if(channelCount<0 || channelCount>MAX_CHANNELS)
		{
			Error(_("Invalid number of channels"));
			return false;
		}

		
		int sampleRate=sampleRateComboBox->currentText().toInt();
		if(sampleRate<1000 || sampleRate>1000000)
		{
			Error(_("Invalid sample rate"));
			return false;
		}

		if(!lengthComboBox->isHidden())
		{
			double length=lengthComboBox->currentText().toDouble();
			if(length<0 || MAX_LENGTH/sampleRate<length)
			{
				Error(_("Invalid length"));
				return false;
			} 
		}
		

		saveDefault();
		return true;
	}

private Q_SLOTS:
	void on_browseButton_clicked()
	{
		string filename;
		bool saveAsRaw=false;
		if(gFrontendHooks->promptForSaveSoundFilename(filename,saveAsRaw))
		{
			filenameEdit->setText(filename.c_str());
			rawFormatCheckBox->setChecked(saveAsRaw);
		}
	}

private:

	void saveDefault()
	{
		// ??? could I possibly use the streaming features of FOX? or would that recreate the widgets on restore?
		const string keyPrefix="Qt4" DOT "Defaults" DOT getOrigTitle().toStdString();
		gSettingsRegistry->removeKey(keyPrefix);
		if(rememberAsDefaultCheckBox->isChecked())
		{
			gSettingsRegistry->setValue(keyPrefix DOT "filename",getFilename());
			gSettingsRegistry->setValue(keyPrefix DOT "rawFormat",getRawFormat());
			gSettingsRegistry->setValue(keyPrefix DOT "channelCount",getChannelCount());
			gSettingsRegistry->setValue(keyPrefix DOT "sampleRate",getSampleRate());
			gSettingsRegistry->setValue(keyPrefix DOT "length",lengthComboBox->currentText().toStdString());
		}
	}

	void restoreDefault()
	{
		const string keyPrefix="Qt4" DOT "Defaults" DOT getOrigTitle().toStdString();
		if(gSettingsRegistry->keyExists(keyPrefix))
		{
			filenameEdit->setText(				gSettingsRegistry->getValue<string>	(keyPrefix DOT "filename").c_str());
			rawFormatCheckBox->setChecked(			gSettingsRegistry->getValue<bool>	(keyPrefix DOT "rawFormat"));
			channelCountComboBox->setEditText(istring(	gSettingsRegistry->getValue<unsigned>	(keyPrefix DOT "channelCount")).c_str()); // ??? test me
			sampleRateComboBox->setEditText(istring(	gSettingsRegistry->getValue<unsigned>	(keyPrefix DOT "sampleRate")).c_str());
			lengthComboBox->setEditText(istring(		gSettingsRegistry->getValue<sample_pos_t>(keyPrefix DOT "length")).c_str());

			rememberAsDefaultCheckBox->setChecked(true);
		}
	}

};
