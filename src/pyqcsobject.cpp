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

#include "pyqcsobject.h"
#include "csoundqt.h"
#include "qutesheet.h"
#include "opentryparser.h"
#include "csoundengine.h"

#include <QMessageBox>
#include <QDir>
#include "csound.h"

PyQcsObject::PyQcsObject():QObject(NULL)
{
	m_qcs = 0;
	m_tablePtr = (MYFLT **) malloc(sizeof(MYFLT *));
}

PyQcsObject::~PyQcsObject()
{
	free(m_tablePtr);
}

void PyQcsObject::setCsoundQt(CsoundQt *qcs)
{
	m_qcs = qcs;
}

QString PyQcsObject::getVersion()
{
	return QString(PYQCSVERSION);
}

void PyQcsObject::refresh()
{
	qApp->processEvents();
}

void PyQcsObject::play(int index, bool realtime)
{
	qDebug() << "PyQcsObject::play " << index << realtime;
	// Notice arguments are inverted...
	m_qcs->play(realtime, index);
}

void PyQcsObject::pause(int index)
{
	m_qcs->pause(index);
}

void PyQcsObject::stop(int index)
{
	m_qcs->stop(index);
}

void PyQcsObject::stopAll()
{
	m_qcs->stopAll();
}

void PyQcsObject::record(int index)
{
	m_qcs->record(true, index); // recording must be stoppend via q.stop()
}

bool PyQcsObject::opcodeExists(QString opcodeName)
{
	return m_qcs->m_opcodeTree->isOpcode(opcodeName);
}

void PyQcsObject::setDocument(int index)
{
	QString name = m_qcs->setDocument(index);
	PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
	QString path = name.left(name.lastIndexOf("/"));
	mainContext.call("os.chdir", QVariantList() << path );
	mainContext.evalScript("print('cd \"" + path + "\"')");
}

int PyQcsObject::loadDocument(QString name, bool runNow)
{
	QDir d(name);
	d.makeAbsolute();
	qDebug() << d.absolutePath();
	if (!QFile::exists(d.absolutePath())) {
		PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
		mainContext.evalScript("print('File not found.')");
		return -1;
	} else {
		return m_qcs->loadFile(d.absolutePath(), runNow);
	}
}

void PyQcsObject::insertText(QString text, int index, int section)
{
	return m_qcs->insertText(text, index,section);
}

void PyQcsObject::setCsd(QString text, int index)
{
	return m_qcs->setCsd(text, index);
}

void PyQcsObject::setFullText(QString text, int index)
{
	return m_qcs->setFullText(text, index);
}

void PyQcsObject::setOrc(QString text, int index)
{
	return m_qcs->setOrc(text, index);
}

void PyQcsObject::setSco(QString text, int index)
{
	return m_qcs->setSco(text, index);
}

void PyQcsObject::setWidgetsText(QString text, int index)
{
	return m_qcs->setWidgetsText(text, index);
}

void PyQcsObject::setPresetsText(QString text, int index)
{
	return m_qcs->setPresetsText(text, index);
}

void PyQcsObject::setOptionsText(QString text, int index)
{
	return m_qcs->setOptionsText(text, index);
}

int PyQcsObject::getDocument(QString name)
{
	return m_qcs->getDocument(name);
}

int PyQcsObject::newDocument(QString name)
{
	PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();

	if (name.isEmpty()) {
		mainContext.evalScript("print('Please specify a filename')");
		return -1;
	}
	QDir d(name);
	qDebug() << d;
	if (QFile::exists(d.absolutePath())) {
		mainContext.evalScript("print('File already exists. Use loadDocument()')");
		return -1;
	}
	m_qcs->newFile();
	if (!m_qcs->saveFile(d.absolutePath())) {
		mainContext.evalScript("print('Error saving file.')");
	}
	return m_qcs->getDocument(name);
}

QString PyQcsObject::getSelectedText(int index, int section)
{
	return m_qcs->getSelectedText(index,section);
}

QString PyQcsObject::getCsd(int index)
{
	return m_qcs->getCsd(index);
}

QString PyQcsObject::getFullText(int index)
{
	return m_qcs->getFullText(index);
}

QString PyQcsObject::getOrc(int index)
{
	return m_qcs->getOrc(index);
}

QString PyQcsObject::getSco(int index)
{
	return m_qcs->getSco(index);
}

QString PyQcsObject::getWidgetsText(int index)
{
	return m_qcs->getWidgetsText(index);
}

QString PyQcsObject::getSelectedWidgetsText(int index)
{
	return m_qcs->getSelectedWidgetsText(index);
}

QString PyQcsObject::getPresetsText(int index)
{
	return m_qcs->getPresetsText(index);
}

QString PyQcsObject::getOptionsText(int index)
{
	return m_qcs->getOptionsText(index);
}

QString PyQcsObject::getFileName(int index)
{
	return m_qcs->getFileName(index);
}

QString PyQcsObject::getFilePath(int index)
{
	return m_qcs->getFilePath(index);
}

void PyQcsObject::setChannelValue(QString channel, double value, int index)
{
	return m_qcs->setChannelValue(channel, value, index);
}


double PyQcsObject::getChannelValue(QString channel, int index)
{
	return m_qcs->getChannelValue(channel, index);
}

void PyQcsObject::setChannelString(QString channel, QString value, int index)
{
	return m_qcs->setChannelString(channel, value, index);
}

QString PyQcsObject::getChannelString(QString channel, int index)
{
	return m_qcs->getChannelString(channel, index);
}

void PyQcsObject::setCsChannel(QString channel, double value, int index)
{
	CsoundEngine *e = m_qcs->getEngine(index);
	MYFLT *p;
	if (e != NULL) {
		CSOUND *cs = e->getCsound();

        if (cs) {
            controlChannelHints_t hints;  // this does not work with csound5
			int ret = csoundGetControlChannelHints(cs, channel.toLocal8Bit(), &hints);
			if (ret == 0) {
				csoundSetControlChannel(cs, channel.toLocal8Bit(), (MYFLT) value);
				return;
			}
		}
    }

	QString message="Channel '" + channel + "' does not exist or is not exposed with chn_k.";
	PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
	mainContext.evalScript("print(\'"+message+"\')");
}

void PyQcsObject::setCsChannel(QString channel, QString stringValue, int index)
{
	CsoundEngine *e = m_qcs->getEngine(index);
	MYFLT *p;
	if (e != NULL) {
		CSOUND *cs = e->getCsound();
		if (cs != NULL && !(csoundGetChannelPtr(cs, &p, channel.toLocal8Bit().constData(), CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL))) {
			// FIXME not thread safe and not checking string bounds.
			strcpy((char*) p,stringValue.toLocal8Bit().constData() );
			return;
		}
	}
	QString message="Could not set string into channel "+ channel;
	PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
	mainContext.evalScript("print(\'"+message+"\')");
}


double PyQcsObject::getCsChannel(QString channel, int index)
{
	CsoundEngine *e = m_qcs->getEngine(index);
	MYFLT *value =  new MYFLT;
	//*value = 0;
	if (e != NULL) {
		CSOUND *cs = e->getCsound();
		if (cs != NULL ) {
			if ( !( csoundGetChannelPtr(cs,&value,channel.toLocal8Bit(),
										CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL)))
				return (double) *value;
		}
	}

	QString message="Could not read from channel "+channel;
	PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
	mainContext.evalScript("print(\'"+message+"\')");
	return 0;//m_qcs->getCsChannel(channel, index);
}



QString PyQcsObject::getCsStringChannel(QString channel, int index)
{
	CsoundEngine *e = m_qcs->getEngine(index);

	if (e != NULL) {
		CSOUND *cs = e->getCsound();

		if (cs != NULL) {
			int maxlen = csoundGetChannelDatasize(cs, channel.toLocal8Bit());
			char *value = new char[maxlen];
			if ( !( csoundGetChannelPtr(cs,(MYFLT **) &value,channel.toLocal8Bit(),
										CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL))) {
				return QString(value);
			}
		}
	}
	QString message="Could not read from channel "+channel;
	PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
	mainContext.evalScript("print(\'"+message+"\')");
	return QString();//m_qcs->getCsChannel(channel, index);

}


void PyQcsObject::setWidgetProperty(QString widgetid, QString property, QVariant value, int index)
{
	return m_qcs->setWidgetProperty(widgetid, property, value, index);
}

QVariant PyQcsObject::getWidgetProperty(QString widgetid, QString property, int index)
{
	QStringList properties = listWidgetProperties(widgetid,index);
	if ( properties.contains(property) ) {
		return m_qcs->getWidgetProperty(widgetid, property, index);

	} else {

		QString message="Widget "+widgetid+" does not have property "+property+" available properties are: "+properties.join(", ")+".";
		PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
		mainContext.evalScript("print(\'"+message+"\')");
	}
	return (int) -1;
}

QStringList PyQcsObject::getWidgetUuids(int index)
{
	return m_qcs->getWidgetUuids(index);
}

QStringList PyQcsObject::listWidgetProperties(QString widgetid, int index)
{

	return m_qcs->listWidgetProperties(widgetid,index);
}


QString PyQcsObject::createNewLabel(int x, int y, QString channel, int index)
{
	// FIXME all this interaction with widgets must be correctly placed on the widgets undo/redo stack. It's currently not!
	return m_qcs->createNewLabel(x,y, channel, index);
}

QString PyQcsObject::createNewDisplay(int x, int y, QString channel, int index)
{
	return m_qcs->createNewDisplay(x,y, channel, index);
}

QString PyQcsObject::createNewScrollNumber(int x, int y, QString channel, int index)
{
	return m_qcs->createNewScrollNumber(x,y, channel, index);
}

QString PyQcsObject::createNewLineEdit(int x, int y, QString channel, int index)
{
	return m_qcs->createNewLineEdit(x,y, channel, index);
}

QString PyQcsObject::createNewSpinBox(int x, int y, QString channel, int index)
{
	return m_qcs->createNewSpinBox(x,y, channel, index);
}

QString PyQcsObject::createNewSlider(QString channel, int index)
{
	return m_qcs->createNewSlider(0,0, channel, index);
}

QString PyQcsObject::createNewSlider(int x, int y, QString channel, int index)
{
	return m_qcs->createNewSlider(x,y, channel, index);
}

QString PyQcsObject::createNewButton(int x, int y, QString channel, int index)
{
	return m_qcs->createNewButton(x,y, channel, index);
}

QString PyQcsObject::createNewKnob(int x, int y, QString channel, int index)
{
	return m_qcs->createNewKnob(x,y, channel, index);
}

QString PyQcsObject::createNewCheckBox(int x, int y, QString channel, int index)
{
	return m_qcs->createNewCheckBox(x,y, channel, index);
}

QString PyQcsObject::createNewMenu(int x, int y, QString channel, int index)
{
	return m_qcs->createNewMenu(x,y, channel, index);
}

QString PyQcsObject::createNewMeter(int x, int y, QString channel, int index)
{
	return m_qcs->createNewMeter(x,y, channel, index);
}

QString PyQcsObject::createNewConsole(int x, int y, QString channel, int index)
{
	return m_qcs->createNewConsole(x,y, channel, index);
}

QString PyQcsObject::createNewGraph(int x, int y, QString channel, int index)
{
	return m_qcs->createNewGraph(x,y, channel, index);
}

QString PyQcsObject::createNewScope(int x, int y, QString channel, int index)
{
	return m_qcs->createNewScope(x,y, channel, index);
}

bool PyQcsObject::destroyWidget(QString widgetid)
{
	return m_qcs->destroyWidget(widgetid);
}

QuteSheet* PyQcsObject::getSheet(int index, int sheetIndex)
{
	return new QuteSheet(this, m_qcs->getSheet(index,sheetIndex));
}

QuteSheet* PyQcsObject::getSheet(int index, QString sheetName)
{
	return new QuteSheet(this, m_qcs->getSheet(index,sheetName));
}

void PyQcsObject::schedule(QVariant time, QVariant event)
{
	qDebug() << "PyQcsObject::schedule" << event;
	if (time.canConvert<double>())  { // a single event
		QString eventLine = "i ";
		if (event.canConvert<QVariantList>()) {
			QVariantList fields = event.value<QVariantList>();
			for (int f = 0; f < fields.size(); f++) {
				if (strcmp(fields[f].typeName(), "double") || strcmp(fields[f].typeName(),"int")) {
					eventLine.append(fields[f].toString() + " ");
				}
				else {
					eventLine.append("\"" + fields[f].toString() + "\"");
				}
			}
		}
		qDebug() << "PyQcsObject::schedule: " << eventLine;
	}
	else if (time.canConvert<QVariantList>()) { // list of events

	}
}

void PyQcsObject::sendEvent(QString events)
{
	//  qDebug() << "PyQcsObject::sendEvent" << events;
	m_qcs->sendEvent(events);
	//  if (events.type() == QVariant::String )  { // a single event in a string
	//    m_qcs->sendEvent(events.toString());
	//    qDebug() << "PyQcsObject::sendEvent sent: " << events.toString();
	//  }
	//  else if (time.canConvert<QVariantList>()) { // list of events
	//
	//  }
}

void PyQcsObject::sendEvent(int index, QString events)
{
	m_qcs->sendEvent(index, events);
}

CSOUND* PyQcsObject::getCurrentCsound()
{
	CSOUND *cs = NULL;
	CsoundEngine *e = m_qcs->getEngine();
	if (e != NULL) {
		cs = e->getCsound();
	}
	return cs;
}


double PyQcsObject::getSampleRate(int index)
{
	double sr = -1.0;
	CsoundEngine *e = m_qcs->getEngine(index);
	if (e != NULL) {
		CSOUND *cs = e->getCsound();
		if (cs != NULL) {
			sr = (double) csoundGetSr (cs);
		}
	}
	return sr;
}

int PyQcsObject::getKsmps(int index)
{
	int ksmps = -1;
	CsoundEngine *e = m_qcs->getEngine(index);
	if (e != NULL) {
		CSOUND *cs = e->getCsound();
		if (cs != NULL) {
			ksmps = csoundGetKsmps (cs);
		}
	}
	return ksmps;
}

int PyQcsObject::getNumChannels(int index)
{
	int nchnls = -1;
	CsoundEngine *e = m_qcs->getEngine(index);
	if (e != NULL) {
		CSOUND *cs = e->getCsound();
		if (cs != NULL) {
			nchnls = csoundGetNchnls (cs);
		}
	}
	return nchnls;
}

MYFLT *PyQcsObject::getTableArray(int ftable, int index)
{
	CsoundEngine *e = m_qcs->getEngine(index);
	if (e != NULL) {
		CSOUND *cs = e->getCsound();
		int ret = csoundGetTable(cs, m_tablePtr, ftable);
	}
	return *m_tablePtr;
}


void PyQcsObject::evaluateCsound(QString code)
{
	m_qcs->evaluateCsound(code);
}


//void PyQcsObject::writeListToTable(int ftable, QVariantList values, int offset, int count)
//{
//
//}

//QVariantList PyQcsObject::readTableToList(int ftable, int offset, int count)
//{
//  CSOUND *cs = m_qcs->getEngine()->getCsound();
//  PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
//  QVariantList list;
//  if (cs == 0) {
//    mainContext.evalScript("print 'Csound Engine invalid.'");
//    return list;
//  }
//  int tabLen = csoundTableLength(cs, ftable);
//  if (tabLen < 0) {
//    mainContext.evalScript("print '''readTableToList(): Invalid table " + QString::number(ftable) + "'''");
////    return QVariantList();
//  }
//  MYFLT **tablePtr = 0;
//  int ret = csoundGetTable(cs, tablePtr, ftable);
////  qDebug() << "PyQcsObject::readTableToList " << tablePtr << "--" << tabLen;
////  while (offset < tabLen && count > 0) {
////    list << (double) *(tablePtr[offset]);
////    offset++;
////    count--;
////  }
//  return list;
//}

//void PyQcsObject::writeArrayToTable(int ftable, QVariantList values, int offset, int count)
//{
//  qDebug() << "Not implemented";
//}
//
//QVariantList PyQcsObject::readArrayToList(int ftable, int offset, int count)
//{
//  qDebug() << "Not implemented";
//  return QVariantList();
//}

void  PyQcsObject::registerProcessCallback(QString func, int skipPeriods, int index)
{
	m_qcs->getEngine(index)->registerProcessCallback(func, skipPeriods);
	qDebug() << "PyQcsObject::registerProcessCallback" << func << skipPeriods;
	//  PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
	//  mainContext.evalScript(func);
}

void PyQcsObject::loadPreset(int presetIndex,int index)
{
	m_qcs->loadPreset(presetIndex, index);
}




