INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_MIXALOT mixalot)

FIND_PATH(
    MIXALOT_INCLUDE_DIRS
    NAMES mixalot/api.h
    HINTS $ENV{MIXALOT_DIR}/include
        ${PC_MIXALOT_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    MIXALOT_LIBRARIES
    NAMES gnuradio-mixalot
    HINTS $ENV{MIXALOT_DIR}/lib
        ${PC_MIXALOT_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MIXALOT DEFAULT_MSG MIXALOT_LIBRARIES MIXALOT_INCLUDE_DIRS)
MARK_AS_ADVANCED(MIXALOT_LIBRARIES MIXALOT_INCLUDE_DIRS)

