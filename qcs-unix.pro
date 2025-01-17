!unix:error(This project file is only for Unix-based systems.)
!no_messages {
        message(" ")
	message(Building CsoundQt for Unix-based system.)
}

# Set default paths
DEFAULT_CSOUND_API_INCLUDE_DIRS += /usr/local/include/csound \
	/usr/include/csound
DEFAULT_CSOUND_INTERFACES_INCLUDE_DIRS += $${DEFAULT_CSOUND_API_INCLUDE_DIRS}
DEFAULT_CSOUND_LIBRARY_DIRS += /usr/local/lib \
        /usr/lib
build32:DEFAULT_CSOUND_LIBS = libcsound.so \
	libcsound.a
build64:DEFAULT_CSOUND_LIBS = libcsound64.so \
        libcsound64.a

DEFAULT_LIBSNDFILE_LIBRARY_DIRS = /usr/local/lib \
        /usr/lib
build32:DEFAULT_LIBSNDFILE_LIBRARY_DIRS = /usr/lib/i386-linux-gnu \
	/usr/local/lib \
        /usr/lib

DEFAULT_PYTHON_INCLUDE_DIR += /usr/local/include \
    /usr/include
#no need to define PYTHONQT_LIB_DIR since set by PythonQt.prf and PythonQt_QtAll

PYTHONQT_VARIANTS = "pythonqt" "PythonQt3.2" "PythonQt3.1" "PythonQt3.0" "PythonQt" "PythonQt2.0.1"
for (pyqtdir, PYTHONQT_VARIANTS) {
	DEFAULT_PYTHONQT_SRC_DIRS += ../../../$$pyqtdir \
		../$$pyqtdir \
		$$pyqtdir
}

DEFAULT_PORTMIDI_DIR +=  /usr/local/include

# Do configuration step
include(config.pri)

# Use results from config step
#LIBS *= -L$${CSOUND_LIBRARY_DIR}


rtmidi {
DEFINES += __LINUX_ALSASEQ__
DEFINES += __LINUX_ALSA__
LIBS += -lasound
# check if jack is present
exists(/usr/lib64/libjack.so) | exists(/usr/lib/libjack.so) | exists(/usr/local/lib/libjack.so)  { # maybe there is better way to test if library is presesnt
	message("FOUND JACK")
	DEFINES += __UNIX_JACK__
	LIBS += -ljack
	}
}

system_rtmidi {
    message(System rtmidi in qcs-unix.pro)
    INCLUDEPATH += $${RTMIDI_DIR}
    LIBS += -lrtmidi
}

quteapp_f {
message(Bundling QuteApp_f)
RESOURCES += "src/quteapp_f.qrc"
}
quteapp_d {
message(Bundling QuteApp_d)
RESOURCES += "src/quteapp_d.qrc"
}
build32:LCSOUND = -L$${CSOUND_LIBRARY_DIR} -lcsound
build64:LCSOUND = -L$${CSOUND_LIBRARY_DIR} -lcsound64

#  libcsnd7 is integrated to libcsound64 in CS7
#csound6: LCSND = -lcsnd6
#else: LCSND =  -lcsnd

