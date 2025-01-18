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

#include "quteslider.h"
#ifdef Q_OS_MAC
    #include "myslider.h"
#endif

QuteSlider::QuteSlider(QWidget *parent) : QuteWidget(parent)
{
#ifdef Q_OS_MAC
    m_widget = new MySlider(this);
#else
    m_widget = new QSlider(this);
#endif

	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
	m_widget->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels
	canFocus(false);
    if (width() > height())    {
        static_cast<QSlider *>(m_widget)->setOrientation(Qt::Horizontal);
    } else {
        static_cast<QSlider *>(m_widget)->setOrientation(Qt::Vertical);
    }

	connect(static_cast<QSlider *>(m_widget), SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));

	setProperty("CSQT_minimum", 0.0);
	setProperty("CSQT_maximum", 1.0);
	setProperty("CSQT_value", 0.0);
	setProperty("CSQT_mode", "lin");
	setProperty("CSQT_mouseControl", "continuous");
	setProperty("CSQT_mouseControlAct", "jump");
	setProperty("CSQT_resolution", -1.0);
	setProperty("CSQT_randomizable", false);
	setProperty("CSQT_randomizableGroup", 0);
}

QuteSlider::~QuteSlider()
{
}

//void QuteSlider::sliderMoved(int value)
//{
//  QuteWidget::valueChanged(m_value);
//}

void QuteSlider::setValue(double value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setInternalValue(value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteSlider::setMidiValue(int value)
{	//qDebug()<<Q_FUNC_INFO<<value;
	double max = property("CSQT_maximum").toDouble();
	double min = property("CSQT_minimum").toDouble();
	double newval = min + ((value / 127.0)* (max - min));
	setValue(newval);
	QPair<QString, double> channelValue(m_channel, newval);
	emit newValue(channelValue);
}

void QuteSlider::refreshWidget()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	//  qDebug() << "QuteSlider::refreshWidget " << m_value;
	double min = property("CSQT_minimum").toDouble();
	double max = property("CSQT_maximum").toDouble();
	int val = (int) (m_len * (m_value - min)/(max- min));
	m_valueChanged = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	m_widget->blockSignals(true);
	static_cast<QSlider *>(m_widget)->setValue(val);
	m_widget->blockSignals(false);
}

void QuteSlider::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
	//  qDebug() << "QuteSlider::applyInternalProperties()";

	m_value = property("CSQT_value").toDouble();
	//  m_value2 = property("CSQT_value2").toDouble();
	//  m_stringValue = property("CSQT_stringValue").toString();
	double max = property("CSQT_maximum").toDouble();
	double min = property("CSQT_minimum").toDouble();
	if (max < min) {
		double temp = max;
		max = min;
		min = temp;
	}
}

void QuteSlider::setWidgetGeometry(int x, int y, int w, int h)
{
	QuteWidget::setWidgetGeometry(x,y,w,h);
	m_widget->blockSignals(true);
	if (width() > height()) {
		static_cast<QSlider *>(m_widget)->setOrientation(Qt::Horizontal);
		static_cast<QSlider *>(m_widget)->setMaximum(w);
		m_len = w;
	}
	else {
		static_cast<QSlider *>(m_widget)->setOrientation(Qt::Vertical);
		static_cast<QSlider *>(m_widget)->setMaximum(h);
		m_len = h;
	}
	m_widget->blockSignals(false);
}

QString QuteSlider::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioSlider {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line += QString::number(property("CSQT_minimum").toDouble(), 'f', 6) + " ";
	line += QString::number(property("CSQT_maximum").toDouble(), 'f', 6) + " ";
	line += QString::number(m_value, 'f', 6) + " " + m_channel;
	//   qDebug("QuteSlider::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteSlider::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	QString line = "";
	if (m_widget->width() > m_widget->height()) {
		line += "hslider ";
	}
	else {
		line += "vslider ";
	}
	line += "channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	line += QString("range(%1,%2,%3) " ).arg(property("CSQT_minimum").toDouble()).arg(property("CSQT_maximum").toDouble()).arg(m_value);
	//line += QString(", text(%1), ").arg(m_channel); // Is it good idea to put channel as name - not really since geometry probably does not allow it
	if (property("CSQT_midicc").toInt() >= 0 && property("CSQT_midichan").toInt()>0) { // insert only if midi channel is above 0
		line += ", midiCtrl(\"" + QString::number(property("CSQT_midichan").toInt()) + ",";
		line +=  QString::number(property("CSQT_midicc").toInt()) + "\")";
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}


QString QuteSlider::getQml()
{
	QString qml = QString();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	qml = "\n\tSlider {\n";
    //qml += QString("\t\t\id: %1Slider\n").arg(m_channel);
	qml += QString("\t\tx: %1 * scaleItem.scale\n").arg(x());
	qml += QString("\t\ty: %1  * scaleItem.scale\n").arg(y());
	qml += QString("\t\twidth: %1 * scaleItem.scale\n").arg(width());
	qml += QString("\t\theight: %1 * scaleItem.scale\n").arg(height());
	qml += QString("\t\tfrom: %1\n").arg(property("CSQT_minimum").toString());
	qml += QString("\t\tto: %1\n").arg(property("CSQT_maximum").toString());
	qml += QString("\t\tvalue: %1\n").arg(getValue());
	if ( width() > height() ) {
		qml += "\t\torientation: Qt.Horizontal\n";
	} else {
		qml += "\t\torientation: Qt.Vertical\n";
	}
	qml += QString("\t\tonPositionChanged: csound.setControlChannel(\"%1\", valueAt(position))\n").arg(m_channel); // NB! this is for QtQuick.Controls 2! since onValueChanged works onlu on drag end

	qml += "\t}";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif

	return qml;
}

QString QuteSlider::getCsladspaLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ControlPort=" + m_channel + "|" + m_channel + "\n";
	line += "Range=" + QString::number(property("CSQT_minimum").toDouble(), 'f', 8)
			+ "|" + QString::number(property("CSQT_maximum").toDouble(), 'f', 8);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteSlider::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	s.writeTextElement("minimum", QString::number(property("CSQT_minimum").toDouble(), 'f', 8));
	s.writeTextElement("maximum", QString::number(property("CSQT_maximum").toDouble(), 'f', 8));
	s.writeTextElement("value", QString::number(m_value, 'f', 8));
	s.writeTextElement("mode", property("CSQT_mode").toString());

	s.writeStartElement("mouseControl");
	s.writeAttribute("act", property("CSQT_mouseControlAct").toString());
	s.writeCharacters(property("CSQT_mouseControl").toString());
	s.writeEndElement();
	s.writeTextElement("resolution", QString::number(property("CSQT_resolution").toDouble(), 'f', 8));
	s.writeStartElement("randomizable");
	s.writeAttribute("group", QString::number(property("CSQT_randomizableGroup").toInt()));
	s.writeCharacters(property("CSQT_randomizable").toBool() ? "true": "false");
	s.writeEndElement();

	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    return xmlText;
}


QString QuteSlider::getWidgetType()
{
	return (width() > height()? QString("BSBHSlider"):QString("BSBVSlider"));
}

void QuteSlider::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
	dialog->setWindowTitle("Slider");
	QLabel *label = new QLabel(dialog);
	label->setText("Min =");
	layout->addWidget(label, 2, 0, Qt::AlignRight|Qt::AlignVCenter);
	minSpinBox = new QDoubleSpinBox(dialog);
	minSpinBox->setDecimals(6);
	minSpinBox->setRange(-99999.0, 99999.0);
	layout->addWidget(minSpinBox, 2,1, Qt::AlignLeft|Qt::AlignVCenter);
	label = new QLabel(dialog);
	label->setText("Max =");
	layout->addWidget(label, 2, 2, Qt::AlignRight|Qt::AlignVCenter);
	maxSpinBox = new QDoubleSpinBox(dialog);
	maxSpinBox->setDecimals(6);
	maxSpinBox->setRange(-99999.0, 99999.0);
	layout->addWidget(maxSpinBox, 2,3, Qt::AlignLeft|Qt::AlignVCenter);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	minSpinBox->setValue(property("CSQT_minimum").toDouble());
	maxSpinBox->setValue(property("CSQT_maximum").toDouble());
	//  setProperty("CSQT_value", m_value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteSlider::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setProperty("CSQT_maximum", maxSpinBox->value());
	setProperty("CSQT_minimum", minSpinBox->value());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	QuteWidget::applyProperties();  // Must be last to make sure the widgetChanged signal is last
}

void QuteSlider::sliderChanged(int value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	double normalized = (double) value / (double) m_len;
	double min = property("CSQT_minimum").toDouble();
	double max = property("CSQT_maximum").toDouble();
	double scaledValue =  min + (normalized * (max-min));
	setInternalValue(scaledValue);
	QPair<QString, double> channelValue(m_channel, m_value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	emit newValue(channelValue);
}

void QuteSlider::setInternalValue(double value)
{
	double max = property("CSQT_maximum").toDouble();
	double min = property("CSQT_minimum").toDouble();
	if (value > max)
		m_value = max;
	else if (value < min)
		m_value = min;
	else
		m_value = value;
	m_valueChanged = true;
	//  setProperty("CSQT_value", m_value);
}
