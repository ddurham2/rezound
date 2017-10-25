#include "CModalDialog.h"

#include "../backend/AAction.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"
#include "../backend/CSoundPlayerChannel.h"
#include "CStatusComm.h"

#include "ui_CMacroActionParamsDialogContent.h"
class CMacroActionParamsDialog : public CModalDialog, private Ui::CMacroActionParamsDialogContent
{
public:
	CMacroActionParamsDialog(QWidget *parent) :
		CModalDialog(parent,_("Macro Action Parameters"))
	{
		Ui::CMacroActionParamsDialogContent::setupUi(getFrame());
		QMetaObject::connectSlotsByName(this);
	}

	virtual ~CMacroActionParamsDialog()
	{
	}

	bool showIt(const AActionFactory *actionFactory,AFrontendHooks::MacroActionParameters &macroActionParameters,CLoadedSound *loadedSound)
	{
		const string actionName=actionFactory->getName();
		const bool selectionPositionsAreApplicable=actionFactory->selectionPositionsAreApplicable;
		const bool hasDialog=actionFactory->hasDialog();

		if(!hasDialog && !selectionPositionsAreApplicable)
		{
			macroActionParameters.askToPromptForActionParametersAtPlayback=false;
			return true; // don't bother even showing the dialog
		}

		askToPromptForActionParametersAtPlaybackCheckBox->setEnabled(hasDialog);

		string startPositionCueName;
		string stopPositionCueName;
		if(loadedSound && selectionPositionsAreApplicable)
		{
			startPosPositioningGroupBox->setEnabled(true);
			stopPosPositioningGroupBox->setEnabled(true);

			size_t index;

			if(loadedSound->sound->findCue(loadedSound->channel->getStartPosition(),index))
			{
				startPositionCueName=loadedSound->sound->getCueName(index);
				startPosRadioButton9->setEnabled(true);
			}
			else
			{
				startPosRadioButton9->setEnabled(false);
				if(startPosRadioButton9->isChecked())
				{
					startPosRadioButton1->setChecked(true);
					startPosRadioButton9->setChecked(false);
				}
			}

			if(loadedSound->sound->findCue(loadedSound->channel->getStopPosition(),index))
			{
				stopPositionCueName=loadedSound->sound->getCueName(index);
				stopPosRadioButton9->setEnabled(true);
			}
			else
			{
				stopPosRadioButton9->setEnabled(false);
				if(stopPosRadioButton9->isChecked())
				{
					stopPosRadioButton1->setChecked(true);
					stopPosRadioButton9->setChecked(false);
				}
			}
		}
		else
		{
			startPosPositioningGroupBox->setEnabled(false);
			stopPosPositioningGroupBox->setEnabled(false);
		}

		actionNameLabel->setText(("Action: "+actionName).c_str());

	reshow:
		
		if(exec())
		{
			macroActionParameters.askToPromptForActionParametersAtPlayback=(askToPromptForActionParametersAtPlaybackCheckBox->isChecked());

			if(startPosRadioButton1->isChecked())
				macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spLeaveAlone;
			else if(startPosRadioButton2->isChecked())
				macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromBeginning;
			else if(startPosRadioButton3->isChecked())
				macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromEnd;
			else if(startPosRadioButton4->isChecked())
				macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spProportionateTimeFromBeginning;
			else if(startPosRadioButton5->isChecked())
				macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromStopPosition;
			else if(startPosRadioButton6->isChecked())
				macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spProportionateTimeFromStopPosition;
			else if(startPosRadioButton9->isChecked())
				macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spSameCueName;
			macroActionParameters.startPosCueName=startPositionCueName;

			if(stopPosRadioButton1->isChecked())
				macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spLeaveAlone;
			else if(stopPosRadioButton2->isChecked())
				macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromBeginning;
			else if(stopPosRadioButton3->isChecked())
				macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromEnd;
			else if(stopPosRadioButton4->isChecked())
				macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spProportionateTimeFromBeginning;
			else if(stopPosRadioButton7->isChecked())
				macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromStartPosition;
			else if(stopPosRadioButton8->isChecked())
				macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spProportionateTimeFromStartPosition;
			else if(stopPosRadioButton9->isChecked())
				macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spSameCueName;
			macroActionParameters.stopPosCueName=stopPositionCueName;

			if(macroActionParameters.positionsAreRelativeToEachOther())
			{
				close();
				Error(_("The positioning of the start and stop positions cannot be relative to each other."));
				goto reshow;
			}

			return true;
		}
		return false;
	}

};
