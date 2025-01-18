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

#include "qutecheckbox.h"

QuteCheckBox::QuteCheckBox(QWidget *parent) : QuteWidget(parent)
{
	m_widget = new QCheckBox(this);
    auto w = static_cast<QCheckBox *>(m_widget);
    // w->setStyleSheet("QCheckBox::indicator { width:50px; height: 50px; }");

    // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setMouseTracking(true);
	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
	canFocus(false);

	setProperty("CSQT_selected", false);
    setProperty("CSQT_label", "");
	setProperty("CSQT_pressedValue", 1.0);
	setProperty("CSQT_randomizable", false);
	setProperty("CSQT_randomizableGroup", 0);

	m_currentValue = 0;

    w->setStyleSheet(QString("QCheckBox::indicator { "
                             "width:%1px; "
                             "height: %1px; }").arg(this->height()));

    connect(w, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
}

QuteCheckBox::~QuteCheckBox()
{
}

void QuteCheckBox::setValue(double value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	if (value >=0) {
		m_currentValue = value != 0 ? m_value : 0.0;
	}
	else {
		m_value = -value;
		m_currentValue = -value;
	}
	m_valueChanged = true;
	//  qDebug( ) << "QuteCheckBox::setValue " << value << "---" << m_currentValue;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteCheckBox::setLabel(QString label)
{
	static_cast<QCheckBox *>(m_widget)->setText(label);
}

double QuteCheckBox::getValue()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	double value = m_currentValue;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return value;
}

//QString QuteCheckBox::getLabel()
//{
//  return static_cast<QCheckBox *>(m_widget)->text();
//}

QString QuteCheckBox::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioCheckbox {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line += (static_cast<QCheckBox *>(m_widget)->isChecked()? QString("on "):QString("off "));
	line += m_channel;
	//   qDebug("QuteText::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteCheckBox::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "checkbox channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	line += "value(" + (static_cast<QCheckBox *>(m_widget)->isChecked()?
							QString("1"):QString("0")) + ")";
	//line += QString(", caption(%1)").arg(m_channel);
	if (property("CSQT_midicc").toInt() >= 0 && property("CSQT_midichan").toInt()>0) { // insert only if midi channel is above 0
		line += ", midiCtrl(\"" + QString::number(property("CSQT_midichan").toInt()) + ",";
		line +=  QString::number(property("CSQT_midicc").toInt()) + "\")";
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteCheckBox::getWidgetType()
{
    return QString("BSBCheckBox");
}

QString QuteCheckBox::getQml()
{
    QString qml = QString();
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif

	qml = "\n\tCheckBox {\n";
    qml += QString("\t\tx: %1 * scaleItem.scale\n").arg(x());
    qml += QString("\t\ty: %1  * scaleItem.scale\n").arg(y());
    qml += QString("\t\twidth: %1  * scaleItem.scale\n").arg(width());
    qml += QString("\t\theight: %1  * scaleItem.scale\n").arg(height());
    qml += QString("\t\tchecked: %1\n").arg(getValue()==0 ? "false" : "true" );
    qml += QString("\t\tonCheckedChanged: csound.setControlChannel(\"%1\", checked ? %2 : 0)\n").arg(m_channel).arg(property("CSQT_pressedValue").toDouble());
    //color, font and other outlook properties ignored by now
    qml += "\t}";
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif
    return qml;

}

void QuteCheckBox::setMidiValue(int value)
{
	double pressedVal = property("CSQT_pressedValue").toDouble();
	double newval= value == 0 ? 0 : pressedVal;
	setValue(newval);
	QPair<QString, double> channelValue(m_channel, newval);
	emit newValue(channelValue);
}

QString QuteCheckBox::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	s.writeTextElement("selected",
					   m_currentValue != 0 ? QString("true"):QString("false"));
	s.writeTextElement("label", property("CSQT_label").toString());
    s.writeTextElement("pressedValue",
                       QString::number(property("CSQT_pressedValue").toDouble()));
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

void QuteCheckBox::refreshWidget()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	//  qDebug() << "QuteCheckBox::refreshWidget " << "--" << m_currentValue;
	m_widget->blockSignals(true);
	static_cast<QCheckBox *>(m_widget)->setChecked(m_currentValue != 0);
	m_widget->blockSignals(false);
	//  setProperty("CSQT_pressedValue", m_value);
	setProperty("CSQT_selected", m_currentValue != 0);

	m_valueChanged = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteCheckBox::resizeEvent(QResizeEvent *event) {
    int size = qMin(this->height(), this->width());
    static_cast<QCheckBox*>(m_widget)->setStyleSheet(QString("QCheckBox::indicator { "
                             "width:%1px; "
                             "height: %1px; }").arg(size));
    QuteWidget::resizeEvent(event);
}


void QuteCheckBox::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
	//  qDebug() << "QuteSlider::applyInternalProperties()";
    // Pressed value must go before selected to make sure it has the right value...
    m_value = property("CSQT_pressedValue").toDouble();
	setValue(property("CSQT_selected").toBool() ? 1:0);
    setLabel(property("CSQT_label").toString());

    static_cast<QCheckBox*>(m_widget)->setStyleSheet(QString("QCheckBox::indicator { "
                             "width:%1px; "
                             "height: %1px; }").arg(this->height()));

}

void QuteCheckBox::stateChanged(int state)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	m_currentValue = (state != Qt::Unchecked ? property("CSQT_pressedValue").toDouble() : 0);
	QPair<QString, double> channelValue(m_channel, m_currentValue);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    emit newValue(channelValue);
}

void QuteCheckBox::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setProperty("CSQT_pressedValue", valueBox->value());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    //Must be last to make sure the widgetChanged signal is last
    QuteWidget::applyProperties();
}

void QuteCheckBox::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
	dialog->setWindowTitle("Check Box");

	QLabel * label = new QLabel(dialog);
	label->setText("Pressed Value");
	layout->addWidget(label, 4, 2, Qt::AlignRight|Qt::AlignVCenter);

    valueBox = new QDoubleSpinBox(dialog);
	valueBox->setDecimals(6);
	valueBox->setRange(-9999999.0, 9999999.0);
	layout->addWidget(valueBox, 4, 3, Qt::AlignLeft|Qt::AlignVCenter);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	valueBox->setValue(property("CSQT_pressedValue").toDouble());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}
