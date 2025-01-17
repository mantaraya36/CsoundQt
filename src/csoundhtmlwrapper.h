/*
	Copyright (C) 2008-2016 Andres Cabrera
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

#ifndef CsoundHtmlWrapper_H
#define CsoundHtmlWrapper_H

#include <QObject>
#include <QDebug>
#include <thread>
#include "csoundengine.h"

class CsoundHtmlView;

class CsoundHtmlWrapper : public QObject
{
    Q_OBJECT
public:
	explicit CsoundHtmlWrapper(QObject *parent = 0);
    void setCsoundHtmlView(CsoundHtmlView *csoundHtmlView);
public slots:
    CSOUND *getCsound();
    CsoundPerformanceThread *getThread();
    CsoundUserData *getUserData();
    void setCsoundEngine(CsoundEngine *csEngine); // necessay to get CsoundEngine::isPlaying()
    int compileCsd(const QString &filename);
    int compileCsdText(const QString &text);
    int compileOrc(const QString &text);
    double evalCode(const QString &text);
    double get0dBFS();
    //int getApiVersion(); // not present in Csound7
    double getControlChannel(const QString &name);
    qint64 getCurrentTimeSamples();
    QString getEnv(const QString &name);
    int getKsmps();
    int getNchnls();
    int getNchnlsInput();
    QString getOutputName();
    double getScoreOffsetSeconds();
    double getScoreTime();
    int getSr();
    QString getStringChannel(const QString &name);
    int getVersion();
    bool isPlaying();
    int isScorePending();
    void message(const QString &text);
    int perform();
    int readScore(const QString &text);
    void reset();
    void rewindScore();
    int runUtility(const QString &command, int argc, char **argv);
    int scoreEvent(char type, double *pFields, long numFields);
    void setControlChannel(const QString &name, double value);
    int setGlobalEnv(const QString &name, const QString &value);
    void setInput(const QString &name);
    // Dummy for now.
    void setMessageCallback(QObject *callback);
    int setOption(const QString &name);
    void setOutput(const QString &name, const QString &type, const QString &format);
    void setScoreOffsetSeconds(double value);
    void setScorePending(bool value);
    void setStringChannel(const QString &name, const QString &value);
    int start();
    void stop();
    double tableGet(int table_number, int index);
    int tableLength(int table_number);
    void tableSet(int table_number, int index, double value);
public:
    static void csoundMessageCallback_(CSOUND *csound,
                                             int attributes,
                                             const char *format,
                                             va_list args);
    void csoundMessageCallback(int attributes,
                               const char *format,
                               va_list args);
private:
    QString csoundMessageBuffer;
    CsoundHtmlView *csoundHtmlView;
    CsoundEngine *m_csoundEngine;
    QObject *message_callback;
};

#endif // CsoundHtmlWrapper_H
