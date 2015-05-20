#	CMake module to find libogg
#	Alan Witkowski
#
#	Input:
# 		OGG_ROOT          - Environment variable that points to the ogg root directory
#
#	Output:
#		OGG_FOUND         - Set to true if ogg was found
#		OGG_INCLUDE_DIR   - Path to ogg.h
#		OGG_LIBRARIES     - Contains the list of ogg libraries
#

set(OGG_FOUND false)

# find include path
find_path(
		OGG_INCLUDE_DIR
	NAMES
		ogg.h
	HINTS
		ENV OGG_ROOT
		ENV OGGDIR
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		include/ogg
		ogg
)

# find library
find_library(
		OGG_LIBRARY ogg libogg
	HINTS
		ENV OGG_ROOT
		ENV OGGDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# set libraries var
set(OGG_LIBRARIES ${OGG_LIBRARY})

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ogg DEFAULT_MSG OGG_LIBRARIES OGG_INCLUDE_DIR)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(OGG_INCLUDE_DIR OGG_LIBRARIES)
