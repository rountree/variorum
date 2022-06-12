# Copyright 2019-2021 Lawrence Livermore National Security, LLC and other
# Variorum Project Developers. See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

set(variorum_msr_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/msr_core.h
  CACHE INTERNAL "")

set(variorum_msr_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/msr_core.c
  CACHE INTERNAL "")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${variorum_includes})

set( msr_shim_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/msr_core.h
  ${CMAKE_CURRENT_SOURCE_DIR}/063f_msr_samples.h
  CACHE INTERNAL "")

set( msr_shim_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/msr_shim.c
  CACHE INTERNAL "")

add_library( msr_shim
  ${msr_shim_headers}
  ${msr_shim_sources} )
set_property(TARGET ${msr_shim} PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(msr_shim PUBLIC "/usr/lib/x86_64-linux-gnu/libdl.so.2")


