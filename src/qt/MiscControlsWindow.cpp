#include "MiscControlsWindow.h"

#include "MainWindow.h"
#include "settings.h"
#include "SoundFileManager.h"
#include "SoundWindow.h"
#include "../backend/ASoundClipboard.h"
#include "../backend/AAction.h"

#include "CrossfadeEdgesDialog.h"

MiscControlsWindow::MiscControlsWindow(QWidget *parent, Qt::WindowFlags flags) :
	QWidget(parent,flags),
	m_mainWindow(NULL)
{
	setupUi(this);
}

MiscControlsWindow::~MiscControlsWindow()
{
}

void MiscControlsWindow::setMainWindow(MainWindow *mainWindow)
{
	m_mainWindow=mainWindow;
}

void MiscControlsWindow::init()
{
	followPlayPositionCheckBox->setChecked(gFollowPlayPosition);

	clippingWarningCheckBox->setChecked(gRenderClippingWarning);

	drawVerticalCuePositionsCheckBox->setChecked(gDrawVerticalCuePositions);

	if(gCrossfadeEdges>=cetNone && gCrossfadeEdges<=cetOuter)
		crossfadeEdgesComboBox->setCurrentIndex((int)gCrossfadeEdges);
	else
		crossfadeEdgesComboBox->setCurrentIndex(0);


	// populate combo box to select clipboard
	for(size_t t=0;t<AAction::clipboards.size();t++)
		clipboardComboBox->addItem(AAction::clipboards[t]->getDescription().c_str());

	if(gWhichClipboard>=AAction::clipboards.size())
		gWhichClipboard=1;

	clipboardComboBox->setCurrentIndex(gWhichClipboard);
}

void MiscControlsWindow::setWhichClipboard(size_t whichClipboard)
{
	clipboardComboBox->setCurrentIndex(whichClipboard);
	gWhichClipboard=clipboardComboBox->currentIndex();
}

void MiscControlsWindow::on_followPlayPositionCheckBox_toggled()
{
	gFollowPlayPosition=followPlayPositionCheckBox->isChecked();
}

void MiscControlsWindow::on_clippingWarningCheckBox_toggled()
{
	gRenderClippingWarning=clippingWarningCheckBox->isChecked();
	if(gSoundFileManager && gSoundFileManager->getActiveWindow())
		gSoundFileManager->getActiveWindow()->updateFromEdit();
}

void MiscControlsWindow::on_drawVerticalCuePositionsCheckBox_toggled()
{
	gDrawVerticalCuePositions=drawVerticalCuePositionsCheckBox->isChecked();
	if(gSoundFileManager && gSoundFileManager->getActiveWindow())
		gSoundFileManager->getActiveWindow()->updateFromEdit();
}

void MiscControlsWindow::on_recordMacroButton_clicked()
{
	m_mainWindow->actionRegistry["Record Macro"]->trigger();
}

void MiscControlsWindow::on_crossfadeEdgesComboBox_activated(int index)
{
	gCrossfadeEdges=(CrossfadeEdgesTypes)index;
	
}

void MiscControlsWindow::on_crossfadeEdgesSettingsButton_clicked()
{
	gCrossfadeEdgesDialog->showIt();
}

void MiscControlsWindow::on_clipboardComboBox_activated(int index)
{
	gWhichClipboard=index;
}

