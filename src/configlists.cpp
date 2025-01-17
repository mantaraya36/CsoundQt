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

#include <QProcess>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QDir>
#include <QApplication>
#include <QDebug>

#include "csound.h"
#include "configlists.h"

typedef QPair<QString, QString> QStringPair;



ConfigLists::ConfigLists()
{
    fileTypeNames      << "wav" << "aiff" << "au" << "avr"
                       << "caf" << "flac" << "htk" << "ircam"
                       << "mat4" << "mat5" << "nis" << "paf"
                       << "pvf" << "raw" << "sd2" << "sds"
                       << "svx" << "voc" << "w64" << "wavex";

    fileTypeExtensions << "*.wav" << "*.aif;*.aiff" << "*.au" << "*.avr"
                       << "*.caf" << "*.flac" << "*.htk;*.*" << "*.ircam;*.*"
                       << "*.mat4;*.*" << "*.mat5;*.*" << "*.nis;*.*" << "*.paf;*.*"
                       << "*.pvf" << "*.raw;*.*" << "*.sd2;*.*" << "*.sds;*.*"
					   << "*.svx;*.*" << "*.voc;*.*" << "*.w64;*.wav" << "*.wavex;*.wav";

    fileTypeLongNames  << "WAVE" << "AIFF" << "au" << "avr"
                       << "CAF" << "FLAC" << "htk" << "ircam"
                       << "mat4" << "mat5" << "nis" << "paf"
                       << "pvf" << "Raw (Headerless)" << "Sound Designer II" << "sds"
                       << "svx" << "voc" << "WAVE (w64)" << "WAVE (wavex)";

	fileFormatFlags << "24bit" << "short"<< "uchar"
					<< "schar"<< "float"<< "long";
	fileFormatNames << "24 Bit" << "16 Bit (short)" << "unsigned 8-bit"
					<< "signed 8-bit" << "32 bit float"<< "long (32-bit)";

	refreshModules();

    languages << "English" << "Spanish" << "German" << "French" << "Portuguese"
              << "Italian"  << "Turkish"  << "Finnish" << "Russian" << "Korean"
              << "Persian";

    languageCodes << "en" << "es" << "de" << "fr" << "pt"
                  << "it" << "tr" << "fi" << "ru" << "kr"
                  << "fa";
}


ConfigLists::~ConfigLists()
{
}

void ConfigLists::msgCallback(CSOUND *csound, int attr, const char *fmt, va_list args)
{
	Q_UNUSED(attr);
	QString *ud = (QString *) csoundGetHostData(csound);
	QString msg;
    msg = msg.vasprintf(fmt, args);
	if (msg.isEmpty()) {
		return;
	}
	ud->append(msg);
}

void ConfigLists::refreshModules()
{
    rtMidiNames.clear();
	rtAudioNames.clear();
    CSOUND *csound = csoundCreate(nullptr, nullptr);
	char *name, *type;
	int n = 0;

	while(!csoundGetModule(csound, n++, &name, &type)) {
		if (strcmp(type, "audio") == 0) {
			rtAudioNames << name;
		}
	}

	rtAudioNames << "null"; // add also none (-+rtaudio=null)
	n = 0;

	while(!csoundGetModule(csound, n++, &name, &type)) {
		if (strcmp(type, "midi") == 0) {
			rtMidiNames << name;
		}
	}

    if (rtAudioNames.contains("jack")) {
        rtMidiNames <<  "jack";
	}
    rtMidiNames << "virtual" << "none";
    csoundDestroy(csound);
}

QHash<QString,QString> ConfigLists::getMidiInputDevices(QString module)
{
	// based on code by Steven Yi
    QHash<QString,QString> deviceList;

    CSOUND *cs = csoundCreate(nullptr, nullptr);
	if (module=="jack") {
        csoundSetOption(cs, "-+rtaudio=jack");
        csoundSetOption(cs, "-+rtmidi=jack");
	}

    csoundSetMIDIModule(cs, module.toLatin1().data());
    int i,newn, n = csoundGetMIDIDevList(cs, nullptr, 0);
	CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(n*sizeof(CS_MIDIDEVICE));
	newn = csoundGetMIDIDevList(cs,devs,0);
	if (newn != n) {
		qDebug()  << "Device number changed";
		return deviceList;
	}
	for (i = 0; i < n; i++) {
		qDebug() << "Device "<<i << devs[i].device_name;
		QString displayName, id;
		if (module=="jack") {
			displayName = devs[i].device_name;
			id = devs[i].device_name;
		} else {
            displayName = QString("%1 (%2)")
                    .arg(devs[i].device_name)
                    .arg(devs[i].interface_name);
			id = QString(devs[i].device_id);
		}
		deviceList.insert(displayName, id);
	}
	free(devs);
    csoundDestroy(cs);

	qDebug()<<"Devices found: "<<deviceList;
	return deviceList;
}

QList<QPair<QString, QString> > ConfigLists::getMidiOutputDevices(QString module)
{
	QList<QPair<QString, QString> > deviceList;

    CSOUND *cs = csoundCreate(nullptr, nullptr);
	if (module=="jack") {
        csoundSetOption(cs, "-+rtaudio=jack");
        csoundSetOption(cs, "-+rtmidi=jack");
	}

    csoundSetMIDIModule(cs, module.toLatin1().data());
    int i,newn, n = csoundGetMIDIDevList(cs, nullptr, 1);
	CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(n*sizeof(CS_MIDIDEVICE));
	newn = csoundGetMIDIDevList(cs,devs,1);
	if (newn != n) {
		qDebug()  << "Device number changed";
		return deviceList;
	}
	for (i = 0; i < n; i++) {
        // qDebug() << devs[i].device_name;
		QString displayName, id;
		if (module=="jack") {
			displayName = devs[i].device_name;
			id = devs[i].device_name;
		} else {
            displayName = QString("%1 (%2)")
                    .arg(devs[i].device_name)
                    .arg(devs[i].interface_name);
			id = QString(devs[i].device_id);
		}

        deviceList.append(QStringPair(displayName, id));
	}
	free(devs);
    csoundDestroy(cs);

	return deviceList;
}

QList<QPair<QString, QString> > ConfigLists::getAudioInputDevices(QString module)
{
    QList<QStringPair> deviceList;
    CSOUND *cs = csoundCreate(nullptr, nullptr);

    csoundSetRTAudioModule(cs, module.toLatin1().data());
    int i,newn, n = csoundGetAudioDevList(cs, nullptr, 0);
	CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(n*sizeof(CS_AUDIODEVICE));
	newn = csoundGetAudioDevList(cs,devs,0);
	if (newn != n) {
		qDebug()  << "Device number changed";
		return deviceList;
	}
	for (i = 0; i < n; i++) {
        // qDebug() << "evs[i].device_name;
        deviceList.append(QStringPair(devs[i].device_name, devs[i].device_id));
	}
	free(devs);
    csoundDestroy(cs);
	return deviceList;
}


QList<QPair<QString, QString> > ConfigLists::getAudioOutputDevices(QString module)
{
    QList<QPair<QString, QString> > deviceList;

    CSOUND *cs = csoundCreate(nullptr, nullptr);

    csoundSetRTAudioModule(cs, module.toLatin1().data());
    int i,newn, n = csoundGetAudioDevList(cs, nullptr, 1);
	CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(n*sizeof(CS_AUDIODEVICE));
	newn = csoundGetAudioDevList(cs,devs,1);
	if (newn != n) {
		qDebug()  << "OutputDevices Device number changed";
		return deviceList;
	}
	for (i = 0; i < n; i++) {
        // qDebug()  << devs[i].device_name;
        deviceList.append(QStringPair(devs[i].device_name, devs[i].device_id));
	}
	free(devs);
    csoundDestroy(cs);

	return deviceList;
}

QStringList ConfigLists::runCsoundInternally(QStringList flags)
{
//#if CS_APIVERSION>=4 // CS_APIVERSION is not any more in Csound 7 source
	const char *argv[33];
//#else
//	char *argv[33];
//#endif
    int index = 1;
    Q_ASSERT(flags.size() < 32);
	argv[0]  = (char *) calloc(7, sizeof(char));
	strncpy((char *) argv[0], "csound", 6);

	foreach (QString flag, flags) {
		argv[index] = (char *) calloc(flag.size()+1, sizeof(char));
		strncpy((char *) argv[index], flag.toLatin1(), flag.size());
		index++;
	}
    int argc = flags.size() + 1;
#ifdef MACOSX_PRE_SNOW
	//Remember menu bar to set it after FLTK grabs it
	menuBarHandle = GetMenuBar();
#endif
    m_messages.clear();
    CSOUND *csoundD = csoundCreate(&m_messages, nullptr);

	csoundSetMessageCallback(csoundD, msgCallback);
    int result = csoundCompile(csoundD, argc, argv);

	if(!result) {
        //CS7
        //csoundPerform(csoundD);
        while( csoundPerformKsmps(csoundD)==0);
    } else {
        qDebug() << "Error compiling.";
        qDebug() << m_messages;
    }

    // FIXME This crashes on Linux for portmidi! And messes up devices on OS X
    //csoundDestroy(csoundD);
    csoundReset(csoundD); // CS7

#ifdef MACOSX_PRE_SNOW
	// Put menu bar back
	SetMenuBar(menuBarHandle);
#endif
	return m_messages.split("\n");
}


bool ConfigLists::isJackRunning() {
    CSOUND *cs = csoundCreate(nullptr, nullptr);
    csoundSetRTAudioModule(cs, "jack");
    int n = csoundGetAudioDevList(cs, nullptr, 1);
    qDebug() << "isJackRunning" << n;
    csoundDestroy(cs);
    return n > 0;
}
