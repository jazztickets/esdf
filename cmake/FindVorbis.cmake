#	CMake module to find libvorbis
#	Alan Witkowski
#
#	Input:
# 		VORBIS_ROOT          - Environment variable that points to the vorbis root directory
#
#	Output:
#		VORBIS_FOUND         - Set to true if vorbis was found
#		VORBIS_INCLUDE_DIR   - Path to vorbis.h
#		VORBIS_LIBRARIES     - Contains the list of vorbis libraries
#

set(VORBIS_FOUND false)

# find include path
find_path(
		VORBIS_INCLUDE_DIR
	NAMES
		vorbisfile.h
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		include/vorbis
		vorbis
)

# find library
find_library(
		VORBIS_LIBRARY vorbis libvorbis
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# find vorbisfile library
find_library(
		VORBISFILE_LIBRARY vorbisfile libvorbisfile
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# set libraries var
set(VORBIS_LIBRARIES ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY})

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Vorbis DEFAULT_MSG VORBIS_LIBRARIES VORBIS_INCLUDE_DIR)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(VORBIS_INCLUDE_DIR VORBIS_LIBRARIES)
