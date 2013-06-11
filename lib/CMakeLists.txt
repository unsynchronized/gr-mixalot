# Copyright 2011,2012 Free Software Foundation, Inc.
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
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(
    ${GR_INCLUDES}
    ${Boost_INCLUDE_DIR}
    ${ITPP_INCLUDE_DIR}
)
link_directories(${Boost_LIBRARY_DIRS} ${GR_LIBDIRS})
list(APPEND mixalot_sources
    utils.cc
    pocencode_impl.cc
    )

add_library(gnuradio-mixalot SHARED ${mixalot_sources})
target_link_libraries(gnuradio-mixalot ${Boost_LIBRARIES} ${GR_LIBS}  ${ITPP_LIBRARY_NORMAL})
set_target_properties(gnuradio-mixalot PROPERTIES DEFINE_SYMBOL "gnuradio_mixalot_EXPORTS")

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-mixalot
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_mixalot_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_mixalot.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_mixalot.cc
)

add_executable(test-mixalot ${test_mixalot_sources})

target_link_libraries(
  test-mixalot
  ${GR_LIBS}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  ${ITPP_LIBRARY_NORMAL}
  gnuradio-mixalot
)

GR_ADD_TEST(test_mixalot test-mixalot)
