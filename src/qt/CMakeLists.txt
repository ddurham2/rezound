cmake_minimum_required(VERSION 3.1.0)
project(rezound)

#TODO QMAKE_UIC += -tr qt_gettext

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
#set(AUTOGEN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}") # remove when using cmake 3.8

# https://cmake.org/cmake/help/v3.5/manual/cmake-qt.7.html#manual:cmake-qt(7)
# This is how it is documented to work.. but versions aren't yet up to date.. so we'll use pkg-config for now
#find_package(Qt5Widgets REQUIRED)
#if (Qt5Widgets_FOUND)
#	message(STATUS "Qt5Widgets package found: ${QTVERSION}")
#else()
#	message(FATAL_ERROR "Qt5Widgets package not found")
#endif()

find_package(Qt5Widgets QUIET)
if (TARGET Qt5::Widgets)
	message(STATUS "Qt5Widgets Found")
else()
	message(WARNING "\nQt5Widgets not found\ninstall: qtbase5-dev  and also ensure that CMAKE_PREFIX_PATH is set to the directory containing the /.../cmake/ folder which contains a Qt5Widgets/ folder.\ne.g. export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake")
	find_package(Qt5Widgets REQUIRED)
endif()


# remove the 'emit', 'signals' and 'slots' keywords to force use of the Q_EMIT, Q_SIGNALS and Q_SLOTS alternatives
add_definitions(-DQT_NO_EMIT)
add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)



set(srcs
	ActionParam/CActionParamDialog.cpp
	ActionParam/CActionParamDialog.h
	ActionParam/CChannelSelectDialog.cpp
	ActionParam/CChannelSelectDialog.h
	ActionParam/CCheckBoxParamValue.cpp
	ActionParam/CCheckBoxParamValue.h
	ActionParam/CComboTextParamValue.cpp
	ActionParam/CComboTextParamValue.h
	ActionParam/CConstantParamValue.cpp
	ActionParam/CConstantParamValue.h
	ActionParam/CDiskEntityParamValue.cpp
	ActionParam/CDiskEntityParamValue.h
	ActionParam/CGraphParamValue.cpp
	ActionParam/CGraphParamValue.h
	ActionParam/CLFOParamValue.cpp
	ActionParam/CLFOParamValue.h
	ActionParam/CPasteChannelsDialog.cpp
	ActionParam/CPasteChannelsDialog.h
	ActionParam/CPluginRoutingCanvas.h
	ActionParam/CPluginRoutingParamValue.cpp
	ActionParam/CPluginRoutingParamValue.h
	ActionParam/CTextParamValue.cpp
	ActionParam/CTextParamValue.h
	CCrossfadeEdgesDialog.cpp
	CCrossfadeEdgesDialog.h
	CDurationSpinBox.h
	CFrequencyAnalyzer.cpp
	CFrequencyAnalyzer.h
	CFrontendHooks.cpp
	CFrontendHooks.h
	CKeyBindingsDialog.h
	CLADSPAActionDialog.cpp
	CLADSPAActionDialog.h
	CLevelMeters.cpp
	CLevelMeters.h
	ClibaudiofileSaveParametersDialog.cpp
	ClibaudiofileSaveParametersDialog.h
	CMacroActionParamsDialog.h
	CMainWindow.cpp
	CMainWindow.h
	CMIDISampleDumpParametersDialog.cpp
	CMIDISampleDumpParametersDialog.h
	CMiscControlsWindow.cpp
	CMiscControlsWindow.h
	CModalDialog.cpp
	CModalDialog.h
	CMp3SaveParametersDialog.cpp
	CMp3SaveParametersDialog.h
	CNewSoundDialog.h
	COggSaveParametersDialog.cpp
	COggSaveParametersDialog.h
	CProgressDialog.h
	CRawParametersDialog.cpp
	CRawParametersDialog.h
	CRecentActionsMenu.h
	CRecordDialog.h
	CReopenMenu.h
	CRezAction.cpp
	CRezAction.h
	CRezSaveParametersDialog.cpp
	CRezSaveParametersDialog.h
	CSoundFileManager.cpp
	CSoundFileManager.h
	CSoundListWindow.cpp
	CSoundListWindow.h
	CSoundWindow.cpp
	CSoundWindow.h
	CStatusComm.cpp
	CStatusComm.h
	CStereoPhaseMeters.cpp
	CStereoPhaseMeters.h
	CTransportWindow.cpp
	CTransportWindow.h
	CUserNotesDialog.h
	CVoxOpenParametersDialog.cpp
	CVoxOpenParametersDialog.h
	drawPortion.cpp
	drawPortion.h
	EditActionDialogs.cpp
	EditActionDialogs.h
	EffectActionDialogs.cpp
	EffectActionDialogs.h
	FileActionDialogs.cpp
	FileActionDialogs.h
	FilterActionDialogs.cpp
	FilterActionDialogs.h
	GenerateActionDialogs.cpp
	GenerateActionDialogs.h
	LoopingActionDialogs.cpp
	LoopingActionDialogs.h
	main.cpp
	MeterColors.h
	qt_compat.h
	QThumbWheel.cpp
	QThumbWheel.h
	RemasterActionDialogs.cpp
	RemasterActionDialogs.h
	rememberShow.cpp
	rememberShow.h
	settings.cpp
	settings.h
	shortcutKey.h

	resources.qrc
)
	
set(uis
	ActionParam/CActionParamDialogContent.ui
	ActionParam/CChannelSelectDialogContent.ui
	ActionParam/CCheckBoxParamValue.ui
	ActionParam/CComboTextParamValue.ui
	ActionParam/CConstantParamValue.ui
	ActionParam/CDiskEntityParamValue.ui
	ActionParam/CGraphParamValue.ui
	ActionParam/CLFOParamValue.ui
	ActionParam/CPasteChannelsDialogContent.ui
	ActionParam/CPluginRoutingParamValue.ui
	ActionParam/CTextParamValue.ui
	CKeyBindingsDialogContent.ui
	CMacroActionParamsDialogContent.ui
	CMainWindow.ui
	CMiscControlsWindow.ui
	CModalDialog.ui
	CNewSoundDialogContent.ui
	CProgressDialog.ui
	CRecordDialogContent.ui
	CSoundListWindow.ui
	CSoundWindow.ui
	CTransportWindow.ui
	CUserNotesDialogContent.ui
)



add_executable(rezound ${srcs} ${uis})

#target_include_directories(rezound PUBLIC ${AUTOGEN_BUILD_DIR}) # for generated Qt files.. remove when using cmake 3.8

target_link_libraries(rezound backend)
target_link_libraries(rezound Qt5::Widgets)

set_target_properties(rezound
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
)

install(TARGETS rezound RUNTIME DESTINATION bin)

