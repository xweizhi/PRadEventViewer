#!/bin/bash
# You will need root and evio to build the program
# Setup the path for these dependencies first
setenv EVIO_OS Linux-i686

setenv EVIO_LIB ${CODA}/${EVIO_OS}/lib
setenv EVIO_INC ${CODA}/${EVIO_OS}/include
setenv CODA_INC ${CODA}/common/include

setenv LD_LIBRARY_PATH ${PWD}/thirdparty/lib:${EVIO_LIB}:${LD_LIBRARY_PATH}
