cmake_minimum_required(VERSION 3.1)	
project(frontend_fox)

find_package(PkgConfig)
pkg_check_modules(FOX fox)

if (NOT FOX_FOUND)
	message(FATAL_ERROR "libfox not found")
	set(HAVE_LIBFOX OFF CACHE INTERNAL "")
else()
	set(HAVE_LIBFOX ON CACHE INTERNAL "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FOX_CFLAGS}")
endif()

execute_process(COMMAND which reswrap OUTPUT_VARIABLE reswrap OUTPUT_STRIP_TRAILING_WHITESPACE)
if ("${reswrap}" STREQUAL "")
	message(FATAL_ERROR "reswrap (part of fox) not found on $PATH")
endif()

execute_process(COMMAND "${CMAKE_CURRENT_LIST_DIR}/convert_images.sh" "${reswrap}" "${SRC_ROOT}/src/images" "${CMAKE_CURRENT_BINARY_DIR}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(rezound
	CAboutDialog.cpp
	CAboutDialog.h
	CActionMenuCommand.cpp
	CActionMenuCommand.h
	CActionParamDialog.cpp
	CActionParamDialog.h
	CChannelSelectDialog.cpp
	CChannelSelectDialog.h
	CCrossfadeEdgesDialog.cpp
	CCrossfadeEdgesDialog.h
	CCueDialog.cpp
	CCueDialog.h
	CCueListDialog.cpp
	CCueListDialog.h
	CFrontendHooks.cpp
	CFrontendHooks.h
	CJACKPortChoiceDialog.cpp
	CJACKPortChoiceDialog.h
	CKeyBindingsDialog.cpp
	CKeyBindingsDialog.h
	CLADSPAActionDialog.cpp
	CLADSPAActionDialog.h
	ClibaudiofileSaveParametersDialog.cpp
	ClibaudiofileSaveParametersDialog.h
	CMacroActionParamsDialog.cpp
	CMacroActionParamsDialog.h
	CMainWindow.cpp
	CMainWindow.h
	CMetersWindow.cpp
	CMetersWindow.h
	CMIDIDumpSampleIdDialog.cpp
	CMIDIDumpSampleIdDialog.h
	CMp3Dialog.cpp
	CMp3Dialog.h
	CNewSoundDialog.cpp
	CNewSoundDialog.h
	COggDialog.cpp
	COggDialog.h
	CPasteChannelsDialog.cpp
	CPasteChannelsDialog.h
	CProgressDialog.cpp
	CProgressDialog.h
	CRawDialog.cpp
	CRawDialog.h
	CRecordDialog.cpp
	CRecordDialog.h
	CRecordMacroDialog.cpp
	CRecordMacroDialog.h
	CRezSaveParametersDialog.cpp
	CRezSaveParametersDialog.h
	CSoundFileManager.cpp
	CSoundFileManager.h
	CSoundWindow.cpp
	CSoundWindow.h
	CStatusComm.cpp
	CStatusComm.h
	CUserNotesDialog.cpp
	CUserNotesDialog.h
	custom_cursors.h
	CVoxDialog.cpp
	CVoxDialog.h
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
	fox_compat.h
	FXBackBufferedCanvas.h
	FXCheckBoxParamValue.cpp
	FXCheckBoxParamValue.h
	FXComboTextParamValue.cpp
	FXComboTextParamValue.h
	FXConstantParamValue.cpp
	FXConstantParamValue.h
	FXDiskEntityParamValue.cpp
	FXDiskEntityParamValue.h
	FXGraphParamValue.cpp
	FXGraphParamValue.h
	FXLFOParamValue.cpp
	FXLFOParamValue.h
	FXModalDialogBox.cpp
	FXModalDialogBox.h
	FXPluginRoutingParamValue.cpp
	FXPluginRoutingParamValue.h
	FXPopupHint.h
	FXRezWaveView.cpp
	FXRezWaveView.h
	FXTextParamValue.cpp
	FXTextParamValue.h
	FXWaveCanvas.cpp
	FXWaveCanvas.h
	FXWaveScrollArea.cpp
	FXWaveScrollArea.h
	GenerateActionDialogs.cpp
	GenerateActionDialogs.h
	LoopingActionDialogs.cpp
	LoopingActionDialogs.h
	main.cpp
	RemasterActionDialogs.cpp
	RemasterActionDialogs.h
	rememberShow.cpp
	rememberShow.h
	settings.cpp
	settings.h
	utils.h

	"${CMAKE_CURRENT_BINARY_DIR}/CFOXIcons.h"
	"${CMAKE_CURRENT_BINARY_DIR}/CFOXIcons.cpp"
)

target_link_libraries(rezound PUBLIC backend)
target_link_libraries(rezound PUBLIC ${FOX_STATIC_LDFLAGS})

set_target_properties(rezound
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${SRC_ROOT}"
)

install(TARGETS rezound RUNTIME DESTINATION bin)

