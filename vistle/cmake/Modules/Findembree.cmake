## ======================================================================== ##
## Copyright 2009-2013 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

FIND_PATH(EMBREE_INCLUDE_PATH embree2/rtcore.h
  /usr/include
  /usr/local/include
  /opt/local/include)

FIND_LIBRARY(EMBREE_LIBRARY NAMES embree PATHS 
  /usr/lib 
  /usr/local/lib 
  /opt/local/lib)

FIND_LIBRARY(EMBREE_AVX_LIBRARY NAMES embree_avx PATHS 
  /usr/lib 
  /usr/local/lib 
  /opt/local/lib)

FIND_LIBRARY(EMBREE_SSE41_LIBRARY NAMES embree_sse41 PATHS 
  /usr/lib 
  /usr/local/lib 
  /opt/local/lib)

FIND_LIBRARY(EMBREE_SYS_LIBRARY NAMES sys PATHS 
  /usr/lib 
  /usr/local/lib 
  /opt/local/lib)

FIND_LIBRARY(EMBREE_SIMD_LIBRARY NAMES simd PATHS 
  /usr/lib 
  /usr/local/lib 
  /opt/local/lib)

FIND_LIBRARY(EMBREE_LIBRARY_MIC NAMES embree_xeonphi PATHS 
  /usr/lib 
  /usr/local/lib 
  /opt/local/lib)

IF (EMBREE_INCLUDE_PATH AND EMBREE_LIBRARY)
  SET(EMBREE_FOUND TRUE)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARY})
  IF(EMBREE_AVX_LIBRARY)
      SET(EMBREE_LIBRARIES "${EMBREE_LIBRARIES} ${EMBREE_AVX_LIBRARY}")
  ENDIF()
  IF(EMBREE_SSE41_LIBRARY)
      SET(EMBREE_LIBRARIES "${EMBREE_LIBRARIES} ${EMBREE_SSE41_LIBRARY}")
  ENDIF()
  IF(EMBREE_SIMD_LIBRARY)
      SET(EMBREE_LIBRARIES "${EMBREE_LIBRARIES} ${EMBREE_SIMD_LIBRARY}")
  ENDIF()
  IF(EMBREE_SYS_LIBRARY)
      SET(EMBREE_LIBRARIES "${EMBREE_LIBRARIES} ${EMBREE_SYS_LIBRARY}")
  ENDIF()
ENDIF ()
