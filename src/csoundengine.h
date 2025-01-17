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

#ifndef CSOUNDENGINE_H
#define CSOUNDENGINE_H

#include <QStringList>
#include <QTimer>
#include <QFuture>
#include <QAtomicInt>

#include <csound.hpp>
#include <csPerfThread.hpp>
#include <cwindow.h> // Necessary for WINDAT struct
#include <csound_circular_buffer.h> // necessary for csoundCreateCircularBuffer and similar
#include <csound_graph_display.h> // necessary for WINDAT declaration


#ifdef QCS_DEBUGGER
#include "csdebug.h"
#endif

#include "types.h"
#include "csoundoptions.h"
#ifdef QCS_PYTHONQT
#include "pythonconsole.h"
#endif


class ConsoleWidget;
class QuteScope;
class QuteGraph;
class Curve;
class CsoundEngine;
class WidgetLayout;
class MidiHandler;
class QuteWidget;

// Csound 5.10 needs to be destroyed for opcodes like ficlose to flush the output
// This still necessary for 5.12 and Csound6

#define QCS_DESTROY_CSOUND

typedef enum {
	QCS_NO_FLAGS = 0,
	QCS_NO_COPY_BUFFER = 1,
	QCS_NO_PYTHON_CALLBACK = 2,
	QCS_NO_CONSOLE_MESSAGES = 4,
	QCS_NO_RT_EVENTS = 8
} PerfFlags;

struct CsoundUserData {
	int result; //result of csoundCompile()
	CSOUND *csound; // instance of csound
	CsoundPerformanceThread *perfThread;
	CsoundEngine *csEngine; // Pass engine
	WidgetLayout *wl; // Pass widgets
	MidiHandler *midiHandler; // For MIDI out
	QMutex *playMutex; //perfThread access Mutex
	/* performance */
	bool runDispatcher;
	QVector<double> mouseValues;
	RingBuffer audioOutputBuffer;
	bool enableWidgets; // Whether widget values are processed in the callback

	/* current configuration */
	// These should not be changed while Csound is running,
	PerfFlags flags;
	// Store some values to avoid function calls
	MYFLT zerodBFS; //0dBFS value
	int numChnls;
	int sampleRate;
	long outputBufferSize;
	int msgRefreshTime; // In micro seconds

	// Channels are only queried at the start of run, so only channels defined in instr 0 are available
	QList<QString> inputChannelNames;
	QList<QString> outputChannelNames;
	QList<QString> outputStringChannelNames;
	QList<QVariant> previousOutputValues;
	QList<QVariant> previousStringOutputValues;
    QString lastRecordingOutfile;

	void *midiBuffer; //Csound Circular Buffer
	void *virtualMidiBuffer; //Csound Circular Buffer

#ifdef QCS_PYTHONQT
	PythonConsole *m_pythonConsole;
	QString m_pythonCallback;
	int m_pythonCallbackCounter;
	int m_pythonCallbackSkip;
#endif
};


class CsoundEngine : public QObject
{
	Q_OBJECT
public:
	CsoundEngine(ConfigLists *configlists);
	~CsoundEngine();

	static void outputValueCallback (CSOUND *csound,
									 const char *channelName,
									 void *channelValuePtr,
									 const void *channelType);
	static void inputValueCallback (CSOUND *csound,
									const char *channelName,
									void *channelValuePtr,
									const void *channelType);

	static int midiInOpenCb(CSOUND *csound, void **ud, const char *devName);
	static int midiReadCb(CSOUND *csound, void *ud_, unsigned char *buf, int nBytes);
	static int midiInCloseCb(CSOUND *csound, void *ud);
	static int midiOutOpenCb(CSOUND *csound, void **ud, const char *devName);
	static int midiWriteCb(CSOUND *csound, void *ud_, const unsigned char *buf, int nBytes);
	static int midiOutCloseCb(CSOUND *csound, void *ud);
	static const char *midiErrorStringCb(int);
	void queueMidiIn(std::vector<unsigned char> *message);
	void queueVirtualMidiIn(std::vector<unsigned char> &message);
	void sendMidiOut(QVector<unsigned char> &message);

	static void makeGraphCallback(CSOUND *csound, WINDAT *windat, const char *name);
	static void drawGraphCallback(CSOUND *csound, WINDAT *windat);
	static void killGraphCallback(CSOUND *csound, WINDAT *windat);
	static int exitGraphCallback(CSOUND *csound);
	static int keyEventCallback(void *userData,
								void *p,
								unsigned int type);
	static void csThread(void *data);  //Thread function (called after each performance pass by the performance thread)

	static void readWidgetValues(CsoundUserData *ud);
	static void writeWidgetValues(CsoundUserData *ud);

	//    void setCsoundOptions(const CsoundOptions &options);
	// Options unsafe to change while running
	void setWidgetLayout(WidgetLayout *wl);
	void setMidiHandler(MidiHandler *mh);
	// Options safe to change while running
	void enableWidgets(bool enable);

	void registerConsole(ConsoleWidget *c);  // Messages generated by Csound and CsoundQt are passed to the consoles registered here, and nowhere else
	QList<QPair<int, QString> > getErrorLines();
	void setConsoleBufferSize(int size);
	int popKeyPressEvent();
	int popKeyReleaseEvent();

	void processEventQueue();
	void passOutValue(QString channelName, double value);
	void passOutString(QString channelName, QString value);
	void flushQueues();
	void queueMessage(QString message);

	bool isRunning();
	bool isRecording();
	bool isPaused() {return m_paused; }

	// To pass to parent document for access from python scripting
	CSOUND * getCsound();
    CsoundUserData *getUserData();
    void clearConsoles(void);
#ifdef QCS_PYTHONQT
	void registerProcessCallback(QString func, int skipPeriods);
	void setPythonConsole(PythonConsole *pc);
#endif

#ifdef QCS_DEBUGGER
	bool m_debugging;

	static void breakpointCallback(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *udata);
	void setDebug();
	void pauseDebug();
	void continueDebug();
	void stopDebug();
	void addInstrumentBreakpoint(double instr, int skip);
	void removeInstrumentBreakpoint(double instr);
	void addBreakpoint(int line, int instr, int skip);
	void removeBreakpoint(int line, int instr);
	void setStartingBreakpoints(QVector<QVariantList> bps);
	QVector<QVariantList> getVaribleList();
	QVector<QVariantList> getInstrumentList();
	int getCurrentLine();
	QVector<QVariantList> m_varList;
	QVector<QVariantList> m_instrumentList;
	QAtomicInt m_currentLine;
	QMutex variableMutex;
	QMutex instrumentMutex;
	QVector<QVariantList> m_startBreakpoints;
#endif

public slots:
	int play(CsoundOptions *options);
	void stop();
	void pause();
	int startRecording(int format, QString filename);
	void stopRecording();
	void queueEvent(QString eventLine, int delay = 0);
	void keyPressForCsound(int key);  // For key press events from consoles and widget panel
	void keyReleaseForCsound(int key);

	void registerScope(QuteScope *scope);
	//    void unregisterScope(QuteScope *scope);
	void registerGraph(QuteGraph *scope);
    void requestCsoundUserData(QuteWidget *widget);
	void setFlags(PerfFlags flags) {ud->flags = flags;}

	void evaluate(QString code);

public:
    QVector<ConsoleWidget *> consoles;  // Consoles registered for message printing
    int runCsound();
	void stopCsound();
	void cleanupCsound();
    int checkSyntax();

private:
	void setupChannels();
	QList <int> getAnsiKeySequence(int key);

	QFuture<void> m_msgUpdateThread;
	static void messageListDispatcher(void *data); // Function run in updater thread

	CsoundUserData *ud;

	CsoundOptions m_options;

	int m_consoleBufferSize;
	QMutex m_messageMutex; // Protection for message queue
	QStringList messageQueue;  // Messages from Csound execution
	QMutex keyMutex; // For keys pressed to pass to Csound from console and widget panel
	QList <int> keyPressBuffer; // protected by keyMutex
	QList <int> keyReleaseBuffer; // protected by keyMutex

	bool m_recording;
	bool m_paused;
    // To prevent from starting a Csound instance while another is starting or closing
    QMutex m_playMutex;
	QMutex eventMutex;
    QMutex csoundMutex;
	QVector<QString> eventQueue;
	int m_refreshTime; // time in milliseconds for widget value updates (both input and output)
	QVector<unsigned long> eventTimeStamps;
	int eventQueueSize;

private slots:

signals:
	void errorLines(QList<QPair<int, QString> >);
	void passMessages(QString msg);
	void stopSignal(); // Sent when performance has stopped internally to inform others.playFromParent()
	void breakpointReached();
};

#endif // CSOUNDENGINE_H
