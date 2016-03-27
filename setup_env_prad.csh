#!/bin/bash
# You will need root and evio to build the program
# Setup the path for these dependencies first
setenv THIRD_LIB ${PWD}/thirdparty/lib

setenv ET_LIB ${CODA_LIB}
setenv ET_INC ${CODA}/common/include

setenv LD_LIBRARY_PATH ${THIRD_LIB}:${LD_LIBRARY_PATH}
