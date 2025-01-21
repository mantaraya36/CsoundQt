
!macx: error(This project file is only for Macintosh OS X systems.)
!no_messages {
    message(" ")
	message(Building CsoundQt for Macintosh OS X.)
}


#CONFIG += x86_64
#QMAKE_CXXFLAGS += -arch x86_64

#Test:
QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64


build32: MAC_LIB = CsoundLib
build64: MAC_LIB = CsoundLib64

#paths set up for using the csound from installed package
HOME_DIRECTORY =


# Set default paths. Put homebrew paths at the end
#Csound 7:
CSOUND_FRAMEWORK_DIR = /Applications/Csound/CsoundLib64.framework/Versions/Current
DEFAULT_CSOUND_API_INCLUDE_DIRS =  $${CSOUND_FRAMEWORK_DIR}/Headers \
        /$${CSOUND_FRAMEWORK_DIR}/Headers \
        $${CSOUND_FRAMEWORK_DIR}/Headers \
        /usr/local/include/csound \
        /usr/local/opt/csound/Frameworks/CsoundLib64.framework/Headers \
        /opt/homebrew/opt/csound/Frameworks/CsoundLib64.framework/Versions/6.0/Headers

        
DEFAULT_CSOUND_INTERFACES_INCLUDE_DIRS = $${DEFAULT_CSOUND_API_INCLUDE_DIRS}
DEFAULT_CSOUND_LIBRARY_DIRS = $${HOME_DIRECTORY}/$${CSOUND_FRAMEWORK_DIR} \
        /$${CSOUND_FRAMEWORK_DIR} \
        /usr/local/lib \
        /usr/local/opt/csound/Frameworks/CsoundLib64.framework/Versions/Current \
        /opt/homebrew/opt/csound/Frameworks/CsoundLib64.framework/Versions/6.0

build32:DEFAULT_CSOUND_LIBS = CsoundLib
build64:DEFAULT_CSOUND_LIBS = CsoundLib64

# For OS X, the PythonQt.1.0.0.dylib and the libPythonQt.1.dylib must be on /usr/local/lib or other lib path
DEFAULT_PYTHON_INCLUDE_DIR = /usr/local/include \
        /usr/include
DEFAULT_PYTHONQT_LIBRARY_DIRS = /usr/local/lib \
        /usr/lib

PYTHONQT_VARIANTS = "pythonqt" "PythonQt3.2" "PythonQt3.1" "PythonQt3.0" "PythonQt" "PythonQt2.0.1"
for (pyqtdir, PYTHONQT_VARIANTS) {
	DEFAULT_PYTHONQT_SRC_DIRS += ../../../$$pyqtdir \
		../$$pyqtdir \
		$$pyqtdir
}

#PYTHONQT_LIB = PythonQt_QtAll$${DEBUG_EXT}
PYTHONQT_LIB = PythonQt
# Do configuration step
include(config.pri)

# Use results from config step
LIBS *= -L$${CSOUND_LIBRARY_DIR}


rtmidi {
DEFINES += __MACOSX_CORE__
LIBS += -framework CoreFoundation
LIBS += -framework CoreMidi -framework CoreAudio
}

quteapp_f {
message(Bundling QuteApp_f)
RESOURCES += "src/quteapp_f_osx.qrc"
}

quteapp_d {
message(Bundling QuteApp_d)
RESOURCES += "src/quteapp_d_osx.qrc"
}

#LIBS += -framework QtCore -framework QtGui -framework QtXml
#LCSOUND = -F$${HOME_DIRECTORY}/Library/Frameworks -F/Library/Frameworks -F/usr/local/opt/csound/Frameworks -F/opt/homebrew/opt/csound/Frameworks -framework $${MAC_LIB}
LCSOUND = -F /Applications/Csound -framework $${MAC_LIB}

#clumsy test for github actions homebrew build. needs rewrite and setting different variable for LCSND_DIR
# exists (/opt/homebrew/opt/csound/lib/libcsnd6.6.0.dylib) {
#     LCSND = -L/opt/homebrew/opt/csound/lib/ -lcsnd6.6.0
# } else: exists(/usr/local/lib/libcsnd6.6.0.dylib) {
#     LCSND = -L/usr/local/lib/ -lcsnd6.6.0
# } else {
#     LCSND = -lcsnd6.6.0
# }


QMAKE_INFO_PLIST = $${PWD}/src/MyInfo.plist
ICON = $${PWD}/images/csoundqt.icns
