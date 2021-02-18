
mark_as_advanced(IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR IRRLICHT_DLL)
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

else()
	find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		DOC "Path to the directory with Irrlicht includes"
		PATHS
		/usr/local/include/irrlicht
		/usr/include/irrlicht
		/system/develop/headers/irrlicht #Haiku
		PATH_SUFFIXES "include/irrlicht"
	)

	find_library(IRRLICHT_LIBRARY NAMES libIrrlicht Irrlicht
		DOC "Path to the Irrlicht library file"
		PATHS
		/usr/local/lib
		/usr/lib
		/system/develop/lib # Haiku
	)
endif()

# Users will likely need to edit these
mark_as_advanced(CLEAR IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

# On Windows, find the DLL for installation
if(WIN32)
	# If VCPKG_APPLOCAL_DEPS is ON, dll's are automatically handled by VCPKG
	if(NOT VCPKG_APPLOCAL_DEPS)
		find_file(IRRLICHT_DLL NAMES Irrlicht.dll
			DOC "Path of the Irrlicht dll (for installation)"
		)
	endif()
endif(WIN32)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Irrlicht DEFAULT_MSG IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

