#!/bin/csh

if (`uname -m` == 'x86_64') then
    setenv CODA /site/coda/3.02
    setenv CODA_OS Linux-x86_64
    setenv THIRD_LIB  ${PWD}/thirdparty/lib64
else
    setenv CODA /site/coda/2.6.2
    setenv CODA_OS Linux
    setenv THIRD_LIB  ${PWD}/thirdparty/lib
endif

# comment this if you have your own root installed
#source /apps/root/5.34.21/setroot_CUE.csh

setenv CODA_INC ${CODA}/common/include
setenv CODA_LIB ${CODA}/${CODA_OS}/lib
setenv EVIO_LIB ${CODA_LIB}
setenv EVIO_INC ${CODA_INC}

setenv LD_LIBRARY_PATH ${THIRD_LIB}:${CODA_LIB}:${LD_LIBRARY_PATH}
