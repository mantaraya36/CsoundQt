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

#include "qutetext.h"
#include <math.h>

#define USEFONTPIXELSIZE

QuteText::QuteText(QWidget *parent) : QuteWidget(parent)
{
	m_value = 0.0;
	m_widget = new QLabel(this);
	static_cast<QLabel*>(m_widget)->setWordWrap (true);
	static_cast<QLabel*>(m_widget)->setMargin (5);
	//  static_cast<QLabel*>(m_widget)->setTextFormat(Qt::RichText);
	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setMouseTracking(true);
    setMouseTracking(true);

    //  canFocus(true);

	//   connect(static_cast<QLabel*>(m_widget), SIGNAL(popUpMenu(QPoint)), this, SLOT(popUpMenu(QPoint)));
	setProperty("CSQT_label", "");
	setProperty("CSQT_alignment", "left");
    setProperty("CSQT_valignment", "top");
    setProperty("CSQT_font", "Arial");
	setProperty("CSQT_fontsize", 12.0);
	setProperty("CSQT_bgcolor", QColor());
	setProperty("CSQT_bgcolormode", false);
	setProperty("CSQT_color", QColor(Qt::black));
	setProperty("CSQT_bordermode", "noborder");
	setProperty("CSQT_borderradius", 1);
	setProperty("CSQT_borderwidth", 1);

	m_fontScaling = 1.0;
	m_fontOffset = 1.0;
	m_type = "display";
    m_precision = 3;
    setProperty("CSQT_precision", m_precision);

}

QuteText::~QuteText()
{
}

void QuteText::setValue(double value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	m_value = value;
    m_stringValue = QString::number(value, 'f', m_precision);
    m_valueChanged = true;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteText::setValue(QString value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	//  setText(value);
	m_stringValue = value;
	m_value = value.toDouble();
	m_valueChanged = true;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteText::setType(QString type)
{
	m_type = type;
}

void QuteText::setTransparentForMouse(bool status)
{
    m_widget->setAttribute(Qt::WA_TransparentForMouseEvents, status);
    setAttribute(Qt::WA_TransparentForMouseEvents, status);
}

void QuteText::setAlignment(QString alignment)
{
    Qt::Alignment align;
	if (alignment == "left") {
		align = Qt::AlignLeft|Qt::AlignTop;
	}
	else if (alignment == "center") {
		align = Qt::AlignHCenter|Qt::AlignTop;
	}
	else if (alignment == "right") {
		align = Qt::AlignRight|Qt::AlignTop;
	}
	else {
		align = Qt::AlignLeft|Qt::AlignTop;
	}
	static_cast<QLabel* >(m_widget)->setAlignment(align);
}

void QuteText::setFont(QString font)
{
	setProperty("CSQT_font", font);
}

void QuteText::setFontSize(int fontSize)
{
	qDebug() << "QuteText::setFontSize(int fontSize) " << fontSize;
	setProperty("CSQT_fontsize", fontSize);
}


void QuteText::setTextColor(QColor color)
{
	// For old format only
    setProperty("CSQT_color", color);
	QPalette palette = m_widget->palette();
    palette.setColor(QPalette::WindowText, color);
	m_widget->setPalette(palette);
}

void QuteText::setBgColor(QColor color)
{
	// For old format only
    setProperty("CSQT_bgcolor", color);
	QPalette palette = m_widget->palette();
    palette.setColor(QPalette::Window, color);
	m_widget->setPalette(palette);
}

void QuteText::setBg(bool bg)
{
	// For old format only
	m_widget->setAutoFillBackground(bg);
}

void QuteText::setBorder(bool border)
{
	if (border)
		static_cast<QFrame*>(m_widget)->setFrameShape(QFrame::Box);
	else
		static_cast<QFrame*>(m_widget)->setFrameShape(QFrame::NoFrame);
}

void QuteText::setText(QString text)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif

	setProperty("CSQT_label", text);
	QString displayText = text;
	m_stringValue = text;
	m_valueChanged = true;
	displayText.replace("\n", "<br />");
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif

	m_widget->blockSignals(true);
	static_cast<QLabel*>(m_widget)->setText(displayText);
	m_widget->blockSignals(false);
}

void QuteText::refreshWidget()
{
	setText(m_stringValue);
	m_valueChanged = false;
}

void QuteText::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
    static_cast<QLabel*>(m_widget)->setText(property("CSQT_label").toString());
    m_precision = property("CSQT_precision").toInt();
    m_stringValue = property("CSQT_label").toString();
	m_value = m_stringValue.toDouble();
	m_valueChanged = true;

    Qt::Alignment align;
    QString horizontalAlignment = property("CSQT_alignment").toString();
    QString verticalAlignment = property("CSQT_valignment").toString();

    if(verticalAlignment == "top")
        align = Qt::AlignTop;
    else if(verticalAlignment == "center")
        align = Qt::AlignVCenter;
    else
        align= Qt::AlignBottom;

    if(horizontalAlignment == "left")
        align |= Qt::AlignLeft;
    else if(horizontalAlignment == "center")
        align |= Qt::AlignHCenter;
    else {
        align |= Qt::AlignRight;
    }
	static_cast<QLabel*>(m_widget)->setAlignment(align);
	setTextColor(property("CSQT_color").value<QColor>());
    int borderWidth = property("CSQT_borderwidth").toInt();
    QString bordermode = property("CSQT_bordermode").toString();
    if(bordermode == "noborder" && borderWidth > 0) {
        setProperty("CSQT_borderwidth", 0);
        borderWidth = 0;
    } else if(bordermode == "border" && borderWidth == 0) {
        setProperty("CSQT_bordermode", "noborder");
    }
    QString borderStyle = borderWidth > 0 ? "solid" : "none";

    double scaledFontSize = property("CSQT_fontsize").toDouble() * m_fontScaling;
    double fontSize = scaledFontSize + m_fontOffset;

    /*
	while (totalHeight < fontSize + 1) {
        new_fontSize += 2;
        font.setPointSize(new_fontSize);
        // QFont font(property("CSQT_font").toString(), new_fontSize);
		QFontMetricsF fm(font);
        totalHeight = fm.ascent() + fm.descent();
	}
    */

    QString bgstr = property("CSQT_bgcolormode").toBool() ?
        (QString("; background-color:")+property("CSQT_bgcolor").value<QColor>().name()) :
        QString("");

    m_widget->setStyleSheet(
        "QLabel{ font-family:\"" + property("CSQT_font").toString() + "\""
            // + "; font-size: " + QString::number(new_fontSize) + "pt"
            + "; font-size: " + QString::number(fontSize) + "px"
            + bgstr
            + "; color:" + property("CSQT_color").value<QColor>().name()
            + "; border-color:" + property("CSQT_color").value<QColor>().name()
            + "; border-radius:" + QString::number(property("CSQT_borderradius").toInt()) + "px"
            + "; border-width: " + QString::number(borderWidth) + "px"
            + "; border-style: " + borderStyle
            + "; }");

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteText::setFontScaling(double scaling)
{
	m_fontScaling = scaling;
	//  applyInternalProperties();
}

void QuteText::setFontOffset(double offset)
{
	m_fontOffset = offset;
	//  applyInternalProperties();
}

QString QuteText::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioText {" + QString::number(property("CSQT_x").toInt()) + ", " + QString::number(property("CSQT_y").toInt()) + "} ";
	line += "{"+ QString::number(property("CSQT_width").toInt()) +", "+ QString::number(property("CSQT_height").toInt()) +"} ";
	line += m_type + " ";
	line += QString::number(m_value, 'f', 6) + " 0.00100 \"" + m_channel + "\" ";
	QString alignment = "";
	if (((QLabel *)m_widget)->alignment() & Qt::AlignLeft)
		alignment = "left";
	else if (((QLabel *)m_widget)->alignment() & Qt::AlignCenter)
		alignment = "center";
	else if (((QLabel *)m_widget)->alignment() & Qt::AlignRight)
		alignment = "right";
	line += alignment + " ";
	line += "\"" + property("CSQT_font").toString() + "\" " + QString::number(property("CSQT_fontsize").toInt()) + " ";
	QColor color = m_widget->palette().color(QPalette::WindowText);
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	color = m_widget->palette().color(QPalette::Window);
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	line += m_widget->autoFillBackground()? "background ":"nobackground ";
	line += static_cast<QFrame*>(m_widget)->frameShape()==QFrame::NoFrame ? "noborder ": "border ";
	//   line += ((QLabel *)m_widget)->toPlainText();
	QString outText = property("CSQT_label").toString();
    outText.replace(QRegularExpression("[\n\r]"), "\u00AC");
	line += outText;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteText::getWidgetType()
{
	QString type = (m_type=="label" ? "BSBLabel" : "BSBDisplay");
	return type;
}

QString QuteText::getCabbageLine() // QuteText is used both for label and display. Turn one into Cabbage label, other numberbox
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif

	QString line = (m_type == "label") ? "label " : "numberbox channel(\"" + m_channel + "\"),  ";;
	line += "bounds(" + QString::number(x()) + ", " + QString::number(y()) + ","  + QString::number(width()) +", "+ QString::number(height()) + "), ";
	QString alignment = property("CSQT_alignment").toString();
	alignment.replace("center","centre");
	line += "align(\"" + alignment + "\"), ";
	QColor color = property("CSQT_color").value<QColor>();
	line += "fontcolour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + "), ";
	color = property("CSQT_bgcolor").value<QColor>();
	line += "colour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + "), ";
    // Cabbage does not set font or fontsize.
    // Text is scaled according to heigth. Maybe set heigth = fontsize + something?
	if ( m_type == "label" ) 	{ // then it is a label
		line += "text(\"" + property("CSQT_label").toString() + "\") " ;
	} else { // display
        line += QString("range(-1000000000000,1000000000000,%1), ").arg(m_value); // set redicolously large min and max value and hope the user will not use larger numbers...
		line += "active(0)";
	}
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    return line;
}

QString QuteText::getQml()
{
    QString qml = QString();
#ifdef  USE_WIDGET_MUTEX
    widgetLock.lockForWrite();
#endif

    if (m_type == "label" || m_type == "display") {

        // check if it has background and border
        bool hasBackground = property("CSQT_bgcolormode").toBool();
        bool hasBorder = (property("CSQT_bordermode").toString() == "border");
        QString color = property("CSQT_color").value<QColor>().name();

        qml += "\n\tRectangle {\n"; // place within rectangle
        qml += QString("\t\tx: %1 * scaleItem.scale\n").arg(x());
        qml += QString("\t\ty: %1  * scaleItem.scale\n").arg(y());
        qml += QString("\t\twidth: %1 * scaleItem.scale\n").arg(width());
        qml += QString("\t\theight: %1 * scaleItem.scale\n").arg(height());
        QString bgColor = hasBackground ? property("CSQT_bgcolor").value<QColor>().name() : "transparent";
        qml += QString("\t\tcolor: \"%1\"\n").arg(bgColor);
        if (hasBorder) {
            qml += QString("\t\tborder.width: %1\n").arg(QString::number(property("CSQT_borderwidth").toInt()));
            qml += QString("\t\tradius: %1\n").arg(QString::number(property("CSQT_borderradius").toInt()));
            qml += QString("\t\tborder.color: \"%1\"\n").arg(color);
        } else {
            qml += "\n\tborder.width: 0";
        }

        qml += "\n\t\tLabel {\n";

        if (m_type == "display" && !m_channel.isEmpty()) {
            qml += QString("\t\t\tid: "+m_channel + "\n");    // display has a channel ad mark it as ID
        }

        qml += QString("\t\t\tanchors.centerIn: parent\n");
        qml += QString("\t\t\tfont.pixelSize: %1 * scaleItem.scale\n").arg(property("CSQT_fontsize").toString()); // is it OK on ndroid? not pointsize?
        qml += QString("\t\t\ttext: \"%1\"\n").arg(property("CSQT_label").toString());

        qml += QString("\t\t\tcolor: \"%1\"\n").arg(color);
        qml += QString("\t\tfont.family: \"%1\"\n").arg(property("CSQT_font").toString());
        // TODO: alignment
        qml += "\t\t}\n"; // end label
        qml += "\t}\n"; // end rectangle
    }
#ifdef  USE_WIDGET_MUTEX
    widgetLock.unlock();
#endif

    return qml;

}


QString QuteText::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	s.writeTextElement("label", property("CSQT_label").toString());
	s.writeTextElement("alignment", property("CSQT_alignment").toString());
    s.writeTextElement("valignment", property("CSQT_valignment").toString());


	s.writeTextElement("font", property("CSQT_font").toString());
	s.writeTextElement("fontsize", QString::number(property("CSQT_fontsize").toInt()));
	s.writeTextElement("precision", QString::number(property("CSQT_precision").toInt()));

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

	s.writeTextElement("bordermode", property("CSQT_bordermode").toString());
	s.writeTextElement("borderradius", QString::number(property("CSQT_borderradius").toInt()));
	s.writeTextElement("borderwidth", QString::number(property("CSQT_borderwidth").toInt()));
	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

void QuteText::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
	if (m_type == "label") {
		dialog->setWindowTitle("Label");
		channelLabel->hide();
		nameLineEdit->hide();
	}
	else if (m_type == "display") {
		dialog->setWindowTitle("Display");
	}

    labelPtrs.clear();

	QLabel *label = new QLabel(dialog);
	label->setText(tr("Text:"));
	layout->addWidget(label, 5, 0, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["text"] = label;

	text = new QTextEdit(dialog);
	text->setAcceptRichText(false);
	text->setText(property("CSQT_label").toString());
    text->setMinimumWidth(320);
    text->setMaximumWidth(740);
    layout->addWidget(text, 5, 1, 1, 4, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
	label->setText(tr("Text Color"));
	layout->addWidget(label, 6, 0, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["textColor"] = label;

    textColor = new SelectColorButton(dialog);
    textColor->setColor(property("CSQT_color").value<QColor>());
    layout->addWidget(textColor, 6,1, Qt::AlignLeft|Qt::AlignVCenter);
    // connect(textColor, SIGNAL(released()), this, SLOT(selectTextColor()));

    bg = new QCheckBox("Background", dialog);
    layout->addWidget(bg, 6, 2, Qt::AlignRight|Qt::AlignVCenter);
    bgColor = new SelectColorButton(dialog);
    bgColor->setColor(property("CSQT_bgcolor").value<QColor>());
    layout->addWidget(bgColor, 6,3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel("Precision", dialog);
    layout->addWidget(label, 7, 0, Qt::AlignRight|Qt::AlignVCenter);
    precisionSpinBox = new QSpinBox(dialog);
    precisionSpinBox->unsetLocale();
    precisionSpinBox->setValue(property("CSQT_precision").toInt());
    layout->addWidget(precisionSpinBox, 7, 1, Qt::AlignLeft|Qt::AlignVCenter);



    // border = new QCheckBox("Border", dialog);
    // layout->addWidget(border, 7,2, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
	label->setText(tr("Font"));
    layout->addWidget(label, 8, 0, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["font"] = label;

    font = new QFontComboBox(dialog);
    font->setMaximumWidth(200);
    layout->addWidget(font, 8, 1, 1, 2, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
	label->setText(tr("Font Size"));
    layout->addWidget(label, 9, 0, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["fontSize"] = label;

    fontSize = new QSpinBox(dialog);
    fontSize->unsetLocale();
	fontSize->setMaximum(999); // allow also very big fonts
    layout->addWidget(fontSize,9, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
	label->setText(tr("Border Radius"));
    layout->addWidget(label, 9, 2, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["borderRadius"] = label;

    borderRadius = new QSpinBox(dialog);
    borderRadius->unsetLocale();
    layout->addWidget(borderRadius, 9, 3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
	label->setText(tr("Border Width"));
    layout->addWidget(label, 10, 2, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["borderWidth"] = label;

    borderWidth = new QSpinBox(dialog);
    borderWidth->unsetLocale();
    borderWidth->setMinimum(0);
    borderWidth->setToolTip(tr("Set the width to 0 disable the border"));
    layout->addWidget(borderWidth, 10, 3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText(tr("Horiz. Align"));
    layout->addWidget(label, 10, 0, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["horizAlign"] = label;

    alignment = new QComboBox(dialog);
	alignment->addItem(tr("Left", "Alignment"));
	alignment->addItem(tr("Center", "Alignment"));
	alignment->addItem(tr("Right", "Alignment"));
    layout->addWidget(alignment, 10, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(dialog);
    label->setText(tr("Vert. Align"));
    layout->addWidget(label, 11, 0, Qt::AlignRight|Qt::AlignVCenter);
    labelPtrs["vertAlign"] = label;

    vertAlignmentComboBox = new QComboBox(dialog);
    vertAlignmentComboBox->addItem(tr("Top", "Alignment"));
    vertAlignmentComboBox->addItem(tr("Center", "Alignment"));
    vertAlignmentComboBox->addItem(tr("Bottom", "Alignment"));
    layout->addWidget(vertAlignmentComboBox, 11, 1, Qt::AlignLeft|Qt::AlignVCenter);

    // connect(bgColor, SIGNAL(released()), this, SLOT(selectBgColor()));
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
    // QPixmap pixmap(64,64);
    // QPalette palette(property("CSQT_color").value<QColor>());
    /*
    pixmap.fill(property("CSQT_color").value<QColor>());
	textColor->setIcon(pixmap);
    textColor->setPalette(palette);
	palette.color(QPalette::Window);
    */
    /*
	pixmap.fill(property("CSQT_bgcolor").value<QColor>());
	bgColor->setIcon(pixmap);
	palette = QPalette(property("CSQT_bgcolor").value<QColor>());
	bgColor->setPalette(palette);
    palette.color(QPalette::Window);
    */
    bg->setChecked(property("CSQT_bgcolormode").toBool());
    // border->setChecked(property("CSQT_bordermode").toString() == "border");
	font->setCurrentFont(QFont(property("CSQT_font").toString()));
	fontSize->setValue(property("CSQT_fontsize").toInt());
	borderRadius->setValue(property("CSQT_borderradius").toInt());
	borderWidth->setValue(property("CSQT_borderwidth").toInt());

    QString currentAlignment = property("CSQT_alignment").toString();
    if (currentAlignment == "left")
        alignment->setCurrentIndex(0);
    else if (currentAlignment == "center")
        alignment->setCurrentIndex(1);
    else if (currentAlignment == "right")
        alignment->setCurrentIndex(2);
    else {
        qDebug() << "QuteText. Unknown alignment, setting to left";
        alignment->setCurrentIndex(0);
    }

    currentAlignment = property("CSQT_valignment").toString();
    if (currentAlignment == "top")
        vertAlignmentComboBox->setCurrentIndex(0);
    else if (currentAlignment == "center")
        vertAlignmentComboBox->setCurrentIndex(1);
    else if(currentAlignment == "bottom")
        vertAlignmentComboBox->setCurrentIndex(2);
    else
        vertAlignmentComboBox->setCurrentIndex(0);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteText::applyProperties()
{
//	qDebug() << "QuteText::applyProperties()";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	setProperty("CSQT_label", text->toPlainText());

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

    switch (vertAlignmentComboBox->currentIndex()) {
    case 0:
        setProperty("CSQT_valignment", "top");
        break;
    case 1:
        setProperty("CSQT_valignment", "center");
        break;
    case 2:
        setProperty("CSQT_valignment", "bottom");
        break;
    default:
        setProperty("CSQT_valignment", "");
    }

    setProperty("CSQT_font", font->currentFont().family());
	setProperty("CSQT_fontsize", fontSize->value());
    setProperty("CSQT_bgcolor", bgColor->getColor());
    setProperty("CSQT_bgcolormode", bg->isChecked());
    // setProperty("CSQT_color", textColor->palette().color(QPalette::Window));
    setProperty("CSQT_color", textColor->getColor());
    setProperty("CSQT_bordermode", borderWidth->value() > 0);
    setProperty("CSQT_borderradius", borderRadius->value());
	setProperty("CSQT_borderwidth", borderWidth->value());
    m_precision = precisionSpinBox->value();
    setProperty("CSQT_precision", m_precision);


#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	QuteWidget::applyProperties();  //Must be last to make sure the widgetChanged signal is last
}

/*
void QuteText::selectTextColor()
{
	QColor color = QColorDialog::getColor(m_widget->palette().color(QPalette::WindowText), this);
	if (color.isValid()) {
		//    setTextColor(color);
		//    setProperty("CSQT_color", color);
		textColor->setPalette(QPalette(color));
		QPixmap pixmap(64,64);
		pixmap.fill(color);
		textColor->setIcon(pixmap);
	}
}
*/

/*
void QuteText::selectBgColor()
{
	QColor color = QColorDialog::getColor(m_widget->palette().color(QPalette::Window), this);
	if (color.isValid()) {
		//    setBgColor(color);
		//    setProperty("CSQT_bgcolor", color);
		bgColor->setPalette(QPalette(color));
		QPixmap pixmap(64,64);
		pixmap.fill(color);
		bgColor->setIcon(pixmap);
	}
}
*/

/* -----------------------------------------------------------------*/
/*               QuteLineEdit class                                 */
/* -----------------------------------------------------------------*/

QuteLineEdit::QuteLineEdit(QWidget* parent) : QuteText(parent)
{
	delete m_widget; //delete widget created by parent constructor
	m_widget = new QLineEdit(this);
	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
	connect(static_cast<QLineEdit*>(m_widget), SIGNAL(textEdited(QString)),
			this, SLOT(textEdited(QString)));
	//   connect(static_cast<QLineEdit*>(m_widget), SIGNAL(popUpMenu(QPoint)), this, SLOT(popUpMenu(QPoint)));
	m_type = "edit";
	setProperty("CSQT_bordermode", QVariant()); // Remove these property
	setProperty("CSQT_borderradius", QVariant()); // Remove these property
	setProperty("CSQT_borderwidth", QVariant()); // Remove these property

    // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setMouseTracking(true);
    setMouseTracking(true);

    m_widget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

}

QuteLineEdit::~QuteLineEdit()
{
}

void QuteLineEdit::setText(QString text)
{
	setProperty("CSQT_label", text);
	int cursorPos = static_cast<QLineEdit*>(m_widget)->cursorPosition();
	m_widget->blockSignals(true);
	static_cast<QLineEdit*>(m_widget)->setText(text);
	m_widget->blockSignals(false);
	static_cast<QLineEdit*>(m_widget)->setCursorPosition(cursorPos);
}

QString QuteLineEdit::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioText {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line += m_type + " ";
	line += QString::number(m_value, 'f', 6) + " 0.00100 \"" + m_channel + "\" ";
	line += property("CSQT_alignmet").toString() + " ";
	line += "\"" + property("CSQT_font").toString() + "\" " + QString::number(property("CSQT_fontsize").toInt()) + " ";
	QColor color = property("CSQT_color").value<QColor>();
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	color = property("CSQT_bgcolor").value<QColor>();
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	line += property("CSQT_bgcolormode").toBool() ? "true":"false";
	line += "noborder ";
	line += static_cast<QLineEdit*>(m_widget)->text();
	//   qDebug("QuteLineEdit::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteLineEdit::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	s.writeTextElement("label",  static_cast<QLineEdit *>(m_widget)->text());
	s.writeTextElement("alignment", property("CSQT_alignment").toString());

	s.writeTextElement("font", property("CSQT_font").toString());
	s.writeTextElement("fontsize", QString::number(property("CSQT_fontsize").toInt()));
	s.writeTextElement("precision", QString::number(property("CSQT_precision").toInt()));

	QColor color = m_widget->palette().color(QPalette::WindowText);
	s.writeStartElement("color");
	s.writeTextElement("r", QString::number(color.red()));
	s.writeTextElement("g", QString::number(color.green()));
	s.writeTextElement("b", QString::number(color.blue()));
	s.writeEndElement();
	color = m_widget->palette().color(QPalette::Window);
	s.writeStartElement("bgcolor");
	s.writeAttribute("mode", m_widget->autoFillBackground()? "background":"nobackground");
	s.writeTextElement("r", QString::number(color.red()));
	s.writeTextElement("g", QString::number(color.green()));
	s.writeTextElement("b", QString::number(color.blue()));
	s.writeEndElement();

	s.writeTextElement("background", m_widget->autoFillBackground()? "background":"nobackground");
	//  s.writeTextElement("border", "border");
	//
	//  s.writeTextElement("bordermode", property("CSQT_bordermode").toString());
	//  s.writeTextElement("borderradius", QString::number(property("CSQT_borderradius").toInt()));
	//  s.writeTextElement("randomizable", "");
	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

QString QuteLineEdit::getWidgetType()
{
	return QString("BSBLineEdit");
}

QString QuteLineEdit::getStringValue()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString stringValue = static_cast<QLineEdit *>(m_widget)->text();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return stringValue;
}

double QuteLineEdit::getValue()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	double value = static_cast<QLineEdit *>(m_widget)->text().toDouble();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return value;
}

void QuteLineEdit::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
	//  qDebug() << "QuteLineEdit::applyInternalProperties()";

	//  static_cast<QLineEdit*>(m_widget)->setText(property("CSQT_label").toString());
	m_stringValue = property("CSQT_label").toString();
	m_valueChanged = true;
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
	static_cast<QLineEdit*>(m_widget)->setAlignment(align);
	setTextColor(property("CSQT_color").value<QColor>());
	QString borderStyle = (property("CSQT_bordermode").toString() == "border" ? "solid": "none");

	double fontSize = (property("CSQT_fontsize").toDouble()*m_fontScaling) + m_fontOffset;

#ifdef USEFONTPIXELSIZE
    m_widget->setStyleSheet("QLineEdit { font-family:\"" + property("CSQT_font").toString()
                            + "\"; font-size: " + QString::number(fontSize) + "px"
                            + (property("CSQT_bgcolormode").toBool() ?
                                   QString("; background-color:") + property("CSQT_bgcolor").value<QColor>().name() : QString("; "))
                            + "; color:" + property("CSQT_color").value<QColor>().name()
                            + "; border-color:" + property("CSQT_color").value<QColor>().name()
                            + "; border-radius:" + QString::number(property("CSQT_borderradius").toInt()) + "px"
                            + "; border-width:" + QString::number(property("CSQT_borderwidth").toInt()) + "px"
                            + "; border-style: " + borderStyle
                            + "; }");
#else
    int new_fontSize = 0;
    int totalHeight = 0;
	while (totalHeight < fontSize + 1) {
		new_fontSize++;
		QFont font(property("CSQT_font").toString(), new_fontSize);
		QFontMetricsF fm(font);
		totalHeight = fm.ascent() + fm.descent();
	}

	m_widget->setStyleSheet("QLineEdit { font-family:\"" + property("CSQT_font").toString()
							+ "\"; font-size: " + QString::number(new_fontSize)  + "pt"
							+ (property("CSQT_bgcolormode").toBool() ?
								   QString("; background-color:") + property("CSQT_bgcolor").value<QColor>().name() : QString("; "))
							+ "; color:" + property("CSQT_color").value<QColor>().name()
							+ "; border-color:" + property("CSQT_color").value<QColor>().name()
							+ "; border-radius:" + QString::number(property("CSQT_borderradius").toInt()) + "px"
							+ "; border-width:" + QString::number(property("CSQT_borderwidth").toInt()) + "px"
							+ "; border-style: " + borderStyle
							+ "; }");
#endif
	//  qDebug() << "QuteLineEdit::applyInternalProperties() sylesheet" <<  m_widget->styleSheet();
}

QString QuteLineEdit::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	QString line = "texteditor channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	line += QString("text(\"%1\"), ").arg(property("CSQT_label").toString());
	QColor color = property("CSQT_color").value<QColor>();
	line += "fontcolour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + "), ";
	color = property("CSQT_bgcolor").value<QColor>();
	line += "colour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + ") ";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

void QuteLineEdit::dropEvent(QDropEvent *event)
{
	qDebug("QuteLineEdit::dropEvent");
	QWidget::dropEvent(event);
}

void QuteLineEdit::createPropertiesDialog()
{
	QuteText::createPropertiesDialog();
	dialog->setWindowTitle("Line Edit");
	//  fontSize->hide();
	//  font->hide();
	//  border->hide();
	//  bg->hide();
	//  textColor->hide();
	//  bgColor->hide();
	borderRadius->hide();
	borderWidth->hide();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	text->setText(property("CSQT_label").toString());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

//void QuteLineEdit::applyProperties()
//{
//  setProperty("CSQT_label", text->toPlainText());
//  switch (alignment->currentIndex()) {
//    case 0:
//      setProperty("CSQT_alignment", "left");
//      break;
//    case 1:
//      setProperty("CSQT_alignment", "center");
//      break;
//    case 2:
//      setProperty("CSQT_alignment", "right");
//      break;
//    default:
//      setProperty("CSQT_alignment", "");
//  }
//  QuteWidget::applyProperties();  //Must be last to make sure the widgetChanged signal is last
//}

void QuteLineEdit::textEdited(QString text)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	m_stringValue = text;
	m_valueChanged = true;
	QPair<QString, QString> channelValue(m_channel, m_stringValue);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	emit newValue(channelValue);
}

/* -----------------------------------------------------------------*/
/*               QuteScrollNumber class                             */
/* -----------------------------------------------------------------*/

QuteScrollNumber::QuteScrollNumber(QWidget* parent) : QuteText(parent)
{
	delete m_widget; //delete widget created by parent constructor
	m_widget = new ScrollNumberWidget(this);
    auto w = static_cast<ScrollNumberWidget*>(m_widget);
    w->setMargin(4);  // padding between border and start of contents

	//  connect(static_cast<ScrollNumberWidget*>(m_widget), SIGNAL(popUpMenu(QPoint)), this, SLOT(popUpMenu(QPoint)));
    connect(w, SIGNAL(addValue(double)),
            this, SLOT(addValue(double)));
    connect(w, SIGNAL(setValue(double)),
            this, SLOT(setValueFromWidget(double)));
    connect(w, SIGNAL(doubleClick()),
            this, SLOT(setValueFromDialog()));

	m_type = "scroll";

	setProperty("CSQT_value", (double) 0.0);
	setProperty("CSQT_resolution", (double) 0.01);
	setProperty("CSQT_minimum",(double)  -999999999999.0);
	setProperty("CSQT_maximum", (double) 99999999999999.0);
	setProperty("CSQT_randomizable", false);
	setProperty("CSQT_randomizableGroup", 0);
	setProperty("CSQT_mouseControl", "continuous");
	setProperty("CSQT_mouseControlAct", "jump");

	setProperty("CSQT_label", QVariant()); // Remove this property which is part of parent class.
	setProperty("CSQT_precision", QVariant()); // Remove this property which is part of parent class.
	m_places = 2;
	m_min = -999999999999.0;
	m_max = 99999999999999.0;
}

QuteScrollNumber::~QuteScrollNumber()
{
}

void QuteScrollNumber::setResolution(double resolution)
{
	//  qDebug() << "QuteScrollNumber::setResolution " << resolution;
	static_cast<ScrollNumberWidget*>(m_widget)->setResolution(resolution);
	int i;
	for (i = 0; i < 8; i++) {
		//     Check for used decimal places.
		if ((resolution * pow(10, i)) == (int) (resolution * pow(10,i)) )
			break;
	}
	m_places = i;
}

void QuteScrollNumber::setTextAlignment(int alignment)
{
	//   qDebug("QuteText::setAlignment %i", alignment);
	Qt::Alignment align;
	switch (alignment) {
	case 0:
		align = Qt::AlignLeft|Qt::AlignTop;
		break;
	case 1:
		align = Qt::AlignHCenter|Qt::AlignTop;
		break;
	case 2:
		align = Qt::AlignRight|Qt::AlignTop;
		break;
	default:
		align = Qt::AlignLeft|Qt::AlignTop;
	}
	static_cast<ScrollNumberWidget*>(m_widget)->setAlignment(align);
}

QString QuteScrollNumber::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioText {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line += m_type + " ";
	line += QString::number(m_value, 'f', 6) + " ";
	line += QString::number(property("CSQT_resolution").toDouble(), 'f', 6);
	line += + " \"" + m_channel + "\" ";
	line += property("CSQT_alignment").toString() + " ";
	line += "\"" + property("CSQT_font").toString() + "\" "
			+ QString::number(property("CSQT_fontsize").toInt()) + " ";
	QColor color = property("CSQT_color").value<QColor>();
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	color = property("CSQT_bgcolor").value<QColor>();
	line += "{" + QString::number(color.red() * 256)
			+ ", " + QString::number(color.green() * 256)
			+ ", " + QString::number(color.blue() * 256) + "} ";
	line += property("CSQT_bgcolormode").toBool() ? "background ":"nobackground ";
	line += property("CSQT_bordermode").toBool() ? "noborder ": "border ";
	QString outText = property("CSQT_label").toString();
    outText.replace(QRegularExpression("[\n\r]"), "\u00AC");
	line += outText;
	//   qDebug("QuteText::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteScrollNumber::getCsladspaLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ControlPort=" + m_channel + "|" + m_channel + "\n";
	line += "Range=9999|9999";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteScrollNumber::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	s.writeTextElement("alignment", property("CSQT_alignment").toString());
	s.writeTextElement("font", property("CSQT_font").toString());
	s.writeTextElement("fontsize", QString::number(property("CSQT_fontsize").toInt()));

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

	s.writeTextElement("value", QString::number(m_value, 'f', 8));
	s.writeTextElement("resolution", QString::number(property("CSQT_resolution").toDouble(), 'f', 8));
	s.writeTextElement("minimum", QString::number(property("CSQT_minimum").toDouble(), 'f', 8));
	s.writeTextElement("maximum", QString::number(property("CSQT_maximum").toDouble(), 'f', 8));
	s.writeTextElement("bordermode", property("CSQT_bordermode").toString());
	s.writeTextElement("borderradius", QString::number(property("CSQT_borderradius").toInt()));
	s.writeTextElement("borderwidth", QString::number(property("CSQT_borderwidth").toInt()));
	s.writeStartElement("randomizable");
	s.writeAttribute("group", QString::number(property("CSQT_randomizableGroup").toInt()));
	s.writeCharacters(property("CSQT_randomizable").toBool() ? "true": "false");
	s.writeEndElement();
	s.writeStartElement("mouseControl");
	s.writeAttribute("act", property("CSQT_mouseControl").toString());
	s.writeEndElement();

	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

QString QuteScrollNumber::getWidgetType()
{
	return QString("BSBScrollNumber");
}

//QString QuteScrollNumber::getStringValue()
//{
//#ifdef  USE_WIDGET_MUTEX
//  widgetLock.lockForRead();
//#endif
//  QString string = static_cast<ScrollNumberWidget *>(m_widget)->text();
//#ifdef  USE_WIDGET_MUTEX
//  widgetLock.unlock();
//#endif
//  return string;
//}

//double QuteScrollNumber::getValue()
//{
//  return QuteText::getValue();
//}

void QuteScrollNumber::refreshWidget()
{
	//  qDebug() << "QuteScrollNumber::refreshWidget " << m_stringValue;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString text = m_stringValue;
	m_valueChanged = false;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	m_widget->blockSignals(true);
	static_cast<ScrollNumberWidget*>(m_widget)->setText(text);
	m_widget->blockSignals(false);
}

void QuteScrollNumber::createPropertiesDialog()
{
	//   qDebug("QuteScrollNumber::createPropertiesDialog()");
	QuteText::createPropertiesDialog();

	text->setText(QString::number(m_value, 'f', 8));
	dialog->setWindowTitle("Scroll Number");
	QLabel *label = new QLabel(dialog);
	label->setText("Resolution");
	layout->addWidget(label, 4, 0, Qt::AlignRight|Qt::AlignVCenter);
	resolutionSpinBox = new QDoubleSpinBox(dialog);
	resolutionSpinBox->setDecimals(6);
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
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	resolutionSpinBox->setValue(property("CSQT_resolution").toDouble());
	minSpinBox->setValue(property("CSQT_minimum").toDouble());
	maxSpinBox->setValue(property("CSQT_maximum").toDouble());

    vertAlignmentComboBox->hide();
    labelPtrs["vertAlign"]->hide();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteScrollNumber::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	setProperty("CSQT_resolution", resolutionSpinBox->value());
	setProperty("CSQT_value",text->toPlainText().toDouble());
	setProperty("CSQT_minimum",minSpinBox->value());
	setProperty("CSQT_maximum",maxSpinBox->value());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	//  setValue(m_value);
	QuteText::applyProperties();  //Must be last to make sure the widgetChanged signal is last
}

void QuteScrollNumber::applyInternalProperties()
{
	//  qDebug() << "QuteScrollNumber::applyInternalProperties()";

    // QuteWidget::applyInternalProperties();
    QuteText::applyInternalProperties();

	m_min = property("CSQT_minimum").toDouble();
	m_max = property("CSQT_maximum").toDouble();
	setResolution(property("CSQT_resolution").toDouble());
	setValue(property("CSQT_value").toDouble());
	//  m_value = property("CSQT_value").toDouble();
	//  m_value2 = property("CSQT_value2").toDouble();
	//  m_stringValue = property("CSQT_stringValue").toString();
	//  static_cast<QLabel*>(m_widget)->setText(property("CSQT_label").toString());
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
	static_cast<ScrollNumberWidget*>(m_widget)->setAlignment(align);
	setTextColor(property("CSQT_color").value<QColor>());
    /*
	QString borderStyle = (property("CSQT_bordermode").toString() == "border" ? "solid": "none");


	int new_fontSize = 0;
	int totalHeight = 0;
	double fontSize = (property("CSQT_fontsize").toDouble()*m_fontScaling) + m_fontOffset;

	while (totalHeight < fontSize + 1) {
		new_fontSize++;
		QFont font(property("CSQT_font").toString(), new_fontSize);
		QFontMetricsF fm(font);
		totalHeight = fm.ascent() + fm.descent();
	}

	m_widget->setStyleSheet("QLabel { font-family:\"" + property("CSQT_font").toString()
							+ "\"; font-size: " + QString::number(new_fontSize)  + "pt"
							+ (property("CSQT_bgcolormode").toBool() ?
								   QString("; background-color:") + property("CSQT_bgcolor").value<QColor>().name() : QString("; "))
							+ "; color:" + property("CSQT_color").value<QColor>().name()
							+ "; border-color:" + property("CSQT_color").value<QColor>().name()
							+ "; border-radius:" + QString::number(property("CSQT_borderradius").toInt()) + "px"
							+ "; border-width:" + QString::number(property("CSQT_borderwidth").toInt()) + "px"
							+ "; border-style: " + borderStyle
							+ "; }");
	//  qDebug() << property("CSQT_bgcolormode").toBool();
	//  qDebug() << "QuteScrollNumber::applyInternalProperties() sylesheet" <<  m_widget->styleSheet();
    */
	m_valueChanged = true;
}

QString QuteScrollNumber::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	QString line = "numberbox channel(\"" + m_channel + "\"),  ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	QString alignment = property("CSQT_alignment").toString();
	alignment.replace("center","centre");
	line += "align(\"" + alignment + "\"), ";
	QColor color = property("CSQT_color").value<QColor>();
	line += "fontcolour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + "), ";
	color = property("CSQT_bgcolor").value<QColor>();
	line += "colour(" + QString::number(color.red()) + "," +  QString::number(color.green()) + "," +  QString::number(color.blue()) + "), ";
	line += QString("range(-1000000000000,1000000000000,%1), ").arg(m_value); // set redicolously large min and max value and hope the user will use larger numbers...
	line += "active(0)";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

void QuteScrollNumber::addValue(double delta)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	double value = m_value + delta;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	if (value > m_max) {
		value = m_max;
	}
	if (value < m_min) {
		value = m_min;
	}
	//  m_resolution = static_cast<ScrollNumberWidget*>(m_widget)->getResolution();
	//   qDebug("QuteScrollNumber::addValue places = %i resolution = %f", places, m_resolution);
	setValue(value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QPair<QString, double> channelValue(m_channel, m_value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	emit newValue(channelValue);
}

void QuteScrollNumber::setValue(double value)
{
	//  qDebug() << "QuteScrollNumber::setValue " << value;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	m_value = value;
	double displayValue = m_value;
	if (displayValue < m_min) {
		displayValue = m_min;
	}
	if (displayValue > m_max) {
		displayValue = m_max;
	}
	m_stringValue = QString::number(displayValue, 'f', m_places);
	m_valueChanged = true;
	//   qDebug("QuteScrollNumber::setValue places = %i value = %f", m_places, m_value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	//  valueChanged(value);
	//  emit widgetChanged(this);
}

void QuteScrollNumber::setMidiValue(int value)
{
	double max = property("CSQT_maximum").toDouble();
	double min = property("CSQT_minimum").toDouble();
	// it seems better to allow it always, then user gets at least some feedback
	//if (max != 99999999999999.0 && min != -999999999999.0) {
		double newval = min + ((value / 127.0)* (max - min));
		setValue(newval);
		QPair<QString, double> channelValue(m_channel, newval);
		emit newValue(channelValue);
	//}
//	else {
//		qDebug() << "QuteScrollNumber::setMidiValue ranges not set.";
//	}
}

void QuteScrollNumber::setValueFromWidget(double value)
{
	//  qDebug() << "QuteScrollNumber::setValueFromWidget";
	if (value > m_max) {
		value = m_max;
	}
	if (value < m_min) {
		value = m_min;
	}
	setValue(value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QPair<QString, double> channelValue(m_channel, m_value);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	emit newValue(channelValue);
}

void QuteScrollNumber::setValueFromDialog() {
    double newvalue = QInputDialog::getDouble(
                this,
                tr("Enter New Value"),
                tr("Value"),
                m_value,
                property("CSQT_minimum").toDouble(),
                property("CSQT_maximum").toDouble(),
                3);
    setValue(newvalue);
    QPair<QString, double> channelValue(m_channel, m_value);
    emit newValue(channelValue);
}
