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

#include "qutecombobox.h"

QuteComboBox::QuteComboBox(QWidget *parent) : QuteWidget(parent)
{
    m_widget = new QComboBox(this);
    auto w = static_cast<QComboBox*>(m_widget);
    // Since combobox menus use native widgets we should not mess with
    // styling and let the underlying engine handle that.
#ifdef Q_OS_LINUX
    w->setStyleSheet("padding-left: 2px;");
#endif
    m_widget->setContextMenuPolicy(Qt::NoContextMenu);
	m_widget->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels
	//  canFocus(false);
	//   connect((QComboBox *)m_widget, SIGNAL(released()), this, SLOT(buttonReleased()));
    connect(static_cast<QComboBox *>(m_widget), SIGNAL(currentIndexChanged(int)),
            this, SLOT(indexChanged(int)));
	setProperty("CSQT_selectedIndex", 0);
	setProperty("CSQT_randomizable", false);
	setProperty("CSQT_randomizableGroup", 0);
}

QuteComboBox::~QuteComboBox()
{
}

QString QuteComboBox::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioMenu {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line += QString::number(((QComboBox *)m_widget)->currentIndex()) + " ";
	line += "303 ";
	line += "\"" + itemList() + "\" ";
	line += m_channel;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteComboBox::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "combobox channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	line += "value(" + QString::number(m_value) + "), ";
	line += "items(\"" + itemList() + "\")";
	if (property("CSQT_midicc").toInt() >= 0 && property("CSQT_midichan").toInt()>0) { // insert only if midi channel is above 0
		line += ", midiCtrl(\"" + QString::number(property("CSQT_midichan").toInt()) + ",";
		line +=  QString::number(property("CSQT_midicc").toInt()) + "\")";
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteComboBox::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	s.writeStartElement("bsbDropdownItemList");
	for (int i = 0; i < static_cast<QComboBox *>(m_widget)->count(); i++) {
		s.writeStartElement("bsbDropdownItem");
		s.writeTextElement("name", static_cast<QComboBox *>(m_widget)->itemText(i));
		s.writeTextElement("value", QString::number(static_cast<QComboBox *>(m_widget)->itemData(i).toInt()) );
		s.writeTextElement("stringvalue", stringValues[i]);
		s.writeEndElement();
	}
	s.writeEndElement();
	s.writeTextElement("selectedIndex", QString::number(((QComboBox *)m_widget)->currentIndex()));
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

QString QuteComboBox::getWidgetType()
{
	return QString("BSBDropdown");
}

QString QuteComboBox::getQml()
{
	QString qml = QString();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif

	// create model
	QString list = "";
	for (int i = 0; i < static_cast<QComboBox *>(m_widget)->count(); i++) {
		list += "\"" + static_cast<QComboBox *>(m_widget)->itemText(i) + "\",";
	}
	list.chop(1); //remove last comma

	qml = QString(R"(
				  ComboBox {
					  property string channel: "%1"
					  x: %2 * scaleItem.scale
					  y: %3 * scaleItem.scale
					  width: %4 * scaleItem.scale
					  height: %5 * scaleItem.scale
					  model: [ %6 ]
					  currentIndex: %7
					  onCurrentIndexChanged: if (typeof(csound)!='undefined') csound.setControlChannel(channel, currentIndex);
				  }
	)").arg(m_channel).arg(x()).arg(y()).arg(width()).arg(height()).
			arg(list).arg(((QComboBox *)m_widget)->currentIndex());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return qml;
}

void QuteComboBox::setMidiValue(int value)
{
	QComboBox * cb = static_cast<QComboBox *>(m_widget);

	int index = int (value/127.0 * (cb->count()-1));
	cb->setCurrentIndex(index);

	// not sure if this is necessary
	double newval = static_cast<double>(index);
	setValue( newval );
	QPair<QString, double> channelValue(m_channel, newval);
	emit newValue(channelValue);

}

QString QuteComboBox::itemList()
{
	// For old format
	QString list = "";
	for (int i = 0; i < static_cast<QComboBox *>(m_widget)->count(); i++) {
		list += static_cast<QComboBox *>(m_widget)->itemText(i) + ",";
	}
	list.chop(1); //remove last comma
	return list;
}

void QuteComboBox::setText(QString text)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	clearItems();
	QStringList items = text.split(",");
	int counter = 0;
	foreach (QString item, items) {
		addItem(item, counter++, "");
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteComboBox::clearItems()
{
	m_widget->blockSignals(true);
	static_cast<QComboBox *>(m_widget)->clear();
	m_widget->blockSignals(false);
}

void QuteComboBox::addItem(QString text, double value, QString stringvalue)
{
	m_widget->blockSignals(true);
	static_cast<QComboBox *>(m_widget)->addItem(text, value);
	stringValues.append(stringvalue);
	m_widget->blockSignals(false);
}

void QuteComboBox::refreshWidget()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	int val = (int) m_value;
	m_valueChanged = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	m_widget->blockSignals(true);
	static_cast<QComboBox *>(m_widget)->setCurrentIndex(val);
	m_widget->blockSignals(false);
}

void QuteComboBox::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
    setValue(property("CSQT_selectedIndex").toInt());
}

void QuteComboBox::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
	dialog->setWindowTitle("Menu");
	QLabel *label = new QLabel(dialog);

    auto spacer = new QSpacerItem(20, 20, QSizePolicy::Expanding);
    layout->addItem(spacer, 4, 4, Qt::AlignLeft);

	label = new QLabel(dialog);
    label->setText("Items (comma-separated)");
    layout->addWidget(label, 5, 0, Qt::AlignRight|Qt::AlignVCenter);

	text = new QLineEdit(dialog);
    text->setToolTip("Items in the menu, separated by commas");
    text->setMinimumWidth(420);
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(text, 5,1,1,4, Qt::AlignLeft|Qt::AlignVCenter);
    // text->setSizePolicy()
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	text->setText(itemList());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteComboBox::applyProperties()
{
	setText(text->text()); // This line can't be locked, as itlocks itself internally

	//#ifdef  USE_WIDGET_MUTEX
	//#endif
	//  widgetLock.lockForWrite();
	//  widgetLock.unlock();
	//  setWidgetGeometry(xSpinBox->value(), ySpinBox->value(), wSpinBox->value(), hSpinBox->value());
	QuteWidget::applyProperties();  //Must be last to make sure the widgetChanged signal is last
}


void QuteComboBox::indexChanged(int value)
{
	//  qDebug() << "QuteComboBox::indexChanged" << value;

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	m_value = value;
	m_valueChanged = true;
	QPair<QString, double> channelValue(m_channel, m_value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif

	emit newValue(channelValue);
}
