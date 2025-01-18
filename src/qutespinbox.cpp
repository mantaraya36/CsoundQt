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

#include "qutespinbox.h"
#include <QStringBuilder>
#include <cmath>

#define USEFONTPIXELSIZE

QuteSpinBox::QuteSpinBox(QWidget* parent) : QuteText(parent)
{
	delete m_widget; //delete widget created by parent constructor
	m_widget = new QDoubleSpinBox(this);
	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setMouseTracking(true);
	static_cast<QDoubleSpinBox*>(m_widget)->setAccelerated(true);
	static_cast<QDoubleSpinBox*>(m_widget)->setRange(-999999999999.0, 999999999999.0);
	static_cast<QDoubleSpinBox*>(m_widget)->setKeyboardTracking (false);
	connect(static_cast<QDoubleSpinBox *>(m_widget), SIGNAL(valueChanged(double)),
			this, SLOT(valueChanged(double)));

    m_type = "editnum";

	setProperty("CSQT_value", (double) 0.0);
	setProperty("CSQT_resolution", (double) 0.1);
	setProperty("CSQT_minimum",(double)  -999999999999.0);
	setProperty("CSQT_maximum", (double) 999999999999.0);
	setProperty("CSQT_randomizable", false);
	setProperty("CSQT_randomizableGroup", 0);
    // For numbers with fixed resolution it makes more sense to align to the right
    setProperty("CSQT_alignment", "right");

    // Remove these properties, which are part of parent class but
    // have no meaning here
    setProperty("CSQT_label", QVariant());
    setProperty("CSQT_color", QVariant());
    setProperty("CSQT_borderradius", QVariant());
    setProperty("CSQT_borderwidth", QVariant());
}

QuteSpinBox::~QuteSpinBox() {}

void QuteSpinBox::setText(QString text)
{
	setValue(text.toDouble());
}

void QuteSpinBox::setMidiValue(int value)
{
	double max = property("CSQT_maximum").toDouble();
	double min = property("CSQT_minimum").toDouble();
	if (max != 999999999999.0 && min != -999999999999.0) {
		double newval = min + ((value / 127.0)* (max - min));
		setValue(newval);
		QPair<QString, double> channelValue(m_channel, newval);
		emit newValue(channelValue);
	}
	else {
		qDebug() << "QuteSpinBox::setMidiValue ranges not set.";
	}
}

//void QuteSpinBox::setResolution(double resolution)
//{
//  setProperty("CSQT_resolution", resolution);
//}

QString QuteSpinBox::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioText {" + QString::number(property("CSQT_x").toInt()) + ", " + QString::number(property("CSQT_y").toInt()) + "} ";
	line += "{"+ QString::number(property("CSQT_width").toInt()) +", "+ QString::number(property("CSQT_height").toInt()) +"} ";
	line += m_type + " ";
	line += QString::number(static_cast<QDoubleSpinBox*>(m_widget)->value(), 'f', 6) + " ";
	line += QString::number(property("CSQT_resolution").toDouble(), 'f', 6) + " \"" + m_channel + "\" ";
	line += property("CSQT_alignment").toString() + " ";
	line += "\"" + m_widget->property("CSQT_font").toString() + "\" "
			+ QString::number(m_widget->property("CSQT_fontsize").toInt()) + " ";
	QColor color = m_widget->palette().color(QPalette::WindowText);
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	color = m_widget->palette().color(QPalette::Window);
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	line += m_widget->autoFillBackground()? "background ":"nobackground ";
	line += "noborder ";
	line += QString::number(static_cast<QDoubleSpinBox*>(m_widget)->value(), 'f', 6);
	// For this type of ioText widget, value and text are redundant. CsoundQt reads mthe value coming
	// in the text field, but writes to both.
	//   qDebug("QuteText::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteSpinBox::getCsladspaLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ControlPort=" + m_channel + "|" + m_channel + "\n";
	line += "Range=-9999|9999";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteSpinBox::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "numberbox channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	//line += QString("text(%1), ").arg(m_channel); // Is it good idea to put channel as name?
	line += QString( "range(%1,%2,%3,1,%4),  ").arg(property("CSQT_minimum").toDouble()).arg(property("CSQT_maximum").toDouble()).arg(m_value).arg(property("CSQT_resolution").toDouble());
	//color does not work CsoundQt, why? Leave it out now... line += QString("colour(\"%1\")").arg( property("CSQT_color").toString());
	QColor color = property("CSQT_color").value<QColor>();
	line += "fontcolour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + "), ";
	color = property("CSQT_bgcolor").value<QColor>();
	line += "colour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + ") ";
	if (property("CSQT_midicc").toInt() >= 0 && property("CSQT_midichan").toInt()>0) { // insert only if midi channel is above 0
			line += ", midiCtrl(\"" + QString::number(property("CSQT_midichan").toInt()) + ",";
			line +=  QString::number(property("CSQT_midicc").toInt()) + "\")";
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    return line;
}

QString QuteSpinBox::getQml()
{
    QString qml = QString();
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif
    // currently QtQuick.Controls 2 SpindBox can be programmes to handle floating point
    // see https://doc.qt.io/qt-5.9/qml-qtquick-controls2-spinbox.html accepting floating point numbers
    // but this is not supported here yet. Use integer stepSize for your spingbox for now
    float resolution = property("CSQT_resolution").toFloat();
    if (floorf(resolution) != resolution ) { // check if integer
        qDebug() << "stepsize not integer";
		return QString();

//        QMessageBox::warning(nullptr, tr("SpinBox conversion"),
//            tr("Currently only integer resolution (stepsize) is supported for QML spinbox.\nPlease change your spinbox to use it in QML"));

        // messagebox freezes here...
        //return "";
    }

    qml = "\n\tSpinBox {\n"; // NB! Does not accept floating point numbers...

    //qml += QString("\t\tid: %1SpinBox\n").arg(m_channel);
	qml += QString("\t\tx: %1 * scaleItem.scale\n").arg(x());
	qml += QString("\t\ty: %1  * scaleItem.scale\n").arg(y());
	qml += QString("\t\twidth: (%1 + up.indicator.width + down.indicator.width)  * scaleItem.scale\n").arg(width());
	qml += QString("\t\theight: %1  * scaleItem.scale\n").arg(height());
    qml += QString("\t\tfrom: %1\n").arg(property("CSQT_minimum").toString());
    qml += QString("\t\tto: %1\n").arg(property("CSQT_maximum").toString());
    qml += QString("\t\tstepSize: %1\n").arg(property("CSQT_resolution").toInt()); // Int for now...
    qml += QString("\t\tvalue: %1\n").arg(int(getValue())); // TODO: use double
    qml += "\t\teditable: true\n";
    qml += QString("\t\tonValueChanged: csound.setControlChannel(\"%1\", value)\n").arg(m_channel);
    //color, font and other outlook properties ignored by now
    qml += "\t}";
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif

    return qml;

}

QString QuteSpinBox::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	//  s.writeTextElement("type", m_type);

	//These are not implemented in blue
	s.writeTextElement("alignment", property("CSQT_alignment").toString());

	s.writeTextElement("font", property("CSQT_font").toString());
	s.writeTextElement("fontsize", QString::number(property("CSQT_fontsize").toDouble()));

	QColor color = property("CSQT_color").value<QColor>();
	s.writeStartElement("color");
	s.writeTextElement("r", QString::number(color.red()));
	s.writeTextElement("g", QString::number(color.green()));
	s.writeTextElement("b", QString::number(color.blue()));
	s.writeEndElement();
	color = property("CSQT_bgcolor").value<QColor>();
	s.writeStartElement("bgcolor");
	s.writeAttribute("mode", property("CSQT_bgcolormode").toBool()? "background":"nobackground");
	s.writeTextElement("r", QString::number(color.red()));
	s.writeTextElement("g", QString::number(color.green()));
	s.writeTextElement("b", QString::number(color.blue()));
	s.writeEndElement();
	s.writeTextElement("resolution", QString::number(property("CSQT_resolution").toDouble(), 'f', 8));

	s.writeTextElement("minimum", QString::number(property("CSQT_minimum").toDouble()));
	s.writeTextElement("maximum", QString::number(property("CSQT_maximum").toDouble()));
	//  s.writeTextElement("bordermode", property("CSQT_bordermode").toString());
	//  s.writeTextElement("borderradius", QString::number(property("CSQT_borderradius").toInt()));
	//  s.writeTextElement("borderwidth", QString::number(property("CSQT_borderwidth").toInt()));
	s.writeStartElement("randomizable");
	s.writeAttribute("group", QString::number(property("CSQT_randomizableGroup").toInt()));
	s.writeCharacters(property("CSQT_randomizable").toBool() ? "true": "false");
	s.writeEndElement();
	s.writeTextElement("value", QString::number(m_value));
	s.writeEndElement();

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

QString QuteSpinBox::getWidgetType()
{
	return QString("BSBSpinBox");
}

void QuteSpinBox::refreshWidget()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	//  double resolution = property("CSQT_resolution").toDouble();
	//  double val = resolution * (int(m_value /resolution) + 1.0); // ceil or int won't work here...
	double val = m_value;
	//  qDebug()<< "QuteSpinBox::refreshWidget()" << val << m_value << resolution;
	m_valueChanged = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	//  int cursorPos = static_cast<QLineEdit*>(m_widget)->cursorPosition();
	m_widget->blockSignals(true);
	static_cast<QDoubleSpinBox*>(m_widget)->setValue(val);
	//  static_cast<QDoubleSpinBox*>(m_widget)->setCursorPosition(cursorPos);
	m_widget->blockSignals(false);
}

void QuteSpinBox::applyInternalProperties()
{
    QuteWidget::applyInternalProperties();

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
    auto w = static_cast<QDoubleSpinBox*>(m_widget);
    w->setRange(property("CSQT_minimum").toDouble(),property("CSQT_maximum").toDouble());
	m_value = property("CSQT_value").toDouble();
	double resolution = property("CSQT_resolution").toDouble();
	m_valueChanged = true;
	int i;
    for (i=0; i < 8; i++) {//     Check for used decimal places.
		double fractpart, intpart;
		fractpart = modf (resolution * pow(10, i), &intpart);
		if (fractpart < 1e-200) {
			break;
		}
	}

    w->setDecimals(i);
    w->setSingleStep(resolution);
    Qt::Alignment align;
	QString alignText = property("CSQT_alignment").toString();
	if (alignText == "left") {
		align = Qt::AlignLeft|Qt::AlignVCenter;
	}
	else if (alignText == "center") {
		align = Qt::AlignHCenter|Qt::AlignVCenter;
	}
	else if (alignText == "right") {
		align = Qt::AlignRight|Qt::AlignVCenter;
	}
    w->setAlignment(align);
	setTextColor(property("CSQT_color").value<QColor>());

	double fontSize = (property("CSQT_fontsize").toDouble()*m_fontScaling) + m_fontOffset;

    auto bgstr = property("CSQT_bgcolormode").toBool() ?
        QString("background-color:")+property("CSQT_bgcolor").value<QColor>().name()+";" :
        QString("");

#ifdef USEFONTPIXELSIZE
    auto sheet = QString(
        "QDoubleSpinBox {"
        "font-family: \"%1\";"
        "font-size: %2px;"
        "color: %3;"
        "%4 }")
        .arg(property("CSQT_font").toString())
        .arg(QString::number(fontSize))
        .arg(property("CSQT_color").value<QColor>().name())
        .arg(bgstr);
#else
    int new_fontSize = 0;
    int totalHeight = 0;
    while (totalHeight < fontSize + 1) {
        new_fontSize++;
        QFont font(property("CSQT_font").toString(), new_fontSize);
        QFontMetricsF fm(font);
        totalHeight = fm.ascent() + fm.descent();
    }
    auto sheet = QString(
        "QDoubleSpinBox {"
        "font-family: \"%1\";"
        "font-size: %2pt;"
        "color: %3;"
        "%4 }")
        .arg(property("CSQT_font").toString())
        .arg(QString::number(new_fontSize))
        .arg(property("CSQT_color").value<QColor>().name())
        .arg(bgstr);

#endif
    m_widget->setStyleSheet(sheet);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif

}

void QuteSpinBox::createPropertiesDialog()
{
	//   qDebug("QuteSpinBox::createPropertiesDialog()");
	QuteText::createPropertiesDialog();
	dialog->setWindowTitle("SpinBox");
	QLabel *label = new QLabel(dialog);
	label->setText("Resolution");
	layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);
	resolutionSpinBox = new QDoubleSpinBox(dialog);
	resolutionSpinBox->setDecimals(6);
	resolutionSpinBox->setRange(0,999999999999.0);
	layout->addWidget(resolutionSpinBox, 4, 1, Qt::AlignLeft|Qt::AlignVCenter);

	label = new QLabel(dialog);
	label->setText("Min =");
	layout->addWidget(label, 2, 0, Qt::AlignRight|Qt::AlignVCenter);
	minSpinBox = new QDoubleSpinBox(dialog);
	minSpinBox->setDecimals(6);
	minSpinBox->setRange(-999999999999.0, 999999999999.0);
	layout->addWidget(minSpinBox, 2,1, Qt::AlignLeft|Qt::AlignVCenter);
	label = new QLabel(dialog);
	label->setText("Max =");
	layout->addWidget(label, 2, 2, Qt::AlignRight|Qt::AlignVCenter);
	maxSpinBox = new QDoubleSpinBox(dialog);
	maxSpinBox->setDecimals(6);
	maxSpinBox->setRange(-999999999999.0, 999999999999.0);
	layout->addWidget(maxSpinBox, 2,3, Qt::AlignLeft|Qt::AlignVCenter);
	borderRadius->hide();
    labelPtrs["borderRadius"]->hide();
	borderWidth->hide();
    labelPtrs["borderWidth"]->hide();
    vertAlignmentComboBox->hide();
    labelPtrs["vertAlign"]->hide();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	resolutionSpinBox->setValue(property("CSQT_resolution").toDouble());
	minSpinBox->setValue(property("CSQT_minimum").toDouble());
	maxSpinBox->setValue(property("CSQT_maximum").toDouble());
	text->setText(static_cast<QDoubleSpinBox *>(m_widget)->text());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteSpinBox::applyProperties()
{
	switch (alignment->currentIndex()) {
	case 0:
		setProperty("CSQT_alignment", "left");
		break;
	case 1:
		setProperty("CSQT_alignment", "center");
		break;
	case 2:
		setProperty("CSQT_alignment", "right");
		break;
	default:
		setProperty("CSQT_alignment", "");
	}
	setProperty("CSQT_font", font->currentFont().family());
	setProperty("CSQT_fontsize", fontSize->value());
	setProperty("CSQT_bgcolor", bgColor->palette().color(QPalette::Window));
	setProperty("CSQT_bgcolormode", bg->isChecked());
	setProperty("CSQT_color", textColor->palette().color(QPalette::Window));
    setProperty("CSQT_resolution", resolutionSpinBox->value());
	setProperty("CSQT_value", text->toPlainText().toDouble());
	setProperty("CSQT_maximum", maxSpinBox->value());
	setProperty("CSQT_minimum", minSpinBox->value());
	//  setProperty("CSQT_randomizable",false);
    // setProperty("CSQT_bordermode", border->isChecked() ? "border" : "noborder");
    QuteWidget::applyProperties();  // Must be last so that the widgetChanged signal is last
}

void QuteSpinBox::valueChanged(double value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setInternalValue(value);
	QPair<QString, double> channelValue(m_channel, value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	emit newValue(channelValue);
}

void QuteSpinBox::editingFinishedSlot()
{
	double value = static_cast<QDoubleSpinBox*>(m_widget)->value();

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setInternalValue(value);
	QPair<QString, double> channelValue(m_channel, value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	emit newValue(channelValue);
}

void QuteSpinBox::setInternalValue(double value)
{
	m_value = value;
    m_valueChanged = true;
}
