mark_as_advanced(ZSTD_LIBRARY ZSTD_INCLUDE_DIR)

option(ENABLE_ZSTD "Enable zstd support" TRUE)
set(USE_ZSTD FALSE)
option(REQUIRE_ZSTD "Require zstd support" FALSE)
if(REQUIRE_ZSTD)
	set(ENABLE_ZSTD TRUE)
endif()

if(ENABLE_ZSTD)
	find_path(ZSTD_INCLUDE_DIR NAMES zstd.h)

	find_library(ZSTD_LIBRARY NAMES zstd)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(Zstd DEFAULT_MSG ZSTD_LIBRARY ZSTD_INCLUDE_DIR)

	if (ZSTD_FOUND)
		set(USE_ZSTD TRUE)
		message(STATUS "Using zstd provided by system")
	elseif(REQUIRE_ZSTD)
		message(FATAL_ERROR "Zstd not found whereas REQUIRE_ZSTD=\"TRUE\" is used.\n"
			"To continue, either install zstd or do not use REQUIRE_ZSTD=\"TRUE\".")
	else()
		message(STATUS "Could not find zstd, support for zstd maps disabled.")
	endif()
else()
	message(STATUS "Zstd detection disabled! (ENABLE_ZSTD=0)")
endif()
