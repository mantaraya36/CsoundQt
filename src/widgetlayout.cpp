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

#include <cstdlib>

#include <QThread>

#include "widgetlayout.h"
#include "qutewidget.h"
#include "quteslider.h"
#include "qutetext.h"
#include "qutespinbox.h"
#include "qutebutton.h"
#include "quteknob.h"
#include "qutecheckbox.h"
#include "qutecombobox.h"
#include "qutemeter.h"
#include "quteconsole.h"
#include "qutegraph.h"
#include "qutescope.h"
#include "qutedummy.h"
#include "framewidget.h"

#include "csoundqt.h" // For passing the actions from button reserved channels


WidgetLayout::WidgetLayout(QWidget* parent) : QWidget(parent)
{
    selectionFrame = new QRubberBand(QRubberBand::Rectangle, this);
    selectionFrame->hide();
    m_trackMouse = true;
    m_editMode = false;
    m_enableEdit = true;
    m_xmlFormat = true;
    m_currentPreset = -1;
    m_activeWidgets = 0;
    m_updateRate = 30;

    auto palette = qApp->palette();
    auto isLightTheme = palette.text().color().lightness() < palette.window().color().lightness();

    m_modified = false;
    closing = 0;
    mouseX = mouseY = mouseRelX = mouseRelY = mouseBut1 = mouseBut2 = 0;
    m_posx = m_posy =  m_w =  m_h = 0;
    xOffset = yOffset = 0;
    mouseRelX = mouseRelY = 0;
    m_contained = false;

    midiWriteCounter = 0;
    midiReadCounter = 0;
    midiQueue.resize(CSQT_MAX_MIDI_QUEUE);
    for (int i = 0; i <CSQT_MAX_MIDI_QUEUE; i++ ) {
        midiQueue[i].resize(3);
    }

    curveUpdateBuffer.resize(CSQT_CURVE_BUFFER_SIZE);
    curveUpdateBufferCount = 0;

    createSliderAct = new QAction(tr("Slider"),this);
    connect(createSliderAct, SIGNAL(triggered()), this, SLOT(createNewSlider()));
    createLabelAct = new QAction(tr("Label"),this);
    connect(createLabelAct, SIGNAL(triggered()), this, SLOT(createNewLabel()));
    createDisplayAct = new QAction(tr("Display"),this);
    connect(createDisplayAct, SIGNAL(triggered()), this, SLOT(createNewDisplay()));
    createScrollNumberAct = new QAction(tr("ScrollNumber"),this);
    connect(createScrollNumberAct, SIGNAL(triggered()),
            this, SLOT(createNewScrollNumber()));
    createLineEditAct = new QAction(tr("LineEdit"),this);

    connect(createLineEditAct, SIGNAL(triggered()), this, SLOT(createNewLineEdit()));

    createSpinBoxAct = new QAction(tr("SpinBox"),this);
    connect(createSpinBoxAct, SIGNAL(triggered()), this, SLOT(createNewSpinBox()));

    createButtonAct = new QAction(tr("Button"),this);
    connect(createButtonAct, SIGNAL(triggered()), this, SLOT(createNewButton()));

    createKnobAct = new QAction(tr("Knob"),this);
    connect(createKnobAct, SIGNAL(triggered()), this, SLOT(createNewKnob()));

    createCheckBoxAct = new QAction(tr("Checkbox"),this);
    connect(createCheckBoxAct, SIGNAL(triggered()), this, SLOT(createNewCheckBox()));

    createMenuAct = new QAction(tr("Menu"),this);
    connect(createMenuAct, SIGNAL(triggered()), this, SLOT(createNewMenu()));

    createMeterAct = new QAction(tr("Controller"),this);
    connect(createMeterAct, SIGNAL(triggered()), this, SLOT(createNewMeter()));

    createConsoleAct = new QAction(tr("Console"),this);
    connect(createConsoleAct, SIGNAL(triggered()), this, SLOT(createNewConsole()));

    createGraphAct = new QAction(tr("Graph"),this);
    connect(createGraphAct, SIGNAL(triggered()), this, SLOT(createNewGraph()));

    createScopeAct = new QAction(tr("Scope"),this);
    connect(createScopeAct, SIGNAL(triggered()), this, SLOT(createNewScope()));

    createTableDisplayAct = new QAction(tr("Table Plot"), this);
    connect(createTableDisplayAct, SIGNAL(triggered()), this, SLOT(createNewTableDisplay()));

    propertiesAct = new QAction(tr("Properties"),this);
    connect(propertiesAct, SIGNAL(triggered()), this, SLOT(propertiesDialog()));

    reloadWidgetsAct = new QAction(tr("Reload Widgets"), this);
    connect(reloadWidgetsAct, SIGNAL(triggered()), this, SLOT(reloadWidgets()));

    cutAct = new QAction(tr("Cut"), this);
    //  cutAct->setShortcut(tr("Ctrl+X"));
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));
    copyAct = new QAction(tr("Copy"), this);
    //  copyAct->setShortcut(tr("Ctrl+C"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));
    pasteAct = new QAction(tr("Paste"), this);
    //  pasteAct->setShortcut(tr("Ctrl+V"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
    duplicateAct = new QAction(tr("Duplicate Selected"), this);
    connect(duplicateAct, SIGNAL(triggered()), this, SLOT(duplicate()));
    deleteAct = new QAction(tr("Delete Selected"), this);
    connect(deleteAct, SIGNAL(triggered()), this, SLOT(deleteSelected()));
    clearAct = new QAction(tr("Clear all widgets"), this);
    connect(clearAct, SIGNAL(triggered()), this, SLOT(clearWidgets()));
    selectAllAct = new QAction(tr("Select all widgets"), this);
    connect(selectAllAct, SIGNAL(triggered()), this, SLOT(selectAll()));

    QString iconPrefix = isLightTheme ? ":/themes/breeze/" : ":/themes/breeze-dark/";

    alignLeftAct = new QAction(QIcon(iconPrefix + "align-horizontal-left.svg"),
                               tr("Align Left"), this);
    connect(alignLeftAct, SIGNAL(triggered()), this, SLOT(alignLeft()));

    alignRightAct = new QAction(QIcon(iconPrefix + "align-horizontal-right.svg"),
                                tr("Align Right"), this);
    connect(alignRightAct, SIGNAL(triggered()), this, SLOT(alignRight()));

    alignTopAct = new QAction(QIcon(iconPrefix + "align-vertical-top.svg"),
                              tr("Align Top"), this);
    connect(alignTopAct, SIGNAL(triggered()), this, SLOT(alignTop()));

    alignBottomAct = new QAction(QIcon(iconPrefix + "align-vertical-bottom.svg"),
                                 tr("Align Bottom"), this);
    connect(alignBottomAct, SIGNAL(triggered()), this, SLOT(alignBottom()));

    sendToBackAct = new QAction(QIcon(iconPrefix + "object-order-back.svg"),
                                tr("Send to back"), this);
    connect(sendToBackAct, SIGNAL(triggered()), this, SLOT(sendToBack()));

    sendToFrontAct = new QAction(QIcon(iconPrefix + "object-order-front.svg"),
                                 tr("Send to Front"), this);
    connect(sendToFrontAct, SIGNAL(triggered()), this, SLOT(sendToFront()));

    distributeHorizontalAct = new QAction(tr("Distribute Horizontally"), this);
    connect(distributeHorizontalAct, SIGNAL(triggered()),
            this, SLOT(distributeHorizontal()));

    distributeVerticalAct = new QAction(tr("Distribute Vertically"), this);
    connect(distributeVerticalAct, SIGNAL(triggered()),
            this, SLOT(distributeVertical()));

    alignCenterHorizontalAct = new QAction(QIcon(iconPrefix + "align-horizontal-center.svg"),
                                           tr("Center Horizontally"), this);
    connect(alignCenterHorizontalAct, SIGNAL(triggered()),
            this, SLOT(alignCenterHorizontal()));

    alignCenterVerticalAct = new QAction(QIcon(iconPrefix + "align-vertical-center.svg"),
                                         tr("Center Vertically"), this);
    connect(alignCenterVerticalAct, SIGNAL(triggered()),
            this, SLOT(alignCenterVertical()));

    storePresetAct = new QAction(tr("Store Preset"), this);
    connect(storePresetAct, SIGNAL(triggered()), this, SLOT(savePreset()));
    newPresetAct = new QAction(tr("New Preset"), this);
    connect(newPresetAct, SIGNAL(triggered()), this, SLOT(newPreset()));
    recallPresetAct = new QAction(tr("Recall Preset"), this);
    connect(recallPresetAct, SIGNAL(triggered()), this, SLOT(loadPreset()));

    setFocusPolicy(Qt::StrongFocus);

    setMouseTracking(true);

    // Default values for properties
    m_objectName = "";
    setWindowTitle("Widgets");
    m_uuid = "";
    m_visible = true;

    // Set background ON by default, since any change in the app's palette (light/dark
    // mode) will change this background and make many existing scripts/examples
    // unreadable.
    setBackground(true, QColor(240, 240, 240));
    m_updating = true;
    updateData(); // Starts updateData timer

    m_widgetNameToType["BSBSpinBox"] = QuteWidgetType::SPINBOX;
    m_widgetNameToType["BSBLineEdit"] = QuteWidgetType::LINEEDIT;
    m_widgetNameToType["BSBCheckBox"] = QuteWidgetType::CHECKBOX;
    m_widgetNameToType["BSBSlider"] = QuteWidgetType::SLIDER;
    m_widgetNameToType["BSBKnob"] = QuteWidgetType::KNOB;
    m_widgetNameToType["BSBScrollNumber"] = QuteWidgetType::SCROLLNUMBER;
    m_widgetNameToType["BSBButton"] = QuteWidgetType::BUTTON;
    m_widgetNameToType["BSBDropDown"] = QuteWidgetType::DROPDOWN;
    m_widgetNameToType["BSBController"] = QuteWidgetType::CONTROLLER;
    m_widgetNameToType["BSBGraph"] = QuteWidgetType::GRAPH;
    m_widgetNameToType["BSBScope"] = QuteWidgetType::SCOPE;
    m_widgetNameToType["BSBConsole"] = QuteWidgetType::CONSOLE;
    m_widgetNameToType["BSBTableDisplay"] = QuteWidgetType::TABLEDISPLAY;
}

WidgetLayout::~WidgetLayout()
{
    disconnect(this, 0,0,0);
    layoutMutex.lock();
    closing = 1;
    layoutMutex.unlock();
    while (closing == 1) {
        qApp->processEvents();
        QThread::usleep(10000);
    }
    clearGraphs();  // To free memory from curves.
}

//unsigned int WidgetLayout::widgetCount()
//{
//  widgetsMutex.lock();
//  unsigned int number = m_widgets.size();
//  widgetsMutex.unlock();
//  return number;
//}

QuteWidgetType WidgetLayout::widgetNameToType(QString widgetName) {
    if(!m_widgetNameToType.contains(widgetName))
        return QuteWidgetType::UNKNOWN;
    return m_widgetNameToType[widgetName];
}

void WidgetLayout::loadXmlWidgets(QString xmlWidgets)
{
    m_xmlFormat = true;
    clearWidgetLayout();
    QDomDocument doc;
    if (!doc.setContent(xmlWidgets)) {
        QMessageBox::warning(this, tr("Widget Error"),
                             tr("Widgets can't be read! No widgets created."));
        qDebug() << "WidgetLayout::loadXmlWidgets Error parsing xml text! Aborting.";
        return;
    }
    QDomNodeList panel = doc.elementsByTagName("bsbPanel");
    if (panel.size() > 1) {
        QMessageBox::warning(this, tr("More than one panel"),
                             tr("The csd file contains more than one widget panel!\n"
                                "This is not supported by the current version,\n"
                                "Additional widget panels will be lost if the file is saved!"));
    }
    QDomNode p = panel.item(0);
    if (p.isNull()) {
        qDebug() << "WidgetLayout::loadXmlWidgets no bsbPanel element! Aborting.";
        return;
    }
    QDomNodeList c = p.childNodes();
    int version = 0;
    for (int i = 0; i < c.size(); i++) {
        // auto t0 = std::chrono::high_resolution_clock::now();
        auto item = c.item(i);
        int ret = parseXmlNode(item);
        // auto t1 = std::chrono::high_resolution_clock::now();
        // auto diff = std::chrono::duration<double, std::milli>(t1-t0).count();
        // QDEBUG << "parseXmlNode" << item.toElement().attribute("type") << diff << "ms";
        if (ret == -1) {
            qDebug() << "WidgetLayout::loadXmlWidgets Error in Xml node parsing";
            QMessageBox::warning(this, tr("Unrecognized wigdet format"),
                                 tr("There is unrecognized widget information in the file\n"
                                    "It may be saved with errors."));
        }
        if (ret > version) {
            version = ret;
        }
    }
    if (version > QString(CSQT_CURRENT_XML_VERSION).toInt()) {
        qDebug() << "WidgetLayout::loadXmlWidgets Newer Widget Format version";
        QMessageBox::warning(this, tr("Newer Widget Format"),
                             tr("The file was saved by a more recent version of CsoundQt.\n"
                                "Some features may not be available and will not be saved!"));
    }
    else if (version < QString(CSQT_CURRENT_XML_VERSION).toInt()) {  // Just print a silent warning
        qDebug() << "Older widget format.";
    }
    if (m_editMode) {
        setEditMode(true);
    }

    //	if (m_contained) {
    //		adjustLayoutSize();
    //	}
    //	else {
    //		this->move(m_posx, m_posy);
    //		this->resize(m_w, m_h);
    //		//    setOuterGeometry();
    //	}
    qDebug() << "Finished loading xml widgets";
}

void WidgetLayout::loadXmlPresets(QString xmlPresets)
{
    QDomDocument doc;
    if (!doc.setContent(xmlPresets)) {
        qDebug() << "WidgetLayout::loadXmlPresets Error parsing preset text.";
    }
    QDomElement pl = doc.firstChildElement("bsbPresets");
    QDomNodeList presetElements = pl.elementsByTagName("preset");
    QList<int> usedNumbers;
    for (int i = 0; i < presetElements.size(); i++) {
        QDomElement presetElement = presetElements.item(i).toElement();
        int newNumber = presetElement.attribute("number", "0").toInt();
        if (!usedNumbers.contains(newNumber)) {
            WidgetPreset newPreset;
            newPreset.setName(presetElement.attribute("name", ""));
            newPreset.setNumber(newNumber);
            QDomNodeList presetValues = presetElement.elementsByTagName("value");
            for (int i = 0; i < presetValues.size(); i++) {
                QDomElement valueElement = presetValues.item(i).toElement();
                QString id = valueElement.attribute("id", "");
                int mode = valueElement.attribute("mode", "0").toInt();
                if (mode & 1) {
                    double val = valueElement.text().toDouble();
                    newPreset.addValue(id, val);
                }
                if (mode & 2) {
                    double val = valueElement.text().toDouble();
                    newPreset.addValue2(id, val);
                }
                if (mode & 4) {
                    QString val = valueElement.text();
                    newPreset.addStringValue(id, val);
                }
            }
            presets.append(newPreset);
            usedNumbers.append(newNumber);
        }
        else {
            qDebug() << "WidgetLayout::loadXmlPresets duplicate number. Ignored. " << newNumber;
        }
    }
}

void WidgetLayout::loadMacWidgets(QString macWidgets)
{
    //  m_xmlFormat = false;
    clearWidgetLayout();
    QStringList widgetLines = macWidgets.split(QRegularExpression("[\n\r]"), SKIP_EMPTY_PARTS);
    foreach (QString line, widgetLines) {
        if (line.startsWith("i")) {
            if (newMacWidget(line) == "") {
                qDebug() << "WidgetPanel::loadMacWidgets error processing line: " << line;
            }
        }
        else {
            if (!line.contains("<MacGUI>") && !line.contains("</MacGUI>"))
                qDebug() << "WidgetPanel::loadMacWidgets error processing line: " << line;
        }
    }
    if (m_editMode) {
        setEditMode(true);
    }
    widgetChanged();
}

QString WidgetLayout::getWidgetsText()
{
    // This function must be used with care as it accesses the widgets, which
    // may cause crashing since widgets are not reentrant
    QStringList txts = {"<bsbPanel>\n"};
    layoutMutex.lock();
    txts << "<label>" << windowTitle() << "</label>\n"
         << "<objectName>" << m_objectName << "</objectName>\n"
         << "<x>" << QString::number(m_posx) << "</x>\n"
         << "<y>" << QString::number(m_posy) << "</y>\n"
         << "<width>" << QString::number(m_w) << "</width>\n"
         << "<height>" << QString::number(m_h) << "</height>\n"
         << "<visible>" << (m_visible ? QString("true"):QString("false")) << "</visible>\n"
         << "<uuid>" << m_uuid << "</uuid>\n";

    QColor bgColor = this->property("CSQT_bgcolor").value<QColor>();
    auto bg = this->property("CSQT_bg").toBool()? QString("background"):QString("nobackground");
    txts << "<bgcolor mode=\"" << bg << "\">\n";
    txts << "<r>"<<QString::number(bgColor.red())<<"</r>\n"
         << "<g>"<<QString::number(bgColor.green())<<"</g>\n"
         << "<b>"<<QString::number(bgColor.blue())<<"</b>\n";
    txts << "</bgcolor>\n";

    layoutMutex.unlock();
    widgetsMutex.lock();
    for (int i = 0; i < m_widgets.size(); i++) {
        txts << m_widgets[i]->getWidgetXmlText() << "\n";
    }
    widgetsMutex.unlock();
    txts << "</bsbPanel>";
    return txts.join("");
}

QString WidgetLayout::getPresetsText()
{
    QStringList txts = {"<bsbPresets>\n"};
    for (int i = 0; i < presets.size(); i++) {
        txts << presets[i].getXmlText();
    }
    txts << "</bsbPresets>";
    return txts.join("");
}

QString WidgetLayout::getSelectedWidgetsText()
{
    QString l;
    l += "<bsbPanel>\n";
    widgetsMutex.lock();
    for (int i = 0; i < editWidgets.size(); i++) {
        if (editWidgets[i]->isSelected()) {
            l += m_widgets[i]->getWidgetXmlText() + "\n";
        }
    }
    widgetsMutex.unlock();
    l += "</bsbPanel>";
    return l;
}

QString WidgetLayout::getMacWidgetsText()
{
    // This function must be used with care as it accesses the widgets, which
    // may cause crashing since widgets are not reentrant
    QString text = "";
    text = "<MacGUI>\n";
    QString bg, color;
    layoutMutex.lock();
    QColor bgColor = this->property("CSQT_bgcolor").value<QColor>();
    bg =  this->property("CSQT_bg").toBool()? QString("background"):QString("nobackground");
    color = QString::number((int) (bgColor.redF()*65535.)) + ", ";
    color +=  QString::number((int) (bgColor.greenF()*65535.)) + ", ";
    color +=  QString::number((int) (bgColor.blueF()*65535.));
    layoutMutex.unlock();
    text += "ioView " + bg + " {" + color +"}\n";

    widgetsMutex.lock();
    for (int i = 0; i < m_widgets.size(); i++) {
        text += m_widgets[i]->getWidgetLine() + "\n";
        //     qDebug() << m_widgets[i]->getWidgetXmlText();
    }
    widgetsMutex.unlock();
    text += "</MacGUI>";
    return text;
}

QStringList WidgetLayout::getSelectedMacWidgetsText()
{
    QStringList l;
    widgetsMutex.lock();
    for (int i = 0; i < editWidgets.size(); i++) {
        if (editWidgets[i]->isSelected()) {
            l << m_widgets[i]->getWidgetLine();
        }
    }
    widgetsMutex.unlock();
    return l;
}

QRect WidgetLayout::getOuterGeometry()
{
    return QRect(m_posx, m_posy, m_w, m_h);
}

void WidgetLayout::setKeyRepeatMode(bool repeat)
{
    m_repeatKeys = repeat;
}

void WidgetLayout::setValue(QString channelName, double value)
{
    // qDebug() << "Setting channel" << channelName << value;
    widgetsMutex.lock();
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->getUuid() == channelName) {
            m_widgets[i]->setValue(value);
            qDebug() << "Setting channel via UUID" << channelName;
            break;
        }
        else if (m_widgets[i]->getChannelName() == channelName) {
            m_widgets[i]->setValue(value);
        }
        if (m_widgets[i]->getChannel2Name() == channelName) {
            m_widgets[i]->setValue2(value);
        }

    }
    widgetsMutex.unlock();
}


void WidgetLayout::setOuterGeometry(QRect r)
{
    if (!r.isValid()) {
        r = this->geometry();
        m_posx = r.x();
        m_posy = r.y();
        m_w = r.width();
        m_h = r.height();
        return;
    }
    int newx = r.x();
    int newy = r.y();
    int neww = r.width();
    int newh = r.height();
    if (newx > 0 && newy > 0 ) {
        m_posx = newx >= 0 && newx < 4096? newx : m_posx;
        m_posy = newy >= 0 && newy < 4096? newy : m_posy;
        m_w = neww >= 0 && neww < 4096? neww : m_w;
        m_h = newh >= 0 && newh < 4096? newh : m_h;
    }
}


void WidgetLayout::setValue(QString channelName, QString value)
{
    widgetsMutex.lock();
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->getChannelName() == channelName) {
            m_widgets[i]->setValue(value);
        }
        if (m_widgets[i]->getUuid() == channelName) {
            m_widgets[i]->setValue(value);
            break;
        }
        //     if (m_widgets[i]->getChannel2Name() == channelName) {
        //       m_widgets[i]->setValue2(value);
        //     }
    }
    widgetsMutex.unlock();
}

void WidgetLayout::setValue(int index, double value)
{
    // there are two values for each widget
    widgetsMutex.lock();
    if (index < m_widgets.size() * 2) {
        m_widgets[index/2]->setValue(value);
    }
    widgetsMutex.unlock();
}

void WidgetLayout::setValue(int index, QString value)
{
    // there are two values for each widget
    widgetsMutex.lock();
    if (index < m_widgets.size() * 2) {
        m_widgets[index/2]->setValue(value);
    }
    widgetsMutex.unlock();
}

QString WidgetLayout::getStringForChannel(QString channelName, bool *modified)
{
    (void) modified;
    for (int i = 0; i < m_activeWidgets ; i++) {
        if (m_widgets[i]->getChannelName() == channelName) {
            QString value = m_widgets[i]->getStringValue();
            return value;
        } else if (m_widgets[i]->getUuid() == channelName) {
            QString value = m_widgets[i]->getStringValue();
            return value;
        }
    }
    return QString();
}

double WidgetLayout::getValueForChannel(QString channelName, bool *modified, double notfound)
{
    (void) modified;
    for (int i = 0; i < m_activeWidgets ; i++) {
        if (m_widgets[i]->getChannelName() == channelName) {
            double value = m_widgets[i]->getValue();
            return value;
        } else if (m_widgets[i]->getChannel2Name() == channelName) {
            double value = m_widgets[i]->getValue2();
            return value;
        } else if (m_widgets[i]->getUuid() == channelName) {
            double value = m_widgets[i]->getValue();
            return value;
        }
    }
    return notfound;
}

void WidgetLayout::getMouseValues(QVector<double> *values)
{
    // values must have size of 6 for _MouseX _MouseY _MouseRelX _MouseRelY MouseBut1 and MouseBut2
    if (this->isEnabled()) {
        mouseLock.lockForRead();
        (*values)[0] = getMouseX();
        (*values)[1] = getMouseY();
        (*values)[2] = getMouseRelX();
        (*values)[3] = getMouseRelY();
        (*values)[4] = getMouseBut1();
        (*values)[5] = getMouseBut2();
        mouseLock.unlock();
    }
}

int WidgetLayout::getMouseX()
{
    Q_ASSERT(mouseX >= 0 && mouseX < 4096);
    return mouseX;
}

int WidgetLayout::getMouseY()
{
    Q_ASSERT(mouseY >= 0 && mouseY < 4096);
    return mouseY;
}
int WidgetLayout::getMouseRelX()
{
    if (mouseRelX >= 0 && mouseRelX < 4096)
        return mouseRelX;
    else return 0;
}

int WidgetLayout::getMouseRelY()
{
    if (mouseRelY >= 0 && mouseRelY < 4096)
        return mouseRelY;
    else return 0;
}

int WidgetLayout::getMouseBut1()
{
    return mouseBut1;
}

int WidgetLayout::getMouseBut2()
{
    return mouseBut2;
}

void WidgetLayout::setWidgetProperty(QString widgetid, QString property, QVariant value)
{
    for (int i = 0; i < m_widgets.size(); i++) {
        if ( (m_widgets[i]->getUuid() == widgetid) || (m_widgets[i]->getChannelName() == widgetid) ) {
            m_widgets[i]->setProperty(property.toLocal8Bit(), value);
            m_widgets[i]->applyInternalProperties();
            widgetChanged();
        }
    }
}

QVariant WidgetLayout::getWidgetProperty(QString widgetid, QString property)
{
    for (int i = 0; i < m_widgets.size(); i++) {
        if ( (m_widgets[i]->getUuid() == widgetid) || (m_widgets[i]->getChannelName() == widgetid) ) {
            return m_widgets[i]->property(property.toLocal8Bit());
        }
    }
    return QVariant();
}

int WidgetLayout::newXmlWidget(QDomNode mainnode, bool offset, bool newId)
{
    if (mainnode.isNull()) {
        qDebug() << "WidgetLayout::newXmlWidget null element! Aborting.";
        return -1;
    }
    //auto t0 = std::chrono::high_resolution_clock::now();

    int ret = 0;
    auto element = mainnode.toElement();
    QuteWidget *widget = nullptr;
    QDomNodeList childNodes = mainnode.childNodes();
    QString type = element.attribute("type");
    bool forceBackground = false;
    ret = element.attribute("version").toInt();
    if (type == "BSBLabel" || type == "BSBDisplay") {

        QuteText *w = new QuteText(this);
        w->setTransparentForMouse(true);
        w->setFontOffset(m_fontOffset);
        w->setFontScaling(m_fontScaling);
        w->setType(type == "BSBLabel" ? "label" : "display");
        QDomElement ebg = element.firstChildElement("bgcolor");
        if (!ebg.isNull()) {
            if (element.attribute("mode")== "background") {
                w->setProperty("CSQT_bgcolormode", true);
            }
            w->setProperty("CSQT_bgcolor", QVariant(getColorFromElement(ebg)));
        }
        widget = static_cast<QuteWidget *>(w);
    }
    else if (type == "BSBSpinBox") {
        widget = static_cast<QuteSpinBox *>(new QuteSpinBox(this));
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
    }
    else if (type == "BSBLineEdit") {
        widget = static_cast<QuteLineEdit *>(new QuteLineEdit(this));
        forceBackground = true;
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
        connect(widget, SIGNAL(newValue(QPair<QString,QString>)),
                this, SLOT(newValue(QPair<QString,QString>)));
    }
    else if (type == "BSBCheckBox") {
        widget = static_cast<QuteWidget *>(new QuteCheckBox(this));
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
    }
    else if (type == "BSBSlider" || type == "BSBHSlider" || type == "BSBVSlider") {
        widget = static_cast<QuteWidget *>( new QuteSlider(this));
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
    }
    else if (type == "BSBKnob") {
        widget = static_cast<QuteWidget *>(new QuteKnob(this));
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
    }
    else if (type == "BSBScrollNumber") {
        QuteScrollNumber *w = new QuteScrollNumber(this);
        w->setFontOffset(m_fontOffset);
        w->setFontScaling(m_fontScaling);
        widget = static_cast<QuteWidget *>(w);
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
    }
    else if (type == "BSBButton") {
        QuteButton *w = new QuteButton(this);
        widget = static_cast<QuteWidget *>(w);
        connect(widget, SIGNAL(queueEventSignal(QString)),
                this, SLOT(queueEvent(QString)));
        connect(widget, SIGNAL(newValue(QPair<QString,QString>)),
                this, SLOT(newValue(QPair<QString,QString>)));
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
        emit registerButton(w);
    }
    else if (type == "BSBDropdown") {
        widget = static_cast<QuteWidget *>(new QuteComboBox(this));
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
    }
    else if (type == "BSBController") {
        //auto t0 = std::chrono::high_resolution_clock::now();
        auto w = new QuteMeter(this);
        widget = static_cast<QuteWidget *>(w);
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
        // QDEBUG << "... BSBController new" << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now()-t0).count() << "ms";

    }
    else if (type == "BSBGraph") {
        QuteGraph *w = new QuteGraph(this);
        widget = static_cast<QuteWidget *>(w);
        connect(widget, SIGNAL(newValue(QPair<QString,double>)),
                this, SLOT(newValue(QPair<QString,double>)));
        connect(w, SIGNAL(requestUpdateCurve(Curve*)),
                this, SLOT(processUpdateCurve(Curve*)));
        for (int i = 0; i < curves.size(); i++) {
            w->addCurve(curves[i]);
        }
        graphWidgets.append(w);
        emit requestCsoundUserData(widget);
        emit registerGraph(w);
    }
    else if (type == "BSBScope") {
        QuteScope *w = new QuteScope(this);
        widget = static_cast<QuteWidget *>(w);
        scopeWidgets.append(w);
        emit registerScope(w);
    }
    else if (type == "BSBConsole") {
        QuteConsole *w = new QuteConsole(this);
        widget = static_cast<QuteWidget *>(w);
        consoleWidgets.append(w);
        //    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this, SLOT(newValue(QPair<QString,double>)));
    }
    else if (type == "BSBTableDisplay") {
        auto w = new QuteTable(this);
        widget = static_cast<QuteWidget *>(w);
        emit requestCsoundUserData(w);
    }
    else {
        qDebug() << type << " not implemented";
        //    QuteDummy *w = new QuteDummy(this);
    }
    if (widget == nullptr) {
        qDebug() << "WidgetLayout::newXmlWidget ERROR widget has not been created!";
        return -2;
    }
    if(forceBackground) {
        widget->setProperty("CSQT_bgcolormode", true);
        widget->setProperty("CSQT_bgcolor", QColor(240, 240, 240));
    }

    for (int i = 0; i < childNodes.size() ; i++) {
        QDomElement node = childNodes.item(i).toElement();
        QString nodeName = node.nodeName();
        if (nodeName == "color" || nodeName == "bgcolor") {  // COLOR type
            if (node.attribute("mode") == "background") {
                widget->setProperty("CSQT_bgcolormode", true);
            }
            nodeName.prepend("CSQT_");
            widget->setProperty(nodeName.toLocal8Bit(), getColorFromElement(node));
        }
        // It's necessary to do a type conversion here rather than storing
        // the values as QVariant strings and then converting
        // because the property will be rightly typed, and that can be queried
        else if (nodeName == "value" || nodeName == "resolution"
                 || nodeName == "minimum" || nodeName == "maximum"
                 || nodeName == "pressedValue") {  // DOUBLE type
            nodeName.prepend("CSQT_");
            widget->setProperty(nodeName.toLocal8Bit(), node.firstChild().nodeValue().toDouble());
        }
        else if (nodeName == "x") {
            auto value = node.firstChild().nodeValue().toInt();
            if(offset)
                value += 20;
            widget->setProperty("CSQT_x", value);
        }
        else if (nodeName == "y") {  // INT type (with offset)
            auto value = node.firstChild().nodeValue().toInt();
            if(offset)
                value += 20;
            widget->setProperty("CSQT_y", value);
        }
        else if (nodeName == "width" || nodeName == "height"
                 || nodeName == "fontsize"
                 || nodeName == "precision"
                 || nodeName == "borderwidth"
                 || nodeName == "borderradius"
                 || nodeName == "selectedIndex" ) {  // INT type
            QDomNode n = node.firstChild();
            nodeName.prepend("CSQT_");
            widget->setProperty(nodeName.toLocal8Bit(), n.nodeValue().toInt());
        }
        else if (nodeName == "midichan") {
            auto value = node.firstChild().nodeValue().toInt();
            widget->setProperty("CSQT_midichan", value);
            registerWidgetChannel(widget, value);
        }
        else if (nodeName == "midicc") {
            auto value = node.firstChild().nodeValue().toInt();
            widget->setProperty("CSQT_midicc", value);
            registerWidgetController(widget, value);
        }
        else if (nodeName == "randomizable" || nodeName == "selected"
                 || nodeName == "visible" ) {  // BOOL type
            QDomNode n = node.firstChild();
            if (nodeName == "randomizable") {
                if (node.attribute("group") != "") {
                    widget->setProperty("CSQT_randomizableGroup", node.attribute("group").toInt() );
                }
            }
            nodeName.prepend("CSQT_");
            widget->setProperty(nodeName.toLocal8Bit(), n.nodeValue() == "true");

        }
        else if (nodeName == "bsbDropdownItemList") {  // MENU ITEM type
            if (node.attribute("mode") == "value") {
                qDebug() << "bsbDropdownItem modes not implemented!";
            }
            else if (node.attribute("mode") == "string") {
                qDebug() << "bsbDropdownItem modes not implemented!";
            }
            QDomElement item = node.firstChildElement("bsbDropdownItem");
            while (!item.isNull()) {
                QDomElement ename = item.firstChildElement("name");
                QDomElement evalue = item.firstChildElement("value");
                QDomElement estringvalue = item.firstChildElement("stringvalue");
                static_cast<QuteComboBox *>(widget)->addItem(ename.firstChild().nodeValue(),
                                                             evalue.firstChild().nodeValue().toDouble(),
                                                             estringvalue.firstChild().nodeValue());
                item = item.nextSiblingElement("bsbDropdownItem");
            }
        }
        else if (nodeName == "uuid")  {  // STRING type
            QDomNode n = node.firstChild();
            QString uuid = n.nodeValue();
            if (newId || uuid.isEmpty()) {
                uuid = QUuid::createUuid().toString();
            }
            while (!uuidFree(uuid)) {
                uuid = QUuid::createUuid().toString();
            }
            widget->setProperty("CSQT_uuid", uuid);
        }
        else {  // STRING type (all the rest)
            QDomNode n = node.firstChild();
            nodeName.prepend("CSQT_");
            widget->setProperty(nodeName.toLocal8Bit(), n.nodeValue());
        }
    }

    widget->applyInternalProperties();
    registerWidget(widget);

    // auto t1 = std::chrono::high_resolution_clock::now();
    // auto diff = std::chrono::duration<double, std::milli>(t1-t0).count();
    // QDEBUG << "newXmlWidget type" << type << "took " << diff << "ms";

    return ret;
}

bool WidgetLayout::uuidFree(QString uuid)
{
    bool isFree = true; // TODO need to check against uuid for widget panels and widget groups
    widgetsMutex.lock();
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->getUuid() == uuid) {
            isFree = false;
            break;
        }
    }
    widgetsMutex.unlock();
    return isFree;
}

QString WidgetLayout::newMacWidget(QString widgetLine, bool offset)
{
    // This function returns -1 on error, 0 when no widget was created and 1 if widget was created
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QStringList quoteParts = widgetLine.split('"');
    if (parts.size()<5)
        return "";
    if (parts[0]=="ioView") {
        // Colors in MacCsound have a range of 0-65535
        setBackground(parts[1]=="background",
                QColor(parts[2].toInt()/256,
                parts[3].toInt()/256,
                parts[4].toInt()/256
                )
                );
        return "";
    }
    else {
        int x,y,width,height;
        x = parts[1].toInt();
        y = parts[2].toInt();
        if (offset) {
            x += 20;
            y += 20;
        }
        width = parts[3].toInt();
        height = parts[4].toInt();
        if (parts[0]=="ioSlider") {
            return createSlider(x,y,width,height, widgetLine);
        }
        else if (parts[0]=="ioText") {
            if (parts[5]=="label" || parts[5]=="display") {
                return createText(x,y,width,height, widgetLine);
            }
            else if (parts[5]=="edit") {
                return createLineEdit(x,y,width, height, widgetLine);
            }
            else if (parts[5]=="scroll") {
                return createScrollNumber(x,y,width, height, widgetLine);
            }
            else if (parts[5]=="editnum") {
                return createSpinBox(x,y,width, height, widgetLine);
            }
        }
        else if (parts[0]=="ioButton") {
            return createButton(x,y,width,height, widgetLine);
        }
        else if (parts[0]=="ioKnob") {
            return createKnob(x,y,width, height, widgetLine);
        }
        else if (parts[0]=="ioCheckbox") {
            return createCheckBox(x,y,width, height, widgetLine);
        }
        else if (parts[0]=="ioMenu") {
            return createMenu(x,y,width, height, widgetLine);
        }
        else if (parts[0]=="ioListing") {
            return createConsole(x,y,width, height, widgetLine);
        }
        else if (parts[0]=="ioMeter") {
            return createMeter(x,y,width, height, widgetLine);
        }
        else if (parts[0]=="ioGraph") {
            if (parts.size() < 6 || parts[5]=="table")
                return createGraph(x,y,width, height, widgetLine);
            else if (parts[5]=="fft" || parts[5]=="scope" || parts[5]=="lissajou" || parts[5]=="poincare")
                return createScope(x,y,width, height, widgetLine);
        }
        else {
            // Unknown widget...
            qDebug("WidgetPanel::newMacWidget Warning: unknown widget!");
            return createDummy(x,y,width, height, widgetLine);
        }
    }
    return "";
}

void WidgetLayout::registerWidget(QuteWidget * widget)
{
    widgetsMutex.lock();
    connect(widget, SIGNAL(widgetChanged(QuteWidget *)),
            this, SLOT(widgetChanged(QuteWidget *)));
    connect(widget, SIGNAL(deleteThisWidget(QuteWidget *)),
            this, SLOT(deleteWidget(QuteWidget *)));
    connect(widget, SIGNAL(propertiesAccepted()),
            this, SLOT(markHistory()));
    connect(widget, SIGNAL(showMidiLearn(QuteWidget *)),
            this, SIGNAL(showMidiLearn(QuteWidget *)));
    connect(widget, SIGNAL(addChn_kSignal(QString)),
            this, SIGNAL(addChn_kSignal(QString)) );
    m_widgets.append(widget);
    //  qDebug() << "WidgetLayout::registerWidget " << m_widgets.size() << widget;
    if (m_editMode) {
        createEditFrame(widget);
        editWidgets.last()->select();
    }
    setWidgetToolTip(widget, m_tooltips);
    m_activeWidgets++;
    widgetsMutex.unlock();
    adjustLayoutSize();
	widget->show();
}

QString WidgetLayout::getMidiControllerInstrument()
{
	// returns full Csound instrument to pass CC values to channels
	// and/or start events (if eventButton)
	if (registeredControllers.isEmpty()) {
		qDebug() << "There are no registered controllers!";
	}

	QString instrLines ;
	for (int i = 0; i < registeredControllers.size(); i++) {
		if (registeredControllers[i].cc >= 0 && registeredControllers[i].chan>0) {
			QuteWidget * widget = registeredControllers[i].widget;

			bool isEventButton = (widget->getWidgetType()=="BSBButton"  && widget->property("CSQT_type").toString().contains("event") ); // type "event" or "pictevent"
			if ( isEventButton) {
				QString eventLine = widget->property("CSQT_eventLine").toString();

				// find out if event has negative p3
                QStringList lineElements = eventLine.split(QRegularExpression("\\s"),SKIP_EMPTY_PARTS);
				if (lineElements.size() > 0 && lineElements[0] == "i") {
					// Remove first element if it is "i"
					lineElements.removeAt(0);
				}
				else if (lineElements.size() > 0 && lineElements[0][0] == 'i') {
					lineElements[0] = lineElements[0].mid(1); // Remove "i" character
				}

				QString instrNo = lineElements[0].startsWith("\"") ?
							QString("nstrnum(%1)").arg(lineElements[0]) : lineElements[0];

				bool negative_p3;
				if (lineElements.size() > 2 && lineElements[2].toDouble() < 0 ) {
					negative_p3 = true;
				} else {
					negative_p3 = false;
				}

				if (!negative_p3) {
					instrLines += QString(R"(
	if (trigger:k(ctrl7:k(%1,%2,0,1),0.9, 0) == 1) then
		scoreline {{ %3 }}, 1 ; start
	endif)")
							.arg(registeredControllers[i].chan)
							.arg(registeredControllers[i].cc)
							.arg(eventLine) + "\n";
				} else {
					if (widget->property("CSQT_latch").toBool()) {
						// toggle button -  turn off on second press
						instrLines += QString(R"(
	if (trigger:k(ctrl7:k(%1,%2,0,1),0.9, 0)==1) then
		if (active:k(%3)>0) then
			turnoff2 %3, 4, 1 ; stop
		else
			scoreline {{ %4 }}, 1 ; start
		endif
	endif)")
							.arg(registeredControllers[i].chan)
							.arg(registeredControllers[i].cc)
							.arg(lineElements[0]) // lineElements[0] holds p1 (instrument name/number)
							.arg( eventLine )	+ "\n";
					} else { // pushbutton - turn off when released
						instrLines +=
								QString(R"(
	if (trigger:k(ctrl7:k(%1,%2,0,1),0.9, 0) == 1) then
		scoreline {{ %4 }}, 1 ; start
	endif

	if (trigger:k(ctrl7:k(%1,%2,0,1),0.1, 1) == 1) then
		turnoff2 %3, 4, 1 ; stop
	endif)")
								.arg(registeredControllers[i].chan)
								.arg(registeredControllers[i].cc)
								.arg( instrNo )
								.arg( eventLine ) + "\n";
					}
				}
			} else {
				if (widget->getWidgetType()=="BSBButton" &&
						widget->property("CSQT_type").toString().contains("value")) {
					instrLines += QString ("\tchnset ctrl7:k(%1, %2, 0, %3), \"%4\"\n")
							.arg(registeredControllers[i].chan)
							.arg(registeredControllers[i].cc)
							.arg( widget->property("CSQT_pressedValue").toDouble() )
							.arg(widget->getChannelName() );
				} else {
				instrLines += QString ("\tchnset ctrl7:k(%1, %2, %3, %4), \"%5\"\n")
				        .arg(registeredControllers[i].chan)
				        .arg(registeredControllers[i].cc)
				        .arg( widget->property("CSQT_minimum").toDouble() )
				        .arg( widget->property("CSQT_maximum").toDouble() )
				        .arg(widget->getChannelName() );
				}
			}
		}
	}

	QString instrString = instrLines.isEmpty() ? "" :
			"alwayson \"MidiControl\"\ninstr MidiControl\n" + instrLines + "endin\n";
	return instrString;
}

void WidgetLayout::appendMessage(QString message)
{

    for (int i=0; i < consoleWidgets.size(); i++) {
        consoleWidgets[i]->appendMessage(message);
        consoleWidgets[i]->scrollToEnd();
    }
}


void WidgetLayout::flush()
{
    // Called when running Csound to flush queues
    newValues.clear();
}

void WidgetLayout::engineStopped()
{
    flushGraphBuffer();
}

void WidgetLayout::showWidgetTooltips(bool show)
{
    m_tooltips = show;
    widgetsMutex.lock();
    for (int i=0; i < m_widgets.size(); i++) {
        setWidgetToolTip(m_widgets[i], show);
    }
    widgetsMutex.unlock();
}

void WidgetLayout::setWidgetToolTip(QuteWidget *widget, bool show)
{
    // This function must be protected/locked by the caller
    if(!show) {
        widget->setToolTip("");
        if (m_editMode && getEditWidget(widget) != nullptr) {
            getEditWidget(widget)->setToolTip("");
        }
        return;
    }

    QStringList lines;

    auto description = widget->getDescription();
    if(!description.isEmpty()) {
        lines << description;
        lines << " ";
    }

    auto channelName = widget->getChannelName();
    auto channel2Name = widget->getChannel2Name();
    if(!channelName.isEmpty() && channel2Name.isEmpty()) {
        lines << tr("Channel: ") + channelName;
    }
    else if(!channel2Name.isEmpty()){
        lines << tr("ChannelH: ") + channelName + ", "
                 + tr("ChannelV: ")+ channel2Name;
    }

    int midichan = widget->property("CSQT_midichan").toInt();
    if(midichan > 0) {
        int midicc = widget->property("CSQT_midicc").toInt();
        lines << QString(tr("MIDI chan: %1 CC: %2")).arg(midichan).arg(midicc);
    }

    auto tooltip = lines.join("\n");
    widget->setToolTip(tooltip);

    if (getEditWidget(widget) != nullptr) {
        getEditWidget(widget)->setToolTip(tooltip);
    }

}

void WidgetLayout::setContained(bool contained)
{
    if (m_contained == contained) {
        return;
    }
//    qDebug() << "WidgetLayout::setContained " << contained;
    m_contained = contained;
    bool bg = this->property("CSQT_bg").toBool();
    QColor bgColor = this->property("CSQT_bgcolor").value<QColor>();
    setBackground(bg, bgColor);
    if (m_contained) {
        this->setAutoFillBackground(false);
        //    qDebug() << "WidgetLayout::setContained" << this->selectionFrame;
        this->selectionFrame->setParent(this->parentWidget());
    }
    else {
        this->selectionFrame->setParent(this); // To Avoid having it destroyed by parent
    }
}

void WidgetLayout::setCurrentPosition(QPoint pos)
{
    currentPosition = pos;
}

void WidgetLayout::setFontOffset(double offset)
{
    m_fontOffset = offset;
    widgetsMutex.lock();
    for (int i=0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->getWidgetType() == "BSBLabel" ||
                m_widgets[i]->getWidgetType() == "BSBScrollNumber") {
            static_cast<QuteText *>(m_widgets[i])->setFontOffset(offset);
            m_widgets[i]->applyInternalProperties();
        }
    }
    widgetsMutex.unlock();
}

void WidgetLayout::setFontScaling(double scaling)
{
    m_fontScaling = scaling;
    widgetsMutex.lock();
    for (int i=0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->getWidgetType() == "BSBLabel" ||
                m_widgets[i]->getWidgetType() == "BSBScrollNumber") {
            static_cast<QuteText *>(m_widgets[i])->setFontScaling(scaling);
            m_widgets[i]->applyInternalProperties();
        }
    }
    widgetsMutex.unlock();
}

void WidgetLayout::setWidgetsLocked(bool lock)
{
    widgetsMutex.lock();
    for (int i=0; i < m_widgets.size(); i++) {
        m_widgets[i]->setLocked(lock);
    }
    widgetsMutex.unlock();
}

void WidgetLayout::appendCurve(WINDAT *windat)
{
    // Called from the Csound callback, creates a curve and queues it for processing
    // Csound itself deletes the WINDAT structures, that's why we retain a copy of
    // the data for when Csound stops
    // It would be nice if Csound used a single windat for every f-table, but it reuses them...
    //  for (int i = 0; i < curves.size(); i++) {
    //    // Check if windat is already handled by one of the existing curves
    //    if (windat == curves[i]->getOriginal()) {
    //      return;
    //    }
    //  }
    for (int i = 0; i < curves.size(); i++) {
        // Check if caption is already present to replace curve rather than create a new one.
        if (curves[i]->get_caption() == windat->caption) {
            windat->windid = (uintptr_t) curves[i];
            return;
        }
    }
    if (curves.size() > CSQT_CURVE_BUFFER_MAX) {
        qDebug() << "WidgetLayout::appendCurve curve size exceeded. Curve discarded!";
        return;
    }
    windat->caption[CAPSIZE - 1] = 0; // Just in case...
    Polarity polarity;
    switch (windat->polarity) {
    case NEGPOL:
        polarity = POLARITY_NEGPOL;
        break;
    case POSPOL:
        polarity = POLARITY_POSPOL;
        break;
    case BIPOL:
        polarity = POLARITY_BIPOL;
        break;
    default:
        polarity = POLARITY_NOPOL;
    }
    int indexInBuffer = -1;
    //This is not working... makes fft graphs ge ignored...
    //  for (int i = 0; i < newCurveBuffer.size(); i++) {
    //    if (strcmp(newCurveBuffer[i]->getOriginal()->caption,windat->caption) && strlen(newCurveBuffer[i]->getOriginal()->caption) > 0) {
    //      indexInBuffer = i;
    //      break;
    //    }
    //  }
    if (indexInBuffer < 0) {
        auto curve = new Curve(windat->fdata,
                               (size_t)windat->npts,
                               windat->caption,
                               polarity,
                               windat->max,
                               windat->min,
                               windat->absmax,
                               windat->oabsmax,
                               windat->danflag,
                               windat);  //FIXME delete these when starting a new run
        windat->windid = (uintptr_t) curve;
        newCurveBuffer.append(curve);
        // qDebug() << "WidgetLayout::appendCurve " << curve << "__--__" << windat;
    }
    else {
        qDebug() << "WidgetLayout::appendCurve reusing curve buffer " << indexInBuffer;
        newCurveBuffer[indexInBuffer]->set_data(windat->fdata);
        newCurveBuffer[indexInBuffer]->setOriginal(windat);
    }
}

void WidgetLayout::killCurve(WINDAT *windat)
{
    qDebug() << "WidgetLayout::killCurve()";
    Curve *curve = (Curve *) getCurveById(windat->windid);
    curve->setOriginal(nullptr);
}

void WidgetLayout::newCurve(Curve* curve)
{
    // qDebug() << "WidgetPanel::newCurve" << curve << curve->get_caption();
    //  FIXME check if curve is a new curve for a old ftable
    //  for (int i = 0; i < curves.size(); i++) {
    //    if (curves[i]->get_caption() == curve->get_caption()) {
    //      return;
    //    }
    //  }

    curves.append(curve);
    for (int i = 0; i < graphWidgets.size(); i++) {
        graphWidgets[i]->addCurve(curve);
    }
}

void WidgetLayout::setCurveData(Curve *curve)
{
    //  qDebug() << "WidgetPanel::setCurveData" <<curve;
    for (int i = 0; i < graphWidgets.size(); i++) {
        graphWidgets[i]->setCurveData(curve);
    }
}

//void WidgetLayout::passWidgetClipboard(QString text)
//{
//  m_clipboard = text;
//}

uintptr_t WidgetLayout::getCurveById(uintptr_t id)
{
    //  qDebug() << "WidgetLayout::getCurveById ";
    foreach (Curve *thisCurve, curves) {
        // qDebug() << "WidgetLayout::getCurveById " << (uintptr_t)thisCurve << "id" << id;
        if ((uintptr_t) thisCurve == id)
            return (uintptr_t) thisCurve;
    }
    return 0;
}

void WidgetLayout::updateCurve(WINDAT *windat)
{
    //  qDebug() << "WidgetLayout::updateCurve(WINDAT *windat) " << windat->windid;
    if (curveUpdateBufferCount+1 < CSQT_CURVE_BUFFER_SIZE) {
        curveUpdateBufferCount++;
        curveUpdateBuffer[curveUpdateBufferCount] = *windat;
    }
}

void WidgetLayout::processUpdateCurve(Curve *curve) {
    WINDAT *orig = curve->getOriginal();
    qDebug() << "processUpdateCurve" << orig->npts << orig->caption;
    this->updateCurve(orig);
}


int WidgetLayout::killCurves(CSOUND * /*csound*/)
{
    //  qDebug() << "qutecsound::killCurves";
    // TODO this is a great idea, to copy data from the tables at the end of run,
    // but the API is not working as expected
    //  for (int i = 0; i < curves.size(); i++) {
    //    WINDAT * windat = curves[i]->getOriginal();
    //    if (windat->npts > 0 && windat->windid == (uintptr_t)curves[i]) { // Check for sanity of pointer
    //      curves[i]->set_size(windat->npts);      // number of points
    //      curves[i]->set_data(windat->fdata);
    //      curves[i]->set_caption(QString(windat->caption)); // title of curve
    //      //    curves[i]->set_polarity(windat->polarity); // polarity
    //      curves[i]->set_max(windat->max);        // curve max
    //      curves[i]->set_min(windat->min);        // curve min
    //      curves[i]->set_absmax(windat->absmax);     // abs max of above
    //      //    curves[i]->set_y_scale(windat->y_scale);    // Y axis scaling factor
    //      curves[i]->setOriginal(0);
    //    }
    //  }

    // Curves are freed not when Csound finishes, but when Csound is run again,
    // to have them available even if Csound is stopped.
    return 0;
}

void WidgetLayout::clearGraphs()
{
    flushGraphBuffer();
    bool updating = m_updating;
    m_updating = false;
    QMutexLocker locker(&layoutMutex);
    for (int i = 0; i < graphWidgets.size(); i++) {
        graphWidgets[i]->clearCurves();
    }
    while (curves.size() > 0) {
        Curve * c = curves.takeFirst();
        delete c;
    }
    m_updating = updating;
}

void WidgetLayout::flushGraphBuffer()
{
    bool updating = m_updating;
    m_updating = false;
    if(!layoutMutex.tryLock(200)) {
        QDEBUG << "Could not acquire lock, not flushing...";
        m_updating = updating;
        return;
    }
    while (newCurveBuffer.size() > 0) {
        Curve * c = newCurveBuffer.takeFirst();
        delete c;
    }
    curveUpdateBufferCount = 0;
    layoutMutex.unlock();
    m_updating = updating;
}

void WidgetLayout::refreshWidgets()
{
    while (midiReadCounter != midiWriteCounter) {
        // TODO it is inefficient to have this per layout (when more than one
        // layout is available)
        int index =midiReadCounter;
        int status = midiQueue[index][0];
        if (status & 176) { // MIDI control change
            int channel = (status ^ 176) + 1;
            for (int i = 0; i < registeredControllers.size(); i++) {
                if (registeredControllers[i].cc == midiQueue[index][1]) {
					if ( channel == registeredControllers[i].chan) {
                        registeredControllers[i].widget->setMidiValue(midiQueue[index][2]);
                    }
                }
            }
        }
        midiReadCounter++;
        midiReadCounter = midiReadCounter%CSQT_MAX_MIDI_QUEUE;
    }
    QMutexLocker locker(&widgetsMutex);
    for (int i=0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->m_valueChanged || m_widgets[i]->m_value2Changed) {
            m_widgets[i]->refreshWidget();
        }
        if (m_trackMouse) {
            QString ch1name = m_widgets[i]->getChannelName();
            if (ch1name.startsWith("_Mouse")) {
                if (ch1name == "_MouseX") {
                    m_widgets[i]->setValue(getMouseX());
                }
                else if (ch1name == "_MouseY") {
                    m_widgets[i]->setValue(getMouseY());
                }
                else if (ch1name == "_MouseRelX") {
                    m_widgets[i]->setValue(getMouseRelX());
                }
                else if (ch1name == "_MouseRelY") {
                    m_widgets[i]->setValue(getMouseRelY());
                }
                else if (ch1name == "_MouseBut1") {
                    m_widgets[i]->setValue(getMouseBut1());
                }
                else if (ch1name == "_MouseBut2") {
                    m_widgets[i]->setValue(getMouseBut2());
                }
            }
            QString ch2name = m_widgets[i]->getChannel2Name();
            if (ch2name.startsWith("_Mouse")) {
                if (ch2name == "_MouseX") {
                    m_widgets[i]->setValue2(getMouseX());
                }
                else if (ch2name == "_MouseY") {
                    m_widgets[i]->setValue2(getMouseY());
                }
                else if (ch2name == "_MouseRelX") {
                    m_widgets[i]->setValue2(getMouseRelX());
                }
                else if (ch2name == "_MouseRelY") {
                    m_widgets[i]->setValue2(getMouseRelY());
                }
                else if (ch2name == "_MouseBut1") {
                    m_widgets[i]->setValue2(getMouseBut1());
                }
                else if (ch2name == "_MouseBut2") {
                    m_widgets[i]->setValue2(getMouseBut2());
                }
            }
        }
    }
}

QString WidgetLayout::getCsladspaLines()
{
    QString text = "";
    int unsupported = 0;
    widgetsMutex.lock();
    foreach(QuteWidget *widget, m_widgets) {
        QString line = widget->getCsladspaLine();
        if (line != "") {
            text += line + "\n";
        }
        else {
            unsupported++;
        }
    }
    widgetsMutex.unlock();
    qDebug() << "WidgetPanel:getCsladspaLines() " << unsupported << " Unsupported widgets";
    return text;
}

QString WidgetLayout::getQml()
{
    QString qml = "import QtQuick 2.0\nimport QtQuick.Controls 2.0 // NB! Requires Qt 5.7 or later. Rewrite with QtQuick.Controls 1.X if using older Qt versions \n\n";
    //QString s2 = QString(R"()");
    qml += "Rectangle {\n";
    QColor bgcolor = this->palette().color(QWidget::backgroundRole());
    qml+=QString("\tcolor: \"%1\"\n").arg(bgcolor.name());

    // get width and hight currently from panel size, maybe not best method -
    // test how if works if some of the window is hidden
    int w = this->geometry().width() + 20;
    int h = this->geometry().height() +20;

    qml += QString("\twidth: %1\n").arg(w);
    qml += QString("\theight: %1\n").arg(h);

    qml += "\tanchors.fill: parent\n";
    qml += "\n\n\tFlickable {\n\tanchors.fill: parent\n";
    qml += QString("\tcontentWidth: %1\n").arg(w);
    qml += QString("\tcontentHeight: %1\n").arg(h);

    qml += "\n";

    qml += QString(
        R"(
            Item {
                id: scaleItem
                anchors.fill: parent
            }

            PinchArea {
                anchors.fill: parent
                pinch.maximumScale: 2.5
                pinch.minimumScale: 0.6
                pinch.target: scaleItem

               MouseArea {  // necessary for co-existense of flickable an pincharea
                   anchors.fill: parent
                   onClicked: console.log("Clicked tiny")
                   propagateComposedEvents: true
               }
           }
        )");


    int unsupported = 0;
    widgetsMutex.lock();
    foreach(QuteWidget *widget, m_widgets) {
        QString line = widget->getQml();
        if (line != "") {
            qml += line + "\n";
        }
        else {
            unsupported++;
        }
    }
    widgetsMutex.unlock();
    qDebug() << "WidgetPanel:getQml() " << unsupported << " Unsupported widgets";
    qml += "\t} // end Flickable\n";
    qml += "} // end Rectangle\n"; // end Rectangle
    //qDebug() << qml;
    return qml;
}

QString WidgetLayout::getCabbageWidgets()
{
    QString title = windowTitle();
    QString text = "form caption(\"" + title  + "\"),";
    //text += "size(" + QString::number(m_w+20) + "," + QString::number(m_h+20) +")\n"; // m_w and m_h not returning correct results always
    int w = this->geometry().width();
    int h = this->geometry().height();
    text += QString(" size(%1,%2)\n").arg(w+20).arg(h+20);

    int unsupported = 0;
    widgetsMutex.lock();
    QString unsupportedWidgets;
    foreach(QuteWidget *widget, m_widgets) {
        QString line = widget->getCabbageLine();
        if (line != "") {
            text += line + "\n";
        }
        else {
            unsupported++;
            unsupportedWidgets += widget->getWidgetType() + " ";
        }
    }
    widgetsMutex.unlock();
    qDebug() << "WidgetPanel:getCabbageWidgets()"<<unsupported<<"Unsupported widgets";
    if (unsupported) {
        QMessageBox::warning(this, tr("CsoundQt"), tr("Could not convert %1 widget(s):\n%2")
                             .arg(unsupported).arg(unsupportedWidgets));
    }
    return text;
}

bool WidgetLayout::isModified()
{
    return m_modified;
}

void WidgetLayout::addCreateWidgetActionsToMenu(QMenu &menu) {
    menu.addAction(createSliderAct);
    menu.addAction(createLabelAct);
    menu.addAction(createDisplayAct);
    menu.addAction(createScrollNumberAct);
    menu.addAction(createLineEditAct);
    menu.addAction(createSpinBoxAct);
    menu.addAction(createButtonAct);
    menu.addAction(createKnobAct);
    menu.addAction(createCheckBoxAct);
    menu.addAction(createMenuAct);
    menu.addAction(createMeterAct);
    menu.addAction(createConsoleAct);
    menu.addAction(createGraphAct);
    menu.addAction(createScopeAct);
    menu.addAction(createTableDisplayAct);
}

void WidgetLayout::createContextMenu(QContextMenuEvent *event)
{
    QMenu menu;
    QMenu newMenu(tr("New Widget", "New Widget submenu in widget layout popup"), &menu);
    this->addCreateWidgetActionsToMenu(newMenu);
    menu.addMenu(&newMenu);
    menu.addSeparator(); // ------------------------------------------------------------

    QMenu editMenu(tr("Edit", "Edit menu in widget layout right-click menu"), &menu);
    editMenu.addAction(cutAct);
    editMenu.addAction(copyAct);
    editMenu.addAction(pasteAct);
    editMenu.addAction(selectAllAct);
    // Shortcut from the duplicate action is not working from some reason...
    //  menu.addAction(duplicateAct);
    editMenu.addAction(deleteAct);
    editMenu.addAction(clearAct);
    menu.addMenu(&editMenu);

    menu.addSeparator(); // ------------------------------------------------------------

    QMenu presetMenu(tr("Presets", "Preset submenu in widget layout popup"), &menu);
    presetMenu.addAction(storePresetAct);
    presetMenu.addAction(recallPresetAct);
    presetMenu.addAction(newPresetAct);
    menu.addMenu(&presetMenu);

    menu.addSeparator(); // ------------------------------------------------------------

    menu.addAction(propertiesAct);
    menu.addAction(reloadWidgetsAct);

    currentPosition = event->pos();
    //  if (m_sbActive) {
    //    currentPosition.setX(currentPosition.x() + scrollArea->horizontalScrollBar()->value());
    //    currentPosition.setY(currentPosition.y() + scrollArea->verticalScrollBar()->value() - 20);
    //  }
    menu.exec(event->globalPos());
    //  menu.exec(event->pos());
}

void WidgetLayout::deselectAll()
{
    for (int i = 0; i< editWidgets.size(); i++) {
        editWidgets[i]->deselect();
    }
}

void WidgetLayout::selectAll()
{
    for (int i = 0; i< editWidgets.size(); i++) {
        editWidgets[i]->select();
    }
}

void WidgetLayout::widgetMoved(QPair<int, int> delta)
{
    widgetsMutex.lock();
    for (int i = 0; i < m_widgets.size(); i++) {
        if (editWidgets[i]->isSelected()) {
            int newx = m_widgets[i]->x() + delta.first;
            int newy = m_widgets[i]->y() + delta.second;
            m_widgets[i]->move(newx, newy);
            editWidgets[i]->move(newx, newy);
			// set the position to properties otherwis MidiLearn -> applyInetrnalProperties sets it to wrong (old) position
			m_widgets[i]->setProperty("CSQT_x", newx);
			m_widgets[i]->setProperty("CSQT_y", newy);

        }
    }
    widgetsMutex.unlock();
    adjustLayoutSize();
}

void WidgetLayout::widgetResized(QPair<int, int> delta)
{
    //   qDebug("WidgetPanel::widgetResized %i  %i", delta.first, delta.second);
    widgetsMutex.lock();
    for (int i = 0; i< editWidgets.size(); i++) {
        if (editWidgets[i]->isSelected()) {
            int neww = m_widgets[i]->width() + delta.first;
            int newh = m_widgets[i]->height() + delta.second;
            neww = neww > 5 ? neww : 5;
            newh = newh > 5 ? newh : 5;
            m_widgets[i]->setWidgetGeometry(m_widgets[i]->x(), m_widgets[i]->y(), neww, newh);
            editWidgets[i]->resize(neww, newh);
			// set the position to properties otherwis MidiLearn -> applyInetrnalProperties sets it to wrong (old) position
			m_widgets[i]->setProperty("CSQT_width", neww);
			m_widgets[i]->setProperty("CSQT_height", newh);
        }
    }
    widgetsMutex.unlock();
    adjustLayoutSize();
}

void WidgetLayout::mousePressEventParent(QMouseEvent *event)
{
    WidgetLayout::mousePressEvent(event);
}

void WidgetLayout::mouseReleaseEventParent(QMouseEvent *event)
{
    WidgetLayout::mouseReleaseEvent(event);
}

void WidgetLayout::mouseMoveEventParent(QMouseEvent *event)
{
    WidgetLayout::mouseMoveEvent(event);
}

QSize WidgetLayout::getUsedSize()
{
    int width = 30, height = 30;
    for (int i = 0; i< m_widgets.size(); i++) {
        if (m_widgets[i]->x() + m_widgets[i]->width() > width) {
            width = m_widgets[i]->x() + m_widgets[i]->width();
        }
        if (m_widgets[i]->y() + m_widgets[i]->height() > height) {
            height = m_widgets[i]->y() + m_widgets[i]->height();
        }
    }
    return QSize(width, height);
}

void WidgetLayout::adjustLayoutSize()
{
    if (m_contained) {
        QSize s = this->childrenRect().size();
        QPoint pos = this->childrenRect().topLeft();
        s.rwidth() += pos.x();
        s.rheight() += pos.y();
        if (this->size() != s) {
            this->resize(s);
        }
    }
}

void WidgetLayout::adjustWidgetSize()
{
    if (!m_contained) {
        this->resize(m_w, m_h);
        this->move(m_posx, m_posy);
    }
}

void WidgetLayout::selectionChanged(QRect selection)
{
    //   qDebug("WidgetLayout::selectionChanged %i %i %i %i", selection.x(), selection.y(), selection.width(), selection.height());
    if (editWidgets.isEmpty())
        return; //not in edit mode
    deselectAll();
    widgetsMutex.lock();
    for (int i = 0; i< m_widgets.size(); i++) {
        int x = m_widgets[i]->x();
        int y = m_widgets[i]->y();
        int w = m_widgets[i]->width();
        int h = m_widgets[i]->height();
        if (x > selection.x() - w && x < selection.x() + selection.width() &&
                y > selection.y() - h && y < selection.y() + selection.height() ) {
            editWidgets[i]->select();
        }
    }
    widgetsMutex.unlock();
}

QString WidgetLayout::createNewSlider(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "slider" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createSlider(posx, posy, 20, 100,
                        QString("ioSlider {"+ QString::number(posx) +", "+
                                QString::number(posy) +
                                "} {20, 100} 0.000000 1.000000 0.000000 " + channel));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}


QString WidgetLayout::createNewLabel(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "label" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    QString line = "ioText {"+ QString::number(posx) +", "+ QString::number(posy) +"} {80, 25} label 0.000000 0.001000 \"\" left \"Arial\" 8 {0, 0, 0} {65535, 65535, 65535} nobackground noborder "+ channel;
    uuid = createText(posx, posy, 80, 25, line);
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewDisplay(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "display" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    QString line = "ioText {"+ QString::number(posx) +", "+ QString::number(posy) +"} {80, 25} display 0.000000 0.001000 \"" + channel + "\" left \"Arial\" 8 {0, 0, 0} {65535, 65535, 65535} nobackground border Display";
    uuid = createText(posx, posy, 80, 25, line);
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewScrollNumber(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "scroll" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    QString line = "ioText {"+ QString::number(posx) +", "+ QString::number(posy) +"} {80, 25} scroll 0.000000 0.001000 \"" + channel + "\" left \"Arial\" 8 {0, 0, 0} {65535, 65535, 65535} background border 0.000000";
    uuid = createScrollNumber(posx, posy, 80, 25, line);
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewLineEdit(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "line" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    auto line = QString("ioText {%1, %2} {100, 25} edit 0.000000 0.001000 "
                        "\"%3\" left \"Arial\" 8 {0, 0, 0} {65535, 65535, 65535} "
                        "background border Type here")
            .arg(QString::number(posx), QString::number(posy), channel);

    // QString line = "ioText {" + QString::number(posx) +", "+ QString::number(posy) +"} {100, 25} edit 0.000000 0.001000 \"" + channel + "\" left \"Arial\" 8 {0, 0, 0} {65535, 65535, 65535} nobackground noborder Type here";
    qDebug() << "createNewLineEdit";
    uuid = createLineEdit(posx, posy, 100, 25, line);
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewSpinBox(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "spinbox" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    QString line = "ioText {"+ QString::number(posx) +", "+ QString::number(posy) +"} {80, 25} editnum 0.000000 0.001000 \"" + channel + "\" left \"Arial\" 8 {0, 0, 0} {65535, 65535, 65535} nobackground noborder Type here";
    uuid = createSpinBox(posx, posy, 80, 25, line);
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewButton(int x, int y, QString channel)
{
    qDebug() << "WidgetLayout::createNewButton";
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "button" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    //IS IT OK if the channel name is alsoset as the text of the button?
    QString line = "ioButton {"+ QString::number(posx) +", "+ QString::number(posy) +"} {100, 30} event 1.000000 \"" + channel + "\" \"" + channel + "\" \"/\" i1 0 10";
    uuid = createButton(posx, posy, 100, 30, line);
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewKnob(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "knob" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createKnob(posx, posy, 80, 80, QString("ioKnob {"+ QString::number(posx) +", "+ QString::number(posy) + "} {80, 80} 0.000000 1.000000 0.010000 0.000000 " + channel));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewCheckBox(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "checkbox" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createCheckBox(posx, posy, 20, 20, QString("ioCheckbox {"+ QString::number(posx) +", "+ QString::number(posy) + "} {20, 20} off " + channel));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewMenu(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "menu" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createMenu(posx, posy, 80, 30, QString("ioMenu {"+ QString::number(posx) +", "+ QString::number(posy) + "} {80, 25} 1 303 \"item1,item2,item3\" " + channel));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewMeter(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "meter" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createMeter(posx, posy, 30, 80, QString("ioMeter {"+ QString::number(posx) +", "+ QString::number(posy) + "} {30, 80} {0, 60000, 0} \"" + channel + "\" 0.000000 \"hor" + QString::number(m_widgets.size()) + "\" 0.000000 fill 1 0 mouse"));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}


QString WidgetLayout::createNewConsole(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "console" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createConsole(posx, posy, 320, 400, QString("ioListing {"+ QString::number(posx) +", "+ QString::number(posy) + "} {320, 400}"));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewGraph(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "graph" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createGraph(posx, posy, 350, 150, QString("ioGraph {"+ QString::number(posx) +", "+ QString::number(posy) + "} {350, 150}"));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewTableDisplay(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "graph" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createTableDisplay(posx, posy, 350, 150,
                              QString("ioGraph {%1, %2} {%1, %2}").arg(posx, posy));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

QString WidgetLayout::createNewScope(int x, int y, QString channel)
{
    QString uuid;
    bool dialog;
    int posx = x >= 0 ? x : currentPosition.x();
    int posy = y >= 0 ? y : currentPosition.y();
    deselectAll();
    if (channel.isEmpty()) {
        channel = "scope" + QString::number(m_widgets.size());
        dialog = true;
    } else {
        dialog = false;
    }
    uuid = createScope(posx, posy, 350, 150,
                       QString("ioGraph {"+ QString::number(posx) +", "+
                               QString::number(posy) +
                               "} {350, 150} scope 2.000000 -1.000000"));
    widgetChanged();
    if (dialog && getOpenProperties()) {
        m_widgets.last()->openProperties();
    }
    markHistory();
    return uuid;
}

void WidgetLayout::clearWidgets()
{
    clearWidgetLayout();
    widgetChanged();
}

void WidgetLayout::clearWidgetLayout()
{
    //   qDebug("WidgetLayout::clearWidgetLayout()");
    widgetsMutex.lock();
    m_activeWidgets = 0;
    foreach (QuteWidget *widget, m_widgets) {
        delete widget;
    }
    m_widgets.clear();
    foreach (FrameWidget *widget, editWidgets) {
        //     qDebug("WidgetLayout::clearWidgetLayout() removed editWidget");
        delete widget;
    }
    editWidgets.clear();
    consoleWidgets.clear();
    graphWidgets.clear();
    scopeWidgets.clear();
    widgetsMutex.unlock();
}


QStringList WidgetLayout::getUuids()
{   QStringList uuids = QStringList();
    for (int i=0; i<m_widgets.size(); i++) {
        uuids.append( m_widgets[i]->getUuid() );
    }
    return uuids;
}

QStringList WidgetLayout::listProperties(QString widgetid)
{

    QStringList prop_names = QStringList();
    for (int i = 0; i < m_widgets.size(); i++) {
        if ( (m_widgets[i]->getUuid() == widgetid) || (m_widgets[i]->getChannelName() == widgetid) ) {
            QList<QByteArray> props= m_widgets[i]->dynamicPropertyNames();

            foreach (QByteArray prop, props) {
                prop_names << QString(prop);
            }
            return prop_names;

        }
    }
    return QStringList();

}


bool WidgetLayout::destroyWidget(QString widgetid)
{
    for (int i = 0; i < m_widgets.size(); i++) {
        if ( (m_widgets[i]->getUuid() == widgetid) || (m_widgets[i]->getChannelName() == widgetid) ) {
            // is it necessary to use widgetsMutex.lock(); / unlock?
            deleteWidget(m_widgets[i]);
            markHistory();  // is it necessary here? probably yes, possible to undo
            return true;
        }
    }

    return false;
}

void WidgetLayout::propertiesDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->resize(300, 120);
    dialog->setModal(true);
    dialog->setWindowTitle("Widget Panel");
    QGridLayout *layout = new QGridLayout(dialog);
    bgCheckBox = new QCheckBox(dialog);
    bgCheckBox->setText("Enable Background");
    if (m_contained) {
        bgCheckBox->setChecked(parentWidget()->autoFillBackground());
    }
    else {
        bgCheckBox->setChecked(this->autoFillBackground());
    }
    layout->addWidget(bgCheckBox, 0, 0, Qt::AlignRight|Qt::AlignVCenter);
    QLabel *label = new QLabel(dialog);
    label->setText("Color");
    label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(label, 1, 0, Qt::AlignRight|Qt::AlignVCenter);
    bgButton = new QPushButton(dialog);
    QPixmap pixmap = QPixmap(64,64);
    QColor color;
    if (m_contained) {
        color = parentWidget()->palette().button().color();
    }
    else {
        color = this->palette().button().color();
    }
    bgButton->setProperty("CSQT_color",QVariant(color));
    pixmap.fill(color);
    bgButton->setIcon(pixmap);
    layout->addWidget(bgButton, 1, 1, Qt::AlignLeft|Qt::AlignVCenter);
    QPushButton *applyButton = new QPushButton(tr("Apply"));
    layout->addWidget(applyButton, 9, 1, Qt::AlignCenter|Qt::AlignVCenter);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    layout->addWidget(cancelButton, 9, 2, Qt::AlignCenter|Qt::AlignVCenter);
    QPushButton *acceptButton = new QPushButton(tr("Ok"));
    layout->addWidget(acceptButton, 9, 3, Qt::AlignCenter|Qt::AlignVCenter);
    connect(acceptButton, SIGNAL(released()), dialog, SLOT(accept()));
    connect(dialog, SIGNAL(accepted()), this, SLOT(applyProperties()));
    connect(applyButton, SIGNAL(released()), this, SLOT(applyProperties()));
    connect(cancelButton, SIGNAL(released()), dialog, SLOT(close()));
    connect(bgButton, SIGNAL(released()), this, SLOT(selectBgColor()));
    dialog->exec();
}

void WidgetLayout::applyProperties()
{
    QColor color = bgButton->property("CSQT_color").value<QColor>();
    setBackground(bgCheckBox->isChecked(), color);
    widgetChanged();
    mouseBut2 = 0;  // Button un clicked is not propagated after opening the edit dialog. Do it artificially here
}

void WidgetLayout::selectBgColor()
{
    QColor color;
    if (m_contained) {
        color = QColorDialog::getColor(parentWidget()->palette().button().color(), this);
    }
    else {
        color = QColorDialog::getColor(this->palette().button().color(), this);
    }
    if (color.isValid()) {
        bgButton->setProperty("CSQT_color",QVariant(color));
        QPixmap pixmap(64,64);
        pixmap.fill(color);
        bgButton->setIcon(pixmap);
    }
}

void WidgetLayout::alignLeft()
{
    int leftmost = 99999;
    if (m_editMode) {
        widgetsMutex.lock();
        int size = editWidgets.size();
        for (int i = 0; i < size ; i++) { // First find leftmost
            if (editWidgets[i]->isSelected())
                leftmost =  qMin(leftmost, editWidgets[i]->x());
        }
        if(leftmost == 99999) {
            // no widgets selected
            widgetsMutex.unlock();
            return;
        }
        for (int i = 0; i < size ; i++) { // Then put all x values to that
            if (editWidgets[i]->isSelected()) {
                editWidgets[i]->move(leftmost, editWidgets[i]->y());
                m_widgets[i]->move(leftmost, editWidgets[i]->y());
            }
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::alignRight()
{
    int rightmost = -1;
    if (m_editMode) {
        widgetsMutex.lock();
        int size = editWidgets.size();
        for (int i = 0; i < size ; i++) { // First find leftmost
            if (editWidgets[i]->isSelected())
                rightmost = qMax(rightmost, editWidgets[i]->x() + editWidgets[i]->width());
        }
        if(rightmost < 0) {
            // no widgets selected
            widgetsMutex.unlock();
            return;
        }
        for (int i = 0; i < size ; i++) { // Then put all x values to that
            if (editWidgets[i]->isSelected()) {
                int x = rightmost - editWidgets[i]->width();
                editWidgets[i]->move(x, editWidgets[i]->y());
                m_widgets[i]->move(x, editWidgets[i]->y());
            }
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::alignTop()
{
    int top = 99999;
    if (m_editMode) {
        widgetsMutex.lock();
        int size = editWidgets.size();
        for (int i = 0; i < size ; i++) { // First find uppermost
            if (editWidgets[i]->isSelected())
                top = editWidgets[i]->y() < top ? editWidgets[i]->y(): top;
        }
        if(top == 99999) {
            // no widgets selected
            widgetsMutex.unlock();
            return;
        }
        for (int i = 0; i < size ; i++) { // Then put all y values to that
            if (editWidgets[i]->isSelected()) {
                editWidgets[i]->move(editWidgets[i]->x(), top);
                m_widgets[i]->move(editWidgets[i]->x(), top);
            }
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::alignBottom()
{
    int bottom = -1;
    if (m_editMode) {
        widgetsMutex.lock();
        int size = editWidgets.size();
        for (int i = 0; i < size ; i++) {
            if (editWidgets[i]->isSelected())
                bottom = qMax(bottom, editWidgets[i]->y()+editWidgets[i]->height());
        }
        if(bottom < 0) {
            // no widgets selected
            widgetsMutex.unlock();
            return;
        }
        for (int i = 0; i < size ; i++) {
            if (editWidgets[i]->isSelected()) {
                int y = bottom - editWidgets[i]->height();
                editWidgets[i]->move(editWidgets[i]->x(), y);
                m_widgets[i]->move(editWidgets[i]->x(), y);
            }
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::sendToBack()
{
    if (!m_editMode)
        return;

    widgetsMutex.lock();
    for (int i = 0; i < editWidgets.size() ; i++) { // First invert selection
        if (editWidgets[i]->isSelected()) {
            editWidgets[i]->deselect();
        }
        else {
            editWidgets[i]->select();
        }
    }
    widgetsMutex.unlock();
    cut();
    paste();
    deselectAll();
    markHistory();
    setModified(true);
    //    widgetsMutex.lock();
    //    for (int i = 0; i < editWidgets.size() ; i++) { // Now invert selection again
    //      if (editWidgets[i]->isSelected()) {
    //        editWidgets[i]->deselect();
    //      }
    //      else {
    //        editWidgets[i]->select();
    //      }
    //    }
    //    widgetsMutex.unlock();
}

void WidgetLayout::sendToFront()
{
    if(!m_editMode)
        return;
    if (m_editMode) {
        cut();
        paste();
        deselectAll();
        markHistory();
        setModified(true);
    }
}


void WidgetLayout::distributeHorizontal()
{
    if (m_editMode) {
        int spacing, emptySpace, max = -9999, min = 9999, widgetWidth = 0;
        int num = 0;
        QVector<int> order;
        widgetsMutex.lock();
        for (int i = 0; i < editWidgets.size() ; i++) { // First check free space
            if (editWidgets[i]->isSelected()) {
                widgetWidth += editWidgets[i]->width();
                num++;
                if (min > editWidgets[i]->x()) { // Left most widget
                    min = editWidgets[i]->x();
                    order.prepend(i);
                }
                if (max < editWidgets[i]->x() + editWidgets[i]->width()) { // Right most widget
                    max = editWidgets[i]->x() + editWidgets[i]->width();
                    if (!order.contains(i))
                        order.append(i);
                }
                if (!order.contains(i)) {
                    int j = 0;
                    while (j < order.size()) {
                        if (editWidgets[order[j]]->x() > editWidgets[i]->x()) {
                            break;
                        }
                        j++;
                    }
                    order.insert(j++, i);
                }
            }
        }
        if (num < 3)
            return;  // do nothing for less than three selected
        emptySpace = max - min - widgetWidth;
        int accum = min;
        for (int i = 1; i < order.size() - 1 ; i++) { // Don't touch first and last
            spacing = emptySpace / (num- i);
            emptySpace -= spacing;
            accum += spacing + editWidgets[order[i-1]]->width();
            editWidgets[order[i]]->move(accum, editWidgets[order[i]]->y());
            m_widgets[order[i]]->move(accum, editWidgets[order[i]]->y());
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::distributeVertical()
{
    if (m_editMode) {
        int spacing, emptySpace, max = -9999, min = 9999, widgetHeight = 0;
        int num = 0;
        QVector<int> order;
        widgetsMutex.lock();
        for (int i = 0; i < editWidgets.size() ; i++) { // First check free space
            if (editWidgets[i]->isSelected()) {
                num++;
                widgetHeight += editWidgets[i]->height();
                if (min > editWidgets[i]->y()) { // Bottom widget
                    min = editWidgets[i]->y();
                    order.prepend(i);
                }
                if (max < editWidgets[i]->y() + editWidgets[i]->height()) { // Topmost widget
                    max = editWidgets[i]->y() + editWidgets[i]->height();
                    if (!order.contains(i))
                        order.append(i);
                }
                if (!order.contains(i)) {
                    int j = 0;
                    while (j < order.size()) {
                        if (editWidgets[order[j]]->y() > editWidgets[i]->y()) {
                            break;
                        }
                        j++;
                    }
                    order.insert(j++, i);
                }
            }
        }
        if (num < 3)
            return;  // do nothing for less than three selected
        emptySpace = max - min - widgetHeight;
        //    qDebug() << "WidgetLayout::distributeHorizontal " << emptySpace <<  "---" << order;
        int accum = min;
        for (int i = 1; i < order.size() - 1 ; i++) { // Don't touch first and last
            spacing = emptySpace / (num - i);
            emptySpace -= spacing;
            accum += spacing + editWidgets[order[i-1]]->height();
            editWidgets[order[i]]->move(editWidgets[order[i]]->x(), accum);
            m_widgets[order[i]]->move(editWidgets[order[i]]->x(), accum);
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::alignCenterHorizontal()
{
    if (m_editMode) {
        int center = 0, min = 9999;
        widgetsMutex.lock();
        for (int i = 0; i < editWidgets.size() ; i++) { // First find center
            if (editWidgets[i]->isSelected()) {
                if (min > editWidgets[i]->y()) { // Upper widget
                    min = editWidgets[i]->y();
                    center = editWidgets[i]->x() + (editWidgets[i]->width()/2);
                }
            }
        }
        for (int i = 0; i < editWidgets.size() ; i++) {
            if (editWidgets[i]->isSelected()) {
                int newx = center -  (editWidgets[i]->width()/2);
                editWidgets[i]->move(newx, editWidgets[i]->y());
                m_widgets[i]->move(newx, m_widgets[i]->y());
            }
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::alignCenterVertical()
{
    if (m_editMode) {
        int center = 0, min = 9999;
        widgetsMutex.lock();
        for (int i = 0; i < editWidgets.size() ; i++) { // First find center
            if (editWidgets[i]->isSelected()) {
                if (min > editWidgets[i]->x()) { // Leftmost widget
                    min = editWidgets[i]->x();
                    center = editWidgets[i]->y() + (editWidgets[i]->height()/2);
                }
            }
        }
        for (int i = 0; i < editWidgets.size() ; i++) {
            if (editWidgets[i]->isSelected()) {
                int newy = center -  (editWidgets[i]->height()/2);
                editWidgets[i]->move(editWidgets[i]->x(), newy);
                m_widgets[i]->move(m_widgets[i]->x(), newy);
            }
        }
        widgetsMutex.unlock();
        markHistory();
        setModified(true);
    }
}

void WidgetLayout::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if(m_editMode) {
        int quant = (event->modifiers() & Qt::AltModifier) ? 1 : 5;
        switch(key) {
        case Qt::Key_Left:
            this->moveSelected(-quant, 0, quant);
            event->accept();
            return;
        case Qt::Key_Right:
            this->moveSelected(quant, 0, quant);
            event->accept();
            return;
        case Qt::Key_Up:
            this->moveSelected(0, -quant, quant);
            event->accept();
            return;
        case Qt::Key_Down:
            this->moveSelected(0, quant, quant);
            event->accept();
            return;
        case Qt::Key_S:
            this->createNewSlider();
            event->accept();
            return;
        case Qt::Key_K:
            this->createNewKnob();
            event->accept();
            return;
        case Qt::Key_B:
            this->createNewButton();
            event->accept();
            return;
        case Qt::Key_A:
            if(event->modifiers() & Qt::ControlModifier) {
                this->selectAll();
                event->accept();
                return;
            }
        }

    }

    if (!event->isAutoRepeat() || m_repeatKeys) {
        QString keyText = event->text();
        if (key == Qt::Key_D && (event->modifiers() & Qt::ControlModifier )) {
            // TODO why is this necessary? The shortcut from the duplicate
            // action in the main app is not working!
            this->duplicate();
            event->accept();
            return;
        }

        //    if (event->key() == Qt::Key_X && (event->modifiers() & Qt::ControlModifier )) {
        //      this->cut();
        //      event->accept();
        //      return;
        //    }
        //    else if (event->key() == Qt::Key_C && (event->modifiers() & Qt::ControlModifier )) {
        //      this->copy();
        //      event->accept();
        //      return;
        //    }
        //    else if (event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier )) {
        //      this->paste();
        //      event->accept();
        //      return;
        //    }
        if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
            this->deleteSelected();
            event->accept();
            return;
        }
        //    else if (event->matches(QKeySequence::Undo)) {
        //      this->undo();
        //      event->accept();
        //    }
        //    else if (event->matches(QKeySequence::Redo)) {
        //      this->redo();
        //      event->accept();
        //    }
        else { //else if (key != "") {
            //      qDebug() << "WidgetLayout::keyPressEvent" <convolve< key;
            //           appendMessage(key);
            QWidget::keyPressEvent(event); // Propagate event if not used
            if (!keyText.isEmpty()) {
                int key = static_cast<int>( keyText[0].toLatin1() ); // if simple character send ASCII code
                emit keyPressed(key);
            } else {
                emit keyPressed(event->key()); // else send keycode and let CsoundEngine process it
            }
            //      event->accept();
        }
    }
}

void WidgetLayout::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat() || m_repeatKeys) {
        QString keyText = event->text();
        int key = -1;
        if (!keyText.isEmpty()) {
            key = (int) keyText[0].toLatin1(); // if simple character send ASCII code
        } else {
            key = event->key();
        }
        emit keyReleased(key);
    }
    QWidget::keyReleaseEvent(event); // Propagate event
}

void WidgetLayout::widgetChanged(QuteWidget* widget)
{
    if (widget != nullptr) {
        //    widgetsMutex.lock();
        int index = m_widgets.indexOf(widget);
        if (index >= 0 && editWidgets.size() > index) {
            int newx = widget->x();
            int newy = widget->y();
            int neww = widget->width();
            int newh = widget->height();
            editWidgets[index]->move(newx, newy);
            editWidgets[index]->resize(neww, newh);
        }
        setWidgetToolTip(widget, m_tooltips);
        //    widgetsMutex.unlock();
        int cc = widget->property("CSQT_midicc").toInt();  // Is it safe to query these here?
        int chan = widget->property("CSQT_midichan").toInt();
        registerWidgetController(widget, cc);
        registerWidgetChannel(widget, chan);
        setModified(true);
    }
    adjustLayoutSize();
}

void WidgetLayout::mousePressEvent(QMouseEvent *event)
{
    if (m_editMode && (event->button() & Qt::LeftButton)) {
        this->setFocus(Qt::MouseFocusReason);
        selectionFrame->show();
        startx = event->x() + xOffset;
        starty = event->y() + yOffset;
        selectionFrame->setGeometry(startx, starty, 0,0);
        if (event->button() & Qt::LeftButton) {
            deselectAll();
        }
    }
    mouseLock.lockForWrite();
    if (event->button() == Qt::LeftButton)
        mouseBut1 = 1;
    else if (event->button() == Qt::RightButton)
        mouseBut2 = 1;
    mouseLock.unlock();
    //  QWidget::mousePressEvent(event);
}

void WidgetLayout::mouseMoveEvent(QMouseEvent *event)
{
    //  QWidget::mouseMoveEvent(event);
    int x = startx;
    int y = starty;
    int width = abs(event->x() - startx + xOffset);
    int height = abs(event->y() - starty + yOffset);
    if (event->buttons() & Qt::LeftButton) {
        // Currently dragging selection
        if (event->x() < (startx - xOffset)) {
            x = event->x() + xOffset;
        }
        if (event->y() < (starty - yOffset)) {
            y = event->y() + yOffset;
        }
        selectionFrame->setGeometry(x, y, width, height);
        selectionChanged(QRect(x - xOffset, y - yOffset, width, height));
    }
    mouseLock.lockForWrite();
    mouseX = event->globalX();
    mouseY = event->globalY();
    mouseRelX = event->x() + xOffset;
    mouseRelY = event->y() + yOffset;
    mouseLock.unlock();
}

void WidgetLayout::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        selectionFrame->hide();
    }
    mouseLock.lockForWrite();
    if (event->button() == Qt::LeftButton)
        mouseBut1 = 0;
    else if (event->button() == Qt::RightButton) {
        emit deselectAll();
        mouseBut2 = 0;
    }
    mouseLock.unlock();
    markHistory();
    //  QWidget::mouseReleaseEvent(event);
}

void WidgetLayout::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_enableEdit) {
        createContextMenu(event);
        event->accept();
    }
}

void WidgetLayout::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event);
    emit this->windowStatus(false);
}

int WidgetLayout::parseXmlNode(QDomNode node)
{
    int ret = 0;
    QString name = node.nodeName();
    if (name == "objectName") {
        m_objectName = node.firstChild().nodeValue();
        //    this->setProperty("CSQT_objectName", node.firstChild().nodeValue());
    }
    else if (name == "label") {
        this->setWindowTitle(node.firstChild().nodeValue());
    }
    else if (name == "x") {
        int newx = node.firstChild().nodeValue().toInt();
        m_posx = newx >= 0 && newx < 4096? newx : m_posx;
    }
    else if (name == "y") {
        int newy = node.firstChild().nodeValue().toInt();
        m_posy = newy >= 0 && newy < 4096? newy : m_posy;
    }
    else if (name == "width") {
        int neww = node.firstChild().nodeValue().toInt();
        m_w = neww >= 0 && neww < 4096? neww : m_w;
    }
    else if (name == "height") {
        int newh = node.firstChild().nodeValue().toInt();
        m_h = newh >= 0 && newh < 4096? newh : m_h;
    }
    else if (name == "visible") {
        m_visible = node.firstChild().nodeValue() == "true";
    }
    else if (name == "uuid") {
        m_uuid = node.firstChild().nodeValue();
    }
    else if (name == "bgcolor") {
        bool bg = false;
        auto nodeElement = node.toElement();

        if (nodeElement.attribute("mode")== "background") {
            //qDebug() << "background true";
            bg = true;
        }
        QDomElement er = nodeElement.firstChildElement("r");
        QDomElement eg = nodeElement.firstChildElement("g");
        QDomElement eb = nodeElement.firstChildElement("b");
        auto bgcolor = QColor(er.firstChild().nodeValue().toInt(),
                              eg.firstChild().nodeValue().toInt(),
                              eb.firstChild().nodeValue().toInt());
        // auto parent = node.parentNode().nodeName();
        //qDebug() << "setting background" << parent << bgcolor << node.toElement().text();
        setBackground(bg, bgcolor);
    }
    else if (name == "bsbObject") {
        ret = newXmlWidget(node);
    }
    else if (name == "bsbGroup") {
        qDebug() << "bsbGroup not implemented";
    }
    else {
        qDebug() << "WidgetLayout::parseXmlNode unknown node name: "<< name;
        return -1;
    }
    return ret;
}

QString WidgetLayout::createSlider(int x, int y, int width, int height, QString widgetLine)
{
    //   qDebug("ioSlider x=%i y=%i w=%i h=%i", x,y, width, height);
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QuteSlider *widget= new QuteSlider(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setProperty("CSQT_minimum",parts[5].toDouble());
    widget->setProperty("CSQT_maximum",parts[6].toDouble());
    widget->setProperty("CSQT_value",parts[7].toDouble());

    if (parts.size()>8) {
        int i=8;
        QString channelName = "";
        while (parts.size()>i) {
            channelName += parts[i] + " ";
            i++;
        }
        channelName.chop(1);  //remove last space
        widget->setProperty("CSQT_objectName", channelName);
    }
    widget->applyInternalProperties();
    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this,
            SLOT(newValue(QPair<QString,double>)));
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createText(int x, int y, int width, int height, QString widgetLine)
{
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QStringList quoteParts = widgetLine.split('"');
    if (parts.size()<20 || quoteParts.size()<5)
        return "";
    QStringList lastParts = quoteParts[4].split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    if (lastParts.size() < 9)
        return "";
    QuteText *widget= new QuteText(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setType(parts[5]);
    widget->setProperty("CSQT_objectName",quoteParts[1]);
    widget->setProperty("CSQT_alignment",quoteParts[2].simplified());
    widget->setProperty("CSQT_font",quoteParts[3].simplified());
    widget->setProperty("CSQT_fontsize",lastParts[0].toInt() + 2);
    widget->setProperty("CSQT_color", QColor(lastParts[1].toDouble()/256.0,
                        lastParts[2].toDouble()/256.0,
            lastParts[3].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolor", QColor(lastParts[4].toDouble()/256.0,
                        lastParts[5].toDouble()/256.0,
            lastParts[6].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolormode", lastParts[7] == "background");
    widget->setProperty("CSQT_bordermode", lastParts[8]);
    QString labelText = widgetLine.mid(widgetLine.indexOf("border") + 7);
    if (parts[5] == "label") {
        widget->setProperty("CSQT_precision", 3);
        widget->setTransparentForMouse(true);
    }
    else if (parts[5] == "display") {
        widget->setProperty("CSQT_precision", 3);
    }
    else {
        widget->setProperty("CSQT_resolution", parts[7].toDouble());
    }
    labelText.replace("\u00AC", "\n");
    widget->setProperty("CSQT_label", labelText);
    widget->setFontOffset(m_fontOffset);
    widget->setFontScaling(m_fontScaling);
    widget->applyInternalProperties();
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createScrollNumber(int x, int y, int width, int height, QString widgetLine)
{
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QStringList quoteParts = widgetLine.split('"');
    if (parts.size()<20 || quoteParts.size()<5)
        return "";
    QStringList lastParts = quoteParts[4].split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    if (lastParts.size() < 9)
        return "";
    QuteScrollNumber *widget= new QuteScrollNumber(this);

    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setType(parts[5]);
    widget->setProperty("CSQT_resolution", parts[7].toDouble());
    widget->setProperty("CSQT_objectName",quoteParts[1]);
    widget->setProperty("CSQT_alignment",quoteParts[2].simplified());
    widget->setProperty("CSQT_font",quoteParts[3].simplified());
    widget->setProperty("CSQT_fontsize",lastParts[0].toInt() + 2);
    widget->setProperty("CSQT_color", QColor(lastParts[1].toDouble()/256.0,
                        lastParts[2].toDouble()/256.0,
            lastParts[3].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolor", QColor(lastParts[4].toDouble()/256.0,
                        lastParts[5].toDouble()/256.0,
            lastParts[6].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolormode", lastParts[7] == "background");
    widget->setProperty("CSQT_bordermode", lastParts[8]);

    widget->setFontOffset(m_fontOffset);
    widget->setFontScaling(m_fontScaling);
    QString labelText = "";
    int i = 9;
    while (lastParts.size() > i) {
        labelText += lastParts[i] + " ";
        i++;
    }
    labelText.chop(1);
    widget->setProperty("CSQT_value", labelText.toDouble());
    //  widget->setValue(labelText.toDouble());
    widget->applyInternalProperties();
    connect(widget, SIGNAL(newValue(QPair<QString,double>)),
            this, SLOT(newValue(QPair<QString,double>)));
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createLineEdit(int x, int y, int width, int height, QString widgetLine)
{
    qDebug() << "createLineEdit";
    // the following line was using splitRef, replacing it with QStringView does not make sens, since not run often
    auto parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QStringList quoteParts = widgetLine.split('"');
    if (parts.size()<20 || quoteParts.size()<5)
        return "";
    QStringList lastParts = quoteParts[4].split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    if (lastParts.size() < 9)
        return "";
    QuteLineEdit *widget= new QuteLineEdit(this);

    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setType(parts[5]);
    widget->setProperty("CSQT_objectName", quoteParts[1]);
    widget->setProperty("CSQT_alignment",quoteParts[2].simplified());
    widget->setProperty("CSQT_font",quoteParts[3].simplified());
    widget->setProperty("CSQT_fontsize",lastParts[0].toInt() + 2);
    widget->setProperty("CSQT_color", QColor(lastParts[1].toDouble()/256.0,
                        lastParts[2].toDouble()/256.0,
            lastParts[3].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolor", QColor(lastParts[4].toDouble()/256.0,
                        lastParts[5].toDouble()/256.0,
            lastParts[6].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolormode", lastParts[7] == "background");
    widget->setProperty("CSQT_bordermode", lastParts[8]);
    QString labelText = "";
    labelText = widgetLine.mid(widgetLine.indexOf("border") + 7);
    widget->setProperty("CSQT_label", labelText);
    widget->applyInternalProperties();
    QDEBUG << ">>>>> name" << quoteParts[1] << ", bgcolormode" << lastParts[7];

    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createSpinBox(int x, int y, int width, int height, QString widgetLine)
{
    const static QRegularExpression regex("[\\{\\}, ]");
    QStringList parts = widgetLine.split(regex, SKIP_EMPTY_PARTS);
    QStringList quoteParts = widgetLine.split('"');
    if (parts.size()<20 || quoteParts.size()<5)
        return "";
    QStringList lastParts = quoteParts[4].split(regex, SKIP_EMPTY_PARTS);
    if (lastParts.size() < 9)
        return "";
    QuteSpinBox *widget= new QuteSpinBox(this);

    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setType(parts[5]);
    widget->setProperty("CSQT_resolution",parts[7].toDouble());
    widget->setProperty("CSQT_objectName",quoteParts[1]);
    widget->setProperty("CSQT_alignment",quoteParts[2].simplified());
    widget->setProperty("CSQT_font",quoteParts[3].simplified());
    widget->setProperty("CSQT_fontsize",lastParts[0].toInt() + 2);
    widget->setProperty("CSQT_color", QColor(lastParts[1].toDouble()/256.0,
                        lastParts[2].toDouble()/256.0,
            lastParts[3].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolor", QColor(lastParts[4].toDouble()/256.0,
                        lastParts[5].toDouble()/256.0,
            lastParts[6].toDouble()/256.0));
    widget->setProperty("CSQT_bgcolormode", lastParts[7] == "background");
    widget->setProperty("CSQT_bordermode", lastParts[8]);

    QString labelText = "";
    int i = 9;
    while (lastParts.size() > i) {
        labelText += lastParts[i] + " ";
        i++;
    }
    labelText.chop(1);
    widget->setProperty("CSQT_value", labelText.toDouble());
    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this, SLOT(newValue(QPair<QString,double>)));
    widget->applyInternalProperties();
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createButton(int x, int y, int width, int height, QString widgetLine)
{
    qDebug("WidgetPanel::createButton");
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QStringList quoteParts = widgetLine.split('"');
    //   if (parts.size()<20 || quoteParts.size()>5)
    //     return -1;
    QStringList lastParts = quoteParts[4].split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    //   if (lastParts.size() < 9)
    //     return -1;
    QuteButton *widget= new QuteButton(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setProperty("CSQT_objectName",quoteParts[1]);
    widget->setProperty("CSQT_type",parts[5]);
    widget->setProperty("CSQT_pressedValue",parts[6].toDouble()); //value produced by button when pushed
    //  widget->setProperty("CSQT_stringvalue",parts[7].toDouble()); // Not available in old format
    widget->setText(quoteParts[3]);
    widget->setProperty("CSQT_image",quoteParts[5]);
    if (quoteParts.size()>6) {
        quoteParts[6].remove(0,1); //remove initial space
        widget->setProperty("CSQT_eventLine", quoteParts[6]);
    }
    connect(widget, SIGNAL(queueEventSignal(QString)), this, SLOT(queueEvent(QString)));
    connect(widget, SIGNAL(newValue(QPair<QString,QString>)),
            this, SLOT(newValue(QPair<QString,QString>)));
    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this, SLOT(newValue(QPair<QString,double>)));
    emit registerButton(widget);
    widget->applyInternalProperties();
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createKnob(int x, int y, int width, int height, QString widgetLine)
{
    auto parts = QStringView(widgetLine).split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS); // splitRef
    // QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QuteKnob *widget= new QuteKnob(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setProperty("CSQT_minimum",parts[5].toDouble());
    widget->setProperty("CSQT_maximum",parts[6].toDouble());
    widget->setProperty("CSQT_resolution",parts[7].toDouble());
    widget->setProperty("CSQT_value",parts[8].toDouble());
    if (parts.size()>9) {
        int i=9;
        QString channelName = "";
        while (parts.size()>i) {
            channelName += parts[i].toString() + " ";
            i++;
        }
        channelName.chop(1);  //remove last space
        widget->setProperty("CSQT_objectName", channelName);
    }
    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this, SLOT(newValue(QPair<QString,double>)));
    widget->applyInternalProperties();
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createCheckBox(int x, int y, int width, int height, QString widgetLine)
{
    auto parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS); // splitRef
    QuteCheckBox *widget= new QuteCheckBox(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setValue(parts[5]=="on");
    if (parts.size()>6) {
        int i=6;
        QString channelName = "";
        while (parts.size()>i) {
            channelName += parts[i] + " ";
            i++;
        }
        channelName.chop(1);  //remove last space
        widget->setProperty("CSQT_objectName", channelName);
    }
    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this, SLOT(newValue(QPair<QString,double>)));
    widget->applyInternalProperties();
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createMenu(int x, int y, int width, int height, QString widgetLine)
{
    auto parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS); // was: splitRef
    auto quoteParts = widgetLine.split('"');
    QuteComboBox *widget= new QuteComboBox(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    if (quoteParts.size() > 2) {
        widget->setProperty("CSQT_objectName", quoteParts[2].trimmed()); //remove initial space from channel name
    }
    widget->setProperty("CSQT_selectedIndex", parts[5].toInt());

    widget->setText(quoteParts[1]);
    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this, SLOT(newValue(QPair<QString,double>)));
    widget->applyInternalProperties();
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createMeter(int x, int y, int width, int height, QString widgetLine)
{
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QStringList quoteParts = widgetLine.split('"');
    if (quoteParts.size() < 5) {
        qDebug("WidgetPanel::createMeter ERROR parsing widget line!");
        return 0;
    }
    QStringList parts2 = quoteParts[4].split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    if (parts2.size() < 5) {
        qDebug("WidgetPanel::createMeter ERROR parsing widget line!");
        return 0;
    }
    QuteMeter *widget= new QuteMeter(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    widget->setProperty("CSQT_color" ,QColor(parts[5].toDouble()/256.0,
                        parts[6].toDouble()/256.0,
            parts[7].toDouble()/256.0));
    widget->setProperty("CSQT_type", parts2[1]);
    widget->setProperty("CSQT_objectName2", quoteParts[1]);
    widget->setProperty("CSQT_yValue", quoteParts[2].toDouble());
    widget->setProperty("CSQT_xValue", parts2[0].toDouble());
    widget->setProperty("CSQT_objectName", quoteParts[3]);
    widget->setProperty("CSQT_pointsize", parts2[2].toInt());
    widget->setProperty("CSQT_fadeSpeed", parts2[3].toInt());
    widget->setProperty("CSQT_xMin", 0.0);
    widget->setProperty("CSQT_xMax", 1.0);
    widget->setProperty("CSQT_yMin", 0.0);
    widget->setProperty("CSQT_yMax", 1.0);
    //  widget->setBehavior(parts2[4]);

    connect(widget, SIGNAL(newValue(QPair<QString,double>)), this, SLOT(newValue(QPair<QString,double>)));
    widget->applyInternalProperties();
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createConsole(int x, int y, int width, int height, QString widgetLine)
{
    //    qDebug("ioListing x=%i y=%i w=%i h=%i", x,y, width, height);
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QuteConsole *widget= new QuteConsole(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);

    widget->applyInternalProperties();
    consoleWidgets.append(widget);
    registerWidget(widget);
    return widget->getUuid();
}

QString WidgetLayout::createGraph(int x, int y, int width, int height, QString widgetLine)
{
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    QuteGraph *widget= new QuteGraph(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    //Graph widget is always of type "graph" part 5 is discarded
    if (parts.size() > 6)
        widget->setValue(parts[6].toDouble());
    if (parts.size() > 7) {
        widget->setProperty("CSQT_zoomx",parts[7].toDouble());
    }
    else
        widget->setProperty("CSQT_zoomx", 1.0);
    if (parts.size()>8) {
        int i = 8;
        QString channelName = "";
        while (parts.size()>i) {
            channelName += parts[i] + " ";
            i++;
        }
        channelName.chop(1);  //remove last space
        widget->setProperty("CSQT_objectName", channelName);
    }
    for (int i = 0; i < curves.size(); i++) {
        widget->addCurve(curves[i]);
    }
    graphWidgets.append(widget);
    emit requestCsoundUserData(widget);
    emit registerGraph(widget);
    registerWidget(widget);
    widget->applyInternalProperties();
    return widget->getUuid();
}

QString WidgetLayout::createScope(int x, int y, int width, int height, QString widgetLine)
{
    QuteScope *widget= new QuteScope(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    QStringList parts = widgetLine.split(QRegularExpression("[\\{\\}, ]"), SKIP_EMPTY_PARTS);
    if (parts.size() > 5) {
        widget->setProperty("CSQT_type",parts[5]);
    }
    if (parts.size() > 6) {
        widget->setProperty("CSQT_zoomx",parts[6].toDouble());
    }
    if (parts.size() > 7) {
        int chans = (int) parts[7].toDouble();
        if (chans < 0) {
            chans = -255; // Force all 8 channels when loading old format
        }
        widget->setProperty("CSQT_value", chans);
    }
    if (parts.size() > 8) {
        int i=8;
        QString channelName = "";
        while (parts.size()>i) {
            channelName += parts[i] + " ";
            i++;
        }
        channelName.chop(1);  //remove last space
        widget->setProperty("CSQT_objectName", channelName);
    }
    emit registerScope(widget);
    scopeWidgets.append(widget);
    registerWidget(widget);
    widget->applyInternalProperties();
    return widget->getUuid();
}

QString WidgetLayout::createDummy(int x, int y, int width, int height, QString widgetLine)
{
    (void) widgetLine;
    QuteWidget *widget= new QuteDummy(this);
    widget->setProperty("CSQT_x",x);
    widget->setProperty("CSQT_y",y);
    widget->setProperty("CSQT_width",width);
    widget->setProperty("CSQT_height",height);
    connect(widget, SIGNAL(propertiesAccepted()), this, SLOT(markHistory()));
    widget->show();
    widgetsMutex.lock();
    m_widgets.append(widget);
    if (m_editMode) {
        createEditFrame(widget);
        editWidgets.last()->select();
    }
    setWidgetToolTip(widget, m_tooltips);
    widgetsMutex.unlock();
    return widget->getUuid();
}

QString WidgetLayout::createTableDisplay(int x, int y, int width, int height, QString widgetLine) {
    Q_UNUSED(widgetLine);
    QuteTable *widget = new QuteTable(this);
    widget->setProperty("CSQT_x", x);
    widget->setProperty("CSQT_y", y);
    widget->setProperty("CSQT_width", width);
    widget->setProperty("CSQT_height", height);
    widget->setProperty("CSQT_tableNumber", 0);

    emit requestCsoundUserData(widget);
    registerWidget(widget);
    widget->applyInternalProperties();
    return widget->getUuid();
}

void WidgetLayout::setBackground(bool bg, QColor bgColor)
{
    QWidget *w;
    layoutMutex.lock();
    w = m_contained ?  this->parentWidget() : this;  // If contained, set background of parent widget

    if (bg) // to get rid of pink backgrounds
        w->setPalette(QPalette(bgColor));
    else
        w->setPalette(QPalette());
    w->setBackgroundRole(QPalette::Window);
    w->setAutoFillBackground(bg);
    layoutMutex.unlock();
    this->setProperty("CSQT_bg", QVariant(bg));
    this->setProperty("CSQT_bgcolor", QVariant(bgColor));
}

FrameWidget *WidgetLayout::getEditWidget(QuteWidget *widget)
{
    // This function is only called from setWidgetTooltip, so it should not be locked,
    // because it already is
    FrameWidget *out = 0;
    for (int i  = 0 ; i < m_widgets.size(); i++) {
        if (m_widgets[i] == widget) {
            if (editWidgets.size() > i) {
                return editWidgets[i];
            }
        }
    }
    return out;
}

void WidgetLayout::registerWidgetController(QuteWidget *widget, int cc)
{
    for (int i = 0; i < registeredControllers.size(); i++) {
        if (registeredControllers[i].widget == widget) {
            registeredControllers[i].cc = cc;
            return;
        }
    }
    // right order of parameters: RegisteredController(QuteWidget * _widget, int _chan, int _cc)
    registeredControllers << RegisteredController(widget, 0, cc);
}

void WidgetLayout::registerWidgetChannel(QuteWidget *widget, int chan)
{
    if (chan < 0) { // No control
        return;
    }
    for (int i = 0; i < registeredControllers.size(); i++) {
        if (registeredControllers[i].widget == widget) {
            registeredControllers[i].chan = chan;
            return;
        }
    }
    // RegisteredController(QuteWidget * _widget, int _chan,int  _cc)
    registeredControllers << RegisteredController(widget, chan, 1);
}

void WidgetLayout::unregisterWidgetController(QuteWidget *widget)
{
    for (int i = 0; i < registeredControllers.size(); i++) {
        if (registeredControllers[i].widget == widget) {
            registeredControllers.removeAt(i);
            return;
        }
    }
}

void WidgetLayout::clearWidgetControllers()
{
    registeredControllers.clear();
}

void WidgetLayout::setModified(bool mod)
{
    m_modified = mod;
    if (mod) {
        emit changed();
    }
}

void WidgetLayout::setMouseOffset(int x, int y)
{
    xOffset = -x;
    yOffset = -y;
}

void WidgetLayout::clearHistory()
{
    m_history.clear();
    m_history << "";
    m_historyIndex = 0;
}

int WidgetLayout::getPresetIndex(int number)
{
    for (int i = 0; i < presets.size(); i++) {
        if (presets[i].getNumber() == number) {
            return i;
        }
    }
    return -1;
}

void WidgetLayout::loadPreset()
{
    QDialog d(this);
    QVBoxLayout *l = new QVBoxLayout(&d);
    QPushButton *okButton = new QPushButton(tr("Close"),&d);
    QPushButton *newButton = new QPushButton(tr("New Preset"),&d);

    QTreeWidget *treeWidget = new QTreeWidget(&d);

    treeWidget->setHeaderLabel(tr("Double-click Preset to Load"));
    l->addWidget(treeWidget);
    l->addWidget(okButton);
    l->addWidget(newButton);

    connect(okButton, SIGNAL(released()), &d, SLOT(accept()));
    connect(newButton, SIGNAL(released()), this, SLOT(newPreset()));
    connect(newButton, SIGNAL(released()), &d, SLOT(reject()));  // For now just close the load preset window (instead of refreshing...

    treeWidget->setColumnCount(1);
    QList<QTreeWidgetItem *> items;
    for (int i = 0; i < presets.size(); i++) {
        QString itemText = QString::number(presets[i].getNumber()) + "  " + presets[i].getName();
        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0, QStringList(itemText));
        item->setData(0,Qt::UserRole,presets[i].getNumber());
        items.append(item);
    }
    QList<QTreeWidgetItem *> sortedItems;
    for (int i = 0; i < items.size(); i++) {
        int j = 0;
        for (j = 0; j < sortedItems.size(); j++) {
            if (items[i]->data(0,Qt::UserRole).toInt() < sortedItems[j]->data(0,Qt::UserRole).toInt()) {
                break;
            }
        }
        sortedItems.insert(j, items[i]);
    }
    treeWidget->insertTopLevelItems(0, sortedItems);
    connect(treeWidget,SIGNAL(itemDoubleClicked (QTreeWidgetItem * , int)),
            this, SLOT(loadPresetFromItem(QTreeWidgetItem * , int)) );

    d.setModal(false);
    d.exec();
}

void WidgetLayout::loadPresetFromAction()
{
    QAction *s = static_cast<QAction *>(sender());
    loadPreset(s->data().toInt());
}

void WidgetLayout::loadPresetFromItem(QTreeWidgetItem * item, int column)
{
    loadPreset(item->data(column,Qt::UserRole).toInt());
}

void WidgetLayout::loadPreset(int num)
{
    int index = getPresetIndex(num);
    loadPresetFromIndex(index);
}

void WidgetLayout::loadPresetFromIndex(int index)
{
    if (index < 0 || index >= presets.size()) {
        qDebug() << "WidgetLayout::loadPreset num invalid.";
        return;
    }
    m_currentPreset = index;
    WidgetPreset p = presets[index];
    QStringList ids = p.getWidgetIds();
    widgetsMutex.lock();
    for (int i = 0; i < ids.size(); i++) {
        QString savedId = ids[i];
        for (int j = 0; j < m_widgets.size(); j++) {
            QString id = m_widgets[j]->getUuid();
            if (savedId != id)
                continue;
            int mode = p.getMode(i);
            if (mode & 1) {
                m_widgets[j]->setValue(p.getValue(i));
                QString channel = m_widgets[j]->getChannelName();
                // Store the value in the changes buffer to read from chnget
                if (!channel.isEmpty()) {
                    valueMutex.lock();
                    if(newValues.contains(channel))
                        newValues[channel] = p.getValue(i);
                    else
                        newValues.insert(channel, p.getValue(i));
                    valueMutex.unlock();
                }
            }
            if (mode & 2) {
                m_widgets[j]->setValue2(p.getValue2(i));
                QString channel = m_widgets[j]->getChannelName();
                // store the value in the changes buffer to read from chnget
                if (!channel.isEmpty()) {
                    valueMutex.lock();
                    if(newValues.contains(channel))
                        newValues[channel] = p.getValue2(i);
                    else
                        newValues.insert(channel, p.getValue2(i));
                    valueMutex.unlock();
                }
            }
            if (mode & 4) {
                m_widgets[j]->setValue(p.getStringValue(i));
                QString channel = m_widgets[j]->getChannelName();
                // Store the value in the changes buffer to read from chnget
                if (!channel.isEmpty()) {
                    stringValueMutex.lock();
                    if(newStringValues.contains(channel))
                        newStringValues[channel] = p.getStringValue(i);
                    else
                        newStringValues.insert(channel, p.getStringValue(i));
                    stringValueMutex.unlock();
                }
            }
        }
    }
    widgetsMutex.unlock();
    QPair<QString, QString> channelValue;
    channelValue.first = "_GetPresetName";
    channelValue.second = p.getName();
    newValue(channelValue);
    QPair<QString, double> channelValue2;
    channelValue2.first = "_GetPresetNumber";
    channelValue2.second = p.getNumber();
    newValue(channelValue2);
}

void WidgetLayout::newPreset()
{
    QDialog d(this);
    QGridLayout *l = new QGridLayout(&d);
    QLabel *nameLabel = new QLabel(tr("New Preset Name:"), &d);
    QLineEdit *nameLineEdit = new QLineEdit(&d);
    QLabel *numberLabel = new QLabel(tr("Number:"), &d);
    QSpinBox *numberSpinBox = new QSpinBox(&d);
    numberSpinBox->unsetLocale();
    QPushButton *okButton = new QPushButton(tr("Ok"),&d);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"),&d);
    l->addWidget(nameLabel, 0,0, Qt::AlignRight);
    l->addWidget(nameLineEdit, 0,1, Qt::AlignLeft);
    l->addWidget(numberLabel, 1,0, Qt::AlignRight);
    l->addWidget(numberSpinBox, 1,1, Qt::AlignLeft);
    l->addWidget(cancelButton, 2,0, Qt::AlignCenter);
    l->addWidget(okButton, 2,1, Qt::AlignCenter);
    connect(okButton, SIGNAL(released()), &d, SLOT(accept()));
    connect(cancelButton, SIGNAL(released()), &d, SLOT(reject()));

    int ret = d.exec();
    if (ret == QDialog::Accepted) {
        if (presetExists(numberSpinBox->value())) {
            int ret = QMessageBox::question(this, tr("Preset Already Exists"),
                                            tr("Preset %i already exists. Overwrite?").arg(numberSpinBox->value()),
                                            QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                savePreset(numberSpinBox->value(), nameLineEdit->text() );
            }
        }
        else {
            savePreset(numberSpinBox->value(), nameLineEdit->text() );
        }
    }
}


void WidgetLayout::savePreset()
{
    QDialog d(this);
    QVBoxLayout *l = new QVBoxLayout(&d);
    QLabel *lab= new QLabel(&d);
    QComboBox *box = new QComboBox(&d);
    QPushButton *okButton = new QPushButton(tr("Ok"),&d);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"),&d);
    QPushButton *newButton = new QPushButton(tr("New Preset"),&d);

    lab->setText(tr("Select Preset to save"));
    l->addWidget(lab);
    l->addWidget(box);
    l->addWidget(cancelButton);
    l->addWidget(newButton);
    l->addWidget(okButton);
    connect(okButton, SIGNAL(released()), &d, SLOT(accept()));
    connect(cancelButton, SIGNAL(released()), &d, SLOT(reject()));
    connect(newButton, SIGNAL(released()), this, SLOT(newPreset()));
    connect(newButton, SIGNAL(released()), &d, SLOT(reject()));  // For now just close the load preset window (instead of refreshing...
    for (int i = 0; i < presets.size(); i++) {
        QString itemText = QString::number(presets[i].getNumber()) + "  " + presets[i].getName();
        box->addItem(itemText, presets[i].getNumber() );
    }
    box->setCurrentIndex(m_currentPreset);

    int ret = d.exec();
    if (ret == QDialog::Accepted) {
        if (box->currentIndex() < 0) {
            newPreset();
        }
        else {
            savePreset(presets[box->currentIndex()].getNumber(), presets[box->currentIndex()].getName());
        }
    }
}

void WidgetLayout::savePreset(int num, QString name)
{
    int index = getPresetIndex(num);
    WidgetPreset p;
    p.setName(name);
    p.setNumber(num);
    if (index == -1) {
        qDebug() << "WidgetLayout::savePreset new preset";
        presets.append(p);
        index = presets.size() - 1;
    }
    widgetsMutex.lock();
    for (int i = 0; i < m_widgets.size(); i++) {
        QString id = m_widgets[i]->getUuid();
        if (!(m_widgets[i]->getWidgetType() == "BSBLabel")
                && !(m_widgets[i]->getWidgetType() == "BSBLineEdit")
                && !(m_widgets[i]->getWidgetType() == "BSBButton")
                && !(m_widgets[i]->getWidgetType() == "BSBConsole")) {
            p.addValue(id, m_widgets[i]->getValue());
        }
        if (m_widgets[i]->getWidgetType() == "BSBButton") {
            if (static_cast<QuteButton *>(m_widgets[i])->property("CSQT_latch").toBool()) {
                p.addValue(id, m_widgets[i]->getValue());
            }
        }
        if (m_widgets[i]->getWidgetType() == "BSBController"
                || m_widgets[i]->getWidgetType() == "BSBXYController") {
            p.addValue2(id, m_widgets[i]->getValue2());
        }
        if (m_widgets[i]->getWidgetType() == "BSBButton"
                || m_widgets[i]->getWidgetType() == "BSBLineEdit"
                || m_widgets[i]->getWidgetType() == "BSBDisplay") {
            p.addStringValue(id, m_widgets[i]->getStringValue());
        }
        // Note that BSBLabel is left out from presets
    }
    widgetsMutex.unlock();
    presets[index] = p;
    m_currentPreset = index;
}

void WidgetLayout::setPresetName(int num, QString name)
{
    if (num >= 0 && num < presets.size()) {
        int index = getPresetIndex(num);
        if (index >= 0) {
            presets[getPresetIndex(num)].setName(name);
        }
        else {
            qDebug() << "WidgetLayout::setPresetName invalid number.";
        }
    }
}

QList<int> WidgetLayout::getPresetNums()
{
    QList<int> list;
    for (int i = 0; i < presets.size(); i++) {
        list.append(presets[i].getNumber());
    }
    return list;
}

QString WidgetLayout::getPresetName(int num)
{
    int index = getPresetIndex(num);
    if (index >= 0) {
        return presets[getPresetIndex(num)].getName();
    }
    else {
        return QString();
        qDebug() << "WidgetLayout::getPresetName invalid number.";
    }
}

bool WidgetLayout::presetExists(int num)
{
    return getPresetNums().contains(num);
}

QColor WidgetLayout::getColorFromElement(QDomElement elem)
{
    QDomElement er = elem.firstChildElement("r");
    QDomElement eg = elem.firstChildElement("g");
    QDomElement eb = elem.firstChildElement("b");
    return QColor(er.firstChild().nodeValue().toInt(),
                  eg.firstChild().nodeValue().toInt(),
                  eb.firstChild().nodeValue().toInt());
}

void WidgetLayout::copy()
{
    qDebug() << "WidgetLayout::copy()";
    QString text;
    if (m_editMode) {
        widgetsMutex.lock();
        if (m_xmlFormat) {
            for (int i = 0; i < editWidgets.size() ; i++) {
                if (editWidgets[i]->isSelected()) {
                    text += m_widgets[i]->getWidgetXmlText()+ "\n";
                }
            }
        }
        else {
            for (int i = 0; i < editWidgets.size() ; i++) {
                if (editWidgets[i]->isSelected()) {
                    text += m_widgets[i]->getWidgetLine() + "\n";
                }
            }
        }
        widgetsMutex.unlock();
        QClipboard *clipboard = qApp->clipboard();
        clipboard->setText(text);
    }
}

void WidgetLayout::cut()
{
    qDebug() << "WidgetLayout::cut()";
    if (m_editMode) {
        WidgetLayout::copy();
        widgetsMutex.lock();
        for (int i = editWidgets.size() - 1; i >= 0 ; i--) {
            if (editWidgets[i]->isSelected()) {
                widgetsMutex.unlock();
                deleteWidget(m_widgets[i]);
                widgetsMutex.lock();
            }
        }
        widgetsMutex.unlock();
        markHistory();
    }
}

void WidgetLayout::paste()
{
    qDebug() << "WidgetLayout::paste()";
    if (m_editMode) {
        deselectAll();
        QClipboard *clipboard = qApp->clipboard();
        QString clipboardText = clipboard->text();
        if (m_xmlFormat) {
            QDomDocument doc;
            QString errorText;
            int lineNumber, column;
            if (!doc.setContent("<doc>" + clipboardText + "</doc>",
                                &errorText, &lineNumber, &column)) {
                qDebug() <<"WidgetLayout::paste Parsing Error: " << errorText <<  lineNumber
                       << column << clipboardText;
            }
            QDomElement docElement = doc.firstChildElement("doc");
            QDomElement pl = docElement.firstChildElement("bsbObject");
            while (!pl.isNull()) {
                newXmlWidget(pl);
                editWidgets.last()->select();
                pl = pl.nextSiblingElement("bsbObject");
            }
        }
        else {
            QStringList lines = clipboardText.split("\n");
            foreach (QString line, lines) {
                newMacWidget(line);
                editWidgets.last()->select();
            }
        }
    }
    markHistory();
}

void WidgetLayout::setEditEnabled(bool enabled)
{
    setEditMode(enabled);
    m_enableEdit = enabled;
}

void WidgetLayout::setEditMode(bool active)
{
    if (!m_enableEdit) {
        return;
    }
    if (active) {
        widgetsMutex.lock();
        foreach (FrameWidget *widget, editWidgets) {
            delete widget;
        }
        editWidgets.clear();
        foreach (QuteWidget * widget, m_widgets) {
            createEditFrame(widget);
            setWidgetToolTip(widget,m_tooltips);
        }
        widgetsMutex.unlock();
    }
    else {
        foreach (QFrame* frame, editWidgets) {
            delete(frame);
        }
        editWidgets.clear();
    }
    m_editMode = active;
}

void WidgetLayout::createEditFrame(QuteWidget* widget)
{
    // This function should be locked from outside always
    FrameWidget * frame = new FrameWidget(this);
    QPalette palette = QPalette(QColor(Qt::red),QColor(Qt::red));
    palette.setColor(QPalette::WindowText, QColor(Qt::red));
    frame->setWidget(widget);
    frame->setPalette(palette);
    frame->setFocusProxy(this);
    frame->setGeometry(widget->x(), widget->y(), widget->width(), widget->height());
    frame->setFrameShape(QFrame::Box);
    //       frame->setMouseTracking(false);  //Only track mouse when buttons are pressed
    frame->show();
    editWidgets.append(frame);
    connect(frame, SIGNAL(popUpMenu(QPoint)), widget, SLOT(popUpMenu(QPoint)));
    connect(frame, SIGNAL(deselectAllSignal()), this, SLOT(deselectAll()));
    connect(frame, SIGNAL(moved( QPair<int, int> )),
            this, SLOT(widgetMoved( QPair<int, int> )));
    connect(frame, SIGNAL(resized( QPair<int, int> )),
            this, SLOT(widgetResized( QPair<int, int> )));
    connect(frame, SIGNAL(mouseReleased()), this, SLOT(markHistory()));
    connect(frame, SIGNAL(editWidget()), widget, SLOT(openProperties()));
    connect(frame, SIGNAL(widgetSelected(QuteWidget*)),
            this, SLOT(widgetSelected(QuteWidget*)));
    connect(frame, SIGNAL(widgetUnselected(QuteWidget*)),
            this, SLOT(widgetUnselected(QuteWidget*)));

}

void WidgetLayout::markHistory()
{
    QString text = m_xmlFormat ? getWidgetsText() : getMacWidgetsText();
    if (m_history.isEmpty()) {
        m_history << "";
        m_historyIndex = 0;
    }
    if (m_history[m_historyIndex] != text) {
        if (! m_history[m_historyIndex].isEmpty())
            m_historyIndex++;
        if (m_historyIndex >= CSQT_MAX_UNDO) {
            m_history.pop_front();
            (m_historyIndex)--;
        }
        if (m_history.size() != m_historyIndex + 1)
            m_history.resize(m_historyIndex + 1);
        m_history[m_historyIndex] = text;
    }
}

void WidgetLayout::deleteWidget(QuteWidget *widget)
{
    widgetsMutex.lock();
    int index = m_widgets.indexOf(widget);
    m_activeWidgets = index;  // Allow all widgets before this one to be active
    widget->close();
    m_widgets.remove(index);
    if (!editWidgets.isEmpty()) {
        delete(editWidgets[index]);
        editWidgets.remove(index);
    }
    index = consoleWidgets.indexOf(dynamic_cast<QuteConsole *>(widget));
    if (index >= 0) {
        consoleWidgets.remove(index);
    }
    index = graphWidgets.indexOf(dynamic_cast<QuteGraph *>(widget));
    if (index >= 0)
        graphWidgets.remove(index);
    index = scopeWidgets.indexOf(dynamic_cast<QuteScope *>(widget));
    if (index >= 0)
        scopeWidgets.remove(index);
    m_activeWidgets = m_widgets.size();  // Allow all widgets again
    widgetsMutex.unlock();
    widgetChanged(widget);
}

void WidgetLayout::newValue(QPair<QString, double> channelValue)
{
    if (channelValue.first == "_SetPreset") {
        loadPreset((int)channelValue.second);
    }
    if (channelValue.first == "_SetPresetIndex") {
        loadPresetFromIndex((int)channelValue.second);
    }
    QString path, channelName = channelValue.first;
    auto idx = channelName.indexOf("/");
    if (idx >= 0) {
        path = channelName.mid(idx+1);
        channelName = channelName.left(idx);
    }
    widgetsMutex.lock();
    if (!channelName.isEmpty()) {
        // Pass the value on to the other widgets
        for (int i = 0; i < m_widgets.size(); i++){
            if (m_widgets[i]->getChannelName() == channelName) {
                if (path.isEmpty()) {
                    m_widgets[i]->setValue(channelValue.second);
                }
                else
                    m_widgets[i]->widgetMessage(path,channelValue.second);
            }
            if (m_widgets[i]->getChannel2Name() == channelValue.first) {
                m_widgets[i]->setValue2(channelValue.second);
            }
        }
    }
    widgetsMutex.unlock();
    // Now store the value in the changes buffer to read from chnget
    if (!channelValue.first.isEmpty()) {
        valueMutex.lock();
        if(newValues.contains(channelValue.first))
            newValues[channelValue.first] = channelValue.second;
        else
            newValues.insert(channelValue.first, channelValue.second);
        valueMutex.unlock();
    }
}

//FIXME there's no need to go through here coming from the widgets...
// at least not to set the widget's value...
void WidgetLayout::newValue(QPair<QString, QString> channelValue)
{
    QString channelName = channelValue.first;
    if (channelValue.first.contains("/")) {
        channelName = channelValue.first.left(channelValue.first.indexOf("/"));
    }
    QString path = channelValue.first.mid(channelValue.first.indexOf("/") + 1);
    // Send value to a widget if channel matches
    widgetsMutex.lock();
    if (!channelName.isEmpty()) {
        for (int i = 0; i < m_widgets.size(); i++){
            if (m_widgets[i]->getChannelName() != channelName)
                continue;
            if (path == channelName)
                m_widgets[i]->setValue(channelValue.second);
            else
                m_widgets[i]->widgetMessage(path,channelValue.second);
        }
    }
    widgetsMutex.unlock();
    // Now store the value in the changes buffer to read from chnget
    if (!channelValue.first.isEmpty()) {
        stringValueMutex.lock();
        if(newStringValues.contains(channelValue.first))
            newStringValues[channelValue.first] = channelValue.second;
        else
            newStringValues.insert(channelValue.first, channelValue.second);
        stringValueMutex.unlock();
    }
}

void WidgetLayout::processNewValues()
{
    // Apply values received
    //   qDebug("WidgetPanel::processNewValues");

    //  if (closing != 0)
    //    return;
    //  QList<QString> channelNames;
    //  valueMutex.lock();
    //  channelNames = newValues.keys();
    //  valueMutex.unlock();
    //  widgetsMutex.lock();
    //  foreach(QString name, channelNames) {
    //    for (int i = 0; i < m_widgets.size(); i++){
    //      if (m_widgets[i]->getChannelName() == name) {
    //        m_widgets[i]->setValue(newValues.value(name));
    //      }
    //      if (m_widgets[i]->getChannel2Name() == name) {
    //        m_widgets[i]->setValue2(newValues.value(name));
    //      }
    //      if (m_trackMouse) {
    //        QString ch1name = m_widgets[i]->getChannelName();
    //        if (ch1name == "_MouseX") {
    //          m_widgets[i]->setValue(getMouseX());
    //        }
    //        else if (ch1name == "_MouseY") {
    //          m_widgets[i]->setValue(getMouseY());
    //        }
    //        else if (ch1name == "_MouseRelX") {
    //          m_widgets[i]->setValue(getMouseRelX());
    //        }
    //        else if (ch1name == "_MouseRelY") {
    //          m_widgets[i]->setValue(getMouseRelY());
    //        }
    //        else if (ch1name == "_MouseBut1") {
    //          m_widgets[i]->setValue(getMouseBut1());
    //        }
    //        else if (ch1name == "_MouseBut2") {
    //          m_widgets[i]->setValue(getMouseBut2());
    //        }
    //        QString ch2name = m_widgets[i]->getChannel2Name();
    //        if (ch2name == "_MouseX") {
    //          m_widgets[i]->setValue2(getMouseX());
    //        }
    //        else if (ch2name == "_MouseY") {
    //          m_widgets[i]->setValue2(getMouseY());
    //        }
    //        else if (ch2name == "_MouseRelX") {
    //          m_widgets[i]->setValue2(getMouseRelX());
    //        }
    //        else if (ch2name == "_MouseRelY") {
    //          m_widgets[i]->setValue2(getMouseRelY());
    //        }
    //        else if (ch2name == "_MouseBut1") {
    //          m_widgets[i]->setValue2(getMouseBut1());
    //        }
    //        else if (ch2name == "_MouseBut2") {
    //          m_widgets[i]->setValue2(getMouseBut2());
    //        }
    //      }
    //    }
    //  }
    //  widgetsMutex.unlock();
    //  valueMutex.lock();
    //  newValues.clear();
    //  valueMutex.unlock();
    //  stringValueMutex.lock();
    //  channelNames = newStringValues.keys();
    //  // Now set string values
    //  stringValueMutex.unlock();
    //  widgetsMutex.lock();
    //  foreach(QString name, channelNames) {
    //    for (int i = 0; i < m_widgets.size(); i++){
    //      if (m_widgets[i]->getChannelName() == name) {
    //        m_widgets[i]->setValue(newStringValues.value(name));
    //      }
    //    }
    //  }
    //  widgetsMutex.unlock();
    //  stringValueMutex.lock();
    //  newStringValues.clear();
    //  stringValueMutex.unlock();
}

void WidgetLayout::queueEvent(QString eventLine)
{
    emit queueEventSignal(eventLine);
}

void WidgetLayout::duplicate()
{
    if(!m_editMode)
        return;
    widgetsMutex.lock();
    QList<int> selectedWidgets;
    for (int i = 0; i < editWidgets.size() ; i++) {
        if (editWidgets[i]->isSelected()) {
            selectedWidgets << i;
        }
    }
    for (int i = 0; i < selectedWidgets.size() ; i++) {
        int index = selectedWidgets[i];
        editWidgets[index]->deselect();
        if (m_xmlFormat) {
            QDomDocument doc;
            doc.setContent(m_widgets[index]->getWidgetXmlText());
            widgetsMutex.unlock();
            newXmlWidget(doc.firstChildElement("bsbObject"), true, true);
            widgetsMutex.lock();
        }
        else {
            widgetsMutex.unlock();
            newMacWidget(m_widgets[index]->getWidgetLine(), true);
            widgetsMutex.lock();
        }
        // editWidgets.last()->select();
    }
    widgetsMutex.unlock();
    markHistory();
    setModified(true);
}

void WidgetLayout::deleteSelected()
{
    widgetsMutex.lock();
    for (int i = editWidgets.size() - 1; i >= 0 ; i--) {
        if (!editWidgets[i]->isSelected())
            continue;
        widgetsMutex.unlock();
        deleteWidget(m_widgets[i]);
        widgetsMutex.lock();
    }
    widgetsMutex.unlock();
    markHistory();
    setModified(true);
}

void WidgetLayout::moveSelected(int horiz, int vert, int grid) {
    widgetsMutex.lock();
    for (int i = editWidgets.size() - 1; i >= 0 ; i--) {
        if(!editWidgets[i]->isSelected())
            continue;
        widgetsMutex.unlock();
        QPoint pos = m_widgets[i]->pos();
        int x = pos.x() + horiz;
        int y = pos.y() + vert;
        if(grid > 1) {
            double xq = round(x / (double)grid) * grid;
            x = (int)xq;
            double yq = round(y / (double)grid) * grid;
            y = (int)yq;

        }
        m_widgets[i]->move(x, y);
        editWidgets[i]->move(x, y);
        widgetsMutex.lock();
    }
    widgetsMutex.unlock();
    markHistory();
    setModified(true);
}

void WidgetLayout::undo()
{
    if(m_historyIndex <= 0)
        return;
    m_historyIndex--;
    if (m_xmlFormat)
        loadXmlWidgets(m_history[m_historyIndex]);
    else
        loadMacWidgets(m_history[m_historyIndex]);
}

void WidgetLayout::reloadWidgets() {
    if(m_historyIndex <= 0)
        return;
    loadXmlWidgets(m_history[m_historyIndex]);
}

void WidgetLayout::redo()
{
    if (m_historyIndex >= m_history.size() - 1)
        return;
    m_historyIndex++;
    if (m_xmlFormat)
        loadXmlWidgets(m_history[m_historyIndex]);
    else
        loadMacWidgets(m_history[m_historyIndex]);
}


void WidgetLayout::updateData()
{
    if (closing == 1) {
        closing = 0;
        return;
    }

    if(!m_updating)
        return;

    refreshWidgets();
    int const refresh_rate = m_updateRate;
    int const msec = 1000 / refresh_rate;
    if (!layoutMutex.tryLock(1)) {
        updateTimer.singleShot(msec, this, SLOT(updateData()));
        return;
    }
    while (!newCurveBuffer.isEmpty()) {
        Curve * curve = newCurveBuffer.takeFirst();
        newCurve(curve);  // Register new curve
    }
    // Check for graph updates after creating new curves
    while (curveUpdateBufferCount > 0) {
        WINDAT * curveData = &curveUpdateBuffer[curveUpdateBufferCount--];
        Curve *curve = (Curve *) getCurveById(curveData->windid);
        if (curve != nullptr && curveData != nullptr) {
            curve->set_size(curveData->npts);    // number of points
            curve->set_data(curveData->fdata);
            curve->set_caption(curveData->caption);
            curve->set_max(curveData->max);
            curve->set_min(curveData->min);
            curve->set_absmax(curveData->absmax);
            // Y axis scaling factor
            // curve->set_y_scale(windat->y_scale);
            setCurveData(curve);
        }
        // delete curveData; //FIXME don't do this deleting here...
    }
    for (int i = 0; i < scopeWidgets.size(); i++) {
        scopeWidgets[i]->updateData();
    }
    layoutMutex.unlock();
    closing = 0;
    updateTimer.singleShot(msec, this, SLOT(updateData()));
}

void WidgetLayout::widgetSelected(QuteWidget *widget)
{
    emit widgetSelectedSignal(widget);
}

void WidgetLayout::widgetUnselected(QuteWidget *widget)
{
    emit widgetUnselectedSignal(widget);
}
