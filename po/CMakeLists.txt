cmake_minimum_required(VERSION 3.1)	
project(rezound-nls)


find_package(Gettext)
if (GETTEXT_FOUND)
	set(GETTEXT_PACKAGE "${PROJECT_NAME}")
	set(PACKAGE_LOCALE_DIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")

	#set(x "${CMAKE_CURRENT_BINARY_DIR}")
	#set(CMAKE_CURRENT_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}")
	#GETTEXT_PROCESS_POT_FILE("rezound.pot" ALL INSTALL_DESTINATION "${CMAKE_CURRENT_LIST_DIR}/po" LANGUAGES 
	#	ru
	#	de
	#	es
	#	fi
	#	fr
	#	cs
	#)
	#set(CMAKE_CURRENT_BINARY_DIR "${x}")

	foreach(lang ru de es fi fr cs)
		GETTEXT_PROCESS_PO_FILES(${lang} ALL PO_FILES ${CMAKE_CURRENT_LIST_DIR}/${lang}.po)

		# not using INSTALL_DESTINATION on GETTEXT_PROCESS_PO_FILES macro because it doesn't rename it as the package-name.mo when installed
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo" DESTINATION "share/locale/${lang}/LC_MESSAGES/" RENAME ${REZOUND_PACKAGE}.mo)

	endforeach(lang)

	# TODO define a custom target that regenerates the rezound.pot file by enumerating all files in git and running xgettext.. refer to what the old autotools did


else()
	message(WARNING "gettext tools not found.  Internationalization may not work correctly")
endif()

