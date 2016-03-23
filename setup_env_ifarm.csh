source /apps/root/6.02.00/setroot_CUE.csh

setenv CODA /site/coda/3.02
setenv CODA_OS Linux-x86_64
setenv CODA_INC ${CODA}/common/include
setenv CODA_LIB ${CODA}/${CODA_OS}/lib
setenv EVIO_LIB ${CODA_LIB}
setenv EVIO_INC ${CODA_INC}

setenv LD_LIBRARY_PATH ${PWD}/thirdparty/lib:${CODA_LIB}:${LD_LIBRARY_PATH}
