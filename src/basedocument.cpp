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

#include "basedocument.h"

#include "widgetlayout.h"
#include "documentview.h"
#include "csoundengine.h"
#include "csoundqt.h"
#include "qutebutton.h"
#include "console.h"


BaseDocument::BaseDocument(QWidget *parent, OpEntryParser *opcodeTree, ConfigLists *configlists) :
	QObject(parent), m_opcodeTree(opcodeTree), m_csEngine(0)
{
	m_view = 0;
	m_csEngine = new CsoundEngine(configlists);
	//FIXME widgetlayout should have the chance of being empty
	m_widgetLayouts.append(this->newWidgetLayout());
	m_csEngine->setWidgetLayout(m_widgetLayouts[0]);  // Pass first widget layout to engine
	//  m_view->setOpcodeNameList(opcodeNameList);
	//  m_view->setOpcodeTree(m_opcodeTree);
	m_console = new ConsoleWidget(0);
	m_console->setReadOnly(true);
	// Register the console with the engine for message printing
	m_csEngine->registerConsole(m_console);
    m_status = PlayStopStatus::Ok;
	connect(m_console, SIGNAL(keyPressed(int)),
			m_csEngine, SLOT(keyPressForCsound(int)));
	connect(m_console, SIGNAL(keyReleased(int)),
			m_csEngine, SLOT(keyReleaseForCsound(int)));

	acceptsMidiCC = true;
}

BaseDocument::~BaseDocument()
{
	disconnect(m_csEngine, 0,0,0);
	//  disconnect(m_widgetLayout, 0,0,0);
    m_csEngine->stop();
    delete m_csEngine;
	while (!m_widgetLayouts.isEmpty()) {
		WidgetLayout *wl = m_widgetLayouts.takeLast();
		wl->hide();
        delete wl;
    }
}

/* Returns the number of panels
 */
int BaseDocument::parseAndRemoveWidgetText(QString &text)
{

	QStringList xmlPanels;
    while(true) {
        auto panelStart = text.indexOf("<bsbPanel");
        if(panelStart < 0) {
            QDEBUG "Didn't find any more panels";
            break;
        }
        auto panelEnd = text.indexOf("</bsbPanel>", panelStart);
        if(panelEnd < 0) {
            QDEBUG << "Did not find matching </bsbPanel> tag";
            return 0;
        }
        auto panel = text.mid(panelStart, panelEnd+11-panelStart);
        xmlPanels << panel;
        text.remove(panelStart, panelEnd+11-panelStart);

    }
    /*
    while (text.contains("<bsbPanel") && text.contains("</bsbPanel>")) {
		QString panel = text.right(text.size()-text.indexOf("<bsbPanel"));
		panel.resize(panel.indexOf("</bsbPanel>") + 11);
        if (text.indexOf("</bsbPanel>") + 11 < text.size() &&
                text[text.indexOf("</bsbPanel>") + 13] == '\n')
			text.remove(text.indexOf("</bsbPanel>") + 13, 1); //remove final line break
        if (text.indexOf("<bsbPanel") > 0 && text[text.indexOf("<bsbPanel") - 1] == '\n')
			text.remove(text.indexOf("<bsbPanel") - 1, 1); //remove initial line break
		text.remove(text.indexOf("<bsbPanel"), panel.size());
		xmlPanels << panel;
		// TODO enable creation of several panels
	}
    */
	if (!xmlPanels.isEmpty()) {
		//FIXME allow multiple layouts
        auto t0 = std::chrono::high_resolution_clock::now();
        m_widgetLayouts[0]->loadXmlWidgets(xmlPanels[0]);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration<double, std::milli>(t1-t0).count();
        QDEBUG << "loadXmlWidgets" << diff << "ms";

        m_widgetLayouts[0]->markHistory();
        auto presetsStart = text.indexOf("<bsbPresets>");
        if(presetsStart >= 0) {
            auto presetsEnd = text.indexOf("</bsbPresets>", presetsStart);
            if(presetsEnd < 0) {
                QDEBUG << "Missing </bsbPresets> tag";
            }
            else {
                auto presetsSize = presetsEnd - presetsStart + 13;
                auto presets = text.mid(presetsStart, presetsSize);
                m_widgetLayouts[0]->loadXmlPresets(presets);
                text.remove(presetsStart, presetsSize);
            }
        }
        /*
        if (text.contains("<bsbPresets>") && text.contains("</bsbPresets>")) {
			QString presets = text.right(text.size()-text.indexOf("<bsbPresets>"));
			presets.resize(presets.indexOf("</bsbPresets>") + 13);
            if (text.indexOf("</bsbPresets>") + 13 < text.size() &&
                    text[text.indexOf("</bsbPresets>") + 15] == '\n') {
                //remove final line break
                text.remove(text.indexOf("</bsbPresets>") + 15, 1);
            }
            if (text.indexOf("<bsbPresets>") > 0
                    && text[text.indexOf("<bsbPresets>") - 1] == '\n') {
                // remove initial line break
                text.remove(text.indexOf("<bsbPresets>") - 1, 1);
            }
			text.remove(text.indexOf("<bsbPresets>"), presets.size());
            // FIXME allow multiple
			m_widgetLayouts[0]->loadXmlPresets(presets);
		}
        */
    }
    else {
        QString defaultPanel = "<bsbPanel><visible>true</visible><x>100</x><y>100</y>"
                               "<width>320</width><height>240</height></bsbPanel>";
		m_widgetLayouts[0]->loadXmlWidgets(defaultPanel);
		m_widgetLayouts[0]->markHistory();
	}
	return xmlPanels.size();
}

WidgetLayout* BaseDocument::newWidgetLayout()
{
	WidgetLayout* wl = new WidgetLayout(0);
    wl->setWindowFlags(Qt::Window | wl->windowFlags());
	connect(wl, SIGNAL(queueEventSignal(QString)),this,SLOT(queueEvent(QString)));
	connect(wl, SIGNAL(registerButton(QuteButton*)),
			this, SLOT(registerButton(QuteButton*)));
	return wl;
}

void BaseDocument::widgetsVisible(bool visible)
{
	for (int i = 0; i < m_widgetLayouts.size(); i++) {
		m_widgetLayouts[i]->setVisible(visible);
		m_widgetLayouts[i]->raise();
	}
}

void BaseDocument::setFlags(int flags)
{
	m_csEngine->setFlags((PerfFlags) flags);
}

void BaseDocument::setAppProperties(AppProperties properties)
{
	m_view->setAppProperties(properties);
}

QString BaseDocument::getFullText()
{
    QStringList parts;
    parts << m_view->getFullText();
    if (fileName.endsWith(".csd",Qt::CaseInsensitive) || fileName == "") {
        parts << getWidgetsText() ;
        parts << getPresetsText() << "\n";
	}
	else { // Not a csd file
        for(auto wl: m_widgetLayouts) {
        // foreach (WidgetLayout * wl, m_widgetLayouts) {
			wl->clearWidgets(); // make sure no widgets are used.
		}
	}
    return parts.join("");
}

QString BaseDocument::getBasicText()
{
	QString text = m_view->getBasicText();
	return text;
}

QString BaseDocument::getOrc()
{
	QString text = m_view->getOrc();
	return text;
}

QString BaseDocument::getSco()
{
	QString text = m_view->getSco();
	return text;
}

QString BaseDocument::getOptionsText()
{
	QString text = m_view->getOptionsText();
	return text;
}

QString BaseDocument::getWidgetsText()
{
	//FIXME allow multiple
	QString text = m_widgetLayouts[0]->getWidgetsText();
	QDomDocument d;
	d.setContent(text);
	//  QDomElement n = d.firstChildElement("bsbPanel");
	//  if (!n.isNull()) {
	//  }
	return d.toString();
}

QString BaseDocument::getPresetsText()
{
	//FIXME allow multiple
	return m_widgetLayouts[0]->getPresetsText();
}


//void BaseDocument::setOpcodeNameList(QStringList opcodeNameList)
//{
//  m_view->setOpcodeNameList(opcodeNameList);
//}

WidgetLayout *BaseDocument::getWidgetLayout()
{
	//FIXME allow multiple layouts
	return m_widgetLayouts[0];
}

ConsoleWidget *BaseDocument::getConsole()
{
	return m_console;
}

CsoundEngine *BaseDocument::getEngine()
{
	return m_csEngine;
}

int BaseDocument::play(CsoundOptions *options)
{
    while(m_status == PlayStopStatus::Stopping) {
        QDEBUG << "Stopping, waiting";
        QThread::msleep(100);
    }
    mutex.lock();
    m_status = PlayStopStatus::Starting;
    if (!m_csEngine->isRunning()) {
		foreach (WidgetLayout *wl, m_widgetLayouts) {
			wl->flush();   // Flush accumulated values
			wl->clearGraphs();
		}
	}
    m_status = PlayStopStatus::Ok;
    mutex.unlock();
    return m_csEngine->play(options);
}

void BaseDocument::pause()
{
	m_csEngine->pause();
}

void BaseDocument::stop()
{
    if (!m_csEngine->isRunning()) {
        QDEBUG << "Csound is not running";
        return;
    }
    if(m_status == PlayStopStatus::Stopping) {
        qDebug() << "Already stopping";
        return;
    }
    if(m_status == PlayStopStatus::Starting) {
        QDEBUG << "Asked to stop, but we are already starting";
        return;
    }
    QDEBUG << "getting lock";
    mutex.lock();
    QDEBUG << "locked, stopping engine";
    m_status = PlayStopStatus::Stopping;

    m_csEngine->stop();
    QDEBUG << "Engine stopped, signaling widgets...";
    foreach (WidgetLayout *wl, m_widgetLayouts) {
        // TODO only needed to flush graph buffer, but this should be moved to this class
        wl->engineStopped();
    }
    m_status = PlayStopStatus::Ok;
    mutex.unlock();
    QDEBUG << "Stopped OK";
}

int BaseDocument::record(int format)
{
#ifdef PERFTHREAD_RECORD
	return m_csEngine->startRecording(format, "output.wav");
#else
    (void) format;
    QMessageBox::warning(NULL, tr("Recording not possible"),
                         tr("This version of CsoundQt was not built with recording support."));
	return 0;
#endif
}

void BaseDocument::stopRecording()
{
	m_csEngine->stopRecording();
}

void BaseDocument::queueEvent(QString eventLine, int delay)
{
	m_csEngine->queueEvent(eventLine, delay);  //TODO  implement passing of timestamp
}

void BaseDocument::loadTextString(QString &text)
{
	setTextString(text);
	m_view->clearUndoRedoStack();
}

void BaseDocument::setFileName(QString name)
{
	fileName = name;
	if (name.endsWith(".csd") || name.isEmpty()) {
		m_view->setFileType(EDIT_CSOUND_MODE);
	}
	else if (name.endsWith(".py")) {
		m_view->setFileType(EDIT_PYTHON_MODE);
	}
	else if (name.endsWith(".xml")) {
		m_view->setFileType(EDIT_XML_MODE);
	}
	else if (name.endsWith(".orc")) {
		m_view->setFileType(EDIT_ORC_MODE);
	}
	else if (name.endsWith(".udo")) {
		m_view->setFileType(EDIT_ORC_MODE);
	}
	else if (name.endsWith(".sco")) {
		m_view->setFileType(EDIT_SCO_MODE);
	}
    else if (name.endsWith(".inc")) {
        m_view->setFileType(EDIT_INC_MODE);
    }
	else if (name.endsWith(".html")) {
		m_view->setFileType(EDIT_HTML_MODE);
	}
	else {
		m_view->setFileType(EDIT_GENERIC_MODE);
	}
}

AppProperties BaseDocument::getAppProperties()
{
	return m_view->getAppProperties();
}
