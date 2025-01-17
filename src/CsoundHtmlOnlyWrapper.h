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

#ifndef CsoundHtmlOnlyWrapper_H
#define CsoundHtmlOnlyWrapper_H

// csound7 : this class need to be rewritten to use Csound class in QThread or csoundPerformanceThread
// to get rid of csound_threaded.hpp that is not ported to csound7

#include <QObject>
#include <QDebug>
#include "csound_threaded_csqt.hpp"
//#include <csound.h>
//#include <csound.hpp> // temporaray substitution
//#include <csPerfThread.h>
#include "csoundengine.h"

class CsoundHtmlView;

class CsoundHtmlOnlyWrapper : public QObject
{
    Q_OBJECT
public:
	explicit CsoundHtmlOnlyWrapper(QObject *parent = 0);
    virtual ~CsoundHtmlOnlyWrapper();
    void setCsoundHtmlView(CsoundHtmlView *csoundHtmlView);
public slots:
    void registerConsole(ConsoleWidget *console);
    int compileCsd(const QString &filename);
	// Uppercase aliases to comply with Webaudio Csound, see https://vlazzarini.github.io/paw/examples/docs/Csound.html
	// NB! midi related methods not implemented yet
	int CompileCsd(const QString &filename) { return compileCsd(filename);}
    int compileCsdText(const QString &text);
	int CompileCsdText(const QString &text) { return compileCsdText(text);}
    int compileOrc(const QString &text);
    int CompileOrc(const QString &text) { return compileOrc(text);}
    double evalCode(const QString &text);
    double EvalCode(const QString &text) { return evalCode(text); }
    double get0dBFS();
	double Get0dBFS() {return get0dBFS();}
    //int getApiVersion(); // not present in Csound7 any more
    double getControlChannel(const QString &name);
	//double RequestChannel(const QString &name) { return getControlChannel(const QString &name)  } // NB! this works differently, better do not provide this alias!
    qint64 getCurrentTimeSamples();
    QString getEnv(const QString &name);
    int getKsmps();
    int getNchnls();
    int getNchnlsInput();
    QString getOutputName();
    double getScoreOffsetSeconds();
    double getScoreTime();
	double GetScoreTime() {return getScoreTime();}
    int getSr();
    QString getStringChannel(const QString &name);
    int getVersion();
    bool isPlaying();
    int isScorePending();
    void message(const QString &text);
    int perform();
    int readScore(const QString &text);
	int ReadScore(const QString &text) {return readScore(text);}
	int Event(const QString &text) {return readScore(text);}
    void reset();
	void Reset() {reset();}
    void rewindScore();
    int runUtility(const QString &command, int argc, char **argv);
    int scoreEvent(char type, const double *pFields, long numFields);
    void setControlChannel(const QString &name, double value);
	void SetChannel(const QString &name, double value) {setControlChannel(name, value);}
    int setGlobalEnv(const QString &name, const QString &value);

    //csound 7 comment out// void setInput(const QString &name);
    // Dummy for now.
    void setMessageCallback(QObject *callback);
    int setOption(const QString &name);
    //csound7 // void setOutput(const QString &name, const QString &type, const QString &format);
    void setScoreOffsetSeconds(double value);
    void setScorePending(bool value);
    void setStringChannel(const QString &name, const QString &value);
	void SetStringChannel(const QString &name, const QString &value) { setStringChannel(name, value); }
    int start();
	int Start(); // NB! Start() does not require have to call for perform separately!
	void Play() {start(); perform();}
	void PlayCsd(const QString &csd) { compileCsd(csd); Play(); } // untested
    void stop();
	void Stop() { stop();}
	//TODO: void Pause();
    double tableGet(int table_number, int index);
    int tableLength(int table_number);
    void tableSet(int table_number, int index, double value);
	void SetTable(int table_number, int index, double value) { tableSet(table_number, index, value); }
    void setOptions(CsoundOptions * options);
signals:
    void passMessages(QString message);
private:
    static void csoundMessageCallback_(CSOUND *csound,
                                             int attributes,
                                             const char *format,
                                             va_list args);
    void csoundMessageCallback(int attributes,
                               const char *format,
                               va_list args);
    QObject *message_callback;
    ConsoleWidget *console;
    CsoundThreaded csound;
    //Csound csound;
    CsoundHtmlView *csoundHtmlView;
private:
    CsoundOptions *m_options;
    QString csoundMessageBuffer;
};

#endif // CsoundHtmlWrapper_H
