#!/bin/bash
# You will need root and evio to build the program
# Setup the path for these dependencies first

#source /home/chao/root-6.06.00/bin/thisroot.sh


if [ `uname -m` == 'x86_64' ]; then
    export OS_TYPE=Linux-x86_64
    export THIRD_LIB=$PWD/thirdparty/lib64
else
    export OS_TYPE=Linux
    export THIRD_LIB=$PWD/thirdparty/lib
fi

export CODA=/home/chao/PRad/coda/$OS_TYPE
export CODA_LIB=$CODA/lib
export CODA_INC=$CODA/include

export EVIO=/home/chao/PRad/evio
export EVIO_LIB=$EVIO/$OS_TYPE/lib
export EVIO_INC=$EVIO/$OS_TYPE/include

export LD_LIBRARY_PATH=$THIRD_LIB:$EVIO_LIB:$CODA_LIB:$LD_LIBRARY_PATH
