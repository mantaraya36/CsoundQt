/*
	Copyright (C) 2010 Andres Cabrera
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

#include "csoundoptions.h"
#include "configlists.h"
#include "types.h"

#include <QDir> // for static QDir::separator()
#include <QDebug>
#include <cstdlib> // for calloc

CsoundOptions::CsoundOptions(ConfigLists *configlists) :
	m_configlists(configlists)
{
	m_jackNameSize = 16; //a small default

	rt = true;

	enableFLTK = true;
    bufferSize = 1024;
	bufferSizeActive = false;
	HwBufferSize = 2048;
	HwBufferSizeActive = false;
	dither = false;
	newParser = false;
	multicore = false;
	numThreads = 1;
    realtimeFlag = false;
    sampleAccurateFlag = false;
	additionalFlags = "";
	additionalFlagsActive = false;

	fileUseOptions = false;
	fileOverrideOptions = false;
	fileAskFilename = false;
	filePlayFinished = false;
	fileFileType = 0;
	fileSampleFormat = 0;
	fileInputFilenameActive = false;
	fileInputFilename = "";
	fileOutputFilenameActive = false;
	fileOutputFilename = "";

	sampleFormat = 0;

	rtUseOptions = true;
	rtOverrideOptions = false;
	rtAudioModule = "";
	rtInputDevice = "adc";
	rtOutputDevice = "dac";
	rtJackName = "*";
	rtMidiModule = "";
	rtMidiInputDevice = "0";
	rtMidiOutputDevice = "0";
    useSystemSamplerate = false;
    overrideNumChannels = false;
    numChannels = 2;
    numInputChannels = 2;

    useLimiter = csoundGetVersion() >= 6160;
    limitValue = 1.0;

	useCsoundMidi = false;
	simultaneousRun = true; // Allow running various instances (tabs) simultaneously.
    checkSyntaxOnly = false;
    checkSyntaxBeforeRun = false;

	csdocdir = "";
    opcode7dir64 = "";
    opcode7dir64Active = false;
	sadir = "";
	sadirActive = false;
	ssdir = "";
	ssdirActive = false;
	sfdir = "";
	sfdirActive = false;
	incdir = "";
	incdirActive = false;
}

QString CsoundOptions::generateCmdLineFlags()
{
	return generateCmdLineFlagsList().join(" ");
}

QStringList CsoundOptions::generateCmdLineFlagsList()
{
    QStringList opts;
	if (bufferSizeActive)
        opts << "-b" + QString::number(bufferSize);
	if (HwBufferSizeActive)
        opts << "-B" + QString::number(HwBufferSize);
	if (additionalFlagsActive && !additionalFlags.trimmed().isEmpty()) {
		QStringList addFlags = additionalFlags.split(QRegExp("[\\s]"),QString::SkipEmptyParts);
		foreach (QString f, addFlags) {
            QDEBUG << "Additional Flags:" << additionalFlags;
            opts << f;
		}
	}
    if (dither)
        opts << " -Z";
    if (multicore)
        opts << "-j" + QString::number(numThreads);
    if (realtimeFlag)
        opts << "--realtime";
    if (sampleAccurateFlag)
        opts << "--sample-accurate";
    if (checkSyntaxOnly)
        opts << "--syntax-check-only";
    if (rt && rtUseOptions) {
		if (rtOverrideOptions)
            opts << "-+ignore_csopts=1";
		if (m_configlists->rtAudioNames.indexOf(rtAudioModule) >= 0
				&& rtAudioModule != "none") {
            opts << "-+rtaudio=" + rtAudioModule;
			if (rtInputDevice != "") {
                opts << "-i" + rtInputDevice;
			}
            opts << "-o" + (rtOutputDevice == "" ? "dac":rtOutputDevice);
			if (rtJackName != "" && rtAudioModule == "jack") {
				QString jackName = rtJackName;
				if (jackName.contains("*")) {
                    // auto base = fileName1.mid(fileName1.lastIndexOf(QDir::separator())+1);
                    auto base = docName.mid(docName.lastIndexOf(QDir::separator())+1);
                    jackName.replace("*", base);
                    jackName.replace(" ", "_");
				}
				if (jackName.size() > m_jackNameSize) {
					jackName = jackName.left(m_jackNameSize);
				}
                opts << "-+jack_client=" + jackName;
			}
		}
		if (useCsoundMidi &&
				m_configlists->rtMidiNames.indexOf(rtMidiModule) >= 0 &&
				rtMidiModule != "none") {
            opts << "-+rtmidi=" + rtMidiModule;
			if (rtMidiInputDevice != "")
                opts << "-M" + rtMidiInputDevice;
			if (rtMidiOutputDevice != "")
                opts << "-Q" + rtMidiOutputDevice;
		}
        if (useSystemSamplerate)
            opts << "--use-system-sr";
        else if(samplerate > 0) {
            opts << "--sample-rate=" + QString::number(samplerate);
        }
        if (overrideNumChannels) {
            if(numChannels <= 0) {
                qDebug("Setting nchnls, but numChannels is invalid: %d", numChannels);
            } else
                opts << ("--nchnls=" + QString::number(numChannels));
            if(numInputChannels < 0) {
                qDebug("Setting nchnls_i, but numInputChannels is invalid: %d", numInputChannels);
            } else {
                QDEBUG << "nchnls_i" << numInputChannels;
                opts << ("--nchnls_i=" + QString::number(numInputChannels));
            }

        }
        if (useLimiter) {
            opts << "--limiter="+QString::number(limitValue);
        }

        // add --print_version by default if Csound 6.17 or later
        if ( csoundGetVersion() >=6170 ) {
            opts << "--print_version";
        }
	}
	else {
        opts << "--format=" + m_configlists->fileTypeNames[fileFileType]
				+ ":" + m_configlists->fileFormatFlags[fileSampleFormat];
		if (fileInputFilenameActive)
            opts << "-i" + fileInputFilename + "";
        if (fileOutputFilenameActive || fileAskFilename) {
            opts << "-o" + fileOutputFilename + "";
		}
	}
	// These lines break setting of variables. Are they actually needed here for some reason?
	//  if (sadirActive)
	//    list << "--env:SADIR='" + sadir + "'";
	//  if (ssdirActive)
	//    list << "--env:SSDIR='" + ssdir + "'";
	//  if (sfdirActive)
	//    list << "--env:SFDIR='" + sfdir + "'";
	//  if (incdirActive)
	//    list << "--env:INCDIR='" + incdir + "'";
    opts << "--env:CSNOSTOP=yes";
    qDebug() << ">>>>> Csound options: " << opts;
    return opts;
}

int CsoundOptions::generateCmdLine(char **argv)
{
	int index = 0;
	argv[index] = (char *) calloc(7, sizeof(char));
	strcpy((char *)argv[index++], "csound");
	QStringList indFlags = generateCmdLineFlagsList();
	foreach (QString flag, indFlags) {
		flag = flag.simplified();
		argv[index] = (char *) calloc(flag.size()+1, sizeof(char));
		strcpy(argv[index], flag.toLocal8Bit());
		index++;
	}
	argv[index] = (char *) calloc(fileName1.size()+1, sizeof(char));
	strcpy(argv[index++],fileName1.toLocal8Bit());
	if (fileName2 != "") {
		argv[index] = (char *) calloc(fileName2.size()+1, sizeof(char));
		strcpy(argv[index++],fileName2.toLocal8Bit());
	}
	return index;
}

void CsoundOptions::setJackNameSize(int size)
{
	m_jackNameSize = size;
}
