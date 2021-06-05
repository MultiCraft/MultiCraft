
mark_as_advanced(IRRLICHT_DLL)
set(IRRLICHT_SOURCE_DIR "" CACHE PATH "Path to irrlicht source directory (optional)")

# Find include directory and libraries

if(NOT IRRLICHT_SOURCE_DIR STREQUAL "")
	set(IRRLICHT_SOURCE_DIR_INCLUDE
		"${IRRLICHT_SOURCE_DIR}/include"
	)

	set(IRRLICHT_LIBRARY_NAMES libIrrlicht.a Irrlicht Irrlicht.lib)

	if(WIN32)
		if(MSVC)
			set(IRRLICHT_SOURCE_DIR_LIBS "${IRRLICHT_SOURCE_DIR}/lib/Win32-visualstudio")
			set(IRRLICHT_LIBRARY_NAMES Irrlicht.lib)
		else()
			set(IRRLICHT_SOURCE_DIR_LIBS "${IRRLICHT_SOURCE_DIR}/lib/Win32-gcc")
			set(IRRLICHT_LIBRARY_NAMES libIrrlicht.a libIrrlicht.dll.a)
		endif()
	else()
		set(IRRLICHT_SOURCE_DIR_LIBS "${IRRLICHT_SOURCE_DIR}/lib/Linux")
		set(IRRLICHT_LIBRARY_NAMES libIrrlicht.a)
	endif()

	find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		PATHS
		${IRRLICHT_SOURCE_DIR_INCLUDE}
		NO_DEFAULT_PATH
	)

	find_library(IRRLICHT_LIBRARY NAMES ${IRRLICHT_LIBRARY_NAMES}
		PATHS
		${IRRLICHT_SOURCE_DIR_LIBS}
		NO_DEFAULT_PATH
	)
endif()

# find our fork first, then upstream (TODO: remove this?)
foreach(libname IN ITEMS IrrlichtMt Irrlicht)
	string(TOLOWER "${libname}" libname2)

	find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		DOC "Path to the directory with IrrlichtMt includes"
		PATHS
		/usr/local/include/${libname2}
		/usr/include/${libname2}
		/system/develop/headers/${libname2} #Haiku
		PATH_SUFFIXES "include/${libname2}"
	)

	find_library(IRRLICHT_LIBRARY NAMES lib${libname} ${libname}
		DOC "Path to the IrrlichtMt library file"
		PATHS
		/usr/local/lib
		/usr/lib
		/system/develop/lib # Haiku
	)

	if(IRRLICHT_INCLUDE_DIR OR IRRLICHT_LIBRARY)
		break()
	endif()
endforeach()

# Handholding for users
if(IRRLICHT_INCLUDE_DIR AND (NOT IS_DIRECTORY "${IRRLICHT_INCLUDE_DIR}" OR
	NOT EXISTS "${IRRLICHT_INCLUDE_DIR}/irrlicht.h"))
	message(WARNING "IRRLICHT_INCLUDE_DIR was set to ${IRRLICHT_INCLUDE_DIR} "
		"but irrlicht.h does not exist inside. The path will not be used.")
	unset(IRRLICHT_INCLUDE_DIR CACHE)
endif()
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR APPLE)
	# (only on systems where we're sure how a valid library looks like)
	if(IRRLICHT_LIBRARY AND (NOT EXISTS "${IRRLICHT_LIBRARY}" OR
		NOT IRRLICHT_LIBRARY MATCHES "\\.(a|so|dylib|lib)([.0-9]+)?$"))
		message(WARNING "IRRLICHT_LIBRARY was set to ${IRRLICHT_LIBRARY} "
			"but is not a valid library file. The path will not be used.")
		unset(IRRLICHT_LIBRARY CACHE)
	endif()
endif()

# On Windows, find the DLL for installation
if(WIN32)
	# If VCPKG_APPLOCAL_DEPS is ON, dll's are automatically handled by VCPKG
	if(NOT VCPKG_APPLOCAL_DEPS)
		find_file(IRRLICHT_DLL NAMES IrrlichtMt.dll
			DOC "Path of the IrrlichtMt dll (for installation)"
		)
	endif()
endif(WIN32)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Irrlicht DEFAULT_MSG IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

