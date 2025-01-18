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

#include "quteknob.h"

QVdial::~QVdial() {};


void QVdial::mousePressEvent (QMouseEvent *event) {
    if(!(event->button() & Qt::LeftButton)) {
        return;
    }
    m_mouse_press_point = event->pos();
    m_dragging = true;
    m_base_value = value();
    m_temp_scale_factor = event->modifiers() & Qt::AltModifier ? 0.25 : 1.0;
}

void QVdial::mouseReleaseEvent (QMouseEvent *event) {
    Q_UNUSED(event);
    m_dragging = false;
}

void QVdial::mouseMoveEvent (QMouseEvent *event) {
    if (!m_dragging)
        return;
    int mouse_distance = m_mouse_press_point.y() - event->y();
    double delta = mouse_distance / (double)this->height() * (double)this->maximum();
    double fvalue = m_base_value + delta * m_temp_scale_factor * m_scale_factor;
    setValue((int)fvalue);
}

void QVdial::mouseDoubleClickEvent (QMouseEvent *event) {
    if(!event->modifiers())
        emit doubleClick();
    else
        this->QDial::mouseDoubleClickEvent(event);
}

QPointF find_ellipse_coords(const QRectF &r, qreal angle) {
  QPainterPath path;
  path.arcMoveTo(r, angle);
  return path.currentPosition();
}

void drawKnob(QPainter &painter, QVdial *knob, int border, QColor fg, QColor bg, QColor borderColor) {
    int totaldegrees = knob->getDegreeRange();
    double relvalue = (double)knob->value() / knob->maximum();
    int steps = (int)(relvalue * totaldegrees);

    int const startAngle360 = 90 + (360 - totaldegrees) / 2;
    int const startAngle = startAngle360;
    QPoint center = knob->rect().center();

    // fgPen.setCosmetic(true);
    double r = qMin(knob->rect().height(), knob->rect().width()) / 2.0;
    r = r * 0.97 - border;
    double inner_r = r*0.67;
    QRectF const rect(center.x() - r, center.y() - r, r * 2, r * 2);
    QRectF outer_rect(center.x() - r, center.y() - r, r * 2, r * 2);
    QRectF inner_rect(center.x() - inner_r, center.y() - inner_r, inner_r*2, inner_r*2);

    // background
    // this could be cached, since it only changes when the knob is resized
    QPainterPath bgpath;

    bgpath.arcMoveTo(outer_rect, -startAngle);
    bgpath.arcTo(outer_rect, -startAngle, -totaldegrees);
    bgpath.lineTo(find_ellipse_coords(inner_rect, -startAngle - totaldegrees));
    bgpath.arcTo(inner_rect, -startAngle-totaldegrees, totaldegrees);
    bgpath.closeSubpath();
    painter.fillPath(bgpath, bg);

    // foreground
    QPainterPath path;
    path.arcMoveTo(outer_rect, -startAngle);
    path.arcTo(outer_rect, -startAngle, -steps);
    path.lineTo(find_ellipse_coords(inner_rect, -startAngle - steps));
    path.arcTo(inner_rect, -startAngle-steps, steps);
    path.closeSubpath();

    painter.fillPath(path, fg);

    // border at the end
    if(border) {
        QPen borderPen(borderColor, border, Qt::SolidLine, Qt::FlatCap);
        if(border == 1) {
            borderPen.setWidth(0);
            borderPen.setCosmetic(true);
        }
        painter.setPen(borderPen);
        painter.drawPath(bgpath);
    }

    return ;

}

void QVdial::paintEvent(QPaintEvent *event) {
    if(!m_flat)
        return this->QDial::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor bgcolor = m_color.darker(330);
    double r = qMin(this->rect().height(), this->rect().width()) / 2.0;
    drawKnob(painter, this, m_border, m_color, bgcolor, m_bordercolor);

    if(m_draw_value && r >= 8) {
        double fvalue = static_cast<double>(this->value());
        double display_value = (fvalue/this->maximum()) *
                               (m_display_max - m_display_min) + m_display_min;
        int decimals = m_intDisplay ? 0 : m_decimals;
        QString strValue = QString::number(display_value, 'f', decimals);
        double numChars = static_cast<double>( strValue.size() );
        if(decimals > 0) {
            // do not account for decimal .
            numChars -= 0.4;
        }
        if(display_value < 0) {
            numChars -= 0.5;
        }
        numChars = numChars > 4 ? numChars : 4;
        double fontScale = 0.40 * 4/numChars;
        int fontSize = (int) ((r - m_border) * fontScale);
        if(fontSize >= 6) {
            painter.setFont({"Helvetica", fontSize});
            painter.setPen(m_textcolor);
            painter.drawText(this->rect(), Qt::AlignCenter, strValue);
        }
    }
}


// ------------------------------------------------------


QuteKnob::QuteKnob(QWidget *parent) : QuteWidget(parent)
{
	//TODO add resolution to config dialog and set these values accordingly
    m_widget = new QVdial(this);
    int const maximum = 10000;
    auto w = static_cast<QVdial *>(m_widget);
    w->setMinimum(0);
    w->setMaximum(maximum);
    w->setNotchTarget(maximum / 10);
    w->setNotchesVisible(true);
    w->setDrawValue(true);

    m_widget->setPalette(QPalette(Qt::gray));
	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setMouseTracking(true);
    setProperty("CSQT_minimum", 0.0);
    setProperty("CSQT_maximum", 1.0);
	setProperty("CSQT_value", 0.0);
	setProperty("CSQT_mode", "lin");
	setProperty("CSQT_mouseControl", "continuous");
    // setProperty("CSQT_mouseControlAct", "jump");
	setProperty("CSQT_resolution", 0.01);
	setProperty("CSQT_randomizable", false);
	setProperty("CSQT_randomizableGroup", 0);
    setColor(QColor(245, 124, 0));
    setTextColor(QColor(81, 41, 0));
    auto borderColor = QColor(81, 41, 0);
    setProperty("CSQT_borderColor", borderColor.name());
    setProperty("CSQT_border", 0);
    w->setBorder(0, borderColor);

    setProperty("CSQT_showvalue", true);
    setProperty("CSQT_flatstyle", true);
    setProperty("CSQT_integerMode", false);
    connect(w, SIGNAL(valueChanged(int)),
            this, SLOT(knobChanged(int)));
    connect(w, SIGNAL(doubleClick()),
            this, SLOT(setValueFromDialog()));
}

QuteKnob::~QuteKnob() {}

void QuteKnob::setRange(double min, double max)
{
	// TODO when knob is resized, its internal range should be adjusted...
	if (max < min) {
		double temp = max;
		max = min;
		min = temp;
	}
	if (m_value > max)
		m_value =  max;
	else if (m_value > min)
		m_value = min;
    setProperty("CSQT_maximum", max);
	setProperty("CSQT_minimum", min);
	m_valueChanged = true;
    static_cast<QVdial *>(m_widget)->setDisplayRange(min, max);

}

void QuteKnob::setMidiValue(int value)
{
	double max = property("CSQT_maximum").toDouble();
    double min = property("CSQT_minimum").toDouble();
	double newval= min + ((value / 127.0)* (max - min));
	setValue(newval);
	QPair<QString, double> channelValue(m_channel, newval);
	emit newValue(channelValue);
}

void QuteKnob::refreshWidget()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
    auto w = static_cast<QVdial *>(m_widget);
	double max = property("CSQT_maximum").toDouble();
    double min = property("CSQT_minimum").toDouble();

    int val = (int)(w->maximum() * (m_value - min)/(max-min));
    m_valueChanged = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    w->blockSignals(true);
    w->setValue(val);
    w->blockSignals(false);
}

void QuteKnob::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
    setValue(property("CSQT_value").toDouble());
    auto w = static_cast<QVdial*>(m_widget);

    w->setColor(property("CSQT_color").value<QColor>());
    w->setTextColor(QColor(property("CSQT_textcolor").toString()));
    w->setDrawValue(property("CSQT_showvalue").toBool());
    w->setFlatStyle(property("CSQT_flatstyle").toBool());
    w->setIntegerMode(property("CSQT_integerMode").toBool());
    w->setDisplayRange(property("CSQT_minimum").toDouble(),
                       property("CSQT_maximum").toDouble());
    w->setBorder(property("CSQT_border").toInt(), property("CSQT_borderColor").toString());
}

void QuteKnob::setWidgetGeometry(int x, int y, int width, int height)
{
	QuteWidget::setWidgetGeometry(x,y,width,height);
	m_widget->blockSignals(true);
	m_widget->setFixedSize(width, height);
	m_widget->blockSignals(false);
}

QString QuteKnob::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioKnob {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line += QString::number(property("CSQT_maximum").toDouble(), 'f', 6) + " ";
	line += QString::number(property("CSQT_minimum").toDouble(), 'f', 6) + " ";
	line += QString::number(property("CSQT_resolution").toDouble(), 'f', 6) + " ";
	line += QString::number(m_value, 'f', 6) + " " + m_channel;
	//   qDebug("QuteKnob::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteKnob::getCsladspaLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ControlPort=" + m_channel + "|" + m_channel + "\n";
	line += "Range=" + QString::number(property("CSQT_minimum").toDouble(), 'f', 8);
	line += "|" + QString::number(property("CSQT_maximum").toDouble(), 'f', 8);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}


QString QuteKnob::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	QString line = "rslider ";
	line += "channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	line += QString("range(%1,%2,%3), ").arg(property("CSQT_minimum").toDouble()).arg(property("CSQT_maximum").toDouble()).arg(m_value);
    // insert only if midi channel is above 0
    if (property("CSQT_midicc").toInt() >= 0 && property("CSQT_midichan").toInt() > 0) {
		line += ", midiCtrl(\"" + QString::number(property("CSQT_midichan").toInt()) + ",";
		line +=  QString::number(property("CSQT_midicc").toInt()) + "\")";
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    return line;
}

QString QuteKnob::getQml()
{
    QString qml = QString();
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif
    qml = "\n\tDial {\n";
    //qml += QString("\t\tid: %1Dial\n").arg(m_channel);
	qml += QString("\t\tx: %1 * scaleItem.scale\n").arg(x());
	qml += QString("\t\ty: %1  * scaleItem.scale\n").arg(y());
	qml += QString("\t\twidth: %1 * scaleItem.scale\n").arg(width());
	qml += QString("\t\theight: %1 * scaleItem.scale\n").arg(height());
    qml += QString("\t\tfrom: %1\n").arg(property("CSQT_minimum").toString());
    qml += QString("\t\tto: %1\n").arg(property("CSQT_maximum").toString());
    qml += QString("\t\tvalue: %1\n").arg(getValue());
    qml += QString("\t\tonPositionChanged: csound.setControlChannel(\"%1\", from + position*(to-from))\n").arg(m_channel); // NB! this is for QtQuick.Controls 2! since onValueChanged works onlu on drag end

    qml += "\t}";
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif

    return qml;

}

QString QuteKnob::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

    s.writeTextElement("minimum", QString::number(property("CSQT_minimum").toDouble(), 'f', 8));
    s.writeTextElement("maximum", QString::number(property("CSQT_maximum").toDouble(), 'f', 8));
	s.writeTextElement("value", QString::number(m_value, 'f', 8));
	s.writeTextElement("mode", property("CSQT_mode").toString());
	s.writeStartElement("mouseControl");
	s.writeAttribute("act", property("CSQT_mouseControlAct").toString());
	s.writeCharacters(property("CSQT_mouseControl").toString());
	s.writeEndElement();
    s.writeTextElement("resolution",
                       QString::number(property("CSQT_resolution").toDouble(), 'f', 8));
	s.writeStartElement("randomizable");
	s.writeAttribute("group", QString::number(property("CSQT_randomizableGroup").toInt()));
	s.writeCharacters(property("CSQT_randomizable").toBool() ? "true": "false");
	s.writeEndElement();

    QColor color = property("CSQT_color").value<QColor>();
    s.writeStartElement("color");
    s.writeTextElement("r", QString::number(color.red()));
    s.writeTextElement("g", QString::number(color.green()));
    s.writeTextElement("b", QString::number(color.blue()));
    s.writeEndElement();

    // we save textcolor as a hex value (QColor::name() returns the hex
    // representation, which can be used to create a QColor also
    QColor textcolor = static_cast<QVdial*>(m_widget)->getTextColor();
    s.writeTextElement("textcolor", textcolor.name());
    s.writeTextElement("border", QString::number(property("CSQT_border").toInt()));
    s.writeTextElement("borderColor", property("CSQT_borderColor").toString());
    s.writeTextElement("showvalue",
                       property("CSQT_showvalue").toBool() ? "true" : "false");
    s.writeTextElement("flatstyle",
                       property("CSQT_flatstyle").toBool() ? "true" : "false");
    s.writeTextElement("integerMode",
                       property("CSQT_integerMode").toBool() ? "true" : "false");
    s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

QString QuteKnob::getWidgetType()
{
	return QString("BSBKnob");
}

void buttonSetColorIcon(QPushButton *button, QColor color, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(color);
    button->setPalette(QPalette(color));
    button->setIcon(pixmap);
}

void QuteKnob::selectKnobColor() {
    auto currentColor = m_widget->palette().color(QPalette::WindowText);
    QColor color = QColorDialog::getColor(currentColor, this);
    setColor(color);
    if(knobColorButton != nullptr) {
        buttonSetColorIcon(knobColorButton, color, 64);
    }
}

void QuteKnob::selectKnobTextColor() {
    auto currentColor = m_widget->palette().color(QPalette::WindowText);
    QColor c = QColorDialog::getColor(currentColor, this);
    setTextColor(c);
    if(knobColorButton != nullptr)
        buttonSetColorIcon(knobTextColorButton, c, 64);
}

void QuteKnob::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	dialog->setWindowTitle("Knob");
    QLabel *label;

    label = new QLabel("Min =", dialog);
	layout->addWidget(label, 2, 0, Qt::AlignRight|Qt::AlignVCenter);

	minSpinBox = new QDoubleSpinBox(dialog);
    minSpinBox->setDecimals(4);
	minSpinBox->setRange(-99999.0, 99999.0);
	minSpinBox->setValue(property("CSQT_minimum").toDouble());
	layout->addWidget(minSpinBox, 2,1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel("Max =", dialog);
    layout->addWidget(label, 2, 2, Qt::AlignRight|Qt::AlignVCenter);

    maxSpinBox = new QDoubleSpinBox(dialog);
    maxSpinBox->setDecimals(4);
	maxSpinBox->setRange(-99999.0, 99999.0);
	maxSpinBox->setValue(property("CSQT_maximum").toDouble());
    layout->addWidget(maxSpinBox, 2, 3, Qt::AlignLeft|Qt::AlignVCenter);

    // label->setText("Resolution");
    // layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);

    flatStyleCheckBox = new QCheckBox("Flat style", dialog);
    flatStyleCheckBox->setToolTip("If checked, knob is drawn in a flat style. "
                                  "Otherwise, the native Qt look is used");
    flatStyleCheckBox->setCheckState(
                property("CSQT_flatstyle").toBool()?Qt::Checked:Qt::Unchecked);
    layout->addWidget(flatStyleCheckBox, 5, 1, Qt::AlignLeft|Qt::AlignVCenter);

    displayValueCheckBox = new QCheckBox("Show value", dialog);
    displayValueCheckBox->setToolTip("Show value as text inside the knob");
    displayValueCheckBox->setCheckState(
                property("CSQT_showvalue").toBool()?Qt::Checked:Qt::Unchecked);
    layout->addWidget(displayValueCheckBox, 6, 1, Qt::AlignLeft|Qt::AlignVCenter);

    intModeCheckBox = new QCheckBox("Integer", dialog);
    intModeCheckBox->setToolTip("Constrain displayed value to nearest integer");
    intModeCheckBox->setCheckState(
                property("CSQT_integerMode").toBool()?Qt::Checked:Qt::Unchecked);
    layout->addWidget(intModeCheckBox, 6, 2, Qt::AlignLeft|Qt::AlignVCenter);

    // Color
    label = new QLabel("Color", dialog);
    layout->addWidget(label, 7, 0, Qt::AlignRight|Qt::AlignVCenter);

    knobColorButton = new SelectColorButton(dialog);
    knobColorButton->setColor(property("CSQT_color").value<QColor>());
    layout->addWidget(knobColorButton, 7, 1, Qt::AlignLeft | Qt::AlignVCenter);

    // Text Color
    label = new QLabel("Text Color", dialog);
    layout->addWidget(label, 7, 2, Qt::AlignRight|Qt::AlignVCenter);

    knobTextColorButton = new SelectColorButton(dialog);
    knobTextColorButton->setColor(QColor(property("CSQT_textcolor").toString()));
    layout->addWidget(knobTextColorButton, 7, 3, Qt::AlignLeft | Qt::AlignVCenter);

    // Border
    label = new QLabel("Border", dialog);
    layout->addWidget(label, 8, 0, Qt::AlignRight|Qt::AlignVCenter);

    borderWidthSpinBox = new QSpinBox(dialog);
    borderWidthSpinBox->unsetLocale();
    borderWidthSpinBox->setValue(property("CSQT_border").toInt());
    borderWidthSpinBox->setRange(0, 8);
    borderWidthSpinBox->setToolTip("Set to 1 or more to draw a border around the knob in flat mode");
    layout->addWidget(borderWidthSpinBox, 8, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel("Border Color", dialog);
    layout->addWidget(label, 8, 2, Qt::AlignRight|Qt::AlignVCenter);

    borderColorButton = new SelectColorButton(dialog);
    borderColorButton->setColor(property("CSQT_borderColor").value<QColor>());
    layout->addWidget(borderColorButton, 8, 3, Qt::AlignLeft|Qt::AlignVCenter);

    connect(flatStyleCheckBox, SIGNAL(toggled(bool)),
            displayValueCheckBox, SLOT(setEnabled(bool)));
    connect(flatStyleCheckBox, SIGNAL(toggled(bool)),
            knobColorButton, SLOT(setEnabled(bool)));
    connect(flatStyleCheckBox, SIGNAL(toggled(bool)),
            knobColorButton, SLOT(setEnabled(bool)));
    connect(flatStyleCheckBox, SIGNAL(toggled(bool)),
            knobTextColorButton, SLOT(setEnabled(bool)));
    connect(flatStyleCheckBox, SIGNAL(toggled(bool)),
            label, SLOT(setEnabled(bool)));
    connect(flatStyleCheckBox, SIGNAL(toggled(bool)),
            intModeCheckBox, SLOT(setEnabled(bool)));


#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteKnob::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
    auto w = static_cast<QVdial*>(m_widget);

    setProperty("CSQT_maximum", maxSpinBox->value());
    setProperty("CSQT_minimum", minSpinBox->value());

    // QColor color = knobColorButton->palette().color(QPalette::Window);
    QColor color = knobColorButton->getColor();
    setProperty("CSQT_color", color);
    w->setColor(color);

    QColor textcolor = knobTextColorButton->getColor();
    setProperty("CSQT_textcolor", textcolor.name());
    qDebug() << "knob textcolor" << textcolor;
    w->setTextColor(textcolor);

    w->setDisplayRange(minSpinBox->value(), maxSpinBox->value());

    bool showvalue = displayValueCheckBox->checkState();
    setProperty("CSQT_showvalue", showvalue);
    w->setDrawValue(showvalue);

    bool flatstyle = flatStyleCheckBox->checkState();
    setProperty("CSQT_flatstyle", flatstyle);
    w->setFlatStyle(flatstyle);

    bool intmode = intModeCheckBox->checkState();
    setProperty("CSQT_integerMode", intmode);
    w->setIntegerMode(intmode);

    QColor bordercolor = borderColorButton->getColor();
    setProperty("CSQT_borderColor", bordercolor.name());
    int borderwidth = borderWidthSpinBox->value();
    setProperty("CSQT_border", borderwidth);
    w->setBorder(borderwidth, bordercolor);

#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    //Must be last to make sure the widgetChanged signal is last
    QuteWidget::applyProperties();
}

void QuteKnob::knobChanged(int value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	double min = property("CSQT_minimum").toDouble();
    double max = property("CSQT_maximum").toDouble();
	QDial *knob = static_cast<QDial *>(m_widget);
	double normalized = (double) (value - knob->minimum())
            / (double) (knob->maximum() - knob->minimum());
	m_value =  min + (normalized * (max-min));
    // setInternalValue(scaledValue);
    m_valueChanged = true;
	QPair<QString, double> channelValue(m_channel, m_value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	emit newValue(channelValue);
}

void QuteKnob::setColor(QColor c) {
    setProperty("CSQT_color", c);
    static_cast<QVdial*>(m_widget)->setColor(c);
}

void QuteKnob::setTextColor(QColor c) {
    setProperty("CSQT_textcolor", c.name());
    static_cast<QVdial*>(m_widget)->setTextColor(c);
}

void QuteKnob::setValueFromDialog() {
    qDebug() << "knob value dialog"
             << m_value
             << property("CSQT_minimum").toDouble()
             << property("CSQT_maximum").toDouble();
    double newvalue = QInputDialog::getDouble(this, tr("Enter New Value"), tr("Value"),
                                              m_value, property("CSQT_minimum").toDouble(),
                                              property("CSQT_maximum").toDouble(), 3);
    setValue(newvalue);
    QPair<QString, double> channelValue(m_channel, m_value);
    emit newValue(channelValue);
}

