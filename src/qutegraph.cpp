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

#include "qutegraph.h"
#include "curve.h"
#include <cmath>
#include <QPalette>


#include <QColorDialog>


enum CsoundEngineStatus {
    Running=0,
    UserDataNotSet,
    CsoundInstanceNotSet,
    EngineNotSet,
    NotRunning
};


inline CsoundEngineStatus csoundEngineStatus(CsoundUserData *ud) {
    if(ud == nullptr) {
        return CsoundEngineStatus::UserDataNotSet;
    }
    if(ud->csound == nullptr) {
        return CsoundEngineStatus::CsoundInstanceNotSet;
    }
    if(ud->csEngine == nullptr) {
        return CsoundEngineStatus::EngineNotSet;
    }
    if(!ud->csEngine->isRunning()) {
        return CsoundEngineStatus::NotRunning;
    }
    return CsoundEngineStatus::Running;
}


// -----------------------------------------------------------------------------------------

QuteGraph::QuteGraph(QWidget *parent) : QuteWidget(parent)
{
	m_widget = new StackedLayoutWidget(this);
    //  m_widget->setAutoFillBackground(true);
    // m_widget->show();

    m_numticksY = 6;
	m_label = new QLabel(this);
	QPalette palette = m_widget->palette();
    palette.setColor(QPalette::WindowText, QColor(150, 150, 150));
	m_label->setPalette(palette);
    // m_label->setText("");
    m_label->setFont(QFont({"Helvetica", 7}));
    m_label->move(120, -4);
	m_label->resize(500, 25);
    m_label->setFocusPolicy(Qt::NoFocus);

    m_pageComboBox = new QComboBox(this);
    m_pageComboBox->setMinimumWidth(120);
    m_pageComboBox->setMaximumHeight(14);
    m_pageComboBox->setFont(QFont({"Sans", 7}));
    m_pageComboBox->setFocusPolicy(Qt::NoFocus);
    m_pageComboBox->setStyleSheet("QComboBox QAbstractItemView"
                                  "{ min-width: 150px; }");
    // m_pageComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    m_pageComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_drawGrid = true;
    m_drawTableInfo = true;
    m_showScrollbars = true;
	canFocus(false);
	connect(m_pageComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(indexChanged(int)));
	polygons.clear();

	QPalette Pal(this->palette());
    // set black background
    Pal.setColor(QPalette::Window, Qt::black); // old was QPalette::Background
    this->setAutoFillBackground(true);
    this->setPalette(Pal);
    this->setStyleSheet("QFrame { border: 0px; }");

	// Default properties
	setProperty("QCS_zoomx", 1.0);
	setProperty("QCS_zoomy", 1.0);
	setProperty("QCS_dispx", 1.0);
	setProperty("QCS_dispy", 1.0);
	setProperty("QCS_modex", "auto");
	setProperty("QCS_modey", "auto");
    setProperty("QCS_showSelector", true);
    setProperty("QCS_showGrid", true);
    setProperty("QCS_showTableInfo", true);
    setProperty("QCS_showScrollbars", true);
    setProperty("QCS_enableTables", true);
    setProperty("QCS_enableDisplays", true);
    m_enableTables = true;
    m_enableDisplays = true;
	setProperty("QCS_all", true);

    m_showPeak = false;
    m_showPeakCenterFrequency = 1000.0;
    m_showPeakRelativeBandwidth = 0.25;
    m_lastPeakFreq = 0;
    m_frozen = false;
    m_getPeakChannel = "";
    m_peakChannelPtr = nullptr;

    m_widget->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    m_widget->show();

}

QuteGraph::~QuteGraph()
{
}

QString QuteGraph::getWidgetLine()
{
	// Extension to MacCsound: type of graph (table, ftt, scope), value (which hold the index of the
	// table displayed) zoom and channel name
	// channel number is unused in QuteGraph, but selects channel for scope
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioGraph {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} table ";
	line += QString::number(m_value, 'f', 6) + " ";
	line += QString::number(property("QCS_zoomx").toDouble(), 'f', 6) + " ";
	line += m_channel;
	//   qDebug("QuteGraph::getWidgetLine(): %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteGraph::getWidgetXmlText()
{
	// Graphs are not implemented in blue
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	s.writeTextElement("value", QString::number((int)m_value));
	s.writeTextElement("objectName2", m_channel2);
	s.writeTextElement("zoomx", QString::number(property("QCS_zoomx").toDouble(), 'f', 8));
	s.writeTextElement("zoomy", QString::number(property("QCS_zoomy").toDouble(), 'f', 8));
	s.writeTextElement("dispx", QString::number(property("QCS_dispx").toDouble(), 'f', 8));
	s.writeTextElement("dispy", QString::number(property("QCS_dispy").toDouble(), 'f', 8));
	s.writeTextElement("modex", property("QCS_modex").toString());
	s.writeTextElement("modey", property("QCS_modey").toString());
    s.writeTextElement("showSelector",
                       property("QCS_showSelector").toBool() ? "true" : "false");
    s.writeTextElement("showGrid", property("QCS_showGrid").toBool() ? "true" : "false");
    s.writeTextElement("showTableInfo",
                       property("QCS_showTableInfo").toBool() ? "true" : "false");
    s.writeTextElement("showScrollbars",
                       property("QCS_showScrollbars").toBool() ? "true" : "false");
    s.writeTextElement("enableTables",
                       property("QCS_enableTables").toBool() ? "true" : "false");
    s.writeTextElement("enableDisplays", m_enableDisplays ? "true" : "false");
	s.writeTextElement("all", property("QCS_all").toBool() ? "true" : "false");
	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}


QString QuteGraph::getWidgetType() {
    return QString("BSBGraph");
}

void QuteGraph::setWidgetGeometry(int x,int y,int width,int height)
{
	QuteWidget::setWidgetGeometry(x,y,width, height);
	static_cast<StackedLayoutWidget *>(m_widget)->setWidgetGeometry(0,0,width, height);
	int index = static_cast<StackedLayoutWidget *>(m_widget)->currentIndex();
    changeCurve(-2);

    if (index < 0)
		return;
    // changeCurve(index);
}

void QuteGraph::mousePressEvent(QMouseEvent *event) {
    int index = (int)m_value;
    switch(graphtypes[index]) {
    case GraphType::GRAPH_SPECTRUM: {
        if(event->button() & Qt::LeftButton) {
            // turn on peak detection
            m_showPeakTemp = true;
            m_spectrumPeakMarkers[index]->setVisible(true);
            m_spectrumPeakTexts[index]->setVisible(true);
            m_mouseDragging = true;
            auto view = getView(index);
            auto peakIndex = view->mapToScene(event->x(), 0).x();
            auto curve = curves[index];
            auto freq = peakIndex / curve->get_size() * this->m_ud->sampleRate*0.5;
            m_showPeakTempFrequency = freq;
            return;
        }
        break;
    }
    default:
        break;
    }
}

void QuteGraph::mouseReleased() {
    m_mouseDragging = false;
    if(!m_showPeak ) {
        m_lastPeakFreq = 0;
        m_lastTextMarkerY = 0;
        m_lastTextMarkerX = 0;
    }
    m_showPeakTemp = false;
    int index = (int)m_value;
    m_spectrumPeakTexts[index]->setVisible(m_showPeak);
    // auto scene = getView(index)->scene();
    m_spectrumPeakMarkers[index]->setVisible(m_showPeak);
}

/*
void QuteGraph::mouseMoveEvent(QMouseEvent *event) {
    int index = (int)m_value;
    switch(graphtypes[index]) {
    case GraphType::GRAPH_SPECTRUM: {
        if(!m_mouseDragging)
            return;
        auto peakIndex = getView(index)->mapToScene(event->x(), 0).x();
        auto curveSize = curves[index]->get_size();
        m_showPeakTempFrequency = peakIndex / curveSize * this->m_ud->sampleRate*0.5;
        return;
    }
    default:
        break;
    }
}
*/


void QuteGraph::keyPressEvent(QKeyEvent *event) {
    bool flag;
    int index = m_value;
    auto graphtype = graphtypes[index];
    switch(event->key()) {
    case Qt::Key_F:
        if(graphtype != GraphType::GRAPH_SPECTRUM)
            return;
        freezeSpectrum(!m_frozen);
        event->accept();
        break;
    case Qt::Key_S:
        flag = !property("QCS_showSelector").toBool();
        setProperty("QCS_showSelector", flag?"true":"false");
        if(flag)
            m_pageComboBox->show();
        else
            m_pageComboBox->hide();
        event->accept();
        break;
    case Qt::Key_G:
        flag = !property("QCS_showGrid").toBool();
        setProperty("QCS_showGrid", flag?"true":"false");
        m_drawGrid = flag;
        event->accept();
        break;
    case Qt::Key_C:
        flag = !property("QCS_showTableInfo").toBool();
        setProperty("QCS_showTableInfo", flag?"true":"false");
        m_drawTableInfo = flag;
        if(flag)
            m_label->show();
        else
            m_label->hide();
        event->accept();
        break;
    case Qt::Key_Plus:
        setProperty("QCS_zoomx", property("QCS_zoomx").toDouble()*2);
        applyInternalProperties();
        event->accept();
        break;
    case Qt::Key_Minus:
        setProperty("QCS_zoomx", qMax(1.0, property("QCS_zoomx").toDouble()*0.5));
        applyInternalProperties();
        event->accept();
        break;
    case Qt::Key_Z:
        flag = !property("QCS_showScrollbars").toBool();
        setProperty("QCS_showScrollbars", flag);
        showScrollbars(flag);
        event->accept();
        break;
    case Qt::Key_H:
        QMessageBox mb;
        mb.setText(tr("<span style=\"font-size : 11pt\">"
                      "<ul>"
                      "<li><code>+</code> : zoom in"
                      "<li><code>-</code> : zoom out"
                      "<li><code>F</code> : toggle freeze (spectrum)"
                      "<li><code>S</code> : toggle Selector"
                      "<li><code>G</code> : toggle Grid"
                      "<li><code>C</code> : toggle table information"
                      "<li><code>Z</code> : toggle Scrollbars"
                      "</ul>"
                      "</span>"
                      ));
        mb.setWindowTitle("Graph Shortcuts");
        mb.exec();
        event->accept();
        break;
    }
}


void QuteGraph::setValue(double value)
{
    QuteWidget::setValue(value);
    // m_value2 = this->getTableNumForIndex(value);
    /*
    if(value < 0) {
        int tabnum = -((int)value);
        int index = this->getIndexForTableNum(tabnum);
        QuteWidget::setValue(index);
    } else {
        QuteWidget::setValue(value);
    }
    */

}

void QuteGraph::setValue(QString text)
{
    bool ok;
    //auto parts = text.split(' ', SKIP_EMPTY_PARTS); // splitRef
    auto parts = QStringView(text).split(' ', SKIP_EMPTY_PARTS);
    if(parts[0] == QLatin1String("@set")) {
        bool ok;
        int index = parts[1].toInt(&ok);
        if(parts.size() != 2 || !ok) {
            qDebug() << "@set: value error";
            qDebug() << "@set syntax: @set <curveindex:int>";
            return;
        }
        if(index >= 0 && index < curves.size())
            this->setValue(index);
    }
    else if(parts[0] == QLatin1String("@find")) {
        // @find type text
        // type: one of fft, audio, ftable
        // Example: @find fft asignal
        qDebug() << "@find " << parts;
        if(parts.size() != 3) {
            qDebug() << "@find: Wrong number of arguments";
            qDebug() << "syntax: @find <kind:str> <text:str>";
            qDebug() << "    * kind: one of fft, audio, ftable";
            qDebug() << "    * text: a text to match against the caption";
            return;
        }
        int index;
        if(parts[1] == QLatin1String("fft")) {
            index = findCurve(CURVE_SPECTRUM, parts[2]); // TODO: cresyr overload findCurve(..., QStringView)
        }
        else if (parts[1] == QLatin1String("audio") ) {
            index = findCurve(CURVE_AUDIOSIGNAL, parts[2]);
        }
        else if (parts[1] == QLatin1String("table") ) {
            int tabnum = parts[2].toInt(&ok);
            if(!ok) {
                qDebug()<<"@find table syntax: @find table <tablenumber:int>";
                return;
            }
            index = this->getIndexForTableNum(tabnum);
        }
        else {
            qDebug() << "@find: graph type should be one of 'table', 'fft' or 'audio', got"
                     << parts[1];
            return;
        }
        if(index >= 0 && index < curves.size())
            this->setValue(index);
        else {
            qDebug() << "@find: graph not found";
            return;
        }
    }    
    else if(parts[0] == QLatin1String("@freeze") ) {
        int index = m_value;
        if(graphtypes[index] != GraphType::GRAPH_SPECTRUM)
            return;
        bool ok;
        int status = parts[1].toInt(&ok);
        if(status != 0 && status != 1) {
            qDebug() << "Syntax: @freeze <status:int> (0 or 1)";
            return;
        }
        freezeSpectrum(status);
    }
    else if (parts[0] == QLatin1String("@showPeak") ) {
        if(parts.size() == 2) {
            if(parts[1] == QLatin1String("true") || parts[1] == QLatin1String("1")) {
                m_showPeak = true;
            }
            else if (parts[1] == QLatin1String("false") || parts[1] == QLatin1String("0")) {
                m_showPeak = false;
            } else {
                bool ok;
                double freq = parts[1].toDouble(&ok);
                if(!ok) {
                    qDebug() << "@showPeak: expected a frequency, got" << parts[1];
                    return;
                }
                m_showPeak = true;
                m_showPeakCenterFrequency = freq;
            }
        }
        else if (parts.size() == 3) {
            // @showPeak freq bw
            bool ok;
            double freq = parts[1].toDouble(&ok);
            if(!ok) {
                qDebug() << "@showPeak: expected a frequency, got" << parts[1];
                return;
            }
            double bw = parts[2].toDouble(&ok);
            if(!ok) {
                qDebug() << "@showPeak: expected a bandwidth as third arg, got" << parts[2];
                return;
            }
            m_showPeak = true;
            if(freq > 0)
                m_showPeakCenterFrequency = freq;
            m_showPeakRelativeBandwidth = bw;
        }
        else {
            qDebug() << "@showPeak syntax:";
            qDebug() << "   @showPeak false : turn off peak display";
            qDebug() << "   @showPeak 0     : turn off peak display";
            qDebug() << "   @showPeak true  : turn on peak display";
            qDebug() << "   @showPeak 1     : turn on peak display";
            qDebug() << "   @showPeak <freq:double> : turn on peak display, set freq";
            qDebug() << "   @showPeak <freq:double> <bandwidth:double>";
            qDebug() << "      (use freq=0 to only set bandwidth)";
            return;
        }
    }
    else if(parts[0] == QLatin1String("@getPeak")) {
        m_getPeakChannel = parts[1].toString();
        qDebug() << "@getPeak channel: " << m_getPeakChannel;
        MYFLT *ptr;
        csoundGetChannelPtr(m_ud->csound, (void **) &ptr,
                            m_getPeakChannel.toLocal8Bit().constData(),
                            CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL);
        m_peakChannelPtr = ptr;
    }
    else {
        qDebug() << "Command" << text << "not found";
    }
}


void QuteGraph::refreshWidget()
{
    bool needsUpdate = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	int index = 0;
	if (m_valueChanged) {
        index = (int) m_value;
        if(index < 0) {
            m_value2 = -index;
        } else {
            m_value2 = getTableNumForIndex(index);
        }
        m_value2Changed = false;
		m_valueChanged = false;
		needsUpdate = true;
	}
	else if (m_value2Changed) {
        index = getIndexForTableNum((int)m_value2);
        if (index >= 0) {
            m_value = index;
			//      m_valueChanged = false;
        }
        m_value2Changed = false;
        needsUpdate = true;
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();  // unlock
#endif
    if (needsUpdate) {
		if (index < 0) {
			index = getIndexForTableNum(-index);
		}
        if (index < 0 ||
            index >= curves.size() ||
            curves[index]->get_caption().isEmpty()) {
            // Don't show if curve has no name. Is this likely?
			return;
		}
		//    m_pageComboBox->blockSignals(true);
		//    m_pageComboBox->setCurrentIndex(index);
		//    m_pageComboBox->blockSignals(false);
		changeCurve(index);

	}
    // QComboBox *cb = this->m_pageComboBox;
    // cb->move(this->width() - cb->width(),  this->height()-cb->height());
}

void QuteGraph::createPropertiesDialog()
{
    QLabel *label;
	QuteWidget::createPropertiesDialog();
	dialog->setWindowTitle("Graph");

	channelLabel->setText("Index Channel name =");
	channelLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
	//  nameLineEdit->setText(getChannelName());

    label = new QLabel("F-table Channel name =", dialog);
    layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);
	name2LineEdit = new QLineEdit(dialog);
	name2LineEdit->setText(getChannel2Name());
	name2LineEdit->setMinimumWidth(320);
	layout->addWidget(name2LineEdit, 4,1,1,3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel("Zoom X", dialog);
    layout->addWidget(label, 8, 0, Qt::AlignRight|Qt::AlignVCenter);
	zoomxBox = new QDoubleSpinBox(dialog);
	zoomxBox->setRange(0.1, 10.0);
	zoomxBox->setDecimals(1);
	zoomxBox->setSingleStep(0.1);
	layout->addWidget(zoomxBox, 8, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel("Zoom Y", dialog);
    layout->addWidget(label, 8, 2, Qt::AlignRight|Qt::AlignVCenter);
	zoomyBox = new QDoubleSpinBox(dialog);
	zoomyBox->setRange(0.1, 10.0);
	zoomyBox->setDecimals(1);
	zoomyBox->setSingleStep(0.1);
	layout->addWidget(zoomyBox, 8, 3, Qt::AlignLeft|Qt::AlignVCenter);

    acceptTablesCheckBox = new QCheckBox(dialog);
    acceptTablesCheckBox->setText("Enable tables");
    acceptTablesCheckBox->setToolTip("Enable the display of tables. Each time a table is "
                                     "creted it will be made available to be selected.\n"
                                     "NB: modifications to a table will not be shown in the "
                                     "graph, use a TablePlot widget for that.\n"
                                     "NB2: If you plan to use the Graph widget to display"
                                     "spectra or signals, uncheck this option");
    acceptTablesCheckBox->setChecked(property("QCS_enableTables").toBool());
    layout->addWidget(acceptTablesCheckBox, 9, 0, Qt::AlignLeft|Qt::AlignVCenter);

    acceptDisplaysCheckBox = new QCheckBox(dialog);
    acceptDisplaysCheckBox->setText("Enable Displays");
    acceptDisplaysCheckBox->setToolTip("Enable displaying audio signals/spectra. Check this"
                                       "if you plan to use the graph widget to display audio"
                                       "signals and spectra using the opcodes display/dispfft");
    acceptDisplaysCheckBox->setChecked(property("QCS_enableDisplays").toBool());
    layout->addWidget(acceptDisplaysCheckBox, 9, 1, Qt::AlignLeft|Qt::AlignVCenter);

    showSelectorCheckBox = new QCheckBox("Show Selector", dialog);
    showSelectorCheckBox->setChecked(property("QCS_showSelector").toBool());
    layout->addWidget(showSelectorCheckBox, 10, 0, Qt::AlignLeft|Qt::AlignVCenter);

    showGridCheckBox = new QCheckBox("Show Grid", dialog);
    showGridCheckBox->setChecked(property("QCS_showGrid").toBool());
    showGridCheckBox->setToolTip("Show the grid. Has effect only for spectral graphs");
    layout->addWidget(showGridCheckBox, 10, 1, Qt::AlignLeft|Qt::AlignVCenter);

    showTableInfoCheckBox = new QCheckBox("Show Table Information", dialog);
    showTableInfoCheckBox->setCheckState(
                property("QCS_showTableInfo").toBool()?Qt::Checked:Qt::Unchecked);
    showTableInfoCheckBox->setToolTip("Show the grid. Has effect only for spectral graphs");
    layout->addWidget(showTableInfoCheckBox, 11, 0, Qt::AlignLeft|Qt::AlignVCenter);

    showScrollbarsCheckBox = new QCheckBox("Show Scrollbars", dialog);
    showScrollbarsCheckBox->setChecked(property("QCS_showScrollbars").toBool());
    layout->addWidget(showScrollbarsCheckBox, 11, 1, Qt::AlignLeft|Qt::AlignVCenter);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	zoomxBox->setValue(property("QCS_zoomx").toDouble());
	zoomyBox->setValue(property("QCS_zoomy").toDouble());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	//   channelLabel->hide();
	//   nameLineEdit->hide();
}

void QuteGraph::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setProperty("QCS_objectName2", name2LineEdit->text());
	setProperty("QCS_zoomx", zoomxBox->value());
	setProperty("QCS_zoomy", zoomyBox->value());
    setProperty("QCS_dispx", 1);
	setProperty("QCS_dispy", 1);
	setProperty("QCS_modex", "lin");
	setProperty("QCS_modey", "lin");
	setProperty("QCS_all", true);
    setProperty("QCS_showSelector", showSelectorCheckBox->checkState());
    setProperty("QCS_showGrid", showGridCheckBox->checkState());
    setProperty("QCS_showScrollbars", showScrollbarsCheckBox->isChecked());
    setProperty("QCS_enableTables", acceptTablesCheckBox->isChecked());   
    setProperty("QCS_enableDisplays", acceptDisplaysCheckBox->isChecked());

    m_enableTables = acceptTablesCheckBox->isChecked();
    m_enableDisplays = acceptDisplaysCheckBox->isChecked();

    showTableInfo(showTableInfoCheckBox->checkState());

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	QuteWidget::applyProperties();
}


int QuteGraph::findCurve(CurveType type, QStringView text) {
    // returns the index or -1 if not found
    for (int i = 0; i < curves.size(); i++) {
        if(curves[i]->get_type() != type)
            continue;
        QString caption = curves[i]->get_caption();
        if (caption.contains(text)) {
            return i;
        }
    }
    return -1;
}

void QuteGraph::changeCurve(int index)
{    
    if(curves.size() <= 0)
        return;

    int origRequest = index;
    StackedLayoutWidget *stacked =  static_cast<StackedLayoutWidget *>(m_widget);
    if (index == -1) { // goto last curve
        index = stacked->count() - 1;
	}
	else if (index == -2) { // update curve but don't change which
        if (m_value < 0)
			index = getIndexForTableNum(-m_value);
        else
			index = (int) m_value;
    } else if (index >= stacked->count()) {
        qDebug() << "changeCurve: index out of range. Num indices:"<<stacked->size();
        return;
    }
    else if (stacked->currentIndex() == index) {
        return;
    } else {
        // change curve
        auto view = stacked->currentWidget();
        view->hide();
        stacked->blockSignals(true);
        stacked->setCurrentIndex(index);
        stacked->blockSignals(false);
        m_pageComboBox->blockSignals(true);
        m_pageComboBox->setCurrentIndex(index);
        m_pageComboBox->blockSignals(false);
    }

    if (index < 0  || index >= curves.size()) { // Invalid index
        QDEBUG << "Invalid index" << index;
        return;
    }
    m_value = index;
    switch(graphtypes[index]) {
    case GraphType::GRAPH_FTABLE: {
        int ftable = getTableNumForIndex(index);
        if (m_value2 != ftable) {
            m_value2 = ftable;
            m_value2Changed = true;
        }
        if(m_drawTableInfo) {
            auto curve = curves[index];
            auto text = QString("%1 pts (%2, %3)")
                    .arg(curve->get_size())
                    .arg(curve->get_max(), 0, 'f', 3)
                    .arg(curve->get_min(), 0, 'f', 3);
            m_label->setText(text);
            m_label->show();
        }
        if(origRequest != -2) {
            drawGraph(curves[index], index);
        }
        break;
    }
    case GraphType::GRAPH_SPECTRUM:
        m_value2 = -1;
        m_label->hide();
        break;
    case GraphType::GRAPH_AUDIOSIGNAL:
        m_value2 = -1;
        m_label->hide();
        break;
    }
    scaleGraph(index);

}

void QuteGraph::indexChanged(int index)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	if (m_channel == "") {
		setInternalValue(index);
	}
	else {
        QPair<QString, double> channelValue(m_channel, index);
		emit newValue(channelValue);
	}
    if (m_channel2 == "") {
		setValue2(getTableNumForIndex(index));
	}
	else {
		QPair<QString, double> channel2Value(m_channel2, getTableNumForIndex(index));
		emit newValue(channel2Value);
	}

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif

}

void QuteGraph::clearCurves()
{
	//  curveLock.lock();
    m_widget->blockSignals(true);
	static_cast<StackedLayoutWidget *>(m_widget)->clearCurves();
	m_widget->blockSignals(false);
	m_pageComboBox->blockSignals(true);
	m_pageComboBox->clear();
	m_pageComboBox->blockSignals(false);
	curves.clear();
    graphtypes.clear();
	lines.clear();
	polygons.clear();
	m_gridlines.clear();
    m_gridTextsX.clear();
    m_gridTextsY.clear();
    m_spectrumPeakTexts.clear();
    m_spectrumPeakMarkers.clear();
    m_showPeak = false;
    m_showPeakTemp = false;
    m_showPeakCenterFrequency = 1000;
    //  curveLock.unlock();
}

void QuteGraph::addCurve(Curve * curve)
{
    QMutexLocker locker(&curveLock);
    Q_ASSERT(curve != nullptr);
    GraphType graphType;
    QGraphicsView *view;
    CurveType type = curve->get_type();
    if(type == CURVE_SPECTRUM) {
        if(!m_enableDisplays)
            return;
        graphType = GraphType::GRAPH_SPECTRUM;
        auto spectralView = new SpectralView(m_widget);
        view = static_cast<QGraphicsView*>(spectralView);
        connect(spectralView, SIGNAL(mouseReleased()), this, SLOT(mouseReleased()));
        view->setRenderHint(QPainter::Antialiasing);
    } else if(type == CURVE_FTABLE) {
        if(!m_enableTables)
            return;
        graphType = GraphType::GRAPH_FTABLE;
        view = new QGraphicsView(m_widget);
        view->setRenderHint(QPainter::Antialiasing);
    } else {
        if(!m_enableDisplays)
            return;
        graphType = GraphType::GRAPH_AUDIOSIGNAL;
        view = new QGraphicsView(m_widget);
    }
    QGraphicsScene *scene = new QGraphicsScene(view);
	view->setContextMenuPolicy(Qt::NoContextMenu);
    view->setScene(scene);
    view->setObjectName(curve->get_caption());
    auto scrollbarPolicy = property("QCS_showScrollbars").toBool() ? Qt::ScrollBarAsNeeded
                                                                   : Qt::ScrollBarAlwaysOff;
    view->setHorizontalScrollBarPolicy(scrollbarPolicy);
    view->show();
    scene->setBackgroundBrush(QBrush(Qt::black));
    lines.append(QVector<QGraphicsLineItem *>());
    QVector<QGraphicsLineItem *> gridLinesVector;
    QVector<QGraphicsTextItem *> gridTextVectorX;
    QVector<QGraphicsTextItem *> gridTextVectorY;
    qreal freqStep = 1000.0;
    // Max. samplerate. We generate a grid for this and can make lines/labels visible or
    // not depending on the actual sample rate
    qreal sr = 96000;
    int numTicksX = (int)(sr*0.5 / freqStep);
    int numTicksY = m_numticksY;
    auto gridpen = QPen(QColor(90, 90, 90));
    auto gridpen2 = QPen(QColor(60, 60, 60));
    gridpen.setCosmetic(true);
    gridpen2.setCosmetic(true);

    auto marker = new QGraphicsRectItem();
    marker->setVisible(false);
    auto markerpen = QPen(QColor(250, 250, 250));
    markerpen.setCosmetic(true);
    marker->setPen(markerpen);
    marker->setBrush(QBrush(QColor(255, 255, 255, 100)));
    m_spectrumPeakMarkers.append(marker);
    scene->addItem(marker);
    auto markerText = new QGraphicsTextItem();
    markerText->setDefaultTextColor(markerpen.color());
    markerText->setVisible(false);
    markerText->setFlags(QGraphicsItem::ItemIgnoresTransformations);
    markerText->setFont(QFont("Sans", 8));
    scene->addItem(markerText);
    m_spectrumPeakTexts.append(markerText);
    m_dbRange = 100;
    auto gridTextFont = QFont("Sans", 6);

    if(graphType == GraphType::GRAPH_SPECTRUM) {
        for (int i = 0 ; i < numTicksY; i++) {
            // 0 - 0db, 5 -> 100 dB (6:120
            QGraphicsLineItem *gridLine = new QGraphicsLineItem();
            gridLine->setPen(gridpen);
            scene->addItem(gridLine);
            gridLinesVector.append(gridLine);
            QGraphicsTextItem *gridText = new QGraphicsTextItem();
            gridText->setDefaultTextColor(Qt::gray);
            gridText->setFlags(QGraphicsItem::ItemIgnoresTransformations);
            int dbs = round(float(i)/numTicksY * 120.0);
            gridText->setHtml(QString("<div style=\"background:#000000;\">-%1 </p>"
                                      ).arg(dbs));
            gridText->setFont(gridTextFont);
            gridText->setVisible(false);
            scene->addItem(gridText);
            gridTextVectorY.append(gridText);

        }

        for (int i = 0; i < numTicksX; i++) {
            QGraphicsLineItem *gridLine = new QGraphicsLineItem();
            gridLine->setPen(i%2 == 0 ? gridpen : gridpen2);
            scene->addItem(gridLine);
            gridLinesVector.append(gridLine);
            QGraphicsTextItem *gridText = new QGraphicsTextItem();
            gridText->setDefaultTextColor(Qt::gray);
            gridText->setFlags(QGraphicsItem::ItemIgnoresTransformations);
            if (i > 0 && i%2==0) {
                // double kHz = i*((numTicksX-1.0)/numTicksX) * 2.0;
                double kHz = i * freqStep / 1000.0 ;
                if(i%10==0)
                    gridText->setHtml(
                                QString("<div style=\"background:#000000;\">%1k</p>")
                                .arg(kHz, 2, 'f', 1));
                else
                    gridText->setHtml(
                                QString("<div style=\"background:#000000;\">%1</p>")
                                .arg(kHz, 2, 'f', 1));

            }
            gridText->setFont(gridTextFont);
            gridText->setVisible(false);
            scene->addItem(gridText);
            gridTextVectorX.append(gridText);
        }
    }

    m_gridlines.append(gridLinesVector);
    m_gridTextsX.append(gridTextVectorX);
    m_gridTextsY.append(gridTextVectorY);

    graphtypes.append(graphType);

    QGraphicsPolygonItem * item = new QGraphicsPolygonItem();
    auto graphPen = QPen(Qt::yellow);
    graphPen.setCosmetic(true);
    item->setPen(graphPen);
    item->show();
    polygons.append(item);
    scene->addItem(item);
    view->setResizeAnchor (QGraphicsView::NoAnchor);
    // view->setFocusPolicy(Qt::NoFocus);
	m_pageComboBox->blockSignals(true);
    QString curveTitle = curve->get_caption().trimmed();
    if(curveTitle.endsWith(":"))
        curveTitle = curveTitle.mid(0, curveTitle.size()-1);
    m_pageComboBox->addItem(curveTitle);
    // m_pageComboBox->addItem(curve->get_title());
	m_pageComboBox->blockSignals(false);
	//  curveLock.lock();
	static_cast<StackedLayoutWidget *>(m_widget)->addWidget(view);
    curves.append(curve);
    if (m_value == curves.size() - 1) {
        // If new curve created corresponds to current stored value
		changeCurve(m_value);
	}
}

int QuteGraph::getCurveIndex(Curve * curve)
{
    Q_ASSERT(curve != nullptr);
	int index = -1;
	for (int i = 0; i < curves.size(); i++) {
		if (curves[i] == curve) {
			index = i;
			break;
		}
	}
    return index;
}

QGraphicsView * QuteGraph::getView(int index) {
    StackedLayoutWidget *widget_ = static_cast<StackedLayoutWidget *>(m_widget);
    QGraphicsView *view = static_cast<QGraphicsView *>(widget_->widget(index));
    return view;
}

void QuteGraph::drawGraph(Curve *curve, int index) {
    // QString caption = curve->get_caption();
    // auto view = getView(index);
    // switch(graphtypes[index]) {
    switch(curve->get_type()) {
    // case GraphType::GRAPH_FTABLE:
    case CurveType::CURVE_FTABLE:
        // drawFtable(curve, index);
        // view->setRenderHint(QPainter::Antialiasing);
        drawFtablePath(curve, index);
        scaleGraph(index);
        break;
    // case GraphType::GRAPH_SPECTRUM:
    case CurveType::CURVE_SPECTRUM:
        // view->setRenderHint(QPainter::Antialiasing);
        drawSpectrum(curve, index);
        // drawSpectrumPath(curve, index);
        changeCurve(-2); //update curve
        break;
    case CurveType::CURVE_AUDIOSIGNAL:
    // case GraphType::GRAPH_AUDIOSIGNAL:
        // drawSignal(curve, index);
        drawSignalPath(curve, index);
        changeCurve(-2); //update curve
        break;
    }
}

void QuteGraph::setCurveData(Curve * curve)
{
    Q_ASSERT(curve != nullptr);
	int index = getCurveIndex(curve);

    if (index >= curves.size() ||
        index < 0 ||
        index != m_value) {
        return;
	}

    /*
    if(!m_enableTables && graphtypes[index] == GraphType::GRAPH_FTABLE)
        return;
    */

    auto view = getView(index);

    // Refitting curves in view resets the scrollbar so we need the previous value
    int viewPosx = view->horizontalScrollBar()->value();
	int viewPosy = view->verticalScrollBar()->value();

    drawGraph(curve, index);
    view->horizontalScrollBar()->setValue(viewPosx);
    view->verticalScrollBar()->setValue(viewPosy);
}

void QuteGraph::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
    if(property("QCS_showSelector").toBool()) {
        m_pageComboBox->show();
    } else {
        m_pageComboBox->hide();
    }
	changeCurve(-2);  // Redraw
    m_drawGrid = property("QCS_showGrid").toBool();
    m_drawTableInfo = property("QCS_showTableInfo").toBool();

    showScrollbars(property("QCS_showScrollbars").toBool());
}

void QuteGraph::drawFtablePath(Curve *curve, int index) {
    Q_ASSERT(index >= 0);
    QGraphicsScene *scene = this->getView(index)->scene();
    // QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();

    double max = curve->get_max();
    max = max == 0 ? 1: max;
    int curveSize = curve->get_size();

    int decimate = curveSize /1024;
    if (decimate == 0) {
        decimate = 1;
    }
    auto pen = QPen(QColor(255, 255, 50), 0);
    // auto pen = QPen(QColor(255, 45, 7), 0);

    auto rect = this->rect();
    int width = rect.width();
    int step = curveSize / width;
    if(step == 0)
        step = 1;

    QPainterPath path;
    for (int i = 0; i < curveSize; i+=step) {
        double value = curve->get_data(i);
        path.lineTo(QPointF(i, -value));
    }
    scene->clear();
    if(step > 1) {
        pen.setWidth(0);
    }
    scene->addPath(path, pen);
}


void QuteGraph::drawFtable(Curve * curve, int index)
{
	//  bool live = curve->getOriginal() != 0;
	Q_ASSERT(index >= 0);
    QString caption = curve->get_caption();
    if (caption.isEmpty()) {
        return;
    }
    QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    double max = curve->get_max();
    max = max == 0 ? 1: max;
    int size = (int) curve->get_size();
    int decimate = size /1024;
    if (decimate == 0) {
        decimate = 1;
    }
    auto pen = QPen(QColor(255, 45, 7));
    pen.setCosmetic(true);
    if (lines[index].size() != size) {
        foreach (QGraphicsLineItem *line, lines[index]) {
            scene->removeItem(line);
            delete line;
        }
        lines[index].clear();
        for (int i = 0; i < size; i++) {
            if (decimate == 0 || i%decimate == 0) {
                QGraphicsLineItem *line = new QGraphicsLineItem(i, 0, i, 0);
                line->setPen(pen);
                lines[index].append(line);
                scene->addItem(line);
            }
        }
    }
    for (int i = 0; i < lines[index].size(); i++) { //skip first item, which is base line
        QGraphicsLineItem *line = static_cast<QGraphicsLineItem *>(lines[index][i]);
        MYFLT value = curve->get_data((i * decimate));
        line->setLine((i * decimate), 0, (i * decimate),  -value );
        line->show();
    }
    scaleGraph(index);
}

void QuteGraph::drawSpectrumPath(Curve *curve, int index) {
    int curveSize = curve->get_size();
    QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    QPainterPath path;
    double db0 = m_ud->zerodBFS;
    path.moveTo(0, -20.0*log10(fabs(curve->get_data(0))/db0));
    for(int i=1; i < curveSize; i++) {
        double value = 20.0*log10(fabs(curve->get_data(i))/db0);
        path.lineTo(QPointF(i, -value));
    }
    scene->clear();
    auto pen = QPen(Qt::yellow);
    pen.setCosmetic(true);

    QPainterPath gridPath;
    QPainter painter;

    if(m_drawGrid) {
        int sr = (int)this->getSr();
        int nyquist = sr / 2;
        int step = 1000;
        int numTicksX = nyquist / step;
        int numTicksY = 7;
        // printf("grid: nyquist: %d, numticks: %d\n", nyquist, numTicksX);

        auto gridPen = QPen(QColor(40, 40, 40));
        gridPen.setCosmetic(true);
        auto textColor = QColor(128, 128, 128);
        qreal curveSizeF = (qreal)curveSize;
        auto font = QFont("Sans");
        font.setPixelSize(9);
        const int maxy = 110;
        for (int i = 1; i < numTicksX; i++) {
            qreal freq = i * step;
            qreal x = freq/nyquist * curveSizeF;
            gridPath.moveTo(x, 0);
            gridPath.lineTo(x, maxy);
            if(i%2 == 0) {
                auto item = scene->addText(QString::number(freq/1000.0, 'f', 1), font);
                item->setDefaultTextColor(textColor);
                item->setPos(x, 0);
                item->setFlag(item->ItemIgnoresTransformations, true);
            }
        }
        for (int i=1; i < numTicksY-1; i++) {
            qreal y = (qreal)i/numTicksY * maxy;
            gridPath.moveTo(0, y);
            gridPath.lineTo(curveSizeF, y);
        }
        scene->addPath(gridPath, gridPen);
    }
    scene->addPath(path, pen);
}

size_t QuteGraph::spectrumGetPeak(Curve *curve, double freq, double bandwidth) {
    qreal sr = this->getSr(44100.);
    qreal nyquist = sr * 0.5;
    size_t curveSize = curve->get_size();
    qreal minfreq = freq - bandwidth*0.5;
    qreal maxfreq = freq + bandwidth*0.5;
    minfreq = qMax(1.0, minfreq);
    maxfreq = qMax(minfreq, maxfreq);
    size_t index0 = (size_t)(minfreq / nyquist * curveSize);
    index0 = index0 < curveSize - 1 ? index0 : curveSize - 1;
    size_t index1 = (size_t)(maxfreq / nyquist * curveSize);
    index1 = index1 < curveSize - 1 ? index1 : curveSize - 1;
    qreal maxvalue = 0;
    size_t maxindex = 0;
    if(!m_frozen) {
        for(auto i=index0; i<index1; i++) {
            auto data = curve->get_data(i);
            if (data > maxvalue) {
                maxvalue = data;
                maxindex = i;
            }
        }
    }
    else {
        for(auto i=index0; i<index1; i++) {
            auto data = frozenCurve[i];
            if (data > maxvalue) {
                maxvalue = data;
                maxindex = i;
            }
        }
    }
    if(maxindex > curveSize - 1 || maxindex < 0) {
        qDebug() << "spectrum peak: wrong index " << maxindex;
        return 0;
    }
    return maxindex;
}

QString mton(double midinote) {
    //                                C  C# D D#  E  F  F# G G# A Bb B
    static const int _pc2idx[] = {2, 2, 3, 3, 4, 5, 5, 6, 6, 0, 1, 1};
    static const int _pc2alt[] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 2, 0};
    static const char _alts[] = " #b";

    QString dst = "";
    double m = midinote;

    int octave = (int) (m / 12 - 1);
    int pc = (int)m % 12;
    int cents = round((m - floor(m)) * 100.0);
    int sign;

    if (cents == 0) {
        sign = 0;
    } else if (cents <= 50) {
        sign = 1;
    } else {
        cents = 100 - cents;
        sign = -1;
        pc += 1;
        if (pc == 12) {
            pc = 0;
            octave += 1;
        }
    }
    if(octave >= 0) {
        dst.append(QChar('0' + octave)); // Qt6 CHECK! this might be wrong!
    } else {
        dst.append('-');
        dst.append(QChar('0' - octave));
    }
    dst.append(QChar('A' + _pc2idx[pc]));
    int32_t alt = _pc2alt[pc];
    if(alt > 0) {
        dst.append(_alts[alt]);
    }
    if(sign == 1) {
        dst.append('+');
        if(cents != 50) {
            dst += QString::number(cents);
        }
    } else if(sign == -1) {
        dst.append('-');
        if(cents != 50) {
            dst += QString::number(cents);
        }
    }
    return dst;
}


QPoint quantizePoint(int x, int y, int resx, int resy) {
    x = static_cast<int>(round((double)x / resx) * resx);
    y = static_cast<int>(round((double)y / resy) * resy);
    return QPoint(x, y);
}

void QuteGraph::freezeSpectrum(bool status) {
    int index = m_value;
    auto curve = curves[index];
    if(status) {
        m_frozen = true;
        frozenCurve.resize(curve->get_size());
        for(size_t i=0; i < curve->get_size(); i++) {
            frozenCurve[i] = curve->get_data(i);
        }
    } else {
        m_frozen = false;
    }
}

void QuteGraph::drawSpectrum(Curve *curve, int index) {
    if(!curveLock.tryLock())
        return;
    int curveSize = curve->get_size();
    if(curveSize != frozenCurve.size() && graphtypes[index] == GraphType::GRAPH_SPECTRUM)
        freezeSpectrum(false);
    // auto view = getView(index);
    // QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    QVector<QPointF> polygonPoints;
    polygonPoints.resize(curveSize + 2);
    double dbRange = m_dbRange;
    polygonPoints[0] = QPointF(0, dbRange);
    double db0 = m_ud->zerodBFS;

    if(!m_frozen) {
        for (int i = 0; i < (int) curveSize; i++) {
            auto data = curve->get_data(i);
            double db = data > 0.000001 ? 20.0*log10(data/db0) : -dbRange;
            // double value = 20.0*log10(fabs(curve->get_data(i))/db0);
            polygonPoints[i+1] = QPointF(i, -db); //skip first item, which is base line
        }
    } else {
        for (int i = 0; i < frozenCurve.size(); i++) {
            auto data = frozenCurve[i];
            double db = data > 0.000001 ? 20.0*log10(data/db0) : -dbRange;
            // double value = 20.0*log10(fabs(curve->get_data(i))/db0);
            polygonPoints[i+1] = QPointF(i, -db); //skip first item, which is base line
        }
    }

    polygonPoints.back() = QPointF(curveSize - 1, dbRange);
    polygons[index]->setPolygon(QPolygonF(polygonPoints));

    // m_pageComboBox->setItemText(index, curve->get_caption());
    // draw Grid
    int numTicksY = m_numticksY;
    auto gridlinesvec = m_gridlines[index];
    auto gridtextvecx = m_gridTextsX[index];
    auto gridtextvecy = m_gridTextsY[index];
    qreal sr = this->getSr(44100.0);
    qreal nyquist = sr * 0.5;
    qreal freqStep = 1000.0;  // TODO: allow to configure this
    int numTicksX = (int)(nyquist / freqStep);
    // TODO: fix y axis
    if(m_drawGrid) {
        gridtextvecy[0]->setVisible(true);
        for (int i = 1; i < numTicksY; i++) {
            int y = float(i)/(numTicksY) * dbRange;
            gridlinesvec[i]->setLine(0, y, curveSize, y);
            gridlinesvec[i]->setVisible(true);
            gridtextvecy[i]->setPos(0, y);
            gridtextvecy[i]->setVisible(true);
        }

        for (int i = 1; i < numTicksX; i++) {
            // qreal x = i * qreal(curveSize)/numTicksX;
            qreal freq = i * freqStep;
            qreal x = freq/nyquist * curveSize;
            int idx = i + m_numticksY;
            gridlinesvec[idx]->setLine(x, 0, x, dbRange);
            gridlinesvec[idx]->setVisible(true);
            gridtextvecx[i]->setPos(x, 0);
            gridtextvecx[i]->setVisible(true);
        }
    } else {
        for(int i=0; i < numTicksY; i++) {
            gridlinesvec[i]->setVisible(false);
            gridtextvecy[i]->setVisible(false);
        }
        for (int i = 0; i < numTicksX; i++) {
            gridlinesvec[i+m_numticksY]->setVisible(false);
            gridtextvecx[i]->setVisible(false);
        }
    }

    if(m_showPeak || m_showPeakTemp) {
        auto freq = m_showPeakTemp ? m_showPeakTempFrequency : m_showPeakCenterFrequency;
        double bandwidth;
        if(m_showPeakTemp) {
            // if using the mouth to point at a near peak, we take zoom into account and
            // the bandwidth is a fraction of the displayed frequency range.
            auto zoomx = property("QCS_zoomx").toDouble();
            bandwidth = nyquist / zoomx / 8.0;
        }
        else
            bandwidth = freq * m_showPeakRelativeBandwidth;
        size_t peakIndex = this->spectrumGetPeak(curve, freq, bandwidth);
        auto data = m_frozen ? frozenCurve[peakIndex] : curve->get_data(peakIndex);
        double db = data > 0.000001 ? 20.0*log10(data/db0) : -dbRange;
        auto marker = m_spectrumPeakMarkers[index];
        auto view = this->getView(index);
        auto vcenter = view->mapFromScene(QPointF(peakIndex, -db));
        auto markerLeftTop = view->mapToScene(QPoint(vcenter.x() - 4, vcenter.y()-4));
        auto markerRightBottom = view->mapToScene(vcenter.x() + 4, vcenter.y()+4);
        marker->setRect(markerLeftTop.x(), markerLeftTop.y(),
                        markerRightBottom.x()-markerLeftTop.x(),
                        markerRightBottom.y() - markerLeftTop.y());
        auto markerText = m_spectrumPeakTexts[index];
        auto textPos = view->mapToScene(vcenter.x() + 10, vcenter.y() - 12);
        auto markerY = m_lastTextMarkerY <= 0 ? textPos.y() :
                                                textPos.y() * 0.2 + m_lastTextMarkerY * 0.8;
        auto markerX = m_lastTextMarkerX <= 0 ? textPos.x() :
                                                textPos.x() * 0.3 + m_lastTextMarkerX * 0.7;
        m_lastTextMarkerY = markerY;
        m_lastTextMarkerX = markerX;
        markerText->setPos(markerX, markerY);
        double factor = nyquist / curveSize;
        double peakFreq;
        if(peakIndex <= 1 || peakIndex >= static_cast<size_t>(curveSize - 2) ) {
            peakFreq = factor * peakIndex;
        }
        else {
            double freq0 = qreal(peakIndex-1)*factor;
            double freq1 = qreal(peakIndex)*factor;
            double freq2 = qreal(peakIndex+1)*factor;
            double amp1 = data;
            double amp0, amp2;
            if(!m_frozen) {
                amp0 = curve->get_data(peakIndex-1);
                amp2 = curve->get_data(peakIndex+1);
            } else {
                amp0 = frozenCurve[peakIndex-1];
                amp2 = frozenCurve[peakIndex+1];
            }
            amp1 *= amp1;
            amp0 *= amp0;
            amp2 *= amp2;
            peakFreq = (freq0*amp0 + freq1*amp1 + freq2*amp2) / (amp0+amp1+amp2);
        }
        peakFreq = m_lastPeakFreq <= 0 ? peakFreq : peakFreq * 0.2 + m_lastPeakFreq * 0.8;
        m_lastPeakFreq = peakFreq;
        double a4 = csoundGetA4(this->m_ud->csound);
        double midinote;
        QString notename;
        if(peakFreq > 10) {
            midinote = 12.0 * log2(peakFreq / a4) + 69.0;
            notename = mton(midinote);
        }
        else {
            // midinote = 0;
            notename = "LOW";
        }
        markerText->setPlainText(QString("%1 Hz (%2)").arg((int)(peakFreq+0.5)).arg(notename));

        marker->setVisible(true);
        markerText->setVisible(true);
        if(m_peakChannelPtr != nullptr) {
            *m_peakChannelPtr = peakFreq;
        }
    }
    else {
        if(m_getPeakChannel != nullptr) {
            *m_peakChannelPtr = 0;
        }
    }
    curveLock.unlock();
}

void QuteGraph::drawSignalPath(Curve *curve, int index) {
    int curveSize = curve->get_size();
    QPainterPath path;
    auto zerodbfs = m_ud->zerodBFS;
    for(int i=0; i<curveSize; i++) {
        auto value = curve->get_data(i)/zerodbfs;
        path.lineTo(i, value);
    }
    QPainterPath grid;
    grid.moveTo(0, 0);
    grid.lineTo(curveSize, 0);

    QGraphicsScene *scene = static_cast<QGraphicsView *>(static_cast<StackedLayoutWidget *>(m_widget)->widget(index))->scene();
    auto pen = QPen(QColor(255, 193, 7), 0);
    scene->clear();
    scene->addPath(grid, QPen(QColor(40, 40, 40), 0));
    scene->addPath(path, pen);
}

void QuteGraph::drawSignal(Curve *curve, int index)
{
    int curveSize = curve->get_size();
    QVector<QPointF> polygonPoints;
    polygonPoints.resize(curveSize + 2);
    polygonPoints[0] = QPointF(0,0);
    for (int i = 0; i < (int) curveSize; i++) {
        double value = curve->get_data(i)/m_ud->zerodBFS;
        polygonPoints[i + 1] = QPointF(i, value); //skip first item, which is base line
    }
    polygonPoints.back() = QPointF(curveSize - 1,0);
    polygons[index]->setPolygon(QPolygonF(polygonPoints));
    auto pen = QPen(QColor(255, 193, 7));
    pen.setCosmetic(true);
    polygons[index]->setPen(pen);
    polygons[index]->setBrush(Qt::NoBrush);
    m_pageComboBox->setItemText(index, curve->get_caption());
}

void QuteGraph::scaleGraph(int index)
{
    auto curve = curves[index];

    double max = curves[index]->get_max();
    double min = curves[index]->get_min();
	double zoomx = property("QCS_zoomx").toDouble();
	double zoomy = property("QCS_zoomy").toDouble();
	//  double span = max - min;
    //  FIXME implement dispx, dispy and modex, modey
    int size = curve->get_size();
    double sizef = (double)size;
    auto view = getView(index);
    //  view->setResizeAnchor(QGraphicsView::NoAnchor);
    auto graphType = graphtypes[index];
    if(graphType == GraphType::GRAPH_FTABLE && max != min) {
        double yrange = max - min;
        double factor = 1.17;
        view->setSceneRect(0, -max*factor - 0.05, sizef, yrange*factor);
        view->fitInView(0, -max*factor/zoomy, sizef/zoomx, yrange*factor/zoomy);
    } else if(graphType == GraphType::GRAPH_SPECTRUM) {
        double dbRange = m_dbRange;
        view->setSceneRect (0, -3, size, dbRange);
        view->fitInView(0, -3/zoomy, sizef/zoomx, dbRange/zoomy);
    } else { //from display opcode
        view->setSceneRect (0, -1, size, 2);
        // view->fitInView(0, -10./zoomy, (double) size/zoomx, 10./zoomy);
        view->fitInView(0, -2./zoomy, sizef/zoomx, 2./zoomy);
	}

}

int QuteGraph::getTableNumForIndex(int index) {
    if (index < 0 || index >= curves.size() || curves.size() <= 0) {
        // Invalid index
		return -1;
	}
	int ftable = -1;
    if(graphtypes[index] == GraphType::GRAPH_FTABLE) {
        QString caption = curves[index]->get_caption();
		ftable= caption.mid(caption.indexOf(" ") + 1,
							caption.indexOf(":") - caption.indexOf(" ") - 1).toInt();
	}
    return ftable;
}

int QuteGraph::getIndexForTableNum(int ftable)
{
	int index = -1;
	for (int i = 0; i < curves.size(); i++) {
		QString text = curves[i]->get_caption();
		if (text.contains("ftable")) {
            QStringList parts = text.split(QRegularExpression("[ :]"), Qt::SkipEmptyParts);
            if (parts.size() > 1) {
				int num = parts.last().toInt();
				if (ftable == num) {
					index = i;
					break;
				}
			}
		}
	}
	return index;
}

void QuteGraph::setInternalValue(double value)
{
	m_value = value;
	m_valueChanged = true;
}

void QuteGraph::showScrollbars(bool show) {
    auto policy = show ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
    for(int i=0; i < this->curves.size(); i++) {
        auto view = getView(i);
        view->setHorizontalScrollBarPolicy(policy);
    }
    m_showScrollbars = show;
}

// ----------------------
QuteTableWidget::~QuteTableWidget() {
};

void QuteTableWidget::reset() {
    // This needs to be called with the lock
    m_tabnum = 0;
    m_running = false;
    m_data = nullptr;
    m_tabsize = 0;
    if(m_autorange) {
        m_maxy = 1.0;
        m_miny = -1.0;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    m_path.clear();
#else
    m_path = QPainterPath(); // not sure if it works
#endif
}


void QuteTableWidget::paintGrid(QPainter *painter) {
    int margin = m_margin;
    QString maxystr, minystr;
    auto rect = this->rect();

    if(m_maxy >= 10)
        maxystr = QString::number((int)m_maxy);
    else if(m_maxy >= 1)
        maxystr = QString::number(m_maxy, 'f', 1);
    else
        maxystr = QString::number(m_maxy, 'f', 2);
    double absminy = abs(m_miny);
    if(absminy >= 10)
        minystr = QString::number((int)m_miny);
    else if(absminy >= 1)
        minystr = QString::number(m_miny, 'f', 1);
    else
        minystr = QString::number(m_miny, 'f', 2);

    const int yoffset = 0;
    auto x0 = rect.x() + margin;
    auto x1 = rect.x() + rect.width() - margin + 1;
    auto y0 = rect.y() + margin + yoffset;
    auto y1 = rect.y() + rect.height() - margin + yoffset ;
    auto height = rect.height() - margin*2;
    double yscale = -height / (m_maxy - m_miny);

    auto tabsizestr = QString::number(m_tabsize);
    const int textMargin = 4;
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(96, 96, 96), 0));

    // upper line
    painter->drawLine(rect.x() + gridFontMetrics.boundingRect(maxystr).width()+textMargin*2, y0, x1, y0);
    // lower line
    painter->drawLine(x0, y1, x1 - gridFontMetrics.boundingRect(tabsizestr).width() - 2, y1);

    // 0 line
    if (m_maxy > 0 && m_miny < 0) {
        int yzero = static_cast<int>(-m_miny * yscale + (y0+height));
        painter->drawLine(x0, yzero, x1, yzero);
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(rect.x(), yzero, "0");
    }

    painter->setPen(QColor(200, 200, 200));
    painter->setFont(gridFont);
    painter->drawText(rect.x()+textMargin, y0+textMargin, maxystr);
    painter->drawText(rect, Qt::AlignLeft|Qt::AlignBottom, minystr);

    // right padding
    rect.setWidth(rect.width() - textMargin);
    painter->drawText(rect, Qt::AlignRight|Qt::AlignBottom, tabsizestr);

}


void QuteTableWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    if(!m_running) {
        painter.setBrush(QColor(176, 0, 32));
        painter.drawRect(this->rect());
        painter.setPen(Qt::white);
        painter.drawText(this->rect(), Qt::AlignCenter, "Stopped");
        return;
    }
    if(m_tabnum <= 0) {
        // table not set
        painter.setBrush(QColor(0, 176, 32));
        painter.drawRect(this->rect());
        painter.setPen(Qt::white);
        painter.drawText(this->rect(), Qt::AlignCenter, "Table not set");
        return;
    }
    // running OK
    painter.setBrush(QColor(24, 24, 24));
    painter.drawRect(this->rect());
    painter.setBrush(Qt::NoBrush);
    blockSignals(true);
    mutex.lock();
    if(m_showGrid) {
        this->paintGrid(&painter);
    }
    painter.setPen(QPen(m_color, 0));
    painter.drawPath(m_path);
    mutex.unlock();
    blockSignals(false);
}

void QuteTableWidget::setRange(double maxy) {
    if(maxy == 0.0) {
        m_autorange = true;
    } else {
        m_autorange = false;
        m_maxy = maxy;
        m_miny = -maxy;
    }
}

void QuteTableWidget::updatePath() {
    if(!m_running || m_tabnum <= 0) {
        return;
    }


    //CS7 changes

    int tabsize = csoundTableLength(m_ud->csound, m_tabnum);
    MYFLT *data = nullptr;
    if (tabsize>0) {

        data = new double[tabsize]();
        // old: csoundTableCopyOut(m_ud->csound, m_tabnum, data, 0);
        int result = csoundGetTable(m_ud->csound, &data, m_tabnum);
        if (result<=0) {
            QDEBUG << "Table could not be read: " << m_tabnum;
            return;
        }

    } else {
        QDEBUG << "Table not found: " << m_tabnum;
        return;
    }

    m_tabsize = tabsize;
    int margin = m_margin;

    auto rect = this->rect();
    auto width = rect.width() - margin*2;
    auto height = rect.height() - margin*2;


    double xscale = width / (double)tabsize;
    double y0 = rect.y() + margin;
    double x0 = rect.x() + margin;

    int step = tabsize / width;
    if(step == 0)
        step = 1;

    double maxy = m_maxy;
    double miny = m_miny;

    if(m_autorange) {
        double newmaxy = m_maxy;
        double newminy = m_miny;
        for(int i=0; i < tabsize; i += step) {
            double y = data[i];
            if(y > newmaxy)
                newmaxy = y;
            else if (y < newminy)
                newminy = y;
        }
        maxy = m_maxy = ceil(newmaxy);
        miny = m_miny = floor(newminy);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    m_path.clear();
#else
    m_path = QPainterPath(); // not sure if it works
#endif
    QPolygonF poly;
    double yscale = -height / (maxy-miny);

    double ydata = data[0];
    poly.append(QPoint(x0, (ydata-miny)*yscale+y0+height));

    for(int i=1; i < tabsize; i+=step) {
        ydata = data[i];
        double x2 = i*xscale + x0;
        double y2 = (ydata - miny) * yscale + (y0+height);
        // path->lineTo(x2, y2);
        poly.append(QPointF(x2, y2));
    }
    m_path.addPolygon(poly);
    delete[] data;
}

void QuteTableWidget::updateData(int tabnum) {
    QMutexLocker locker(&mutex);
    if(!m_running) {
        return;
    }
    else if(tabnum >= 0 && tabnum != m_tabnum) {
        if(m_autorange) {
            m_maxy = 1.0;
            m_miny = 0;
        }
        m_tabnum = tabnum;
    }
    this->updatePath();
    this->update();
}


// -------------------------

QuteTable::~QuteTable() {};

// void QuteTable::mousePressEvent(QMouseEvent *event) {};
// void QuteTable::mouseReleaseEvent(QMouseEvent *event) {};

QuteTable::QuteTable(QWidget *parent) : QuteWidget(parent) {
    m_widget = new QuteTableWidget(this);
    m_value = 0;
    m_tabnum = 0;
    // auto w = static_cast<QuteTableWidget*>(m_widget);
    setProperty("QCS_randomizable", false);
    m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    m_widget->setMouseTracking(true);
    setProperty("QCS_range", 0.0);
    setColor(QColor(255, 193, 3));
    setProperty("QCS_showGrid", true);
}

void QuteTable::setColor(QColor color) {
    setProperty("QCS_color", color);
    static_cast<QuteTableWidget*>(m_widget)->setColor(color);
}

void QuteTable::applyInternalProperties() {
    QuteWidget::applyInternalProperties();
    auto w = static_cast<QuteTableWidget*>(m_widget);
    auto color = property("QCS_color").value<QColor>();
    w->setColor(color);
    w->setRange(property("QCS_range").toDouble());
    w->showGrid(property("QCS_showGrid").toBool());
}

void QuteTable::createPropertiesDialog() {
    QuteWidget::createPropertiesDialog();
    dialog->setWindowTitle("Table Plot");
    auto label = new QLabel(dialog);
    label->setText("Color");
    layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);

    colorButton = new SelectColorButton(dialog);
    colorButton->setColor(property("QCS_color").value<QColor>());
    layout->addWidget(colorButton, 4, 1, Qt::AlignLeft|Qt::AlignVCenter);

    double range = property("QCS_range").toDouble();
    rangeCheckBox = new QCheckBox("Fixed Range", dialog);
    rangeCheckBox->setToolTip("If checked the range is fixed to the value set on the right."
                              "If the table has values outside this range these will not be"
                              "displayed");
    rangeCheckBox->setChecked(range > 0.0);
    layout->addWidget(rangeCheckBox, 5, 1, Qt::AlignRight|Qt::AlignVCenter);

    rangeSpinBox = new QDoubleSpinBox(dialog);
    rangeSpinBox->setRange(0.1, 999999.0);
    rangeSpinBox->setSingleStep(0.1);
    rangeSpinBox->setToolTip("Sets the max. range to plot. Disable this to use autorange"
                             "With a fixed range, values outside the range will not be "
                             "visible");
    rangeSpinBox->setValue(range > 0 ? range : 1.0);
    layout->addWidget(rangeSpinBox, 5, 2, Qt::AlignLeft|Qt::AlignVCenter);
    connect(rangeCheckBox, SIGNAL(toggled(bool)), rangeSpinBox, SLOT(setEnabled(bool)));

    gridCheckBox = new QCheckBox("Show Grid", dialog);
    gridCheckBox->setChecked(property("QCS_showGrid").toBool());
    layout->addWidget(gridCheckBox, 6, 1, Qt::AlignLeft|Qt::AlignVCenter);

}

void QuteTable::applyProperties() {
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif
    setProperty("QCS_color", colorButton->getColor());
    bool fixedrange = rangeCheckBox->isChecked();
    double range = rangeSpinBox->value();
    if(fixedrange) {
        Q_ASSERT(range > 0.0);
        setProperty("QCS_range", range);
    } else {
        setProperty("QCS_range", 0.0);
    }
    setProperty("QCS_showGrid", gridCheckBox->isChecked());

#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    QuteWidget::applyProperties();
}

void QuteTable::refreshWidget() {
    // Q_ASSERT(m_valueChanged);
    QMutexLocker locker(&mutex);
    m_valueChanged = false;
    auto status = csoundEngineStatus(m_csoundUserData);
    m_csoundRunning = status == CsoundEngineStatus::Running;
    auto w = static_cast<QuteTableWidget*>(m_widget);
    w->setRunningStatus(m_csoundRunning);
    if(status != CsoundEngineStatus::Running) {
        return;
    }
    w->updateData(m_tabnum);
}

QString QuteTable::getWidgetXmlText() {
    xmlText = "";
    QXmlStreamWriter s(&xmlText);
    createXmlWriter(s);        // --------- start

    QColor color = property("QCS_color").value<QColor>();
    s.writeStartElement("color");
    s.writeTextElement("r", QString::number(color.red()));
    s.writeTextElement("g", QString::number(color.green()));
    s.writeTextElement("b", QString::number(color.blue()));
    s.writeEndElement();

    auto range = property("QCS_range").toDouble();
    s.writeTextElement("range", QString::number(range, 'f', 2));

    s.writeEndElement();      // --------- end
    return xmlText;
}

void QuteTable::onStop() {
    // this->blockSignals(true);
    mutex.lock();
    m_tabnum = 0;
    m_value = 0;
    m_valueChanged = true;

    static_cast<QuteTableWidget*>(m_widget)->stop(m_csoundUserData);
    mutex.unlock();
    // this->applyInternalProperties();
    // this->blockSignals(false);
}

void QuteTable::setCsoundUserData(CsoundUserData *ud) {
    if(ud == nullptr) {
        qDebug()<<"CsoundUserData is null";
        return;
    }
    m_csoundUserData = ud;
    auto w = static_cast<QuteTableWidget*>(m_widget);
    if(w == nullptr) {
        qDebug() << "widget is null";
        return;
    }
    w->setUserData(ud);
    connect(ud->csEngine, SIGNAL(stopSignal()), this, SLOT(onStop()));
}

/*
void QuteTable::setWidgetGeometry(int x, int y, int width, int height) {

    if(csoundEngineStatus(m_csoundUserData)==CsoundEngineStatus::Running) {
        return;
    }
    QuteWidget::setWidgetGeometry(x,y,width, height);
}
*/

void QuteTable::setTableNumber(int tabnum) {
    if(tabnum == m_tabnum)
        return;
    m_valueChanged = true;
    m_value = tabnum;
    m_tabnum = tabnum;
    auto w = static_cast<QuteTableWidget*>(m_widget);
    w->updateData(m_tabnum);
}

void QuteTable::setValue(double value) {
    if(value == 0) {
        m_value = m_tabnum;
        return;
    }
    else if(value == -1) {
        if(m_tabnum <= 0) {
            QDEBUG << "Table number not set, can't update";
            return;
        }
        // update data, don't change table number
        m_valueChanged = true;
        // auto w = static_cast<QuteTableWidget*>(m_widget);
        // w->updateData(m_tabnum);
        return;
    }
    else if(m_value == value) {
        return;
    }
    else if(value > 0) {
        setTableNumber(static_cast<int>(value));
    }
    else {
        qDebug() << "Invalid value for TablePlot:" << value;
        return;
    }
};

void QuteTable::setValue(QString s) {
    auto parts = QStringView(s).split(' ', SKIP_EMPTY_PARTS); // was splitRef
    if(parts.size() == 0) {
        qWarning() << "TablePLot: Message not understood, expected @set <tabnum> "
                    "or @update";
        return;
    }
    if(parts[0] == QLatin1String("@set")) {
        if(parts.size() != 2) {
            qWarning() << "@set message expects a table number (example: @set 103)";
            return;
        }
        int tabnum = parts[1].toInt();
        setTableNumber(tabnum);
    } else if (parts[0] == QLatin1String("@update") && m_tabnum > 0) {
        setValue(-1);
    } else
        qWarning() << "Message not supported:" << s;
}

SpectralView::~SpectralView() {};

void SpectralView::mouseReleaseEvent(QMouseEvent *ev) {
    emit mouseReleased();
    QGraphicsView::mouseReleaseEvent(ev);
}
