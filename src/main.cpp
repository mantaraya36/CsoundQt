/*
    Copyright (C) 2008, 2009 Andres Cabrera
    mantaraya36@gmail.com

    This file is part of CsoundQt.

    CsoundQt is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    CsoundQt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <QApplication>
#include <QSplashScreen>
#include "csoundqt.h"
#include <QLocalSocket>

#ifdef WIN32
#include <tchar.h>
#include <windows.h>

namespace {

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

}  // namespace

// Detect whether the operating system is a 64-bit.
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms684139%28v=vs.85%29.aspx
BOOL IsWow64() {
    BOOL bIsWow64 = FALSE;
    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
                GetModuleHandle(TEXT("kernel32")),
                "IsWow64Process");
    if (NULL != fnIsWow64Process) {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
            // handle error
        }
    }
    return bIsWow64;
}
#endif

int main(int argc, char *argv[])
{
    int result = 0;

    // Set a global template for ALL qDebug messages.
	qSetMessagePattern("[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}][%{file}:%{line} %{function}] %{message}");

    // in Qt6 not needed, High-DPI always enabled
    //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // TO test if this solved hight DPI problems
    //QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QStringList fileNames;
    QApplication qapp(argc, argv);

#ifdef Q_OS_OSX
       if (csoundGetVersion()<6090) { // this build does not work with older Csound on OSX
           qDebug()<<"Csound version too old: "<< VERSION;
           QMessageBox::warning(NULL, QObject::tr("Csound version mismatch"), QObject::tr("This version of CsoundQt requires Csound 6.09 or newer. Please download it from <br>") +"<a href=http://csound.github.io/download>http://csound.github.io/download</a>" );
           return(0);
       }
#endif

    bool autoplay = false;
    QStringList args = qapp.arguments();
    args.removeAt(0); // Remove program name
    for(int i=0; i < args.size(); i++) {
        auto arg = args[i];
        if(arg == "--help") {
            QTextStream out(stdout);
            out << "\n\n";
            out << "Options:" << Qt::endl;
            out << "   --play        Autoplay the last file passed via command line" << Qt::endl;
            out << "   --help        This message" << Qt::endl;
            out << Qt::endl;
            exit(0);
        }
        if(arg == "--play") {
            autoplay = true;
        }
    }

    foreach (QString arg, args) {
        if (!arg.startsWith("-")) {// avoid OS X arguments
            fileNames.append(arg);
        }
    }
    // check if another instance is already running. If yes, ask it to open the file in new tab and quit
    QLocalSocket * socket = new QLocalSocket();
    socket->connectToServer("csoundqt");
    if (socket->waitForConnected(500)) {
        // wait for max 0.5 seconds, returns true if the server in other instance is listening
        if (!fileNames.isEmpty()) {
            qDebug()<<"Opening file(s) in already running instance";
            QString message = "open ***" + fileNames.join("***"); // use a separator that is probably not in the file name
            socket->write(message.toLocal8Bit());
            socket->waitForBytesWritten(1000);
            socket->close();
            return 0; // exit and leave the other instance open the file
        } else {
            socket->close();
            //qDebug()<<"Another instance already running.";
            int answer = QMessageBox::warning(nullptr, QObject::tr("CsoundQt"),
                                              QObject::tr("Another instance is already running. "
                                                          "Are you sure you want to open a new"
                                                          " window?"),
                                              QMessageBox::Open|QMessageBox::Cancel,
                                              QMessageBox::Cancel);
            if (answer==QMessageBox::Cancel) {
                return 0;
            }
        }
    }
    // forming filenames was here before. now before localSocket
    FileOpenEater filterObj;
    qapp.installEventFilter(&filterObj);

    QPixmap pixmap(":/images/splashscreen.png");

    QSplashScreen *splash = new QSplashScreen(pixmap);
    splash->showMessage(QString("Version %1").arg(CSQT_VERSION),
                        Qt::AlignCenter | Qt::AlignBottom, Qt::white);
    splash->show();
    splash->raise();
    qapp.processEvents();
    QSettings qsettings("csoundqt", "csoundqt");
    qsettings.beginGroup("GUI");
    QString language = qsettings.value("language", QLocale::system().name()).toString();
    qsettings.endGroup();
    QTranslator translator;
    bool loaded = translator.load(QString(":/translations/csoundqt_") + language);
    if (loaded) {
        qapp.installTranslator(&translator);
    } else {
        QDEBUG << "Could load translator";
    }

    CsoundQt *csoundQt = new CsoundQt(fileNames);
    if (!csoundQt->startServer())
        qDebug()<<"Could not start local server.";
    splash->finish(csoundQt);
    delete splash;
    csoundQt->show();
    if(autoplay && !fileNames.isEmpty())
        csoundQt->play();

    filterObj.setMainWindow(csoundQt);
    // QDEBUG << "Starting qapp exec";
    result = qapp.exec();
    return result;
}
