#!/bin/bash
# You will need root and evio to build the program
# Setup the path for these dependencies first

#source /home/chao/root-6.06.00/bin/thisroot.sh

export CODA=/home/chao/PRad/coda/Linux-x86_64
export CODA_LIB=$CODA/lib
export CODA_INC=$CODA/include

export EVIO=/home/chao/PRad/evio
export EVIO_OS=Linux-x86_64
export EVIO_LIB=$EVIO/$EVIO_OS/lib
export EVIO_INC=$EVIO/$EVIO_OS/include

export LD_LIBRARY_PATH=$PWD/thirdparty/$SYSTEM/lib:$EVIO_LIB:$CODA_LIB:$LD_LIBRARY_PATH
