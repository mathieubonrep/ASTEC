####################################################################
#
# $Id$
#
# Copyright (c) INRIA 2013
#
# AUTHOR:
# Etienne Delclaux (etienne.delclaux@inria.fr)
# From Gregoire Malandain (gregoire.malandain@inria.fr)
# 
#

# Name
project(libspect)
set(LIB_NAME spect)

## #################################################################
## Input Files 
## #################################################################
# Inludes
include_directories(${BASIC_INCLUDE_DIRS})
include_directories(${IO_INCLUDE_DIRS})
include_directories(${VT_INCLUDE_DIRS})

# Src files
SET(SRC_FILES  vt_levenberg.c
	vt_matrices.c 
	vt_jointhisto.c
	vt_statsutil.c
	)

# Exe names
SET(EXE_NAMES hmpao-xenon
	hmpao-xenon-fit
	hmpao-xenon-alpha
	hmpao-recalib 
	inr2interfile 
	joint-histo
  )

## #################################################################
## Build
## #################################################################

add_library( ${LIB_NAME} ${SRC_FILES} )
target_link_libraries( ${LIB_NAME} ${ZLIB_LIBRARIES} basic io vt )

foreach(E ${EXE_NAMES})

  add_executable(${E} ${E}.c)
  target_link_libraries(${E} ${LIB_NAME} ${ZLIB_LIBRARIES} basic io vt)

endforeach(E)
