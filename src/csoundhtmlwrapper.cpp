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


// Code based partly on CsoundHtmlWrapper by Michael Gogins https://github.com/gogins/gogins.github.io/tree/master/csound_html5.
// This class must be safe against calling with a null or uninitialized CsoundEngine.

#include "csoundhtmlwrapper.h"
#include "csoundhtmlview.h"
#include <QApplication>
#include <QDebug>

CsoundHtmlWrapper::CsoundHtmlWrapper(QObject *parent) :
    QObject(parent),
    m_csoundEngine(nullptr),
    message_callback(nullptr),
    csoundHtmlView(nullptr)
{
}

void CsoundHtmlWrapper::setCsoundHtmlView(CsoundHtmlView *csoundHtmlView_) {
    csoundHtmlView = csoundHtmlView_;
}


CSOUND *CsoundHtmlWrapper::getCsound()
{
    if(!m_csoundEngine) {
        return nullptr;
    }
    return m_csoundEngine->getCsound();
}

CsoundPerformanceThread *CsoundHtmlWrapper::getThread()
{
    if(!m_csoundEngine) {
        return nullptr;
    }
    return m_csoundEngine->getUserData()->perfThread;
}

CsoundUserData *CsoundHtmlWrapper::getUserData()
{
    if(!m_csoundEngine) {
        return nullptr;
    }
    return m_csoundEngine->getUserData();
}

void CsoundHtmlWrapper::setCsoundEngine(CsoundEngine *csEngine)
{
    m_csoundEngine = csEngine;
    if (m_csoundEngine != nullptr) {
        auto csound = m_csoundEngine->getCsound();
        if (csound != nullptr) {
            //TODO: Csound crashes later -- not sure why.
            //csoundSetMessageCallback(csound, CsoundHtmlWrapper::csoundMessageCallback_);
        }
    }
}

int CsoundHtmlWrapper::compileCsd(const QString &filename) {
    if (!m_csoundEngine) {
        return -1;
    }
//#if CS_APIVERSION>=4
    csoundCompileCSD(getCsound(), filename.toLocal8Bit(), 0);
    return 0;
//#else
//    return csoundCompileCSD(getCsound(), filename.toLocal8Bit().data(),0);
//#endif
}

int CsoundHtmlWrapper::compileCsdText(const QString &text) {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundCompileCSD(getCsound(), text.toLocal8Bit(),1);
}

int CsoundHtmlWrapper::compileOrc(const QString &text) {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundCompileOrc(getCsound(), text.toLocal8Bit(),0);
}

double CsoundHtmlWrapper::evalCode(const QString &text) {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundEvalCode(getCsound(), text.toLocal8Bit());
}

double CsoundHtmlWrapper::get0dBFS() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGet0dBFS(getCsound());
}

//int CsoundHtmlWrapper::getApiVersion() {
//    if (!m_csoundEngine) {
//        return -1;
//    }
//    return csoundGetAPIVersion();
//}

double CsoundHtmlWrapper::getControlChannel(const QString &name) {
    if (!m_csoundEngine) {
        return -1;
    }
    int result = 0;
    double value = csoundGetControlChannel(getCsound(), name.toLocal8Bit(), &result);
    return value;
}

qint64 CsoundHtmlWrapper::getCurrentTimeSamples() { // FIXME: unknown type int64_t qint64
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGetCurrentTimeSamples(getCsound());
}

QString CsoundHtmlWrapper::getEnv(const QString &name) { // not sure, if it works... test with setGlobalEnv
    if (!m_csoundEngine) {
        return QString();
    }
    return csoundGetEnv(getCsound(),name.toLocal8Bit());
}

int CsoundHtmlWrapper::getKsmps() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGetKsmps(getCsound());
}

int CsoundHtmlWrapper::getNchnls() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGetChannels(getCsound(),0);
}

int CsoundHtmlWrapper::getNchnlsInput() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGetChannels(getCsound(), 1);
}

QString CsoundHtmlWrapper::getOutputName() {
    if (!m_csoundEngine) {
        return QString();
    }
    return QString(); // csound7 comment out QString(csoundGetOutputName(getCsound()));
}

double CsoundHtmlWrapper::getScoreOffsetSeconds() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGetScoreOffsetSeconds(getCsound());
}

double CsoundHtmlWrapper::getScoreTime() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGetScoreTime(getCsound());
}

int CsoundHtmlWrapper::getSr() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundGetSr(getCsound());
}

QString CsoundHtmlWrapper::getStringChannel(const QString &name) {
    if (!m_csoundEngine) {
        return QString();
    }
    char buffer[0x100];
    csoundGetStringChannel(getCsound(),name.toLocal8Bit(), buffer);
    return QString(buffer);
}

int CsoundHtmlWrapper::getVersion() {
    return csoundGetVersion();
}

bool CsoundHtmlWrapper::isPlaying() {
    if (!m_csoundEngine) {
        return false;
    }
    if (!getThread()) {
        return false;
    }
    return (getThread()->GetStatus() == 0);
}

int CsoundHtmlWrapper::isScorePending() {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundIsScorePending(getCsound());
}

void CsoundHtmlWrapper::message(const QString &text) {
    if (!m_csoundEngine) {
        return;
    }
    qDebug() << text;
    if (isPlaying()) {
        csoundMessage(getCsound(), "%s", text.toLocal8Bit().constData());
    }
}

int CsoundHtmlWrapper::perform() {
    if (!m_csoundEngine) {
        return -1;
    }
    if (getThread()) {
        qDebug() << "Stopping existing Csound performance thread.";
        getThread()->Stop();
        getThread()->Join();
        delete getThread();
    }
    getUserData()->perfThread = new CsoundPerformanceThread(getCsound());
    getThread()->SetProcessCallback(CsoundEngine::csThread, (void*)getUserData());
    getThread()->Play();
    return 0;
}

int CsoundHtmlWrapper::readScore(const QString &text) {
    if (!m_csoundEngine) {
        return -1;
    }
    if (isPlaying()) {
        csoundEventString(getCsound(), text.toLocal8Bit(),0);
        return 0;
    }
    return -1;
}

void CsoundHtmlWrapper::reset() {
    if (!m_csoundEngine) {
        return;
    }
    csoundReset(getCsound());
}

void CsoundHtmlWrapper::rewindScore() {
    if (!m_csoundEngine) {
        return;
    }
    csoundRewindScore(getCsound());
}

int CsoundHtmlWrapper::runUtility(const QString &command, int argc, char **argv) {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundRunUtility(getCsound(), command.toLocal8Bit(), argc, argv); // probably does not work from JS due char **
}

int CsoundHtmlWrapper::scoreEvent(char type, double *pFields, long numFields) {
    if (!m_csoundEngine) {
        return -1;
    }
    csoundEvent(getCsound(),type, pFields, numFields, 0);
    return 0;
}

void CsoundHtmlWrapper::setControlChannel(const QString &name, double value) {
    if (!m_csoundEngine) {
        return;
    }
    csoundSetControlChannel(getCsound(),name.toLocal8Bit(), value);
}

int CsoundHtmlWrapper::setGlobalEnv(const QString &name, const QString &value) {
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundSetGlobalEnv(name.toLocal8Bit(), value.toLocal8Bit());
}

void CsoundHtmlWrapper::setInput(const QString &name){
    if (!m_csoundEngine) {
        return;
    }
//#if CS_APIVERSION>=4
    //csound7 comment out: csoundSetInput(getCsound(), name.toLocal8Bit());
//#else
//    csoundSetInput(getCsound(), name.toLocal8Bit().data());
//#endif
}

void CsoundHtmlWrapper::setMessageCallback(QObject *callback){
    qDebug();
    callback->dumpObjectInfo();
}

int CsoundHtmlWrapper::setOption(const QString &name){
    if (!m_csoundEngine) {
        return -1;
    }
//#if CS_APIVERSION>=4
    return csoundSetOption(getCsound(), name.toLocal8Bit());
//#else
//    return csoundSetOption(getCsound(), name.toLocal8Bit().data());
//#endif
}

void CsoundHtmlWrapper::setOutput(const QString &name, const QString &type, const QString &format){
    if (!m_csoundEngine) {
        return;
    }
//#if CS_APIVERSION>=4
    //csound7 comment out// csoundSetOutput(getCsound(), name.toLocal8Bit(), type.toLocal8Bit(), format.toLocal8Bit());
//#else
//    csoundSetOutput(getCsound(), name.toLocal8Bit().data(), type.toLocal8Bit().data(), format.toLocal8Bit().data());
//#endif
}

void CsoundHtmlWrapper::setScoreOffsetSeconds(double value){
    if (!m_csoundEngine) {
        return;
    }
    csoundSetScoreOffsetSeconds(getCsound(), value);
}

void CsoundHtmlWrapper::setScorePending(bool value){
    if (!m_csoundEngine) {
        return;
    }
    csoundSetScorePending(getCsound(),(int) value);
}

void CsoundHtmlWrapper::setStringChannel(const QString &name, const QString &value){
    if (!m_csoundEngine) {
        return;
    }
    csoundSetStringChannel(getCsound(),  name.toLocal8Bit(), value.toLocal8Bit().data());
}

int CsoundHtmlWrapper::start(){
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundStart(getCsound());
}

void CsoundHtmlWrapper::stop(){
    if (!m_csoundEngine) {
        return;
    }
    csoundReset(getCsound());
}

double CsoundHtmlWrapper::tableGet(int table_number, int index){
    if (!m_csoundEngine) {
        return -1;
    }
    MYFLT *data;
    int tableLength = csoundGetTable(getCsound(), &data, table_number);
    if (tableLength>0 && index<tableLength) {
        return data[index];
    } else {
        QDEBUG << "Could not read table " <<table_number;
        return -1;
    }
}

int CsoundHtmlWrapper::tableLength(int table_number){
    if (!m_csoundEngine) {
        return -1;
    }
    return csoundTableLength(getCsound(), table_number);
}

void CsoundHtmlWrapper::tableSet(int table_number, int index, double value){
    if (!m_csoundEngine) {
        return;
    }
    MYFLT *data;
    int tableLength = csoundGetTable(getCsound(), &data, table_number);
    if (tableLength>0 && index<tableLength) {
        data[index] = value;
    } else {
        QDEBUG << "Could not write to table " <<table_number;
    }
}


void CsoundHtmlWrapper::csoundMessageCallback_(CSOUND *csound,
                                         int attributes,
                                         const char *format,
                                         va_list args) {
        return reinterpret_cast<CsoundHtmlWrapper *>(csoundGetHostData(csound))->csoundMessageCallback(attributes, format, args);
}

void CsoundHtmlWrapper::csoundMessageCallback(int attributes,
                           const char *format,
                           va_list args)
{
    (void) attributes;

    QString message = QString::vasprintf(format, args);

    qDebug() << message;
//    if (!console->isHidden()) { // otherwise crash on exit
//        passMessages(message);
//    }
    for (int i = 0, n = message.length(); i < n; i++) {
        auto c = message[i];
        if (c == '\n') {
            QString code = "console.log(\"" + csoundMessageBuffer + "\\n\");";
            if (csoundHtmlView != nullptr) {
#ifdef USE_WEBKIT
				csoundHtmlView->webView->page()->mainFrame()->evaluateJavaScript(code);
#else
				csoundHtmlView->webView->page()->runJavaScript(code);
#endif

            }
            csoundMessageBuffer.clear();
        } else {
            csoundMessageBuffer.append(c);
        }
    }
}
