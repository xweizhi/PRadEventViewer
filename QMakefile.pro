######################################################################
# PRad Event Viewer, project file for qmake
######################################################################

greaterThan(4, QT_MAJOR_VERSION):
    QT += widgets concurrent

CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

OBJECTS_DIR = obj
MOC_DIR = qt_moc

TEMPLATE = app
TARGET = PRadEventViewer
DEPENDPATH += 
INCLUDEPATH += include \
               thirdparty/include \
               $$(ROOTSYS)/include

# Input
HEADERS += include/PRadEventViewer.h \
           include/HyCalModule.h \
           include/HyCalScene.h \
           include/HyCalView.h \
           include/Spectrum.h \
           include/SpectrumSettingPanel.h \
           include/HtmlDelegate.h \
           include/PRadDAQUnit.h \
           include/PRadTDCGroup.h \
           include/PRadEvioParser.h \
           include/PRadDSTParser.h \
           include/PRadDataHandler.h \
           include/PRadEventStruct.h \
           include/PRadETChannel.h \
           include/PRadETStation.h \
           include/ETSettingPanel.h \
           include/PRadLogBox.h \
           include/PRadHVSystem.h \
           include/CAENHVSystem.h \
           include/PRadException.h \
           include/PRadHistCanvas.h \
           include/QRootCanvas.h \
           include/ConfigParser.h \
           include/PRadBenchMark.h \
           include/PRadReconstructor.h \
           include/PRadIslandWrapper.h \
           include/PRadGEMSystem.h

SOURCES += src/main.cpp \
           src/PRadEventViewer.cpp \
           src/HyCalModule.cpp \
           src/HyCalScene.cpp \
           src/HyCalView.cpp \
           src/Spectrum.cpp \
           src/SpectrumSettingPanel.cpp \
           src/HtmlDelegate.cpp \
           src/PRadDAQUnit.cpp \
           src/PRadTDCGroup.cpp \
           src/PRadEvioParser.cpp \
           src/PRadDSTParser.cpp \
           src/PRadDataHandler.cpp \
           src/PRadETChannel.cpp \
           src/PRadETStation.cpp\
           src/ETSettingPanel.cpp \
           src/PRadLogBox.cpp \
           src/PRadHVSystem.cpp \
           src/CAENHVSystem.cpp \
           src/PRadException.cpp \
           src/PRadHistCanvas.cpp \
           src/QRootCanvas.cpp \
           src/ConfigParser.cpp \
           src/PRadBenchMark.cpp \
           src/PRadReconstructor.cpp \
           src/PRadIslandWrapper.cpp \
           src/PRadGEMSystem.cpp

LIBS += -lexpat -lgfortran\
        -L$$(THIRD_LIB) -lcaenhvwrapper -levio -levioxx -let \
        -L$$(ROOTSYS)/lib -lCore -lRint -lRIO -lNet -lHist \
                          -lGraf -lGraf3d -lGpad -lTree \
                          -lPostscript -lMatrix -lPhysics \
                          -lMathCore -lThread -lGui -lSpectrum

# other compilers
FORTRAN_SOURCES += src/island.F
fortran.output = $${OBJECTS_DIR}/${QMAKE_FILE_BASE}.o
fortran.commands = gfortran -c ${QMAKE_FILE_NAME} -Iinclude -o ${QMAKE_FILE_OUT}
fortran.input = FORTRAN_SOURCES
QMAKE_EXTRA_COMPILERS += fortran


