message(Building with Visual C++)
!no_messages {
    message(Building CsoundQt for Windows.)
    win32-g++: message(Building with gcc)
    win32-msvc2013: message(Building with Visual C++ 2013)
    win32-msvc2015: message(Building with Visual C++ 2015)
    win32-msvc2017: message(Building with Visual C++ 2015)
    win32-msvc: message(Building with Visual C++)
}

CONFIG -= debug_and_release debug_and_release_target \
    copy_dir_files precompile_header shared

# Set default paths
DEFAULT_CSOUND_API_INCLUDE_DIRS = "$$(PROGRAMFILES)\\Csound6\\include\\csound"
DEFAULT_CSOUND_INTERFACES_INCLUDE_DIRS = $${DEFAULT_CSOUND_API_INCLUDE_DIRS}
DEFAULT_CSOUND_LIBRARY_DIRS = "$$(PROGRAMFILES)\\Csound6\\bin"

win32-g++:build32: DEFAULT_CSOUND_LIBS = csound32.dll
win32-g++:build64: DEFAULT_CSOUND_LIBS = csound64.dll

# Need a Visual Studio import library for these...
win32-msvc2013:build32: DEFAULT_CSOUND_LIBS = csound32.lib
win32-msvc2013:build64: DEFAULT_CSOUND_LIBS = csound64.lib
win32-msvc2015:build32: DEFAULT_CSOUND_LIBS = csound32.lib
win32-msvc2015:build64: DEFAULT_CSOUND_LIBS = csound64.lib
win32-msvc2017:build32: DEFAULT_CSOUND_LIBS = csound32.lib
win32-msvc2017:build64: DEFAULT_CSOUND_LIBS = csound64.lib
win32-msvc:build64: DEFAULT_CSOUND_LIBS = csound64.lib

DEFAULT_PYTHON_INCLUDE_DIRS = "$$(HOMEDRIVE)\\Python26\\include"
DEFAULT_PYTHONQT_SRC_DIRS = "$$(PROGRAMFILES)\\PythonQt"

# Do configuration step
include(config.pri)

# Use results from config step
win32-msvc2013: INCLUDEPATH += $${PTHREAD_INCLUDE_DIR} $${DEFAULT_LIBSNDFILE_INCLUDE_DIRS}
win32-msvc2015: INCLUDEPATH += $${PTHREAD_INCLUDE_DIR} $${DEFAULT_LIBSNDFILE_INCLUDE_DIRS}
win32-msvc2017: INCLUDEPATH += $${PTHREAD_INCLUDE_DIR} $${DEFAULT_LIBSNDFILE_INCLUDE_DIRS}
win32-msvc: INCLUDEPATH += $${PTHREAD_INCLUDE_DIR} $${DEFAULT_LIBSNDFILE_INCLUDE_DIRS}

RC_FILE = "src/qutecsound.rc"
LCSOUND = "$${CSOUND_LIBRARY_DIR}/$${CSOUND_LIB}"
#win32-g++:csound6: LCSND = "$${CSOUND_LIBRARY_DIR}/csnd6.dll"

win32-msvc:record_support: LCSND = "$${CSOUND_LIBRARY_DIR}/csnd6.lib" # needed for build on Windows 11

rtmidi {
DEFINES += __WINDOWS_MM__
win32-g++:LIBS += -lwinmm
win32-msvc2013:LIBS += winmm.lib
win32-msvc2015:LIBS += winmm.lib
win32-msvc2017:LIBS += winmm.lib
win32-msvc:LIBS += winmm.lib
}

quteapp_f {
message(Bundling QuteApp_f)
RESOURCES += "src/quteapp_f_win.qrc"
}
quteapp_d {
message(Bundling QuteApp_d)
RESOURCES += "src/quteapp_d_win.qrc"
}

# For OleInitialize() FLTK bug workaround
win32-g++:LIBS *= -lole32
win32-msvc2013:LIBS *= ole32.lib
win32-msvc2015:LIBS *= ole32.lib
win32-msvc2017:LIBS *= ole32.lib
win32-msvc:LIBS *= ole32.lib

