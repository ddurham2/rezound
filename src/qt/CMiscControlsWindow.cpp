#include "CMiscControlsWindow.h"

#include "CMainWindow.h"
#include "settings.h"
#include "CSoundFileManager.h"
#include "CSoundWindow.h"
#include "../backend/ASoundClipboard.h"
#include "../backend/AAction.h"

#include "CCrossfadeEdgesDialog.h"

CMiscControlsWindow::CMiscControlsWindow(QWidget *parent, Qt::WindowFlags flags) :
	QWidget(parent,flags),
	m_mainWindow(NULL)
{
	setupUi(this);
}

CMiscControlsWindow::~CMiscControlsWindow()
{
}

void CMiscControlsWindow::setMainWindow(CMainWindow *mainWindow)
{
	m_mainWindow=mainWindow;
}

void CMiscControlsWindow::init()
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

void CMiscControlsWindow::setWhichClipboard(size_t whichClipboard)
{
	clipboardComboBox->setCurrentIndex(whichClipboard);
	gWhichClipboard=clipboardComboBox->currentIndex();
}

void CMiscControlsWindow::on_followPlayPositionCheckBox_toggled()
{
	gFollowPlayPosition=followPlayPositionCheckBox->isChecked();
}

void CMiscControlsWindow::on_clippingWarningCheckBox_toggled()
{
	gRenderClippingWarning=clippingWarningCheckBox->isChecked();
	if(gSoundFileManager && gSoundFileManager->getActiveWindow())
		gSoundFileManager->getActiveWindow()->updateFromEdit();
}

void CMiscControlsWindow::on_drawVerticalCuePositionsCheckBox_toggled()
{
	gDrawVerticalCuePositions=drawVerticalCuePositionsCheckBox->isChecked();
	if(gSoundFileManager && gSoundFileManager->getActiveWindow())
		gSoundFileManager->getActiveWindow()->updateFromEdit();
}

void CMiscControlsWindow::on_recordMacroButton_clicked()
{
	m_mainWindow->actionRegistry["Record Macro"]->trigger();
}

void CMiscControlsWindow::on_crossfadeEdgesComboBox_activated(int index)
{
	gCrossfadeEdges=(CrossfadeEdgesTypes)index;
	
}

void CMiscControlsWindow::on_crossfadeEdgesSettingsButton_clicked()
{
	gCrossfadeEdgesDialog->showIt();
}

void CMiscControlsWindow::on_clipboardComboBox_activated(int index)
{
	gWhichClipboard=index;
}

