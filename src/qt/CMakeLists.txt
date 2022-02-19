cmake_minimum_required(VERSION 3.1.0)
project(rezound)

#TODO QMAKE_UIC += -tr qt_gettext

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
#endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
#set(AUTOGEN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}") # remove when using cmake 3.8

# https://cmake.org/cmake/help/v3.5/manual/cmake-qt.7.html#manual:cmake-qt(7)
# This is how it is documented to work.. but versions aren't yet up to date.. so we'll use pkg-config for now
#find_package(Qt6Widgets REQUIRED)
#if (Qt6Widgets_FOUND)
#	message(STATUS "Qt6Widgets package found: ${QTVERSION}")
#else()
#	message(FATAL_ERROR "Qt6Widgets package not found")
#endif()

find_package(Qt6 COMPONENTS Widgets REQUIRED)
#find_package(Qt6Widgets QUIET)
if (TARGET Qt6::Widgets)
	message(STATUS "Qt6Widgets Found")
else()
	message(WARNING "\nQt6Widgets not found\ninstall: qtbase6-dev  and also ensure that CMAKE_PREFIX_PATH is set to the directory containing the /.../cmake/ folder which contains a Qt6Widgets/ folder.\ne.g. export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake")
	find_package(Qt6Widgets REQUIRED)
endif()


# remove the 'emit', 'signals' and 'slots' keywords to force use of the Q_EMIT, Q_SIGNALS and Q_SLOTS alternatives
add_definitions(-DQT_NO_EMIT)
add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)



set(srcs
	ActionParam/ActionParamDialog.cpp
	ActionParam/ActionParamDialog.h
	ActionParam/ChannelSelectDialog.cpp
	ActionParam/ChannelSelectDialog.h
	ActionParam/CheckBoxParamValue.cpp
	ActionParam/CheckBoxParamValue.h
	ActionParam/ComboTextParamValue.cpp
	ActionParam/ComboTextParamValue.h
	ActionParam/ConstantParamValue.cpp
	ActionParam/ConstantParamValue.h
	ActionParam/DiskEntityParamValue.cpp
	ActionParam/DiskEntityParamValue.h
	ActionParam/GraphParamValue.cpp
	ActionParam/GraphParamValue.h
	ActionParam/LFOParamValue.cpp
	ActionParam/LFOParamValue.h
	ActionParam/PasteChannelsDialog.cpp
	ActionParam/PasteChannelsDialog.h
	ActionParam/PluginRoutingCanvas.h
	ActionParam/PluginRoutingParamValue.cpp
	ActionParam/PluginRoutingParamValue.h
	ActionParam/TextParamValue.cpp
	ActionParam/TextParamValue.h
	CrossfadeEdgesDialog.cpp
	CrossfadeEdgesDialog.h
	drawPortion.cpp
	drawPortion.h
	DurationSpinBox.h
	EditActionDialogs.cpp
	EditActionDialogs.h
	EffectActionDialogs.cpp
	EffectActionDialogs.h
	FileActionDialogs.cpp
	FileActionDialogs.h
	FilterActionDialogs.cpp
	FilterActionDialogs.h
	FrequencyAnalyzer.cpp
	FrequencyAnalyzer.h
	FrontendHooks.cpp
	FrontendHooks.h
	GenerateActionDialogs.cpp
	GenerateActionDialogs.h
	KeyBindingsDialog.h
	LADSPAActionDialog.cpp
	LADSPAActionDialog.h
	LevelMeters.cpp
	LevelMeters.h
	LibaudiofileSaveParametersDialog.cpp
	LibaudiofileSaveParametersDialog.h
	LoopingActionDialogs.cpp
	LoopingActionDialogs.h
	MacroActionParamsDialog.h
	main.cpp
	MainWindow.cpp
	MainWindow.h
	MeterColors.h
	MIDISampleDumpParametersDialog.cpp
	MIDISampleDumpParametersDialog.h
	MiscControlsWindow.cpp
	MiscControlsWindow.h
	ModalDialog.cpp
	ModalDialog.h
	Mp3SaveParametersDialog.cpp
	Mp3SaveParametersDialog.h
	NewSoundDialog.h
	OggSaveParametersDialog.cpp
	OggSaveParametersDialog.h
	ProgressDialog.h
	qt_compat.h
	QThumbWheel.cpp
	QThumbWheel.h
	RawParametersDialog.cpp
	RawParametersDialog.h
	RecentActionsMenu.h
	RecordDialog.h
	RemasterActionDialogs.cpp
	RemasterActionDialogs.h
	rememberShow.cpp
	rememberShow.h
	ReopenMenu.h
	RezAction.cpp
	RezAction.h
	RezSaveParametersDialog.cpp
	RezSaveParametersDialog.h
	settings.cpp
	settings.h
	shortcutKey.h
	SoundFileManager.cpp
	SoundFileManager.h
	SoundListWindow.cpp
	SoundListWindow.h
	SoundWindow.cpp
	SoundWindow.h
	StatusComm.cpp
	StatusComm.h
	StereoPhaseMeters.cpp
	StereoPhaseMeters.h
	TransportWindow.cpp
	TransportWindow.h
	UserNotesDialog.h
	VoxOpenParametersDialog.cpp
	VoxOpenParametersDialog.h

	resources.qrc
)
	
set(uis
	ActionParam/ActionParamDialogContent.ui
	ActionParam/ChannelSelectDialogContent.ui
	ActionParam/CheckBoxParamValue.ui
	ActionParam/ComboTextParamValue.ui
	ActionParam/ConstantParamValue.ui
	ActionParam/DiskEntityParamValue.ui
	ActionParam/GraphParamValue.ui
	ActionParam/LFOParamValue.ui
	ActionParam/PasteChannelsDialogContent.ui
	ActionParam/PluginRoutingParamValue.ui
	ActionParam/TextParamValue.ui
	KeyBindingsDialogContent.ui
	MacroActionParamsDialogContent.ui
	MainWindow.ui
	MiscControlsWindow.ui
	ModalDialog.ui
	NewSoundDialogContent.ui
	ProgressDialog.ui
	RecordDialogContent.ui
	SoundListWindow.ui
	SoundWindow.ui
	TransportWindow.ui
	UserNotesDialogContent.ui
)

qt6_add_resources(srcs resources.qrc)

add_executable(rezound ${srcs} ${uis})

#target_include_directories(rezound PUBLIC ${AUTOGEN_BUILD_DIR}) # for generated Qt files.. remove when using cmake 3.8

target_link_libraries(rezound backend)
target_link_libraries(rezound Qt6::Widgets)

set_target_properties(rezound
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
)

install(TARGETS rezound RUNTIME DESTINATION bin)

