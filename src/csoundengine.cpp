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


#include <QtConcurrent>
#include <QThread>

#ifdef Q_OS_WIN
#include <ole2.h> // for OleInitialize() FLTK bug workaround
#endif

#include <csound_type_system.h>

#include "csoundengine.h"
#include "widgetlayout.h"
#include "console.h"
#include "qutescope.h"  // Needed for passing the ud to the scope for display data
#include "qutegraph.h"  // Needed for passing the ud to the graph for display data
#include "midihandler.h"

// #define QDEBUG qDebug() << __FUNCTION__ << ":"

CsoundEngine::CsoundEngine(ConfigLists *configlists) :
    m_options(configlists)
{
    QMutexLocker locker(&m_playMutex);
    ud = new CsoundUserData();
    ud->csEngine = this;
    ud->csound = nullptr;
    ud->perfThread = nullptr;
    ud->flags = QCS_NO_FLAGS;
    ud->mouseValues.resize(6); // For _MouseX _MouseY _MouseRelX _MouseRelY _MouseBut1 and _MouseBut2 channels
    ud->wl = nullptr;
    ud->midiBuffer = nullptr;
    ud->virtualMidiBuffer = nullptr;
    ud->playMutex = &m_playMutex;
#ifdef QCS_PYTHONQT
    ud->m_pythonCallback = "";
#endif
    m_consoleBufferSize = 0;
    m_recording = false;
#ifndef QCS_DESTROY_CSOUND
    ud->csound=csoundCreate( (void *) ud, nullptr);
    ud->midiBuffer = csoundCreateCircularBuffer(ud->csound, 1024, sizeof(unsigned char));
    Q_ASSERT(ud->midiBuffer);
#endif
    eventQueue.resize(QCS_MAX_EVENTS);
    eventTimeStamps.resize(QCS_MAX_EVENTS);
    eventQueueSize = 0;
    m_refreshTime = QCS_QUEUETIMER_DEFAULT_TIME;  // TODO Eventually allow this to be changed
    ud->msgRefreshTime = m_refreshTime*1000;
    ud->runDispatcher = true;
    m_msgUpdateThread = QtConcurrent::run(messageListDispatcher, (void *) ud);
#ifdef QCS_DEBUGGER
    m_debugging = false;
#endif
}

CsoundEngine::~CsoundEngine()
{
    disconnect(SIGNAL(passMessages(QString)),0,0);
    disconnect(this, 0,0,0);
    ud->runDispatcher = false;
    m_msgUpdateThread.waitForFinished(); // Join the message thread
    stop();
#ifndef QCS_DESTROY_CSOUND
    //csoundDestroyCircularBuffer(ud->csound, ud->midiBuffer); // CS7 circular buffer destroyed in csoundDestroy
    csoundDestroy(ud->csound);
#endif
    delete ud;
}


void CsoundEngine::outputValueCallback (CSOUND *csound,
                                        const char *channelName,
                                        void *channelValuePtr,
                                        const void *channelType)
{
    // Called by the csound running engine when 'outvalue' opcode is used
    // To pass data from Csound to CsoundQt
    CS_TYPE const *csChannelType = (CS_TYPE *) channelType;
    QString channelTypeName = QString(csChannelType->varTypeName).toLower();
    // OR: use new function in Csound API:  PUBLIC const char *csoundGetChannelVarType(CSOUND *csound, const char *name);

    CsoundUserData *ud = (CsoundUserData *) csoundGetHostData(csound);
    if (channelTypeName == "s") {
        ud->csEngine->passOutString(channelName, (const char *) channelValuePtr);
    }
    else if (channelTypeName == "k"){
        ud->csEngine->passOutValue(channelName, *((MYFLT *)channelValuePtr));
    } else {
        QDEBUG << "Unsupported type";
    }
}

void CsoundEngine::inputValueCallback (CSOUND *csound,
                                       const char *channelName,
                                       void *channelValuePtr,
                                       const void *channelType)
{
    // Called by the csound running engine when 'invalue' opcode is used
    // To pass data from CsoundQt to Csound

    CS_TYPE const *csChannelType = (CS_TYPE *) channelType;
    QString channelTypeName = QString(csChannelType->varTypeName).toLower();
    CsoundUserData *ud = (CsoundUserData *) csoundGetHostData(csound);
    if (channelTypeName == "s") { // channel is a string channel
        char *string = (char *) channelValuePtr;
        QString newValue = ud->wl->getStringForChannel(channelName);
        int maxlen = csoundGetChannelDatasize(csound, channelName);
        if (newValue.size() > 0) {
            strncpy(string, newValue.toLocal8Bit(), maxlen - 1);
            string[newValue.size()] = '\0';
        } else {
            string[0] = '\0';
        }
    }
    else if (channelTypeName == "k") {  // Not a string channel
        //FIXME check if mouse tracking is active, and move this from here
        MYFLT *value = (MYFLT *) channelValuePtr;
        if(!strcmp(channelName, "_Mouse")) {
            const char *suffix = &channelName[6];
            if (!strcmp(suffix, "X")) {
                *value = (MYFLT) ud->mouseValues[0];
            }
            else if (!strcmp(suffix, "Y")) {
                *value = (MYFLT) ud->mouseValues[1];
            }
            else if(!strcmp(suffix, "RelX")) {
                *value = (MYFLT) ud->mouseValues[2];
            }
            else if(!strcmp(suffix, "RelY")) {
                *value = (MYFLT) ud->mouseValues[3];
            }
            else if(!strcmp(suffix, "But1")) {
                *value = (MYFLT) ud->mouseValues[4];
            }
            else if(!strcmp(suffix, "But2")) {
                *value = (MYFLT) ud->mouseValues[5];
            }
        }
        else {
            // QString name(channelName);
            *value = (MYFLT) ud->wl->getValueForChannel(channelName);
        }
    } else {
        QDEBUG << "Unsupported type";
    }
}

int CsoundEngine::midiInOpenCb(CSOUND *csound, void **ud, const char *devName)
{
    CsoundUserData *userData = (CsoundUserData *) csoundGetHostData(csound);
    Q_UNUSED(devName);
    Q_ASSERT(userData->midiBuffer);
    if (userData) {
        *ud = userData;
    } else {
        qDebug()  << "Error! userData not set for midiInOpenCb";
        return CSOUND_ERROR;
    }
    return CSOUND_SUCCESS;
}

int CsoundEngine::midiReadCb(CSOUND *csound, void *ud_, unsigned char *buf, int nBytes)
{
    CsoundUserData *ud = (CsoundUserData *) ud_;
    Q_UNUSED(csound);
    int count, countVirtual;
    count = csoundReadCircularBuffer(ud->csound, ud->midiBuffer, buf, nBytes);
    countVirtual = csoundReadCircularBuffer(ud->csound, ud->virtualMidiBuffer, buf + count, nBytes - count);
    return count + countVirtual;
}

int CsoundEngine::midiInCloseCb(CSOUND *csound, void *ud)
{
    Q_UNUSED(csound);
    Q_UNUSED(ud);
    return CSOUND_SUCCESS;
}

int CsoundEngine::midiOutOpenCb(CSOUND *csound, void **ud, const char *devName)
{
    CsoundUserData *userData = (CsoundUserData *) csoundGetHostData(csound);
    Q_UNUSED(devName);
    Q_ASSERT(userData->midiBuffer);
    if (userData) {
        *ud = userData;
    } else {
        QDEBUG  << "Error! userData not set for midiInOpenCb";
        return CSOUND_ERROR;
    }
    return CSOUND_SUCCESS;

}

int CsoundEngine::midiWriteCb(CSOUND *csound, void *ud_, const unsigned char *buf, int nBytes)
{
    CsoundUserData *userData = (CsoundUserData *) ud_;
    std::vector<unsigned char> message;
    Q_UNUSED(csound);
    message.push_back(*buf++);
    nBytes--;
    while (nBytes--) {
        if (*buf & 0x80) {
            userData->midiHandler->sendMidiOut(&message);
            message.clear();
        }
        message.push_back(*buf++);
    }
    if (message.size() > 0) {
        userData->midiHandler->sendMidiOut(&message);
    }
    return 0;
}

int CsoundEngine::midiOutCloseCb(CSOUND *csound, void *ud)
{
    (void) csound;
    (void) ud;
    return 0;
}

const char *CsoundEngine::midiErrorStringCb(int)
{

    return nullptr;
}

void CsoundEngine::queueMidiIn(std::vector< unsigned char > *message)
{
    if (ud->midiBuffer) {
        csoundWriteCircularBuffer(ud->csound, ud->midiBuffer, message->data(), message->size()* sizeof(unsigned char));
    }
}

void CsoundEngine::queueVirtualMidiIn(std::vector< unsigned char > &message)
{
    if (ud->virtualMidiBuffer) {
        csoundWriteCircularBuffer(ud->csound, ud->virtualMidiBuffer, message.data(), message.size()* sizeof(unsigned char));
    }
}

void CsoundEngine::sendMidiOut(QVector<unsigned char> &message)
{
    (void) message;
}


void CsoundEngine::makeGraphCallback(CSOUND *csound, WINDAT *windat, const char * /*name*/)
{
    CsoundUserData *ud = (CsoundUserData *) csoundGetHostData(csound);
    // Csound reuses windat, so it is not guaranteed to be unique
    // name seems not to be used.
    ud->wl->appendCurve(windat);
}

void CsoundEngine::drawGraphCallback(CSOUND *csound, WINDAT *windat)
{
    CsoundUserData *udata = (CsoundUserData *) csoundGetHostData(csound);
    // This callback paints data on curves
    udata->wl->updateCurve(windat);
}

void CsoundEngine::killGraphCallback(CSOUND *csound, WINDAT *windat)
{
    // When is this callback called??
    CsoundUserData *udata = (CsoundUserData *) csoundGetHostData(csound);
    udata->wl->killCurve(windat);
}

int CsoundEngine::exitGraphCallback(CSOUND *csound)
{
    CsoundUserData *udata = (CsoundUserData *) csoundGetHostData(csound);
    return udata->wl->killCurves(csound);
}

int CsoundEngine::keyEventCallback(void *userData,
                                   void *p,
                                   unsigned int type)
{
    if (type != CSOUND_CALLBACK_KBD_EVENT && type != CSOUND_CALLBACK_KBD_TEXT) {
        return 1;
    }
    CsoundUserData *ud = (CsoundUserData *) userData;
    //  WidgetLayout *wl = (WidgetLayout *) ud->wl;
    int *value = (int *) p;
	int key = ud->csEngine->popKeyPressEvent();
	if (key >= 0) {
		*value = key;
		qDebug()  << "Pressed: " << key;
    }
    else if (type & CSOUND_CALLBACK_KBD_EVENT) {
        key = ud->csEngine->popKeyReleaseEvent();
        if (key >= 0) {
            *value = key | 0x10000;
			qDebug()  << "Released: " << key;
        }
    }
    return 0;
}

void CsoundEngine::csThread(void *data)
{
    CsoundUserData* udata = (CsoundUserData*)data;
    if (!(udata->flags & QCS_NO_COPY_BUFFER)) {
        MYFLT const *outputBuffer = csoundGetSpout(udata->csound);
        // outputBufferSize == ksmps
        long numSamples = udata->outputBufferSize * udata->numChnls;
        udata->audioOutputBuffer.putManyScaled(outputBuffer, numSamples,
                                               1.0/udata->zerodBFS);
        // for (int i = 0; i < udata->outputBufferSize*udata->numChnls; i++) {
        //     udata->audioOutputBuffer.put(outputBuffer[i]/ udata->zerodBFS);
        // }
    }
    //  udata->wl->getValues(&udata->channelNames,
    //                       &udata->values,
    //                       &udata->stringValues);
    if (udata->enableWidgets) {
        //        csoundDeleteChannelList(udata->csound, *channelList);
        writeWidgetValues(udata);
        readWidgetValues(udata);
    }
    if (!(udata->flags & QCS_NO_RT_EVENTS)) {
        udata->csEngine->processEventQueue();
    }
#ifdef QCS_PYTHONQT
    if (!(udata->flags & QCS_NO_PYTHON_CALLBACK)) {
        if (!udata->m_pythonCallback.isEmpty()) {
            if (udata->m_pythonCallbackCounter >= udata->m_pythonCallbackSkip) {
                udata->m_pythonConsole->evaluate(udata->m_pythonCallback, false);
                udata->m_pythonCallbackCounter = 0;
            }
            else {
                udata->m_pythonCallbackCounter++;
            }
        }
    }
#endif
}

void CsoundEngine::readWidgetValues(CsoundUserData *ud)
{
    MYFLT* pvalue;

    if (ud->wl->valueMutex.tryLock()) {
        QHash<QString, double>::const_iterator i;
        QHash<QString, double>::const_iterator end = ud->wl->newValues.constEnd();
        for (i = ud->wl->newValues.constBegin(); i != end; ++i) {
            if(csoundGetChannelPtr(ud->csound, (void **)&pvalue, i.key().toLocal8Bit().constData(),
                                   CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL) == 0) {
                // 0 == success
                *pvalue = (MYFLT) i.value();
            }
        }
        ud->wl->newValues.clear();
        ud->wl->valueMutex.unlock();
    }
    if (ud->wl->stringValueMutex.tryLock()) {
        QHash<QString, QString>::const_iterator i;
        QHash<QString, QString>::const_iterator end = ud->wl->newStringValues.constEnd();
        for (i = ud->wl->newStringValues.constBegin(); i != end; ++i) {
            csoundSetStringChannel(ud->csound, i.key().toLocal8Bit().data(),
                                   i.value().toLocal8Bit().data());
        }
        ud->wl->newStringValues.clear();
        ud->wl->stringValueMutex.unlock();
    }
}

void CsoundEngine::writeWidgetValues(CsoundUserData *ud)
{
    MYFLT* pvalue;
    for (int i = 0; i < ud->outputChannelNames.size(); i++) {
        if (ud->outputChannelNames[i] != ""
                && csoundGetChannelPtr(ud->csound, (void **) &pvalue,
                                       ud->outputChannelNames[i].toLocal8Bit().constData(),
                                       CSOUND_OUTPUT_CHANNEL | CSOUND_CONTROL_CHANNEL) == 0) {
            if(ud->previousOutputValues[i] != *pvalue) {
                ud->wl->setValue(ud->outputChannelNames[i],*pvalue);
                ud->previousOutputValues[i] = *pvalue;
            }
        }
    }
    for (int i = 0; i < ud->outputStringChannelNames.size(); i++) {
        if (ud->outputStringChannelNames[i] != ""
                && csoundGetChannelPtr(ud->csound, (void **) &pvalue,
                                       ud->outputStringChannelNames[i].toLocal8Bit().constData(),
                                       CSOUND_OUTPUT_CHANNEL | CSOUND_STRING_CHANNEL) == 0) {
            char chanString[2048]; // large enough for long strings in displays
            csoundGetStringChannel(ud->csound, ud->outputStringChannelNames[i].toLocal8Bit().constData(),
                                   chanString);
            if(ud->previousStringOutputValues[i] != QString(chanString)) {
                ud->wl->setValue(ud->outputStringChannelNames[i],QString(chanString));
                ud->previousStringOutputValues[i] = QString(chanString);
            }
        }
    }
}

void CsoundEngine::setWidgetLayout(WidgetLayout *wl)
{
    ud->wl = wl;
    //  connect(wl, SIGNAL(destroyed()), this, SLOT(widgetLayoutDestroyed()));
    // Key presses on widget layout and console are passed to the engine
	connect(wl, SIGNAL(keyPressed(int)),
			this, SLOT(keyPressForCsound(int)));
	connect(wl, SIGNAL(keyReleased(int)),
			this, SLOT(keyReleaseForCsound(int)));

    // Register scopes and graphs to pass them the engine's user data
    connect(wl, SIGNAL(registerScope(QuteScope*)),
            this,SLOT(registerScope(QuteScope*)));
    connect(wl, SIGNAL(registerGraph(QuteGraph*)),
            this,SLOT(registerGraph(QuteGraph*)));
    connect(wl, SIGNAL(requestCsoundUserData(QuteWidget*)),
            this, SLOT(requestCsoundUserData(QuteWidget*)));
    connect(this, SIGNAL(passMessages(QString)), wl, SLOT(appendMessage(QString)), Qt::UniqueConnection);
}

void CsoundEngine::setMidiHandler(MidiHandler *mh)
{
    ud->midiHandler = mh;
}

void CsoundEngine::enableWidgets(bool enable)
{
    ud->enableWidgets = enable;
}

void CsoundEngine::registerConsole(ConsoleWidget *c)
{
    consoles.append(c);
    connect(this,SIGNAL(passMessages(QString)), c, SLOT(appendMessage(QString)), Qt::UniqueConnection);
}

QList<QPair<int, QString> > CsoundEngine::getErrorLines()
{
    QList<QPair<int, QString> > list;
    if (consoles.size() > 0) {
        QList<int> lines = consoles[0]->errorLines;
        QStringList texts = consoles[0]->errorTexts;
        for (int i = 0; i < lines.size(); i++) {
            list.append(QPair<int, QString>(lines[i], texts[i]));
        }
    }
    return list;
}

void CsoundEngine::setConsoleBufferSize(int size)
{
    m_consoleBufferSize = size;
}

QList <int> CsoundEngine::getAnsiKeySequence(int key)  // convert sepcial keys (Qt::Key) like Esc, arrows, F1 etc to ANSI escape key sequence for Csound
{
    QList <int> keyArray;
	if (key < 0xff) {
		keyArray << key;
	} else switch (key) {
		case Qt::Key_Escape: keyArray << 27; break;
		case Qt::Key_Tab: keyArray << 9; break;
		case Qt::Key_Backspace: keyArray << 127; break;
		case Qt::Key_Up: keyArray << 27 << 91 << 65; break;
		case Qt::Key_Down: keyArray << 27 << 91 << 66; break;
		case Qt::Key_Right: keyArray << 27 << 91 << 67; break;
		case Qt::Key_Left: keyArray << 27 << 91 << 68; break;
		case Qt::Key_Home: keyArray << 27 << 91 << 72; break;
		case Qt::Key_End: keyArray << 27 << 91 << 70; break;
		case Qt::Key_PageUp: keyArray << 27 << 53 << 126; break;
		case Qt::Key_PageDown: keyArray << 27 << 52 << 126; break;
		case Qt::Key_F1: keyArray << 27 << 91 << 80; break;
		case Qt::Key_F2: keyArray << 27 << 79 << 81; break;
		case Qt::Key_F3: keyArray << 27 << 79 << 82; break;
		case Qt::Key_F4: keyArray << 27 << 79 << 83; break;
		case Qt::Key_F5: keyArray << 27 << 91 << 49 << 53 << 126; break;
		case Qt::Key_F6: keyArray << 27 << 91 << 49 << 55 << 126; break;
		case Qt::Key_F7: keyArray << 27 << 91 << 49 << 56 << 126; break;
		case Qt::Key_F8: keyArray << 27 << 91 << 49 << 57 << 126; break;
		case Qt::Key_F9: keyArray << 27 << 91 << 50 << 48 << 126; break;
		case Qt::Key_F10: keyArray << 27 << 91 << 50 << 49 << 126; break;
		case Qt::Key_F11: keyArray << 27 << 91 << 50 << 51 << 126; break;
		case Qt::Key_F12: keyArray << 27 << 91 << 50 << 52 << 126; break;
		case Qt::Key_Insert: keyArray << 27 << 91 << 50 << 126; break;
		case Qt::Key_Delete: keyArray << 27 << 91 << 51 << 126; break;

        default: qDebug()<<"Key " << key << " ignored."; //keyArray << key;
	}
	return keyArray;
}

void CsoundEngine::keyPressForCsound(int key)
{
    keyMutex.lock();
	keyPressBuffer << getAnsiKeySequence(key);
	keyMutex.unlock();
}

void CsoundEngine::keyReleaseForCsound(int key) // NB! I did not change this to int since seems Csound actually does not use it?
{
    keyMutex.lock();
    keyReleaseBuffer << key;
    keyMutex.unlock();
}

void CsoundEngine::requestCsoundUserData(QuteWidget *widget) {
    widget->setCsoundUserData(ud);
}

void CsoundEngine::registerScope(QuteScope *scope)
{
    scope->setUd(ud);
}

void CsoundEngine::registerGraph(QuteGraph *graph)
{
    graph->setUd(ud);
}

void CsoundEngine::evaluate(QString code)
{
    CSOUND *csound = getCsound();
    if (csound) {
        csoundCompileOrc(csound, code.toLatin1(), 0); // or should the last parameter be 1 fo async?
        queueMessage(tr("Csound code evaluated.\n"));
    } else {
        queueMessage(tr("Csound is not running. Code not evaluated."));
    }
}

//void CsoundEngine::unregisterScope(QuteScope *scope)
//{
//  // TODO is it necessary to unregiter scopes?
//  qDebug()  << "Not implemented";
//}

int CsoundEngine::popKeyPressEvent()
{
    int value = -1;
	if (keyMutex.tryLock()) {
        if (!keyPressBuffer.isEmpty()) {
			value = keyPressBuffer.takeFirst();
		}

        keyMutex.unlock();
    }
	return value;
}

int CsoundEngine::popKeyReleaseEvent()
{
    int value = -1;
    if (keyMutex.tryLock()) {
        if (!keyReleaseBuffer.isEmpty()) {
			value = keyReleaseBuffer.takeFirst();
        }
        keyMutex.unlock();
    }
    return value;
}

void CsoundEngine::processEventQueue()
{
    // This function should only be called when Csound is running
    eventMutex.lock();
    while (eventQueueSize > 0) {
        eventQueueSize--;
        m_playMutex.lock();
#ifdef QCS_DESTROY_CSOUND
        if (ud->perfThread) {
            //ScoreEvent is not working
            //      ud->perfThread->ScoreEvent(0, type, eventElements.size(), pFields);
            //      qDebug()  << eventQueue[eventQueueSize];
            ud->perfThread->InputMessage(eventQueue[eventQueueSize].toLatin1());
        }
        else {
            QDEBUG << "WARNING: ud->perfThread is NULL";
        }
#else
        if (ud->perfThread) {
            ud->perfThread->InputMessage(eventQueue[eventQueueSize].toLatin1());
        }
#endif
        m_playMutex.unlock();
    }
    eventMutex.unlock();
}

void CsoundEngine::passOutValue(QString channelName, double value)
{
    ud->wl->newValue(QPair<QString, double>(channelName, value));
}

void CsoundEngine::passOutString(QString channelName, QString value)
{
    //   qDebug() ;
    ud->wl->newValue(QPair<QString, QString>(channelName, value));
}

int CsoundEngine::play(CsoundOptions *options)
{
    QMutexLocker locker(&m_playMutex);
    if (ud->perfThread && (ud->perfThread->GetStatus() == 0)) {
        // GetStatus == 0 means playing
        // ud->perfThread->TogglePause(); // no need for that when there is Pause button
        QDEBUG << "Already playing";
		return 0;
    }
    if (options) {
        m_options = *options;
    }
    locker.unlock();
    if(options->checkSyntaxOnly) {
        return checkSyntax();
    }
    if(options->checkSyntaxBeforeRun) {
        QDEBUG << "Checking Syntax";
        int ret = checkSyntax();
        if(ret != 0) {
            QDEBUG << "Syntax check failed with return code:" << ret;
            return ret;
        }
        QDEBUG << "Syntax check ok, running csound";
    }
    return runCsound();
}

void CsoundEngine::stop()
{
    stopRecording();
    stopCsound();
}

void CsoundEngine::pause()
{
    QMutexLocker locker(&m_playMutex);
    if (ud->perfThread && (ud->perfThread->GetStatus() == 0))  {		
		//ud->perfThread->Pause();
		ud->perfThread->TogglePause();
		m_paused = !m_paused;
		qDebug() << "Paused: " << m_paused;
    }
}

int CsoundEngine::startRecording(int sampleformat, QString fileName)
{
    qDebug("start recording (%i-bit samples): %s",
           (sampleformat + 2) * 8,
           fileName.toLocal8Bit().constData());
	m_paused = false;
    // clip instead of wrap when converting floats to ints
#ifdef PERFTHREAD_RECORD
    // perfthread record API is only available with csound >= 6.04
    if (ud->perfThread) {
        ud->lastRecordingOutfile = fileName;
        m_recording = true;
        ud->perfThread->Record(fileName.toLocal8Bit().constData(),
                               (sampleformat + 2) * 8);
    }
#endif
    return 0;
}

void CsoundEngine::stopRecording()
{
    m_recording = false;
	m_paused = false;

#ifdef	PERFTHREAD_RECORD
    if (ud->perfThread) {
        ud->perfThread->StopRecord();            
    }
    //qDebug("Recording stopped.");
#endif
}

void CsoundEngine::queueEvent(QString eventLine, int delay)
{
    // TODO: implement delayed events
    (void) delay;
    //   qDebug("CsoundEngine::queueEvent %s", eventLine.toStdString().c_str());
    if (!isRunning()) {
        QMutexLocker lock(&m_messageMutex);
        messageQueue << tr("Csound is not running! Event ignored.\n");
        return;
    }
    if (eventQueueSize < QCS_MAX_EVENTS) {
        eventMutex.lock();
        eventQueue[eventQueueSize] = eventLine;
        eventQueueSize++;
        eventMutex.unlock();
    }
    else {
        qDebug("Warning: event queue full, event not processed");
    }
}

int CsoundEngine::checkSyntax() {
    QDEBUG << "$$$ checkSyntax 0";
    QMutexLocker locker(&m_playMutex);
    QDEBUG << "$$$ checkSyntax 1";

    CsoundOptions options(m_options);
    options.checkSyntaxOnly = true;

    ud->csound = csoundCreate((void *) ud, nullptr);
    QDEBUG << "$$$ checkSyntax 2";

    eventQueueSize = 0;
    ud->msgRefreshTime = m_refreshTime*1000;
    QDir::setCurrent(m_options.fileName1);
    for (int i = 0; i < consoles.size(); i++) {
        consoles[i]->reset();
    }
    csoundCreateMessageBuffer(ud->csound, 0);

    char const **argv;// since there was change in Csound API
    argv = (const char **) calloc(33, sizeof(char*));

    int argc = options.generateCmdLine((char **)argv);

    ud->result = csoundCompile(ud->csound, argc, argv);

    for (int i = 0; i < argc; i++) {
        qDebug()  << argv[i];
        free((char *) argv[i]);
    }
    free(argv);
    int out;
    if (ud->result != 256) {
        qDebug()  << "Csound syntax check failed! "  << ud->result;
        flushQueues();
        locker.unlock(); // otherwise csoundStop will freeze
        stop();
        emit (errorLines(getErrorLines()));

        out = -3;  // compilation error
    } else {
        // this might still have failed...
        QDEBUG << "Syntax check ok, return code: " << ud->result;
        out = 0;   // OK
    }
    // csoundDestroyMessageBuffer(ud->csound);
    // csoundCleanup(ud->csound);
    // csoundReset(ud->csound);
    return out;
}


int CsoundEngine::runCsound()
{
    QMutexLocker locker(&m_playMutex);
#ifdef MACOSX_PRE_SNOW
    // Remember menu bar to set it after FLTK grabs it
    menuBarHandle = GetMenuBar();
#endif
#ifdef Q_OS_WIN
	// Call OleInitialize twice to keep the FLTK opcodes from reducing the COM
	// reference count to zero.
    // OleInitialize(NULL); // Do not initialize here but in CsoundQt onbject
    // OleInitialize(NULL);
#endif
    eventQueueSize = 0;
    // Flush events gathered while idle.
    ud->audioOutputBuffer.allZero();
    ud->msgRefreshTime = m_refreshTime*1000;
    QDir::setCurrent(m_options.fileName1);
    for (int i = 0; i < consoles.size(); i++) {
        consoles[i]->reset();
    }
#ifdef QCS_DESTROY_CSOUND
    ud->csound=csoundCreate((void *) ud, nullptr);
    ud->midiBuffer = csoundCreateCircularBuffer(ud->csound, 1024, sizeof(unsigned char));
    Q_ASSERT(ud->midiBuffer);
    ud->virtualMidiBuffer = csoundCreateCircularBuffer(ud->csound, 1024, sizeof(unsigned char));
    Q_ASSERT(ud->virtualMidiBuffer);
    csoundFlushCircularBuffer(ud->csound, ud->midiBuffer);
#endif
#ifdef QCS_DEBUGGER
    if(m_debugging) {
        csoundDebuggerInit(ud->csound);
        foreach(QVariantList bp, m_startBreakpoints) {
            Q_ASSERT(bp.size() > 1);
            if (bp[0].toString() == "instr") {
                Q_ASSERT(bp.size() == 3);
                csoundSetInstrumentBreakpoint(ud->csound, bp[1].toDouble(), bp[2].toInt());
            } else if (bp[0].toString() == "line") {
                Q_ASSERT(bp.size() == 4);
                csoundSetBreakpoint(ud->csound, bp[2].toInt(), bp[1].toInt(), bp[3].toInt());
            } else {
                qDebug()  << "Wrong breakpoint format";
            }
        }
        csoundSetBreakpointCallback(ud->csound, &CsoundEngine::breakpointCallback, (void *) this);
    }
#endif
    if(!m_options.useCsoundMidi) {
        // CS7
        // csoundSetHostImplementedMIDIIO(ud->csound, 1);
        csoundSetHostMIDIIO(ud->csound);

        csoundSetExternalMidiInOpenCallback(ud->csound, &midiInOpenCb);
        csoundSetExternalMidiReadCallback(ud->csound, &midiReadCb);
        csoundSetExternalMidiInCloseCallback(ud->csound, &midiInCloseCb);
        csoundSetExternalMidiOutOpenCallback(ud->csound, &midiOutOpenCb);
        csoundSetExternalMidiWriteCallback(ud->csound, &midiWriteCb);
        csoundSetExternalMidiOutCloseCallback(ud->csound, &midiOutCloseCb);
        csoundSetExternalMidiErrorStringCallback(ud->csound, &midiErrorStringCb);
    }
    csoundCreateMessageBuffer(ud->csound, 0);

    // CS7 - maybe better ditch FLTK support
    /*
    if (m_options.enableFLTK) {
        // Disable FLTK graphs, but allow FLTK widgets.
        int *var = (int*) csoundQueryGlobalVariable(ud->csound, "FLTK_Flags");
        // use Qt equivalent
        if (var) {
            *var = 4;
        } else {
            if (csoundCreateGlobalVariable(ud->csound, "FLTK_Flags", sizeof(int)) != CSOUND_SUCCESS) {
                qDebug() << "Error creating the FTLK_Flags variable";
            }  else {
                int *var = (int*) csoundQueryGlobalVariable(ud->csound, "FLTK_Flags");
                if (var) {
                    *var = 4;
                } else {
                    qDebug() << "Error reading the FTLK_Flags variable";
                }
            }
        }
    }
    else {
        csoundSetGlobalEnv("CS_OMIT_LIBS", "fluidOpcodes,virtual,widgets");
        int *var = (int*) csoundQueryGlobalVariable(ud->csound, "FLTK_Flags");
        if (var) {
            *var = 3;
        } else {
            qDebug() << "Error reading the FTLK_Flags variable";
        }
    }
    */

    csoundRegisterKeyboardCallback(ud->csound,
                                   &CsoundEngine::keyEventCallback,
                                   (void *) ud, CSOUND_CALLBACK_KBD_EVENT | CSOUND_CALLBACK_KBD_TEXT);
    // necessary to put something into the buffer, otherwise sensekey complains when
    // not started from terminal
    csoundKeyPress(getCsound(),'\0');

#ifdef QCS_RTMIDI
    if (!m_options.useCsoundMidi) {
        csoundSetOption(ud->csound, const_cast<char *>("-+rtmidi=hostbased"));
        csoundSetOption(ud->csound, const_cast<char *>("-M0"));
        csoundSetOption(ud->csound, const_cast<char *>("-Q0"));
    }
#endif
    csoundSetIsGraphable(ud->csound, true);
    csoundSetMakeGraphCallback(ud->csound, &CsoundEngine::makeGraphCallback);
    csoundSetDrawGraphCallback(ud->csound, &CsoundEngine::drawGraphCallback);
    csoundSetKillGraphCallback(ud->csound, &CsoundEngine::killGraphCallback);
    csoundSetExitGraphCallback(ud->csound, &CsoundEngine::exitGraphCallback);
    if (!m_options.fileName1.endsWith(".html", Qt::CaseInsensitive)) {
//#if CS_APIVERSION>=4
        char const **argv;// since there was change in Csound API
        argv = (const char **) calloc(33, sizeof(char*));
//#else
//        char **argv;
//        argv = (char **) calloc(33, sizeof(char*));
//#endif

        int argc = m_options.generateCmdLine((char **)argv);

        qDebug() << "------------ Compiling csd...";
        ud->result=csoundCompile(ud->csound,argc,argv);
        for (int i = 0; i < argc; i++) {
            qDebug()  << argv[i];
            free((char *) argv[i]);
        }
        free(argv);
        int startResult = csoundStart(ud->csound); // must be called in Csound7

        if (ud->result != CSOUND_SUCCESS ||  startResult != CSOUND_SUCCESS)  {
            qDebug()  << "Csound compile / csoundStart failed! "  << ud->result << startResult;
            // Commenting out flushQues fixes the crash.
            // Investigate closer, if it must be here
            // seems that messages are outputted into console anyway...
            flushQueues(); // the line was here in some earlier version. Otherwise errormessaged won't be processed by Console::appendMessage()
            locker.unlock(); // otherwise csoundStop will freeze
            stop();
            emit (errorLines(getErrorLines()));
            return -3;
        }
    }

    ud->zerodBFS = csoundGet0dBFS(ud->csound);
    ud->sampleRate = csoundGetSr(ud->csound);
    ud->numChnls = csoundGetChannels(ud->csound, 0);
    ud->outputBufferSize = csoundGetKsmps(ud->csound);
    if (ud->enableWidgets) {
        setupChannels();
    }
    // Do not run the performance thread if the piece is an HTML file,
    // the HTML code must do that.
    if (!m_options.fileName1.endsWith(".html", Qt::CaseInsensitive)) {
        ud->perfThread = new CsoundPerformanceThread(ud->csound);
        ud->perfThread->SetProcessCallback(CsoundEngine::csThread, (void*)ud);
        ud->perfThread->Play();
		m_paused = false;
    }
    ud->audioOutputBuffer.resize(ud->numChnls * 2048);
    return 0;
}

void CsoundEngine::stopCsound()
{
    //    perfThread->ScoreEvent(0, 'e', 0, 0);
    // m_playMutex.lock();
    QMutexLocker locker(&m_playMutex);
    if(!ud->perfThread)
        return;

    CsoundPerformanceThread *pt = ud->perfThread;

    pt->Stop();
	m_paused = false;

    unsigned int waitTime = 100;

    for(int msecs = 2000; msecs > 0; msecs -= waitTime) {
        int status = pt->GetStatus();
        if(status > 0) {
            QDEBUG << "Csound stopped OK";
            break;
        }
        if(status < 0) {
            QDEBUG << "Error when stopping csound";
            break;
        }
        if(status == 0) {
            QDEBUG << "Csound still playing, trying again in " << waitTime << "msecs";
            QThread::msleep(waitTime);
            // pt->Stop();
        }
    }



    if(pt->GetStatus() <= 0) {
        QDEBUG << "Csound's performance thread failed to stop, stopping csound";
        locker.unlock();
        csoundMutex.lock();
        pt->SetProcessCallback(nullptr, nullptr);
        QThread::msleep(200);
        QDEBUG << "Destroying csound...";
        // delete pt;
        csoundDestroy(ud->csound);
        QDEBUG << "Destroyed ok";
        ud->perfThread = nullptr;
        csoundMutex.unlock();
        emit stopSignal();
        QDEBUG << "Stopped OK...";
        return;
    }

    QDEBUG  << "Joining...";
    pt->Join();
    QDEBUG  << "Joined.";

    ud->perfThread = NULL;
    delete pt;

    QDEBUG << "Cleaning up csound...";

    this->cleanupCsound();

    QDEBUG << "Clean up OK";


#ifdef QCS_DEBUGGER
    stopDebug();
#endif

#ifdef MACOSX_PRE_SNOW
    // Put menu bar back
    SetMenuBar(menuBarHandle);
#endif
    QDEBUG << "emitting stopSignal...";

    locker.unlock();
    emit stopSignal();

    QDEBUG << "Exiting stopCsound";
}

void CsoundEngine::cleanupCsound()
{
    if(ud->csound == nullptr) {
        QDEBUG << "csound is null";
        return;
    }
    QMutexLocker locker(&csoundMutex);
    csoundSetIsGraphable(ud->csound, 0);
    csoundSetMakeGraphCallback(ud->csound, nullptr);
    csoundSetDrawGraphCallback(ud->csound, nullptr);
    csoundSetKillGraphCallback(ud->csound, nullptr);
    csoundSetExitGraphCallback(ud->csound, nullptr);    
    csoundRemoveKeyboardCallback(ud->csound, &CsoundEngine::keyEventCallback);
#ifdef QCS_DEBUGGER
    if(m_debugging) {
        csoundDebuggerClean(ud->csound);
    }
#endif

    //csoundCleanup(ud->csound);
    csoundReset(ud->csound); // CS7  replacement  csoundCleanup?
    flushQueues();
    csoundDestroyMessageBuffer(ud->csound);

#ifdef QCS_DESTROY_CSOUND
    //csoundDestroyCircularBuffer(ud->csound, ud->midiBuffer); // Cs7 destroyed in csoundDestroy
    ud->midiBuffer = nullptr;
    //csoundDestroyCircularBuffer(ud->csound, ud->virtualMidiBuffer); // Cs7 destroyed in csoundDestroy
    ud->virtualMidiBuffer = nullptr;
    csoundDestroy(ud->csound);
    ud->csound = nullptr;
#else
    csoundReset(ud->csound);
#endif
}

void CsoundEngine::setupChannels()
{
    ud->inputChannelNames.clear();
    ud->outputChannelNames.clear();
    ud->outputStringChannelNames.clear();
    ud->previousOutputValues.clear();
    ud->previousStringOutputValues.clear();
    csoundSetInputChannelCallback(ud->csound, &CsoundEngine::inputValueCallback);
    csoundSetOutputChannelCallback(ud->csound, &CsoundEngine::outputValueCallback);
    // For chnget/chnset
    controlChannelInfo_t *channelList;
    int numChannels = csoundListChannels(ud->csound, &channelList);
    controlChannelInfo_t *entry = channelList;

    MYFLT *pvalue;
    QVector<QuteWidget *> widgets = ud->wl->getWidgets();
    // Set channels values for existing channels (i.e. those declared with chn_*
    // in the csound header
    for (int i = 0; i < numChannels; i++) {
        //                                                      name        type
        // if type is 0, no new channel is created if it does not exist,
        // the returned value is the channel type
        int chanType = csoundGetChannelPtr(ud->csound, (void **) &pvalue, entry->name, 0);
        if (chanType & CSOUND_INPUT_CHANNEL) {
            if ((chanType & CSOUND_CHANNEL_TYPE_MASK) == CSOUND_CONTROL_CHANNEL) {
                ud->wl->valueMutex.lock();
                foreach (QuteWidget *w, widgets) {
                    if (w->getChannelName() == QString(entry->name)) {
                        ud->wl->newValues.insert(w->getChannelName(), w->getValue());
                    }
                    if (w->getChannel2Name() == QString(entry->name)) {
                        ud->wl->newValues.insert(w->getChannel2Name(), w->getValue2());
                    }
                }
                ud->wl->valueMutex.unlock();
            } else if ((chanType & CSOUND_CHANNEL_TYPE_MASK) ==  CSOUND_STRING_CHANNEL) {
                ud->wl->stringValueMutex.lock();
                foreach (QuteWidget *w, widgets) {
                    if (w->getChannelName() == QString(entry->name)) {
                        ud->wl->newStringValues.insert(w->getChannelName(), w->getStringValue());
                    }
                }
                ud->wl->stringValueMutex.unlock();
            }
        }
        if (chanType & CSOUND_OUTPUT_CHANNEL) { // Channels can be input and output at the same time
            if ((chanType & CSOUND_CHANNEL_TYPE_MASK) == CSOUND_CONTROL_CHANNEL) {
                ud->outputChannelNames << QString(entry->name);
                ud->previousOutputValues << 0;
                foreach (QuteWidget *w, widgets) {
                    if (w->getChannelName() == QString(entry->name)) {
                        ud->previousOutputValues.last() = w->getValue();
                        continue;
                    }
                    if (w->getChannel2Name() == QString(entry->name)) {
                        ud->previousOutputValues.last() = w->getValue2();
                        continue;
                    }
                }
            } else if ((chanType & CSOUND_CHANNEL_TYPE_MASK) == CSOUND_STRING_CHANNEL) {
                ud->outputStringChannelNames << QString(entry->name);
                ud->previousStringOutputValues << "";
                foreach (QuteWidget *w, widgets) {
                    if (w->getChannelName() == QString(entry->name)) {
                        ud->previousStringOutputValues.last() = w->getStringValue();
                        continue;
                    }
                }
            }
        }
        entry++;
    }
    csoundDeleteChannelList(ud->csound, channelList);

    // Force creation of string channels for _Browse widgets
    foreach (QuteWidget *w, widgets) {
        if (w->getChannelName().startsWith("_Browse")) {
            csoundGetChannelPtr(ud->csound, (void **) &pvalue, w->getChannelName().toLocal8Bit(),
                                CSOUND_INPUT_CHANNEL | CSOUND_OUTPUT_CHANNEL | CSOUND_STRING_CHANNEL);
            ud->wl->newStringValues.insert(w->getChannelName(), w->getStringValue());
        }
    }
}

void CsoundEngine::messageListDispatcher(void *data)
{
    CsoundUserData *ud_local = (CsoundUserData *) data;
    while (ud_local->runDispatcher) {
        ud_local->playMutex->lock();
        if (ud_local->perfThread && (ud_local->perfThread->GetStatus() != 0)) {
            // In case score has ended
            ud_local->playMutex->unlock();
            ud_local->csEngine->stop();
        } else {
            ud_local->playMutex->unlock();
        }
        CSOUND *csound = ud_local->csEngine->getCsound();
        if (csound) {
            if (ud_local->wl) {
                ud_local->wl->getMouseValues(&ud_local->mouseValues);
            }
            int count = csoundGetMessageCnt(csound);
            ud_local->csEngine->m_messageMutex.lock();
            for (int i = 0; i< count; i++) {
                ud_local->csEngine->messageQueue << csoundGetFirstMessage(csound);
                // FIXME: Is this thread safe?
                csoundPopFirstMessage(csound);
            }
            ud_local->csEngine->m_messageMutex.unlock();
        }

        int counter = 0;
        ud_local->csEngine->m_messageMutex.lock();
        while ((ud_local->csEngine->m_consoleBufferSize <= 0 ||
                counter++ < ud_local->csEngine->m_consoleBufferSize)) {
            if (ud_local->csEngine->messageQueue.isEmpty()) {
                break;
            }
            QString msg = ud_local->csEngine->messageQueue.takeFirst();
            // Must use signals to make things thread safe
            emit ud_local->csEngine->passMessages(msg);
        }
        if (!ud_local->csEngine->messageQueue.isEmpty()
                && ud_local->csEngine->m_consoleBufferSize > 0
                && counter >= ud_local->csEngine->m_consoleBufferSize) {
            ud_local->csEngine->messageQueue.clear();
            ud_local->csEngine->messageQueue << "\nCsoundQt: Message buffer overflow. Messages discarded!\n";
        }
        ud_local->csEngine->m_messageMutex.unlock();
        QThread::usleep(ud_local->msgRefreshTime);
    }
}

void CsoundEngine::flushQueues()
{
    m_messageMutex.lock();
    int count = csoundGetMessageCnt(ud->csound);
    for (int i = 0; i < count; i++) {
        messageQueue << csoundGetFirstMessage(ud->csound);
        csoundPopFirstMessage(ud->csound);
    }

    while (!messageQueue.isEmpty()) {
        QString msg = messageQueue.takeFirst();
        ConsoleWidget *console = nullptr;
        for (int i = 0; i < consoles.size(); i++) {
            console = consoles[i];
            console->appendMessage(msg);
        }
        ud->wl->appendMessage(msg);

    }
    m_messageMutex.unlock();
    ud->wl->flushGraphBuffer();
}

void CsoundEngine::queueMessage(QString message)
{
    m_messageMutex.lock();
    messageQueue << message;
    m_messageMutex.unlock();
}

bool CsoundEngine::isRunning()
{
    if(!m_playMutex.tryLock(1)) {
        qDebug() << "Could not acquire lock";
        return false;
    }
    auto out = (ud->perfThread && (ud->perfThread->GetStatus() == 0));
    m_playMutex.unlock();
    return out;
}

bool CsoundEngine::isRecording()
{
    return m_recording;
}

CSOUND *CsoundEngine::getCsound()
{
    return ud->csound;
}

#ifdef QCS_PYTHONQT
void CsoundEngine::registerProcessCallback(QString func, int skipPeriods)
{
    ud->m_pythonCallback = func;
    ud->m_pythonCallbackSkip = skipPeriods;
}

void CsoundEngine::setPythonConsole(PythonConsole *pc)
{
    ud->m_pythonConsole = pc;
}
#endif

#ifdef QCS_DEBUGGER

void CsoundEngine::breakpointCallback(CSOUND *csound, debug_bkpt_info_t *bkpt_info, void *udata)
{
    debug_instr_t *debug_instr = bkpt_info->breakpointInstr;
    CsoundEngine *cs = (CsoundEngine *) udata;
    // Copy variable list
    debug_variable_t* vp = bkpt_info->instrVarList;
    cs->variableMutex.lock();
    cs->m_varList.clear();
    while (vp) {
        if (vp->name[0] != '#') {
            QVariantList varDetails;
            varDetails << vp->name;
            if (strcmp(vp->typeName, "i") == 0
                    || strcmp(vp->typeName, "k") == 0
                    || strcmp(vp->typeName, "r") == 0) {
                varDetails << *((MYFLT *) vp->data);
            } else if(strcmp(vp->typeName, "S") == 0) {
                varDetails << (char *) vp->data;
            } else if (strcmp(vp->typeName, "a") == 0) {
                MYFLT *data = (MYFLT *) vp->data;
                varDetails << *data << *(data + 1)
                           << *(data + 2)<< *(data + 3);
            } else {
                varDetails << QVariant();
            }
            varDetails << vp->typeName;
            cs->m_varList << varDetails;
        }
        vp = vp->next;
    }
    cs->variableMutex.unlock();
    if (bkpt_info->currentOpcode) {
        cs->m_currentLine.store(bkpt_info->currentOpcode->line);
    } else {
        cs->m_currentLine.store(debug_instr->line);
    }

    debug_instr_t *debug_instr_list = bkpt_info->instrListHead;
    //Copy active instrument list
    cs->instrumentMutex.lock();
    cs->m_instrumentList.clear();
    while (debug_instr_list) {
        QVariantList instance;
        instance << debug_instr_list->p1;
        instance << QString("%1 %2").arg(debug_instr_list->p2).arg(debug_instr_list->p3);
        instance << (qulonglong)debug_instr_list->kcounter;
        cs->m_instrumentList << instance;
        debug_instr_list = debug_instr_list->next;
    }
    cs->instrumentMutex.unlock();

    emit cs->breakpointReached();
}

void CsoundEngine::setDebug()
{
    if (isRunning()) {
        ud->perfThread->Pause();
        csoundDebuggerInit(ud->csound);
        csoundSetBreakpointCallback(ud->csound, &CsoundEngine::breakpointCallback, (void *) this);
        //        ud->perfThread->Play();
    }
    m_debugging = true;
}

void CsoundEngine::pauseDebug()
{
    if (isRunning() && m_debugging) {
        csoundDebugStop(ud->csound);
    }
}

void CsoundEngine::continueDebug()
{
    if (isRunning() && m_debugging) {
        qDebug() ;
        csoundDebugContinue(ud->csound);
    }
}

void CsoundEngine::stopDebug()
{
    if (isRunning() && m_debugging) {
        stop();
    }
    m_debugging = false;
}

void CsoundEngine::addInstrumentBreakpoint(double instr, int skip)
{
    if (isRunning() && m_debugging) {
        csoundSetInstrumentBreakpoint(ud->csound, instr, skip);
    }
}

void CsoundEngine::removeInstrumentBreakpoint(double instr)
{
    if (isRunning() && m_debugging) {
        csoundRemoveInstrumentBreakpoint(ud->csound, instr);
    }
}

void CsoundEngine::addBreakpoint(int line, int instr, int skip)
{
    if (isRunning() && m_debugging) {
        csoundSetBreakpoint(ud->csound, line, instr, skip);
    }
}

void CsoundEngine::removeBreakpoint(int line, int instr)
{
    if (isRunning() && m_debugging) {
        csoundRemoveBreakpoint(ud->csound, line, instr);
    }
}

void CsoundEngine::setStartingBreakpoints(QVector<QVariantList> bps)
{
    m_startBreakpoints = bps;
}

QVector<QVariantList> CsoundEngine::getVaribleList()
{
    QVector<QVariantList> outList;
    variableMutex.lock();
    outList = m_varList;
    variableMutex.unlock();
    return outList;
}

QVector<QVariantList> CsoundEngine::getInstrumentList()
{
    QVector<QVariantList> outList;
    instrumentMutex.lock();
    outList = m_instrumentList;
    instrumentMutex.unlock();
    return outList;
}

int CsoundEngine::getCurrentLine()
{
    return m_currentLine.load();
}

#endif

CsoundUserData *CsoundEngine::getUserData()
{
    return ud;
}

void CsoundEngine::clearConsoles(void) {
    for (int i = 1, n = consoles.size(); i < n; ++i) {
        consoles[i]->reset();
    }
}
