find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_MIXALOT gnuradio-mixalot)

FIND_PATH(
    GR_MIXALOT_INCLUDE_DIRS
    NAMES gnuradio/mixalot/api.h
    HINTS $ENV{MIXALOT_DIR}/include
        ${PC_MIXALOT_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_MIXALOT_LIBRARIES
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

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-mixalotTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_MIXALOT DEFAULT_MSG GR_MIXALOT_LIBRARIES GR_MIXALOT_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_MIXALOT_LIBRARIES GR_MIXALOT_INCLUDE_DIRS)
