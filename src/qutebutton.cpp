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

#include "qutebutton.h"

QuteButton::QuteButton(QWidget *parent) : QuteWidget(parent)
{
    m_widget = new QPushButton(this);
    m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    m_currentValue = 0;
    // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setMouseTracking(true);
	setMouseTracking(true);
	canFocus(false);
	//  m_imageFilename = "/";
    connect(static_cast<QPushButton *>(m_widget), SIGNAL(pressed()),
            this, SLOT(buttonPressed()));
    connect(static_cast<QPushButton *>(m_widget), SIGNAL(released()),
            this, SLOT(buttonReleased()));

	setProperty("CSQT_type", "event");
	setProperty("CSQT_pressedValue", 1.0);
	setProperty("CSQT_stringvalue", "");
	setProperty("CSQT_text", "");
	setProperty("CSQT_image", "");
	setProperty("CSQT_eventLine", "");
	setProperty("CSQT_latch", false);
	setProperty("CSQT_momentaryMidiButton", false); // used for latched button if bound to MIDI controller
    setProperty("CSQT_latched", false);
    setProperty("CSQT_fontsize", 10);
    
	QPixmap p = QPixmap(8, 8);
	p.fill(QColor(Qt::green));
	onIcon.addPixmap(p, QIcon::Normal, QIcon::On);
	p.fill(QColor(Qt::black));
	onIcon.addPixmap(p, QIcon::Normal, QIcon::Off);

	m_isPlaying = false; // used for non-latched eventButton to turn the instrument off on second press
}

QuteButton::~QuteButton()
{
}

void QuteButton::setValue(double value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	if (value < 0) {
		m_currentValue = -value;
		m_value = -value;
	} else {
		m_currentValue = value != 0 ? m_value : 0.0;
		if (property("CSQT_latch").toBool()) {
			static_cast<QPushButton *>(m_widget)->setChecked(m_currentValue != 0);
		}
	}
	m_valueChanged = true;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteButton::setValue(QString text)
{
//	qDebug() << "QuteButton::setValue" << text;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	if (m_channel.startsWith("_Browse") ||  m_channel.startsWith("_MBrowse") ) {
		m_stringValue = text;
		m_valueChanged = true;
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

double QuteButton::getValue()
{
	// Returns the value for any button type.
	double value = 0.0;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	if (m_currentValue != 0) {
		value = m_value;
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return value;
}

QString QuteButton::getStringValue()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString stringValue;
	QString name = m_channel;
	if (name.startsWith("_Browse") ||  name.startsWith("_MBrowse") ) {
		stringValue = m_stringValue;
#ifdef  USE_WIDGET_MUTEX
		widgetLock.unlock();
#endif
	}
	else {
#ifdef  USE_WIDGET_MUTEX
		widgetLock.unlock();
#endif
		stringValue =  QString::number(getValue());
	}
	return stringValue;
}

QString QuteButton::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioButton {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line +=  property("CSQT_type").toString()  + " ";
	line +=  QString::number(m_value,'f', 6) + " ";
	line += "\"" + m_channel + "\" ";
    line += "\"" + static_cast<QPushButton *>(m_widget)->text().replace(QRegularExpression("[\n\r]"), "\u00AC") + "\" ";
	line += "\"" + property("CSQT_image").toString() + "\" ";
	line += property("CSQT_eventLine").toString();
	//   qDebug("QuteButton::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteButton::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "button channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	if (property("CSQT_latch").toBool()) {
		line += QString("text(\"%1\", \"%2\"), ").arg(property("CSQT_text").toString() + " OFF").arg(property("CSQT_text").toString() + " ON"); // set different texts for ON/OFF if latced
	} 	else  {
		line += "text(\"" + property("CSQT_text").toString()+ " \"), "; // otherwise just the button text
	}
	line += QString("latched(%1)").arg((int)property("CSQT_latch").toBool());
	if (property("CSQT_midicc").toInt() >= 0 && property("CSQT_midichan").toInt()>0) { // insert only if midi channel is above 0
		line += ", midiCtrl(\"" + QString::number(property("CSQT_midichan").toInt()) + ",";
		line +=  QString::number(property("CSQT_midicc").toInt()) + "\")";
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	//  qDebug() << "Warning: Cabbage does not support button values different than 1, images or event buttons";
	return line;
}

QString QuteButton::getQml()
{
	QString qml = QString();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	qml = "\tButton { \n";
    //qml += QString("\t\tid: %1Button\n").arg(m_channel);
	qml += QString("\t\tx: %1 * scaleItem.scale\n").arg(x());
	qml += QString("\t\ty: %1 * scaleItem.scale\n").arg(y());
	qml += QString("\t\twidth: %1 * scaleItem.scale\n").arg(width());
	qml += QString("\t\theight: %1 * scaleItem.scale\n").arg(height());
	qml += QString("\t\ttext: \"%1\"\n").arg( property("CSQT_text").toString());
	bool checkable = property("CSQT_latch").toBool();
	if (checkable) {
		qml += "\t\tcheckable: true\n";
	}

	qml += QString("\t\tproperty double pressedValue: %1\n").arg(property("CSQT_pressedValue").toDouble()); // to be used for pressing the button.

	QString type = property("CSQT_type").toString();
	qml += QString("\t\tproperty bool isEnventButton: %1\n").arg( (type=="value" ? "false" : "true" )  );
	if (type == "value") {
		qml += QString(R"(
		onPressedChanged: {
            if (pressed) {
				csound.setControlChannel("%1", pressedValue );
			} else {
				csound.setControlChannel("%1", 0 );
			}
		}
				 )").arg(m_channel);
	}


	if (type == "event" || type == "pictevent") {
		QString eventLine = property("CSQT_eventLine").toString();
        QString turnOffLine = QString();
		if (property("CSQT_latch").toBool() && eventLine.size() > 0) {
            QStringList lineElements = eventLine.split(QRegularExpression("\\s"),Qt::SkipEmptyParts);
			if (lineElements.size() > 0 && lineElements[0] == "i") {
				lineElements.removeAt(0); // Remove first element if it is "i"
			}
			else if (lineElements.size() > 0 && lineElements[0][0] == 'i') {
				lineElements[0] = lineElements[0].mid(1); // Remove "i" character
			}

			// this code is necessary to let instruments with line like "i 1 0 -1" to be switched on and off by latched button
			if (lineElements.size() > 2 && lineElements[2].toDouble() < 0) { // If duration is negative, use button to turn note on and off
                if ( lineElements[0].startsWith("\"") || lineElements[0].startsWith("\'")  ) {
                    //qDebug()<<"Stopping named instrument: " << lineElements[0];
                    lineElements[0].insert(1,"-");
                } else {
                    lineElements[0].prepend("-");
                }
                lineElements.prepend("i");

                turnOffLine = lineElements.join(" ");

				qml +=  QString("onCheckedChanged: (checked) ? csound.readScore(\'%1\') : csound.readScore(\'%2\')")
                        .arg(eventLine, turnOffLine); // if unchecked, turnOffLine should consist line to turn off the instrument
			}
		} else { // if not latched, use onClicked event
			qml += QString("\t\tonClicked: csound.readScore(\'%1\') \n").arg(eventLine);
		}
	}


	qml += "\t}\n";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif

	return qml;

}

#define propDouble(prop, decimals) (QString::number(property(prop).toDouble(),'f', decimals))

QString QuteButton::getWidgetXmlText()
{
	// Buttons are not implemented in blue
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	s.writeTextElement("type", property("CSQT_type").toString());
	s.writeTextElement("pressedValue", QString::number(m_value,'f', 8));
	s.writeTextElement("stringvalue", m_stringValue);
	s.writeTextElement("text", property("CSQT_text").toString());
	s.writeTextElement("image", property("CSQT_image").toString());
	s.writeTextElement("eventLine", property("CSQT_eventLine").toString());
	s.writeTextElement("latch", property("CSQT_latch").toString());
	s.writeTextElement("momentaryMidiButton", property("CSQT_momentaryMidiButton").toString());
	s.writeTextElement("latched", property("CSQT_latched").toString());
    s.writeTextElement("fontsize", QString::number(property("CSQT_fontsize").toInt()));
	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

QString QuteButton::getWidgetType()
{
	return QString("BSBButton");
}

void QuteButton::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	QString eventLine = line->text();
    while (eventLine.size() > 0 && eventLine[0] == ' ') {
        // remove all spaces at the beginning. This is needed for event queue lines
        eventLine.remove(0,1);
	}
	setProperty("CSQT_eventLine", eventLine);
    setProperty("CSQT_text", text->toPlainText());
	setProperty("CSQT_image", filenameLineEdit->text());
	setProperty("CSQT_type", typeComboBox->currentText());
	setProperty("CSQT_pressedValue", valueBox->value());
	setProperty("CSQT_latch", latchCheckBox->isChecked());
	setProperty("CSQT_momentaryMidiButton", useMomentaryMidiButtonCheckBox->isChecked());
    setProperty("CSQT_fontsize", fontSizeSpinBox->value());

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    //Must be last to make sure the widgetChanged signal is last
    QuteWidget::applyProperties();
    //  qDebug() << "QuteButton::applyProperties()" << m_value;
}


void QuteButton::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	dialog->setWindowTitle("Button");

	QLabel *label = new QLabel(dialog);
	label->setText("Type");
	layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);
	typeComboBox = new QComboBox(dialog);
	typeComboBox->addItem("event");
	typeComboBox->addItem("value");
	typeComboBox->addItem("pictevent");
	typeComboBox->addItem("pictvalue");
	typeComboBox->addItem("pict");
	typeComboBox->setCurrentIndex(typeComboBox->findText(property("CSQT_type").toString()));
	layout->addWidget(typeComboBox, 4, 1, Qt::AlignLeft|Qt::AlignVCenter);

	latchCheckBox = new QCheckBox(dialog);
	latchCheckBox->setText(tr("Latch"));
	layout->addWidget(latchCheckBox, 5, 1, 1,2, Qt::AlignLeft|Qt::AlignVCenter);
	latchCheckBox->setChecked(property("CSQT_latch").toBool());
	label = new QLabel(dialog);
	label->setText("Value");
	layout->addWidget(label, 4, 2, Qt::AlignRight|Qt::AlignVCenter);

    valueBox = new QDoubleSpinBox(dialog);
	valueBox->setDecimals(6);
	valueBox->setRange(-9999999.0, 9999999.0);
	valueBox->setValue(m_value);
	valueBox->setMaximumWidth(100);
	valueBox->setDecimals(4);
	layout->addWidget(valueBox, 4, 3, Qt::AlignLeft|Qt::AlignVCenter);


    label = new QLabel(dialog);
	label->setText("Text:");
	layout->addWidget(label, 6, 0, Qt::AlignRight|Qt::AlignVCenter);

    text = new QTextEdit(dialog);
	text->setMinimumWidth(320);
	text->setText(property("CSQT_text").toString());
    layout->addWidget(text, 6, 1, 1, 3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText(tr("Font Size"));
    layout->addWidget(label, 7, 0, Qt::AlignRight|Qt::AlignVCenter);

    fontSizeSpinBox = new QSpinBox(dialog);
    fontSizeSpinBox->unsetLocale();
    fontSizeSpinBox->setMinimum(6);
    fontSizeSpinBox->setMaximum(999);
    fontSizeSpinBox->setValue(property("CSQT_fontsize").toInt());
    layout->addWidget(fontSizeSpinBox, 7, 1, Qt::AlignLeft|Qt::AlignVCenter);

	label = new QLabel(dialog);
	label->setText("Image:");
    layout->addWidget(label, 8, 0, Qt::AlignRight|Qt::AlignVCenter);
	filenameLineEdit = new QLineEdit(dialog);
	filenameLineEdit->setMinimumWidth(320);
	filenameLineEdit->setText(property("CSQT_image").toString());
    layout->addWidget(filenameLineEdit, 8, 1, 1, 3, Qt::AlignLeft|Qt::AlignVCenter);

	QPushButton *browseButton = new QPushButton(dialog);
    browseButton->setText("Browse");
	layout->addWidget(browseButton, 8, 4, Qt::AlignLeft|Qt::AlignVCenter);
	connect(browseButton, SIGNAL(released()), this, SLOT(browseFile()));

	label = new QLabel(dialog);
	label->setText("Event:");
    layout->addWidget(label, 9, 0, Qt::AlignRight|Qt::AlignVCenter);
	line = new QLineEdit(dialog);
	//   text->setText(((QuteLabel *)m_widget)->toPlainText());
    layout->addWidget(line, 9,1,1,3, Qt::AlignLeft|Qt::AlignVCenter);
	line->setMinimumWidth(320);
	line->setText(property("CSQT_eventLine").toString());

	useMomentaryMidiButtonCheckBox = new QCheckBox(dialog);
	useMomentaryMidiButtonCheckBox->setText(tr("Momentary"));
	useMomentaryMidiButtonCheckBox->setWhatsThis(tr("Check if you use MIDI push button (momentary button) to toggle the latch - \nFirst push switches on, second off."));
	// TODO: enabling/disabling needs some signal->slot connection
	//useMomentaryMidiButtonCheckBox->setEnabled( latchCheckBox->isChecked() );
	useMomentaryMidiButtonCheckBox->setChecked(property("CSQT_momentaryMidiButton").toBool());

	const int midiRow = layout->rowCount()-2;
	layout->removeWidget(midiLearnButton);
	layout->addWidget(useMomentaryMidiButtonCheckBox, midiRow, 4, Qt::AlignLeft|Qt::AlignVCenter);
	layout->addWidget(midiLearnButton, midiRow, 5, Qt::AlignLeft|Qt::AlignVCenter);



#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteButton::setText(QString text)
{
	// For old widget format conversion of line endings
	setProperty("CSQT_text", text);
    static_cast<QPushButton *>(m_widget)->setText(text);
}

void QuteButton::popUpMenu(QPoint pos)
{
	QuteWidget::popUpMenu(pos);
}

void QuteButton::setMidiValue(int value)
{
	double pressedValue = property("CSQT_pressedValue").toDouble();
	double newValue = 0;

	bool isLatch = property("CSQT_latch").toBool();
	bool useMomentaryMidiButton = property("CSQT_momentaryMidiButton").toBool();
    QString type = property("CSQT_type").toString();

    qDebug () << "Playing: " << m_isPlaying << type;

	if (isLatch && useMomentaryMidiButton) {
			if (value >0 ) {
                if ( (type.contains("event") && m_isPlaying) ||  (type.contains("value") && m_currentValue > 0) ) {
                    qDebug() << "Toggle value to 0 / playing off from MIDI";
					newValue = 0;
				} else {
					newValue = pressedValue;
                    qDebug() << "Toggle value 1 / playing on from MIDI";
				}
			} else {
				qDebug() << "Ignore button release of momentary button";
				return;
			}

	} else {
		newValue = (value > 0) ? pressedValue : 0;
	}

	setValue(newValue);
	performAction();

}

void QuteButton::refreshWidget()
{
	// setValue sets the value the widget outputs while it is pressed
	//  static_cast<QPushButton *>(m_widget)->setChecked(m_value);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	//  setProperty("CSQT_value", m_value);
	//  setProperty("CSQT_stringvalue", m_stringValue);

	setProperty("CSQT_latched", m_currentValue != 0);
	m_valueChanged = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}


void QuteButton::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
    //qDebug() << "QuteButton::applyInternalProperties";
    m_value = property("CSQT_pressedValue").toDouble();
	//  m_value2 = property("CSQT_value2").toDouble();
	m_stringValue = property("CSQT_stringvalue").toString();
	QString type = property("CSQT_type").toString();
    auto w = static_cast<QPushButton*>(m_widget);
    w->setCheckable(property("CSQT_latch").toBool());
    // Set icon here, because it can be overwritten if button is "pict"
    if (property("CSQT_latch").toBool()) {
        w->setIcon(onIcon);
    } else {
        w->setIcon(QIcon());
    }

    if (type == "event" || type == "value") {
        icon = QIcon();
		static_cast<QPushButton *>(m_widget)->setIcon(icon);
        auto fontsizeProperty = property("CSQT_fontsize");
        if(!fontsizeProperty.isValid()) {
            qDebug() << "Button: fontsize invalid / not present. Setting to default";
        } else {
            int fontsize = fontsizeProperty.toInt();
            if(fontsize <= 0)
                qDebug() << "Invalid font size for button, skipping";
            else {

                auto sheet = QString("QPushButton {font-size: %1pt; }").arg(fontsize);
                w->setStyleSheet(sheet);
            }
        }
        w->setText(property("CSQT_text").toString());

    } else if (type == "pictevent" || type == "pictvalue" || type == "pict") {
        qDebug() << "///////////////////////////";
        w->setStyleSheet(nullptr);
        w->setText("");
		icon = QIcon(QPixmap(property("CSQT_image").toString()));
		static_cast<QPushButton *>(m_widget)->setIcon(icon);
		static_cast<QPushButton *>(m_widget)->setIconSize(QSize(width(),height()));
    } else {
        qDebug() << "Warning! QuteButton::applyInternalProperties() unrecognized type:"
                 << type;
	}
}


void QuteButton::performAction() {
    QString type = property("CSQT_type").toString();
    QString eventLine = property("CSQT_eventLine").toString();
    QString name = m_channel;
	bool isLatch = property("CSQT_latch").toBool();
    //bool useMomentaryMidiButton = property("CSQT_momentaryMidiButton").toBool();

	if (type.contains("event") && !eventLine.isEmpty()) {
		if ( hasIndefiniteDuration() ) {		
            if ( m_currentValue == 0 ) { // turn off
                QStringList lineElements = eventLine.split(QRegularExpression("\\s"),SKIP_EMPTY_PARTS);
				if (lineElements.size() > 0 && lineElements[0] == "i") {
					lineElements.removeAt(0); // Remove first element if it is "i"
				}
				else if (lineElements.size() > 0 && lineElements[0][0] == 'i') {
					lineElements[0] = lineElements[0].mid(1); // Remove "i" character
				}
				if ( lineElements[0].startsWith("\"") || lineElements[0].startsWith("\'")  ) {
					//qDebug()<<"Stopping named instrument: " << lineElements[0];
					lineElements[0].insert(1,"-");
				}
				else {
					lineElements[0].prepend("-");
				}
				lineElements.prepend("i");
				setValue(0);
				m_isPlaying = false;
				emit(queueEventSignal(lineElements.join(" ")));
			} else {
				setValue( property("CSQT_pressedValue").toDouble()  ); // was 1
				m_isPlaying = true;
				emit(queueEventSignal(eventLine));
			}
		} else { // if not negative p3 then just fire the event
			if (!isLatch && m_currentValue>0) { //do not fire the event if latched && m_value==0 && is positive p3
				emit(queueEventSignal(eventLine));
			}
		}
    }
    else if (type == "value" || type == "pictvalue") {
        if(!name.startsWith("_")) {
            emit newValue(QPair<QString, double>(name, m_currentValue));
        }
        else if (name == "_Play") {
            if(m_value == 0)
                emit stop();
            else
                emit play();
        }
        else if (name == "_Stop")
            emit stop();
        else if (name == "_Pause")
            emit pause();
        else if (name == "_Render")
            emit render();
        else if (name.startsWith("_Browse")) {
            QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"));
            if (fileName != "") {
                setProperty("CSQT_stringvalue", fileName);
                emit newValue(QPair<QString, QString>(name, fileName));
            }
        }
        else if (name.startsWith("_MBrowse")) {
            // Browse multiple files
            QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select File(s)"));
            if (!fileNames.isEmpty()) {
                QString joinedNames = fileNames.join("|");
                setProperty("CSQT_stringvalue", joinedNames);
                emit newValue(QPair<QString, QString>(name, joinedNames));
            }
        }
        else {
            qDebug() << "Warning: Channel names starting with _ are reserved. This will "
                        "be an error in a next release";
            emit newValue(QPair<QString, double>(name, m_currentValue));
        }
	}
}

bool QuteButton::hasIndefiniteDuration()
{
	QString eventLine = property("CSQT_eventLine").toString();
	if ( !eventLine.isEmpty()) {
        QStringList lineElements = eventLine.split(QRegularExpression("\\s"),Qt::SkipEmptyParts);
		if (lineElements.size() > 0 && lineElements[0] == "i") {
			lineElements.removeAt(0); // Remove first element if it is "i"
		}
		else if (lineElements.size() > 0 && lineElements[0][0] == 'i') {
			lineElements[0] = lineElements[0].mid(1); // Remove "i" character
		}
		// If duration is negative, use button to turn note on and off
		if (lineElements.size() > 2 && lineElements[2].toDouble() < 0) {
			return true;
		}
	}
	return false;
}

void QuteButton::buttonPressed()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
    // open file browser on release
    if (m_channel.startsWith("_Browse") || m_channel.startsWith("_MBrowse")) {
        return;
    }
    auto w = static_cast<QPushButton *>(m_widget);
    if (property("CSQT_latch").toBool()) {
        m_currentValue = !w->isChecked() ? m_value : 0;
    } else {
		m_currentValue = m_value;
    }



#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    performAction();
}

void QuteButton::buttonReleased()
{
	// Only produce events for event types
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
    if (m_channel.startsWith("_Browse") || m_channel.startsWith("_MBrowse")) {
        performAction();
        return;
    }
	bool isLatch = property("CSQT_latch").toBool();

	if (!isLatch ) {
        m_currentValue = 0;
		if (  property("CSQT_type").toString().contains("event") && hasIndefiniteDuration() ) {
			performAction(); // to stop the playing instrument
		}
	}


#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    emit newValue(QPair<QString, double>(m_channel, m_currentValue));

}

void QuteButton::browseFile()
{
    QString file =  QFileDialog::getOpenFileName(this,tr("Select File"));
	if (file!="") {
		filenameLineEdit->setText(file);
	}
}
