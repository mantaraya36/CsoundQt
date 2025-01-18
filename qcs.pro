################################################################################
# The following variables can be set in the qmake argument list if they are not
# found in the default locations.  Don't forget to use quotes.  For example ...
# qmake qcs.pro "CSOUND_INCLUDE_DIR = <path to csound.h>"
# CSOUND_INCLUDE_DIR
# CSOUND_LIBRARY_DIR
# RTMIDI_DIR
# PYTHONQT_SRC_DIR
# If the Csound headers and libraries you are using were built from source but
# not installed, set CSOUND_SOURCE_TREE to the directory containing the Csound
# source tree.  In this case, the CSOUND_INCLUDE_DIR and CSOUND_LIBRARY_DIR
# variables do not need to be set explicitly.
# These variables can also be set in a file named config.user.pri, which is
# used if it is found in the same directory as this file (config.pri).
################################################################################
# BUILD OPTIONS:
# CONFIG+=build32    To build floats version
# CONFIG+=rtmidi     To build with RtMidi support
# CONFIG+=record_support
# CONFIG+=debugger
# To support HTML5 via the <html> element in the csd using the Qt WebEngine
# (preferably use Qt 5.8 or later):
# CONFIG+=html_support # NB! before there were options of html_webengine or html_webkit. webkit dropped in v 7.0.0
# OS X only OPTIONS:
# CONFIG+=universal  #  To build i386/ppc version. Default is x86_64
# CONFIG+=i386  #  To build i386 version. Default is x86_64
# CONFIG+=bundle_csound # to make a package that incudes Csound in the bundle with make install
# LINUX ONLY:
# To install CsoundQt and its dekstop file and icons somewhere else than /usr/local/bin and /usr/share
# use variables INSTALL_DIR (default /usr/local) and SHARE_DIR (default /usr/share).
# For example for local install use:
# qmake qcs.pro INSTALL_DIR=~ SHARE_DIR=~/.local/share
################################################################################

#To prepare for Qt6 build
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00

lessThan(QT_MAJOR_VERSION,6): error("Qt6 is required for this build.")

DEFINES += NOMINMAX
# DEFINES += USE_WIDGET_MUTEX


# Add C++11 support since version 0.9.4
CONFIG += c++11
# CONFIG += c++14

#for csound7 (was csound6 before). Will not have no meaning in CsoundQt7
DEFINES += CSOUND7
CONFIG += csound7
message("Building for Csound 7.")

debugger {
    DEFINES += CSQT_DEBUGGER
    message("Building debugger.")
}

QT += concurrent network widgets printsupport quickwidgets


buildDoubles: message("Doubles is now built by default, no need to specify buildDoubles option")

!build32: CONFIG += build64

build32:build64:CONFIG -= build32
unix {
    macx:include (qcs-macx.pro)
    # else:haiku-g++ {include (qcs-haiku.pro) }
    else: include (qcs-unix.pro)
}
win32-g++:include (qcs-win32.pro)
win32-msvc2013:include (qcs-win32.pro)
win32-msvc2015:include (qcs-win32.pro)
win32-msvc2017:include (qcs-win32.pro)
win32-msvc:include (qcs-win32.pro)

# Requires Csound >= 6.04
record_support|perfThread_build {
    DEFINES += PERFTHREAD_RECORD
    message("Building recording support.")
}


debugger {
    DEFINES += CSQT_DEBUGGER
    message("Building debugger.")
}


include(src/src.pri)
TRANSLATIONS = "src/translations/csoundqt_en.ts" \
    "src/translations/csoundqt_es.ts" \
    "src/translations/csoundqt_de.ts" \
    "src/translations/csoundqt_pt.ts" \
    "src/translations/csoundqt_fr.ts" \
    "src/translations/csoundqt_it.ts" \
    "src/translations/csoundqt_tr.ts" \
    "src/translations/csoundqt_el.ts" \
    "src/translations/csoundqt_uk.ts" \
    "src/translations/csoundqt_fi.ts" \
    "src/translations/csoundqt_ru.ts" \
    "src/translations/csoundqt_fa.ts" \
    "src/translations/csoundqt_kr.ts"



html_support: {
message("Building html support with QtWebengine")
DEFINES += CSQT_QTHTML
QT += network webenginewidgets webchannel
CONFIG += c++11
}


INCLUDEPATH *= $${CSOUND_API_INCLUDE_DIR}
INCLUDEPATH *= $${CSOUND_INTERFACES_INCLUDE_DIR}


#DESTDIR = $${_PRO_FILE_PWD_}/bin
DESTDIR = bin
MOC_DIR = build/moc
UI_DIR = build/ui
RCC_DIR = build/rc

TARGET = CsoundQt

build32:TARGET = $${TARGET}-f
build64:TARGET = $${TARGET}-d
html_webkit|html_webengine:TARGET = $${TARGET}-html

TARGET = $${TARGET}-cs7

CONFIG(debug, debug|release):TARGET = $${TARGET}-debug

# install commands for linux (for make install)
# use 'sudo make install' for system wide installation
unix:!macx {
    isEmpty(INSTALL_DIR) {
		INSTALL_DIR=/usr/local  # ~  #for HOME
    }
	isEmpty(SHARE_DIR) {
        SHARE_DIR=/usr/share # ~/.local/share for HOME install
	}

	target.path = $$INSTALL_DIR/bin
	target.files = $$OUT_PWD/$$DESTDIR/$$TARGET # do not install with the full name
	INSTALLS += target

	postInstall.path = $$INSTALL_DIR/bin
	postInstall.commands = cd  $(INSTALL_ROOT)/$$INSTALL_DIR/bin; ln -sf $$TARGET csoundqt
	INSTALLS += postInstall

	# see comments: https://github.com/CsoundQt/CsoundQt/issues/258
        desktop.path=$$SHARE_DIR/applications
	desktop.files = $$PWD/csoundqt.desktop
	INSTALLS += desktop

	icon.path=$$SHARE_DIR/icons/hicolor/scalable/apps
        icon.files=images/csoundqt.svg
	INSTALLS += icon

	mimetypes.path=$$INSTALL_DIR # in some reason path must be set to create install target in Makefile
	mimetypes.commands = cd $$PWD/mime-types/; ./add_csound_mimetypes.sh  $(INSTALL_ROOT)/$$SHARE_DIR
	INSTALLS += mimetypes

    examples.path = $$SHARE_DIR/csoundqt/
    examples.files = src/Examples
	INSTALLS += examples

	templates.path = $$SHARE_DIR/csoundqt/
    templates.files = templates
	INSTALLS += templates

}

# for OSX add Scripts and Examples to be bundle in Contents->Resources
macx {
    examples.path = Contents/Resources/Examples
    examples.files = "src/Examples/"
    QMAKE_BUNDLE_DATA += examples

    templates.path = Contents
    templates.files = "templates"
    QMAKE_BUNDLE_DATA += templates


    # EXPERIMENTAL INSTALL instructions for making bundle (ie make install)
    first.path = $$PWD
    first.commands = $$[QT_INSTALL_PREFIX]/bin/macdeployqt $$OUT_PWD/$$DESTDIR/$${TARGET}.app -qmldir=$$PWD/src/QML # first deployment
    INSTALLS += first

    cocoa.path = $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/PlugIns/platforms # fix missing plugins (with qt 5.4.2 at least)
    cocoa.files =  $$[QT_INSTALL_PREFIX]/plugins/platforms/libqcocoa.dylib

    printsupport.path =  $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/PlugIns/printsupport
    printsupport.files =  $$[QT_INSTALL_PREFIX]/plugins/printsupport/libcocoaprintersupport.dylib

    pythonlinks.path= $$PWD
    pythonlinks.commands = install_name_tool -change /System/Library/Frameworks/Python.framework/Versions/2.7/Python Python.framework/Versions/2.7/Python $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET ;


    bundle_csound {
        # Nothing special to do for that, just don't delete, leave the links to @rpath
        message("Bundle Csound into  the package")
        csound.path= $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/Frameworks/
        csound.files = /Library/Frameworks/CsoundLib64.framework
        INSTALLS+=csound
        final.commands += install_name_tool -change @rpath/libcsnd6.6.0.dylib @rpath/CsoundLib64.framework/Versions/6.0/libcsnd6.6.0.dylib $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET ;
        final.commands += install_name_tool -change  CsoundLib64.framework/CsoundLib64 @rpath/CsoundLib64.framework/Versions/6.0/CsoundLib64 $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET ;

    } else {
        final.path = $$PWD
        final.commands = rm -rf  $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/Frameworks/CsoundLib64.framework ;
        final.commands += install_name_tool -change @rpath/libcsnd6.6.0.dylib libcsnd6.6.0.dylib $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET ;
        final.commands += install_name_tool -change  @rpath/CsoundLib64.framework/Versions/6.0/CsoundLib64 CsoundLib64.framework/Versions/6.0/CsoundLib64 $$OUT_PWD/$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET ;
    }

    final.path = $$PWD
    #final.commands += codesign -s - --deep $$OUT_PWD/$$DESTDIR/$${TARGET}.app ; #try codesigning to eliminate asking for Docuemtns folder permission every time
    final.commands += hdiutil create -fs HFS+ -srcfolder $$OUT_PWD/$$DESTDIR/$${TARGET}.app -volname CsoundQt $$OUT_PWD/$$DESTDIR/$${TARGET}.dmg # untested!
    INSTALLS += cocoa printsupport pythonlinks final

}

win32 {
    first.path = $$PWD
    first.commands = $$[QT_INSTALL_PREFIX]/bin/windeployqt  -qmldir=$$PWD/src/QML  $$OUT_PWD/$$DESTDIR/$${TARGET}.exe # first deployment

	templates.path = $$OUT_PWD/$$DESTDIR/ # not sure if this works
	templates.files = templates

    INSTALLS += first
}

message(CONFIGs are:    $${CONFIG})
message(DEFINES are:    $${DEFINES})
message(INCLUDEPATH is: $${INCLUDEPATH})
message(LIBS are:       $${LIBS})
message(TARGET is:      $${TARGET})

DISTFILES += \
    Building_on_Windows.md \
    config.user.pri

