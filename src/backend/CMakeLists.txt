cmake_minimum_required(VERSION 3.1)	
project(backend)

#include("../../cmake/base.cmake")

#add_subdirectory("${SRC_ROOT}/src/misc" misc)
#add_subdirectory("${SRC_ROOT}/src/PoolFile" PoolFile)

add_library(backend STATIC
	AAction.cpp
	AActionDialog.cpp
	AActionDialog.h
	AAction.h
	AActionParamMapper.cpp
	AActionParamMapper.h
	ActionParamMappers.h
	AFrontendHooks.cpp
	AFrontendHooks.h
	ALFO.cpp
	ALFO.h
	ApipedSoundTranslator.cpp
	ApipedSoundTranslator.h
	ASoundClipboard.cpp
	ASoundClipboard.h
	ASoundFileManager.cpp
	ASoundFileManager.h
	ASoundPlayer.cpp
	ASoundPlayer.h
	ASoundRecorder.cpp
	ASoundRecorder.h
	ASoundTranslator.cpp
	ASoundTranslator.h
	AStatusComm.cpp
	AStatusComm.h
	CActionParameters.cpp
	CActionParameters.h
	CActionSound.cpp
	CActionSound.h
	CALSASoundPlayer.cpp
	CALSASoundPlayer.h
	CALSASoundRecorder.cpp
	CALSASoundRecorder.h
	CFLACSoundTranslator.cpp
	CFLACSoundTranslator.h
	CGraphParamValueNode.cpp
	CGraphParamValueNode.h
	CJACKSoundPlayer.cpp
	CJACKSoundPlayer.h
	CJACKSoundRecorder.cpp
	CJACKSoundRecorder.h
	ClameSoundTranslator.cpp
	ClameSoundTranslator.h
	ClibaudiofileSoundTranslator.cpp
	ClibaudiofileSoundTranslator.h
	ClibvorbisSoundTranslator.cpp
	ClibvorbisSoundTranslator.h
	CLoadedSound.cpp
	CLoadedSound.h
	CMacroPlayer.cpp
	CMacroPlayer.h
	CMacroRecorder.cpp
	CMacroRecorder.h
	CMIDISDSSoundTranslator.cpp
	CMIDISDSSoundTranslator.h
	CNativeSoundClipboard.cpp
	CNativeSoundClipboard.h
	CNULLSoundPlayer.h
	Cold_rezSoundTranslator.cpp
	Cold_rezSoundTranslator.h
	COSSSoundPlayer.cpp
	COSSSoundPlayer.h
	COSSSoundRecorder.cpp
	COSSSoundRecorder.h
	CPluginMapping.cpp
	CPluginMapping.h
	CPortAudioSoundPlayer.cpp
	CPortAudioSoundPlayer.h
	CPortAudioSoundRecorder.cpp
	CPortAudioSoundRecorder.h
	CPulseSoundPlayer.cpp
	CPulseSoundPlayer.h
	CrawSoundTranslator.cpp
	CrawSoundTranslator.h
	CRecordSoundClipboard.cpp
	CRecordSoundClipboard.h
	CrezSoundTranslator.cpp
	CrezSoundTranslator.h
	CRunMacroAction.cpp
	CRunMacroAction.h
	CSound.cpp
	CSound_defs.h
	CSound.h
	CSoundPlayerChannel.cpp
	CSoundPlayerChannel.h
	CStubAction.cpp
	CStubAction.h
	CTrigger.cpp
	CTrigger.h
	CvoxSoundTranslator.cpp
	CvoxSoundTranslator.h
	DSP/BiquadResFilters.h
	DSP/Compressor.h
	DSP/Convolver.h
	DSP/DelayEffect.h
	DSP/Delay.h
	DSP/Distorter.h
	DSP/FlangeEffect.h
	DSP/LevelDetector.h
	DSP/NoiseGate.h
	DSP/Quantizer.h
	DSP/SinglePoleFilters.h
	DSP/TPitchChanger.h
	DSP/TSoundStretcher.h
	DSP/TTempoChanger.h
	Edits/CAddChannelsEdit.cpp
	Edits/CAddChannelsEdit.h
	Edits/CCopyCutDeleteEdit.cpp
	Edits/CCopyCutDeleteEdit.h
	Edits/CCopyCutToNewEdit.cpp
	Edits/CCopyCutToNewEdit.h
	Edits/CCropEdit.cpp
	Edits/CCropEdit.h
	Edits/CCueAction.cpp
	Edits/CCueAction.h
	Edits/CDuplicateChannelEdit.cpp
	Edits/CDuplicateChannelEdit.h
	Edits/CInsertSilenceEdit.cpp
	Edits/CInsertSilenceEdit.h
	Edits/CMuteEdit.cpp
	Edits/CMuteEdit.h
	Edits/CPasteAsNewEdit.cpp
	Edits/CPasteAsNewEdit.h
	Edits/CPasteEdit.cpp
	Edits/CPasteEdit.h
	Edits/CRemoveChannelsEdit.cpp
	Edits/CRemoveChannelsEdit.h
	Edits/CRotateEdit.cpp
	Edits/CRotateEdit.h
	Edits/CSelectionEdit.cpp
	Edits/CSelectionEdit.h
	Edits/CSwapChannelsEdit.cpp
	Edits/CSwapChannelsEdit.h
	Edits/EditActions.h
	Effects/CChangeAmplitudeEffect.cpp
	Effects/CChangeAmplitudeEffect.h
	Effects/CChangeRateEffect.cpp
	Effects/CChangeRateEffect.h
	Effects/CDelayEffect.cpp
	Effects/CDelayEffect.h
	Effects/CDistortionEffect.cpp
	Effects/CDistortionEffect.h
	Effects/CFlangeEffect.cpp
	Effects/CFlangeEffect.h
	Effects/CQuantizeEffect.cpp
	Effects/CQuantizeEffect.h
	Effects/CReverseEffect.cpp
	Effects/CReverseEffect.h
	Effects/CTestEffect.cpp
	Effects/CTestEffect.h
	Effects/CVariedRepeatEffect.cpp
	Effects/CVariedRepeatEffect.h
	Effects/EffectActions.h
	File/CBurnToCDAction.cpp
	File/CBurnToCDAction.h
	File/CNewAudioFileAction.cpp
	File/CNewAudioFileAction.h
	File/COpenAudioFileAction.cpp
	File/COpenAudioFileAction.h
	File/CSaveAsAudioFileAction.cpp
	File/CSaveAsAudioFileAction.h
	File/CSaveAsMultipleFilesAction.cpp
	File/CSaveAsMultipleFilesAction.h
	File/CSaveAudioFileAction.cpp
	File/CSaveAudioFileAction.h
	File/CSaveSelectionAsAction.cpp
	File/CSaveSelectionAsAction.h
	File/FileActions.h
	File/parse_segment_cues.h
	Filters/CArbitraryFIRFilter.cpp
	Filters/CArbitraryFIRFilter.h
	Filters/CBiquadResFilter.cpp
	Filters/CBiquadResFilter.h
	Filters/CConvolutionFilter.cpp
	Filters/CConvolutionFilter.h
	Filters/CMorphingArbitraryFIRFilter.cpp
	Filters/CMorphingArbitraryFIRFilter.h
	Filters/CSinglePoleFilter.cpp
	Filters/CSinglePoleFilter.h
	Filters/FilterActions.h
	Filters/filters_util.h
	Generate/CGenerateNoiseAction.cpp
	Generate/CGenerateNoiseAction.h
	Generate/CGenerateToneAction.cpp
	Generate/CGenerateToneAction.h
	Generate/GenerateActions.h
	initialize.cpp
	initialize.h
	LADSPA/CLADSPAAction.cpp
	LADSPA/CLADSPAAction.h
	LADSPA/LADSPAActions.cpp
	LADSPA/LADSPAActions.h
	LADSPA/ladspa.h
	LADSPA/utils.cpp
	LADSPA/utils.h
	license.h
	Looping/CAddCuesAction.cpp
	Looping/CAddCuesAction.h
	Looping/CMakeSymetricAction.cpp
	Looping/CMakeSymetricAction.h
	Looping/LoopingActions.h
	main_controls.cpp
	main_controls.h
	mypopen.cpp
	mypopen.h
	Remaster/CAdaptiveNormalizeAction.cpp
	Remaster/CAdaptiveNormalizeAction.h
	Remaster/CBalanceAction.cpp
	Remaster/CBalanceAction.h
	Remaster/CChangePitchAction.cpp
	Remaster/CChangePitchAction.h
	Remaster/CChangeTempoAction.cpp
	Remaster/CChangeTempoAction.h
	Remaster/CCompressorAction.cpp
	Remaster/CCompressorAction.h
	Remaster/CInvertPhaseAction.cpp
	Remaster/CInvertPhaseAction.h
	Remaster/CMarkQuietAreasAction.cpp
	Remaster/CMarkQuietAreasAction.h
	Remaster/CMonoizeAction.cpp
	Remaster/CMonoizeAction.h
	Remaster/CNoiseGateAction.cpp
	Remaster/CNoiseGateAction.h
	Remaster/CNormalizeAction.cpp
	Remaster/CNormalizeAction.h
	Remaster/CRemoveDCAction.cpp
	Remaster/CRemoveDCAction.h
	Remaster/CResampleAction.cpp
	Remaster/CResampleAction.h
	Remaster/CShortenQuietAreasAction.cpp
	Remaster/CShortenQuietAreasAction.h
	Remaster/CUnclipAction.cpp
	Remaster/CUnclipAction.h
	Remaster/RemasterActions.h
	settings.cpp
	settings.h
	unit_conv.h
)

target_link_libraries(backend misc PoolFile)



# audio I/O method
message("Tests for methods to use for audio I/O will follow below.  The method actually used will be determined at run-time depending on the order of the values in the ~/.rezound/registry.dat file in AudioOutput(or Input)Methods and will depend on which method first initializes successfully.  Alternatively, there is an --audio-method=... flag to the rezound binary to choose which method to try first.  See 'rezound --help' for more information.")

if (ENABLE_OSS OR NOT DEFINED ENABLE_OSS)
	check_include_file(sys/soundcard.h HAVE_SOUNDCARD_H)
	if (HAVE_SOUNDCARD_H)
		message(STATUS "Enabling OSS method for audio I/O")
		set(ENABLE_OSS ON)
	else()
		message(STATUS "Disabling OSS method for audio I/O")
		set(ENABLE_OSS OFF)
	endif()
else()
	message(STATUS "Disabling OSS method for audio I/O")
	set(ENABLE_OSS OFF)
endif()
set(ENABLE_OSS ${ENABLE_OSS} CACHE BOOL "Whether to build the OSS method for audio I/O" FORCE)



# deb: libasound2-dev
if (ENABLE_ALSA OR NOT DEFINED ENABLE_ALSA)
	pkg_import_module(ALSA alsa)
	if (TARGET ALSA)
		message(STATUS "Enabling ALSA method for audio I/O")
		# TODO some are enable.. some are HAVE_LIB.. let's be consistent
		set(ENABLE_ALSA ON)
	else()
		message(STATUS "Unable to use ALSA method for audio I/O (install: libasound2-dev)")
		set(ENABLE_ALSA OFF)
	endif()
else()
	message(STATUS "Disabling ALSA method for audio I/O")
	set(ENABLE_ALSA OFF)
endif()
set(ENABLE_ALSA ${ENABLE_ALSA} CACHE BOOL "Whether to build the ALSA method for audio I/O" FORCE)



# deb: portaudio19-dev
if (ENABLE_PORTAUDIO OR NOT DEFINED ENABLE_PORTAUDIO)
	pkg_import_module(PORTAUDIO portaudio-2.0>=19)
	if (TARGET PORTAUDIO)
		message(STATUS "Enabling PortAudio method for audio I/O")
		set(ENABLE_PORTAUDIO ON)
		
		if (_PORTAUDIO_VERSION GREATER 18)
			set(ENABLE_PORTAUDIO_V19 ON)
		endif()
	else()
		message(STATUS "Unable to use PortAudio method for audio I/O (install: portaudio19-dev)")
		set(ENABLE_PORTAUDIO OFF)
	endif()
else()
	message(STATUS "Disabling PortAudio method for audio I/O")
	set(ENABLE_PORTAUDIO OFF)
endif()
set(ENABLE_PORTAUDIO ${ENABLE_PORTAUDIO} CACHE BOOL "Whether to build the PortAudio method for audio I/O" FORCE)
set(ENABLE_PORTAUDIO_V19 ${ENABLE_PORTAUDIO_V19} CACHE INTERNAL "" FORCE)


# deb: libjack-dev
if (ENABLE_JACK OR NOT DEFINED ENABLE_JACK)
	pkg_import_module(JACK jack)
	if (TARGET JACK)
		message(STATUS "Enabling JACK method for audio I/O")
		set(ENABLE_JACK ON)
	else()
		message(STATUS "Unable to use JACK method for audio I/O (install: libjack-dev)")
		set(ENABLE_JACK OFF)
	endif()
else()
	message(STATUS "Disabling JACK method for audio I/O")
	set(ENABLE_JACK OFF)
endif()
set(ENABLE_JACK ${ENABLE_JACK} CACHE BOOL "Whether to build the JACK method for audio I/O" FORCE)


# deb: libpulse-dev
if (ENABLE_PULSE OR NOT DEFINED ENABLE_PULSE)
	pkg_import_module(PULSE libpulse-simple)
	if (TARGET PULSE)
		message(STATUS "Enabling pulseaudio method for audio I/O")
		set(ENABLE_PULSE ON)
	else()
		message(STATUS "Unable to use pulseaudio method for audio I/O (install: libpulse-dev)")
		set(ENABLE_PULSE OFF)
	endif()
else()
	message(STATUS "Disabling pulseaudio method for audio I/O")
	set(ENABLE_PULSE OFF)
endif()
set(ENABLE_PULSE ${ENABLE_PULSE} CACHE BOOL "Whether to build the pulseaudio method for audio I/O" FORCE)


### mp3 ###############################################################
# TODO work toward using libav.. which I think I have some local mods for that.. might replace libaudiofile too
execute_process(COMMAND which lame OUTPUT_VARIABLE x OUTPUT_STRIP_TRAILING_WHITESPACE)
if ("${x}" STREQUAL "")
	message(WARNING "lame not found on \$PATH -- this is necessary for loading and saving MP3s -- http://lame.sourceforge.net (install: lame OR notlame)")
endif()



### libaudiofile ##########################################################
# deb: libaudiofile-dev
pkg_import_module(AUDIOFILE audiofile>=0.2.2)
if (TARGET AUDIOFILE)
	set(HAVE_LIBAUDIOFILE ON)
	message(STATUS "Many common audio file formats will be supported")
else()
	message(WARNING "libaudiofile not found.  Many common audio file formats will not be supported (install: libaudiofile-dev)")
endif()
set(HAVE_LIBAUDIOFILE ${HAVE_LIBAUDIOFILE} CACHE INTERNAL "" FORCE)


### ogg/vorbis ##########################################################
# deb: libogg-dev and libvorbis-dev
pkg_import_module(VORBIS vorbis)
pkg_import_module(OGG ogg)
pkg_import_module(VORBISENC vorbisenc)
pkg_import_module(VORBISFILE vorbisfile)
if (TARGET OGG AND TARGET VORBIS AND TARGET VORBISENC AND TARGET VORBISFILE)
	set(HAVE_LIBOGG ON)
	set(HAVE_LIBVORBIS ON)
	set(HAVE_LIBVORBISENC ON)
	set(HAVE_LIBVORBISFILE ON)
	message(STATUS "The ogg/vorbis file audio format will be supported")
else()
	message(WARNING "libogg, libvorbis, libvorbisenc or libvorbisfile not found.  The ogg/vorbis file audio format will not be supported (install: libogg-dev and libvorbis-dev)")
endif()
set(HAVE_LIBOGG ${HAVE_LIBOGG} CACHE INTERNAL "" FORCE)
set(HAVE_LIBVORBIS ${HAVE_LIBVORBIS} CACHE INTERNAL "" FORCE)
set(HAVE_LIBVORBISENC ${HAVE_LIBVORBISENC} CACHE INTERNAL "" FORCE)
set(HAVE_LIBVORBISFILE ${HAVE_LIBVORBISFILE} CACHE INTERNAL "" FORCE)


### FLAC ##########################################################
# deb: libflac-dev and libflac++-dev
pkg_import_module(FLAC flac)
pkg_import_module(FLACPP flac++)
if (TARGET FLAC AND TARGET FLACPP)
	set(HAVE_LIBFLAC ON)
	set(HAVE_LIBFLACPP ON)
	message(STATUS "The flac file audio format will be supported")
else()
	message(WARNING "libflac or libflac++ not found.  The flac file audio format will not be supported (install: libflac-dev and libflac++-dev)")
endif()
set(HAVE_LIBFLAC ${HAVE_LIBFLAC} CACHE INTERNAL "" FORCE)
set(HAVE_LIBFLACPP ${HAVE_LIBFLAC} CACHE INTERNAL "" FORCE)


### fftw ##########################################################
# deb: libfftw3-dev
pkg_import_module(FFTW fftw3>=3.0.0)
if (TARGET FFTW)
	set(HAVE_FFTW ON)
	message(STATUS "Frequency domain features will be supported.")
else()
	message(WARNING "libfftw3 not found.  Certain frequency domain features will not be supported. (install: libfftw3-dev)")
endif()
set(HAVE_FFTW ${HAVE_FFTW} CACHE INTERNAL "" FORCE)


### libsoundtouch ################################################
# deb: libsoundtouch-dev
pkg_import_module(SOUNDTOUCH soundtouch>=1.0)
if (TARGET SOUNDTOUCH)
	set(HAVE_LIBSOUNDTOUCH ON)
	message(STATUS "Actions to adjust tempo and pitch independently will be supported.")
else()
	message(WARNING "libsoundtouch not found.  Actions to adjust tempo and pitch independently will not be supported. (install: libsoundtouch-dev)")
endif()
set(HAVE_LIBSOUNDTOUCH ${HAVE_LIBSOUNDTOUCH} CACHE INTERNAL "" FORCE)


### LADSPA plugin support ###########################################
if (ENABLE_LADSPA OR NOT DEFINED ENABLE_LADSPA)
	set(ENABLE_LADSPA ON)
	message(STATUS "LDASPA support enabled")
else()
	set(ENABLE_LADSPA OFF)
	message(STATUS "LDASPA support disabled")
endif()
set(ENABLE_LADSPA ${ENABLE_LADSPA} CACHE BOOL "Whether to build in support for loading LADSPA plugins" FORCE)



#################################################33

if (TARGET ALSA)
	target_link_libraries(backend ALSA)
endif()

if (TARGET PORTAUDIO)
	target_link_libraries(backend PORTAUDIO)
endif()


if (TARGET JACK)
	target_link_libraries(backend JACK)
endif()

if (TARGET PULSE)
	target_link_libraries(backend PULSE)
endif()

if (TARGET AUDIOFILE)
	target_link_libraries(backend AUDIOFILE)
endif()

if (TARGET OGG AND TARGET VORBIS AND TARGET VORBISENC AND TARGET VORBISFILE)
	target_link_libraries(backend OGG VORBIS VORBISENC VORBISFILE)
endif()

if (TARGET FLAC AND TARGET FLACPP)
	target_link_libraries(backend FLAC FLACPP)
endif()

if (TARGET FFTW)
	target_link_libraries(backend FFTW)
endif()

if (TARGET SOUNDTOUCH)
	target_link_libraries(backend SOUNDTOUCH)
endif()


