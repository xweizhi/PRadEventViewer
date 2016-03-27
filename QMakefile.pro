######################################################################
# PRad Event Viewer, project file for qmake
######################################################################

TEMPLATE = app
TARGET = PRadEventViewer
DEPENDPATH += 
INCLUDEPATH += include \
               thirdparty/include \
               $$(ET_INC) \
               $$(ROOTSYS)/include

# Input
HEADERS += include/PRadEventViewer.h \
           include/HyCalModule.h \
           include/HyCalScene.h \
           include/HyCalView.h \
           include/Spectrum.h \
           include/SpectrumSettingPanel.h \
           include/HtmlDelegate.h \
           include/PRadEvioParser.h \
           include/PRadDataHandler.h \
           include/PRadETChannel.h \
           include/ETSettingPanel.h \
           include/PRadLogBox.h \
           include/PRadHVChannel.h \
           include/PRadException.h \
           include/PRadHistCanvas.h \
           include/QRootCanvas.h

SOURCES += src/main.cpp \
           src/PRadEventViewer.cpp \
           src/HyCalModule.cpp \
           src/HyCalScene.cpp \
           src/HyCalView.cpp \
           src/Spectrum.cpp \
           src/SpectrumSettingPanel.cpp \
           src/HtmlDelegate.cpp \
           src/PRadEvioParser.cpp \
           src/PRadDataHandler.cpp \
           src/PRadETchannel.cpp \
           src/ETSettingPanel.cpp \
           src/PRadLogBox.cpp \
           src/PRadHVChannel.cpp \
           src/PRadException.cpp \
           src/PRadHistCanvas.cpp \
           src/QRootCanvas.cpp

LIBS += -L$$(THIRD_LIB) -lcaenhvwrapper -levio -levioxx \
        -L$$(ET_LIB) -let \
        -L$$(ROOTSYS)/lib -lCore -lRint -lRIO -lNet -lHist \
                          -lGraf -lGraf3d -lGpad -lTree \
                          -lPostscript -lMatrix -lPhysics \
                          -lMathCore -lThread -lGui
