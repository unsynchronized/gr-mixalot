# Copyright 2011 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Include swig generation macros
########################################################################
find_package(SWIG)
find_package(PythonLibs 2.5)
if(NOT SWIG_FOUND OR NOT PYTHONLIBS_FOUND)
    return()
endif()
include(GrSwig)
include(GrPython)

########################################################################
# Setup swig generation
########################################################################
foreach(incdir ${GR_INCLUDES})
    list(APPEND GR_SWIG_INCLUDE_DIRS ${incdir}/swig)
    list(APPEND GR_SWIG_INCLUDE_DIRS ${incdir}/gnuradio/swig)
endforeach(incdir)

foreach(incdir ${GRUEL_INCLUDE_DIRS})
    list(APPEND GR_SWIG_INCLUDE_DIRS ${incdir}/gruel/swig)
endforeach(incdir)

set(GR_SWIG_LIBRARIES gnuradio-mixalot)
set(GR_SWIG_DOC_FILE ${CMAKE_CURRENT_BINARY_DIR}/mixalot_swig_doc.i)
set(GR_SWIG_DOC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../include)

GR_SWIG_MAKE(mixalot_swig mixalot_swig.i)

########################################################################
# Install the build swig module
########################################################################
GR_SWIG_INSTALL(TARGETS mixalot_swig DESTINATION ${GR_PYTHON_DIR}/mixalot)

if(UNIX)
    SET( swig_lib _mixalot_swig.lnk )
    SET( swig_lib_target ${CMAKE_INSTALL_PREFIX}/${GR_PYTHON_DIR}/mixalot/_mixalot_swig.so )
    ADD_CUSTOM_COMMAND( OUTPUT ${swig_lib}
                        COMMAND ln -sf ${swig_lib_target} ${swig_lib}
                        COMMENT "Generating system symlink for mixalot"
    )
    ADD_CUSTOM_TARGET( mixalot_swig_link
                       ALL
                       DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${swig_lib} )
    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${swig_lib}
             DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
             RENAME _mixalot_swig.so )
endif(UNIX)


########################################################################
# Install swig .i files for development
########################################################################
install(
    FILES
    mixalot_swig.i
    ${CMAKE_CURRENT_BINARY_DIR}/mixalot_swig_doc.i
    DESTINATION ${GR_INCLUDE_DIR}/mixalot/swig
)
