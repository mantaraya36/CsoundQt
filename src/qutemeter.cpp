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

#include "qutemeter.h"

#include <math.h> //for isnan

// This class is called meter for historical reasons (MacCsound's widget was called meter)
// But it is a meter and a controller widget

QuteMeter::QuteMeter(QWidget *parent) : QuteWidget(parent)
{
    setGeometry(0,0, parent->width(), parent->height());
    m_widget = new MeterWidget(this);
    m_widget->setAutoFillBackground(true);
    //  m_widget->setWindowFlags(Qt::WindowStaysOnTopHint);
    canFocus(false);
    //   static_cast<MeterWidget *>(m_widget)->setRenderHints(QPainter::Antialiasing);
    //  connect(static_cast<MeterWidget *>(m_widget), SIGNAL(popUpMenu(QPoint)), this, SLOT(popUpMenu(QPoint)));
    //  connect(static_cast<MeterWidget *>(m_widget), SIGNAL(newValues(double, double)), this, SLOT(setValuesFromWidget(double,double)));

    setProperty("CSQT_xMin", 0.0);
    setProperty("CSQT_xMax", 1.0);
    setProperty("CSQT_yMin", 0.0);
    setProperty("CSQT_yMax", 1.0);

    setProperty("CSQT_type", "fill");
    setProperty("CSQT_pointsize", 1);
    setProperty("CSQT_fadeSpeed", 1.0);
    setProperty("CSQT_mouseControl", "jump");
    setProperty("CSQT_mouseControlAct", "press");

    setProperty("CSQT_color", QColor(Qt::green));
    setProperty("CSQT_bgcolor", QColor(30, 30, 30));
    setProperty("CSQT_bgcolormode", true);
    setProperty("CSQT_randomizable", false);
    setProperty("CSQT_randomizableGroup", 0);
    setProperty("CSQT_randomizableMode", "both");

    setProperty("CSQT_bordermode", "noborder");
    setProperty("CSQT_borderColor", "#00FF00");

    connect(static_cast<MeterWidget *>(m_widget), SIGNAL(newValue1(double)),
            this, SLOT(valueChanged(double)));
    connect(static_cast<MeterWidget *>(m_widget), SIGNAL(newValue2(double)),
            this, SLOT(value2Changed(double)));
    m_widget->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels

}

QuteMeter::~QuteMeter()
{
}


void QuteMeter::setMidiValue(int value)
{
    double max, min;
    bool vertical;
    if (m_widget->width() > m_widget->height()) {
        vertical = false;
    }
    else {
        vertical = true;
    }
    if (vertical) {
        max = property("CSQT_yMax").toDouble();
        min = property("CSQT_yMin").toDouble();
    }
    else {
        max = property("CSQT_xMax").toDouble();
        min = property("CSQT_xMin").toDouble();
    }
    double newval = min + ((value / 127.0)* (max - min));
    setValue(newval);
    QPair<QString, double> channelValue(m_channel, newval);
    emit newValue(channelValue);
}

QString QuteMeter::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForRead();
#endif
    QString line = "ioMeter {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
    line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
    QColor color = property("CSQT_color").value<QColor>();
    line += "{" + QString::number(color.red() * 256)
            + ", " + QString::number(color.green() * 256)
            + ", " + QString::number(color.blue() * 256) + "} ";
    line += "\"" + m_channel + "\" " + QString::number(m_value, 'f', 6) + " ";
    line += "\"" + m_channel2 + "\" " + QString::number(m_value2, 'f', 6) + " ";
    line += property("CSQT_type").toString() + " ";
    line += QString::number(property("CSQT_pointsize").toInt()) + " ";
    line += QString::number(property("CSQT_fadeSpeed").toInt()) + " ";
    line += "mouse";
    //   qDebug("QuteMeter::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    return line;
}

QString QuteMeter::getWidgetXmlText()
{
    xmlText = "";
    QXmlStreamWriter s(&xmlText);
    createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForRead();
#endif
    s.writeTextElement("objectName2", m_channel2);
    s.writeTextElement("xMin", QString::number(property("CSQT_xMin").toDouble(), 'f', 8));
    s.writeTextElement("xMax", QString::number(property("CSQT_xMax").toDouble(), 'f', 8));
    s.writeTextElement("yMin", QString::number(property("CSQT_yMin").toDouble(), 'f', 8));
    s.writeTextElement("yMax", QString::number(property("CSQT_yMax").toDouble(), 'f', 8));
    s.writeTextElement("xValue", QString::number(m_value, 'f', 8));
    s.writeTextElement("yValue", QString::number(m_value2, 'f', 8));

    s.writeTextElement("type", property("CSQT_type").toString());
    s.writeTextElement("pointsize", QString::number(property("CSQT_pointsize").toInt()));
    s.writeTextElement("fadeSpeed",QString::number(property("CSQT_fadeSpeed").toDouble(),'f',8));
    s.writeStartElement("mouseControl");
    s.writeAttribute("act", property("CSQT_mouseControlAct").toString());
    s.writeCharacters(property("CSQT_mouseControl").toString());
    s.writeEndElement();

    s.writeTextElement("bordermode", property("CSQT_bordermode").toString());
    s.writeTextElement("borderColor", property("CSQT_borderColor").toString());

    QColor color = property("CSQT_color").value<QColor>();
    s.writeStartElement("color");
    s.writeTextElement("r", QString::number(color.red()));
    s.writeTextElement("g", QString::number(color.green()));
    s.writeTextElement("b", QString::number(color.blue()));
    s.writeEndElement();
    s.writeStartElement("randomizable");
    s.writeAttribute("mode", property("CSQT_randomizableMode").toString());
    s.writeAttribute("group", QString::number(property("CSQT_randomizableGroup").toInt()));
    s.writeCharacters(property("CSQT_randomizable").toBool() ? "true":"false");
    s.writeEndElement();

    QColor bgcolor = property("CSQT_bgcolor").value<QColor>();
    s.writeStartElement("bgcolor");
    s.writeTextElement("r", QString::number(bgcolor.red()));
    s.writeTextElement("g", QString::number(bgcolor.green()));
    s.writeTextElement("b", QString::number(bgcolor.blue()));
    s.writeEndElement();

    s.writeTextElement("bgcolormode", property("CSQT_bgcolormode").toBool()?"true":"false");

    s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    return xmlText;
}

QString QuteMeter::getWidgetType()
{
    // QString type = static_cast<MeterWidget *>(m_widget)->getType();
    auto type = static_cast<MeterWidget*>(m_widget)->getMeterType();
    if (type == MeterWidgetType::Fill ||
            type == MeterWidgetType::Line ||
            type == MeterWidgetType::Llif) {
        return QString("BSBController");
    }
    //  else if (type == "crosshair" or type == "point") {
    //  }
    return QString("BSBController");
}

QString QuteMeter::getCabbageLine()
{
    // QString type = static_cast<MeterWidget *>(m_widget)->getType();
    auto type = static_cast<MeterWidget*>(m_widget)->getMeterType();
    if (!(type == MeterWidgetType::Crosshair || type == MeterWidgetType::Point)) {
		qDebug()<<"Meter can be converted to XYpad only if the type is crosshair or point";
		return QString();
	}
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif
    QString line = "xypad ";
    line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
    line += QString("channel(\"%1\",\"%2\"), ").arg(m_channel).arg(m_channel2);
    line += QString("rangex(%1, %2, %3), ").arg(property("CSQT_xMin").toDouble()).arg(property("CSQT_xMax").toDouble()).arg(m_value);
    line += QString("rangey(%1, %2, %3)").arg(property("CSQT_yMin").toDouble()).arg(property("CSQT_yMax").toDouble()).arg(m_value2);

    //NB! meter does not have midi2 yet!
    if (property("CSQT_midicc").toInt() >= 0 && property("CSQT_midichan").toInt()>0) { // insert only if midi channel is above 0
        line += ", midiCtrl(\"" + QString::number(property("CSQT_midichan").toInt()) + ",";
        line +=  QString::number(property("CSQT_midicc").toInt()) + "\")";
    }
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    return line;
}

QString QuteMeter::getQml()
{
    QString qml = QString();
    QString type = property("CSQT_type").toString();
    if ( type != "fill") {
        qDebug() << "Currently exporting only meter controllers (type \"fill\") is supported";
        return qml;
    }
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif
    // todo: crosshair jm

    bool vertical;
    if (m_widget->width() > m_widget->height()) {
        vertical = false;
    } else {
        vertical = true;
    }
    double max, min, value;
    QString channel;
    if (vertical) {
        max = property("CSQT_yMax").toDouble();
        min = property("CSQT_yMin").toDouble();
        channel = m_channel2;
        value = m_value2;
    }
    else {
        max = property("CSQT_xMax").toDouble();
        min = property("CSQT_xMin").toDouble();
        channel = m_channel;
        value = m_value;
    }

    qml=QString(R"(
                Rectangle {
                    x: %1 * scaleItem.scale
                    y: %2 * scaleItem.scale
                    width: %3 * scaleItem.scale
                    height: %4 * scaleItem.scale
                    color: "black"
                    border.color: "black"
                    border.width: 2
                    property double min: %5
                    property double max: %6
                    property double value: %7
                    property string channel: "%8"
                    property bool isVertical: height>width

                    MouseArea {
                        anchors.fill: parent

                        function setValue() { // value from min..max
                            var value;
                            if (parent.isVertical) {
                                value = parent.min +  (1 - (mouseY/parent.height)) * (parent.max-parent.min);
                            } else {
                                value = parent.min +  (mouseX/parent.width) * (parent.max-parent.min);
                            }

                            if (value>parent.max) value=parent.max;
                            if (value<parent.min) value=parent.min;
                            if (typeof(csound) !== 'undefined' ) csound.setControlChannel(parent.channel, value);

                            parent.value = value;
                        }

                        onClicked: setValue();
                        onMouseYChanged: setValue();
                    }

                    Rectangle {
                        property double relativeValue: (parent.value-parent.min)/(parent.max-parent.min)
                        width: parent.isVertical ? parent.width - 2*parent.border.width :
                                                  (parent.width-2*parent.border.width) *relativeValue
                        height: parent.isVertical ? (parent.height-2*parent.border.width) *relativeValue :
                                                    parent.height - 2*parent.border.width

                        anchors.margins: parent.border.width
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        color: "%9"
                    }
                }
                )").arg(x()).arg(y()).arg(width()).arg(height()).arg(min).arg(max).arg(value).
            arg(channel).arg(property("CSQT_color").toString());


#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif

    return qml;

}

void QuteMeter::createPropertiesDialog()
{
    QuteWidget::createPropertiesDialog();

#ifdef USE_WIDGET_MUTEX
    widgetLock.lockForRead();
#endif
    dialog->setWindowTitle("Controller");
    channelLabel->setText(tr("Horizontal Channel ="));
    channelLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

    QLabel *label = new QLabel(tr("Vertical Channel ="), dialog);
    layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);

    name2LineEdit = new QLineEdit(dialog);
    name2LineEdit->setText(getChannel2Name());
    // name2LineEdit->setMinimumWidth(320);
    layout->addWidget(name2LineEdit, 4, 1, 1, 3, Qt::AlignLeft|Qt::AlignVCenter);
    //  if (static_cast<MeterWidget *>(m_widget)->getType() != "point" and ((MeterWidget *)m_widget)->getType() != "crosshair") {
    //    if (((MeterWidget *)m_widget)->m_vertical) {
    //      channelLabel->setEnabled(false);
    //      nameLineEdit->setEnabled(false);
    //    }
    //    else {
    //      label->setEnabled(false);
    //      name2LineEdit->setEnabled(false);
    //    }
    //  }
    auto w = static_cast<MeterWidget *>(m_widget);
    label = new QLabel("Color", dialog);
    label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(label, 5, 0, Qt::AlignRight|Qt::AlignVCenter);

    colorButton = new SelectColorButton(dialog);
    colorButton->setColor(static_cast<MeterWidget *>(m_widget)->getColor());
    layout->addWidget(colorButton, 5,1, Qt::AlignLeft|Qt::AlignVCenter);

    bgColorCheckBox = new QCheckBox("Background", dialog);
    bgColorCheckBox->setChecked(property("CSQT_bgcolormode").toBool());
    layout->addWidget(bgColorCheckBox, 5, 2, Qt::AlignVCenter);

    bgColorButton = new SelectColorButton(dialog);
    bgColorButton->setColor(static_cast<MeterWidget *>(m_widget)->getBgColor());
    layout->addWidget(bgColorButton, 5, 3, Qt::AlignLeft|Qt::AlignVCenter);

    connect(bgColorCheckBox, SIGNAL(toggled(bool)), bgColorButton, SLOT(setEnabled(bool)));

    borderCheckBox = new QCheckBox(tr("Border"), dialog);
    borderCheckBox->setCheckState(
        property("CSQT_bordermode").toString()=="border" ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(borderCheckBox, 5, 4, Qt::AlignLeft|Qt::AlignVCenter);

    borderColorButton = new SelectColorButton(dialog);
    borderColorButton->setColor(QColor(property("CSQT_borderColor").toString()));
    layout->addWidget(borderColorButton, 5, 5, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText("Type");
    label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(label, 6, 0, Qt::AlignRight|Qt::AlignVCenter);

    typeComboBox = new QComboBox(dialog);
    typeComboBox->addItem("fill");
    typeComboBox->addItem("llif");
    typeComboBox->addItem("line");
    typeComboBox->addItem("crosshair");
    typeComboBox->addItem("point");
    typeComboBox->setCurrentIndex(typeComboBox->findText(w->getType()));
    layout->addWidget(typeComboBox, 6, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText("Point/Line Width:");
    label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(label, 7, 0, Qt::AlignRight|Qt::AlignVCenter);

    pointSizeSpinBox = new QSpinBox(dialog);
    pointSizeSpinBox->unsetLocale();
    pointSizeSpinBox->setValue(w->getPointSize());
    pointSizeSpinBox->setToolTip("Size of the point / line if applicable");
    layout->addWidget(pointSizeSpinBox, 7,1, Qt::AlignLeft|Qt::AlignVCenter);

    if(w->getMeterType() != MeterWidgetType::Point &&
            w->getMeterType() != MeterWidgetType::Line) {
        // label->setEnabled(false);
        pointSizeSpinBox->setEnabled(false);
    }

    connect(typeComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(checkTypeComboBox()));

    label = new QLabel(dialog);
    label->setText("Fade speed:");
    label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(label, 7, 2, Qt::AlignRight|Qt::AlignVCenter);
    fadeSpeedSpinBox = new QSpinBox(dialog);
    fadeSpeedSpinBox->unsetLocale();
    fadeSpeedSpinBox->setValue(property("CSQT_fadeSpeed").toDouble());
    fadeSpeedSpinBox->setRange(0, 1000);
    layout->addWidget(fadeSpeedSpinBox, 7,3, Qt::AlignLeft|Qt::AlignVCenter);
    label->setEnabled(false);
    fadeSpeedSpinBox->setEnabled(false);

    label = new QLabel(dialog);
    label->setText("Behavior");
    label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    label->setEnabled(false);
    layout->addWidget(label, 8, 0, Qt::AlignRight|Qt::AlignVCenter);

    behaviorComboBox = new QComboBox(dialog);
    behaviorComboBox->addItem("jump");
    behaviorComboBox->addItem("relative");
    behaviorComboBox->addItem("none");
    behaviorComboBox->setCurrentIndex(
                behaviorComboBox->findText(property("CSQT_mouseControl").toString()));
    behaviorComboBox->setEnabled(false);
    layout->addWidget(behaviorComboBox,8, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText("Min X value =");
    layout->addWidget(label,9, 0, Qt::AlignRight|Qt::AlignVCenter);

    m_xMinBox = new QDoubleSpinBox(dialog);
    m_xMinBox->setRange(-999999999.0, 999999999.0);
    m_xMinBox->setValue(property("CSQT_xMin").toDouble());
    layout->addWidget(m_xMinBox, 9,1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText("Max X value =");
    layout->addWidget(label, 9, 2, Qt::AlignRight|Qt::AlignVCenter);

    m_xMaxBox = new QDoubleSpinBox(dialog);
    m_xMaxBox->setRange(-999999999.0, 999999999.0);
    m_xMaxBox->setValue(property("CSQT_xMax").toDouble());
    layout->addWidget(m_xMaxBox, 9,3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText("Min Y value =");
    layout->addWidget(label, 10, 0, Qt::AlignRight|Qt::AlignVCenter);

    m_yMinBox = new QDoubleSpinBox(dialog);
    m_yMinBox->setRange(-999999999.0, 999999999.0);
    m_yMinBox->setValue(property("CSQT_yMin").toDouble());
    layout->addWidget(m_yMinBox, 10, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText("Max Y value =");
    layout->addWidget(label, 10, 2, Qt::AlignRight|Qt::AlignVCenter);

    m_yMaxBox = new QDoubleSpinBox(dialog);
    m_yMaxBox->setRange(-999999999.0, 999999999.0);
    m_yMaxBox->setValue(property("CSQT_yMax").toDouble());
    layout->addWidget(m_yMaxBox, 10,3, Qt::AlignLeft|Qt::AlignVCenter);

    setProperty("CSQT_xValue", m_value);
    setProperty("CSQT_yValue", m_value2);
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif

}

void QuteMeter::checkTypeComboBox() {
    qDebug() << "checkTypeComboBox";
    QString metertype = typeComboBox->currentText();
    if(metertype == "line" || metertype == "point" )
        pointSizeSpinBox->setEnabled(true);
    else {
        pointSizeSpinBox->setEnabled(false);
    }
}

void QuteMeter::refreshWidget()
{
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForRead();
#endif
    double val1 = m_value;
    double val2 = m_value2;
    if (val1 < property("CSQT_xMin").toDouble()) { // Must check this in case number is -inf
        val1 =  property("CSQT_xMin").toDouble();
    }
    if (val2 < property("CSQT_yMin").toDouble()) {
        val2 =  property("CSQT_yMin").toDouble();
    }
    m_widget->blockSignals(true);
    static_cast<MeterWidget *>(m_widget)->setValues(val1, val2);
    m_widget->blockSignals(false);
    m_valueChanged = false;
    m_value2Changed = false;
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
}

void QuteMeter::applyProperties()
{
    //  setProperty("CSQT_objectName", nameLineEdit->text());
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForRead();
#endif
    // Only properties should be changed here as applyInternalProperties takes care of the rest
    setProperty("CSQT_objectName2", name2LineEdit->text());
    // setProperty("CSQT_color", colorButton->palette().color(QPalette::Window));
    setProperty("CSQT_color", colorButton->getColor());
    // setProperty("CSQT_bgcolor", bgColorButton->palette().color(QPalette::Window));
    setProperty("CSQT_bgcolor", bgColorButton->getColor());
    setProperty("CSQT_bgcolormode", bgColorCheckBox->isChecked());
    setProperty("CSQT_type", typeComboBox->currentText());
    setProperty("CSQT_pointsize", pointSizeSpinBox->value());
    setProperty("CSQT_fadeSpeed", fadeSpeedSpinBox->value());
    setProperty("CSQT_mouseControl", behaviorComboBox->currentText());
    setProperty("CSQT_xMin", m_xMinBox->value());
    setProperty("CSQT_xMax", m_xMaxBox->value());
    setProperty("CSQT_yMin", m_yMinBox->value());
    setProperty("CSQT_yMax", m_yMaxBox->value());
    setProperty("CSQT_bordermode", borderCheckBox->checkState()?"border":"noborder");
    setProperty("CSQT_borderColor", borderColorButton->getColor().name());
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    QuteWidget::applyProperties();  //Must be last to make sure the widgetChanged signal is last
}

void QuteMeter::setWidgetGeometry(int x,int y,int width,int height)
{
    QuteWidget::setWidgetGeometry(x,y,width, height);
    /* In MacCsound, meter widgets have an offset of about five pixels in every border
     This has proven problematic in Qt, as the graphicsScene*/
    m_widget->blockSignals(true);
    static_cast<MeterWidget *>(m_widget)->setWidgetGeometry(0,0,width, height);
    m_widget->blockSignals(false);
}

void QuteMeter::applyInternalProperties()
{
    QuteWidget::applyInternalProperties();
    auto meter = static_cast<MeterWidget *>(m_widget);
    meter->setColor(property("CSQT_color").value<QColor>());
    meter->setBgColor(property("CSQT_bgcolor").value<QColor>());
    meter->showBackground(property("CSQT_bgcolormode").toBool());
    meter->setType(property("CSQT_type").toString());
    meter->setPointSize(property("CSQT_pointsize").toInt());
    meter->setRanges(property("CSQT_xMin").toDouble(),
                     property("CSQT_xMax").toDouble(),
                     property("CSQT_yMin").toDouble(),
                     property("CSQT_yMax").toDouble());
    meter->showBorder(property("CSQT_bordermode").toString()=="border");
    meter->setBorderColor(QColor(property("CSQT_borderColor").toString()));

    m_value = property("CSQT_xValue").toDouble();
    m_value2 = property("CSQT_yValue").toDouble();
    //  if (m_value2 == m_value) {
    //    qDebug() << "Warning! Controller Widget can't have the same name for both channels!";
    //    m_value2 = "";
    //  }
    setValue(m_value);
    setValue2(m_value2);
}


void QuteMeter::valueChanged(double value1)
{
    //  QString type = property("CSQT_type").toString();
    //  qDebug() << "QuteMeter::valueChanged " << value1;

#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForRead();
#endif
    m_value = value1;
    m_valueChanged = true;
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    QPair<QString, double> channelValue(m_channel, m_value);
    emit newValue(channelValue);
}

void QuteMeter::value2Changed(double value2)
{
    //  QString type = property("CSQT_type").toString();
    //  qDebug() << "QuteMeter::value2Changed "<< value2;

#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForRead();
#endif
    m_value2 = value2;
    m_value2Changed = true;
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    QPair<QString, double> channelValue(m_channel2, m_value2);
    emit newValue(channelValue);
}

/* Meter Widget ----------------------------------------*/

MeterWidget::MeterWidget(QWidget *parent) : QGraphicsView(parent)
{
    //auto t0 = std::chrono::high_resolution_clock::now();
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setInteractive (true);
    // m_scene = new QGraphicsScene(5,5, 20, 20, this);
    m_scene = new QGraphicsScene(0, 0, 20, 20, this);
    m_scene->setBackgroundBrush(Qt::black);
    m_scene->setSceneRect(0, 0, 20, 20);
    m_showBackground = true;
    m_bgcolor = Qt::black;
    setFrameStyle(QFrame::NoFrame);

    setScene(m_scene);
    m_mouseDown = false;
    auto borderPen = QPen(QColor(Qt::green).darker(150), 2);
    auto blockPen = Qt::NoPen;
    m_block = m_scene->addRect(0, 0, 0, 0, blockPen, QBrush(Qt::green));
    m_point = m_scene->addEllipse(0, 0, 0, 0, QPen(Qt::green) , QBrush(Qt::green));
    m_vline = m_scene->addLine(0, 0, 0, 0, QPen(Qt::green));
    m_hline = m_scene->addLine(0, 0, 0, 0, QPen(Qt::green));
    m_border = m_scene->addRect(m_scene->sceneRect(), borderPen, Qt::NoBrush);
    m_border->hide();
    // auto t1 = std::chrono::high_resolution_clock::now();
    // QDEBUG << "... MeterWidget constructor " << std::chrono::duration<double, std::milli>(t1-t0).count() << "ms";

}

MeterWidget::~MeterWidget()
{
}

QColor MeterWidget::getColor()
{
    return m_block->brush().color();
}

QColor MeterWidget::getBgColor()
{
    return m_scene->backgroundBrush().color();
}

QColor MeterWidget::getBorderColor()
{
    return m_border->pen().color();
}


void MeterWidget::setValue(double value)
{
    //  qDebug() << "MeterWidget::setValue " <<value;
    //  if (isnan(value) != 0)
    //    return;
    //  if (value > m_xmax) {
    //    value = m_xmax;
    //  }
    //  if (value < m_xmin) {
    //    value = m_xmin;
    //  }
    if (m_value == value) {
        return;
    }
    mutex.lock();
    m_value = value;

    double portionx = (m_value -  m_xmin) / (m_xmax - m_xmin);
    double portiony = (m_value2 -  m_ymin) / (m_ymax - m_ymin);

    if (m_metertype == MeterWidgetType::Fill && !m_vertical) {
		m_block->setRect(0, 0, portionx*width(), height());
	}
    else if (m_metertype == MeterWidgetType::Llif && !m_vertical) {
        m_block->setRect(portionx*width(), 0, width(), height());
	}
    else if (m_metertype == MeterWidgetType::Line && !m_vertical) {
        m_vline->setLine(portionx*width(), 0, portionx*width(), height());
    }
    else {
        m_vline->setLine(portionx*width(), 0, portionx*width(), height());
        m_point->setRect(portionx*width() - m_pointSize/2.0,
                         height()*(1 - portiony) - m_pointSize/2.0,
                         m_pointSize,
                         m_pointSize);
    }
    mutex.unlock();
    //  emit valueChanged(m_value);
}

void MeterWidget::setValue2(double value)
{
	//  qDebug() << "MeterWidget::setValue2 " << value;
	//  if (value > m_ymax) {
	//    value = m_ymax;
	//  }
	//  if (value < m_ymin) {
	//    value = m_ymin;
	//  }
	if (m_value2 == value) {
		return;
	}
	mutex.lock();
	m_value2 = value;
	double portionx = (m_value -  m_xmin) / (m_xmax - m_xmin);
	double portiony = (m_value2 -  m_ymin) / (m_ymax - m_ymin);
    if (m_metertype == MeterWidgetType::Fill && m_vertical) {
		m_block->setRect(0, (1-portiony)*height(), width(), height());
	}
    else if (m_metertype == MeterWidgetType::Llif && m_vertical) {
		m_block->setRect(0, 0, width(), (1-portiony)*height());
	}
    else if (m_metertype == MeterWidgetType::Line && m_vertical) {
		m_hline->setLine(0, (1-portiony)*height(), width(), (1-portiony)*height());
		//    m_hline->setLine(portiony*width(), 0 ,portiony*width(), height());
	}
	else {
		m_hline->setLine(0, (1-portiony)*height(), width(), (1-portiony)*height());
        m_point->setRect(portionx*width() - m_pointSize/2.0,
                         (1-portiony)*height() - m_pointSize/2.0,
                         m_pointSize,
                         m_pointSize);
    }
    mutex.unlock();
    //  emit value2Changed(m_value2);
}

void MeterWidget::setValues(double value1, double value2)
{
    if (m_value2 == value2 && m_value == value1) {
        return;
    }
    mutex.lock();
    m_value = value1;
    m_value2 = value2;
    double portionx = (m_value - m_xmin) / (m_xmax - m_xmin);
    double portiony = (m_value2 - m_ymin) / (m_ymax - m_ymin);
    switch(m_metertype) {
    case MeterWidgetType::Fill:
        if (!m_vertical)
            m_block->setRect(0, 0, portionx*width(), height());
        else
            m_block->setRect(0, (1-portiony)*height(), width(), portiony * height());
        break;
    case MeterWidgetType::Llif:
        if (!m_vertical)
            m_block->setRect(portionx*width(),0, width(), height());
        else
            m_block->setRect(0, 0, width(), (1-portiony)*height());
        break;
    case MeterWidgetType::Line:
        if (!m_vertical)
            m_vline->setLine(portionx*width(), 0 ,portionx*width(), height());
        else
            m_hline->setLine(0, (1-portiony)*height(), width(), (1-portiony)*height());
        break;
    default:
        m_vline->setLine(portionx*width(), 0 ,portionx*width(), height());
        m_hline->setLine(0, (1-portiony)*height(), width(), (1-portiony)*height());
        m_point->setRect(portionx*width() - (m_pointSize/2.0),
                         (1-portiony)*height() - (m_pointSize/2.0),
                         m_pointSize,
                         m_pointSize);
    }
    mutex.unlock();

}

void MeterWidget::setType(QString type)
{
	//   qDebug() << "MeterWidget::setType " << type << m_vertical;
	if (type == "fill") {
		m_type = type;
        m_metertype = MeterWidgetType::Fill;
		m_block->show();
		m_point->hide();
		m_vline->hide();
		m_hline->hide();
        setRenderHint(QPainter::Antialiasing, false);
	}
	else if (type == "llif") {
        m_metertype = MeterWidgetType::Llif;
		m_type = type;
		m_block->show();
		m_point->hide();
		m_vline->hide();
		m_hline->hide();
	}
	else if (type == "line") {
        m_metertype = MeterWidgetType::Line;
		m_type = type;
		m_block->hide();
		m_point->hide();
		if (m_vertical) {
			m_vline->hide();
			m_hline->show();
		}
		else {
			m_vline->show();
			m_hline->hide();
		}
	}
	else if (type == "crosshair") {
        m_metertype = MeterWidgetType::Crosshair;
		m_type = type;
		m_block->hide();
		m_point->hide();
		m_vline->show();
		m_hline->show();
	}
	else if (type == "point") {
        m_metertype = MeterWidgetType::Point;
		m_type = type;
		m_block->hide();
		m_point->show();
		m_vline->hide();
		m_hline->hide();
        setRenderHints(QPainter::Antialiasing);
    }
}

void MeterWidget::setRanges(double xmin, double xmax, double ymin, double ymax)
{
    m_xmin = xmin;
    m_xmax = xmax;
    m_ymin = ymin;
    m_ymax = ymax;
}

void MeterWidget::setPointSize(int size)
{
    m_pointSize = size;
    m_point->setRect(m_value2*width()- (m_pointSize/2.0),
                     (1-m_value)*height()- (m_pointSize/2.0),
                     m_pointSize,
                     m_pointSize);
    auto pen = m_hline->pen();
    pen.setWidth(size);
    m_hline->setPen(pen);
    m_vline->setPen(pen);
}

void MeterWidget::setColor(QColor color)
{
    m_block->setBrush(QBrush(color));
    // auto pen = m_border->pen();
    // pen.setColor(color);
    // m_border->setPen(pen);
    m_point->setPen(QPen(Qt::NoPen));
    m_point->setBrush(QBrush(color));
    m_vline->setPen(QPen(color));
    m_hline->setPen(QPen(color));
}

void MeterWidget::setBgColor(QColor color) {
    m_scene->setBackgroundBrush(color);
    m_bgcolor = color;
}

void MeterWidget::setBorderColor(QColor color) {
    auto pen = m_border->pen();
    pen.setColor(color);
    m_border->setPen(pen);
}

void MeterWidget::showBackground(bool show) {
    if(!show) {
        m_scene->setBackgroundBrush(Qt::NoBrush);
        this->setStyleSheet("background: transparent");
    } else {
        // m_scene->setBackgroundBrush(m_bgcolor);
        this->setStyleSheet("background: " + m_bgcolor.name());
    }
    m_showBackground = show;
}

void MeterWidget::setWidgetGeometry(int x,int y,int width,int height)
{
    m_scene->setSceneRect(0,0, width, height);
    // m_border->setRect(0, 0, width-1, height-1);
    m_border->setRect(0, 0, width, height);
    setSceneRect(0,0, width, height);
    setGeometry(x,y,width, height);
    setMaximumSize(width, height);
    m_vertical = width <= height;
    setType(m_type);  // update widgets which depend on verticality
}

void MeterWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        //     if (event->x() > 0 and event->x()< width() and
        //         event->y() > 0 and event->y()< height())
        double newhor = m_xmin + (m_xmax - m_xmin) * (double)event->x()/width();
        double newvert = m_ymin + (m_ymax - m_ymin) * (1-((double)event->y()/height()));
        if (newhor > m_xmax) {
            newhor = m_xmax;
        }
        else if (newhor < m_xmin) {
            newhor = m_xmin;
        }
        if (newvert > m_ymax) {
            newvert = m_ymax;
        }
        else if (newvert < m_ymin) {
            newvert = m_ymin;
        }
        switch(m_metertype) {
        case MeterWidgetType::Fill:
        case MeterWidgetType::Line:
        case MeterWidgetType::Llif:
            if (m_vertical)
                emit newValue2(newvert);
            else
                emit newValue1(newhor);
            break;
        case MeterWidgetType::Crosshair:
        case MeterWidgetType::Point:
            emit newValue1(newhor);
            emit newValue2(newvert);
            break;
        }
    }
}

void MeterWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        //     if (event->x() > 0 and event->x()< width() and
        //         event->y() > 0 and event->y()< height())
        double newhor =  m_xmin + (m_xmax - m_xmin) * (double)event->x()/width();
        double newvert = m_ymin + (m_ymax - m_ymin) * (1-((double)event->y()/height()));
        switch(m_metertype) {
        case MeterWidgetType::Fill:
        case MeterWidgetType::Line:
        case MeterWidgetType::Llif:
            if (m_vertical)
                emit newValue2(newvert);
            else
                emit newValue1(newhor);
            break;
        case MeterWidgetType::Crosshair:
        case MeterWidgetType::Point:
            emit newValue1(newhor);
            emit newValue2(newvert);
       }
    }
}

