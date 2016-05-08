#!/bin/csh

if (`uname -m` == 'x86_64') then
    setenv ET_LIB /site/coda/3.02/Linux/lib64
    setenv ET_INC /site/coda/3.02/common/include
    setenv THIRD_LIB  ${PWD}/thirdparty/lib64
else
    setenv ET_LIB /site/coda/2.6.2/Linux/lib
    setenv ET_INC /site/coda/2.6.2/common/include
    setenv THIRD_LIB  ${PWD}/thirdparty/lib
endif

# comment this if you have your own root installed
source /apps/root/5.34.21/setroot_CUE.csh
if ! $?LD_LIBRARY_PATH then
	setenv LD_LIBRARY_PATH ${THIRD_LIB}:${ET_LIB}
else
	setenv LD_LIBRARY_PATH ${THIRD_LIB}:${ET_LIB}:${LD_LIBRARY_PATH}
endif
