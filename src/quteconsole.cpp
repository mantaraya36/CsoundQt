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

#include "quteconsole.h"

QuteConsole::QuteConsole(QWidget *parent) : QuteWidget(parent)
{
	m_widget = new ConsoleWidget(this);
	m_widget->setAutoFillBackground(true);
	m_widget->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels
	canFocus(false);
    setProperty("CSQT_font", "Courier");
	setProperty("CSQT_fontsize", 8.0);
	setProperty("CSQT_bgcolor", QColor(Qt::white));
	setProperty("CSQT_bgcolormode", false);
	setProperty("CSQT_color", QColor(Qt::black));
	//   connect(static_cast<ConsoleWidget *>(m_widget), SIGNAL(popUpMenu(QPoint)), this, SLOT(popUpMenu(QPoint)));
}

QuteConsole::~QuteConsole()
{
}

QString QuteConsole::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioListing {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"}";
	//   qDebug("QuteText::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteConsole::getWidgetXmlText()
{
	// Consoles are not implemented in blue
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
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

	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

QString QuteConsole::getWidgetType()
{
	return QString("BSBConsole");
}

void QuteConsole::appendMessage(QString message)
{
	static_cast<ConsoleWidget *>(m_widget)->appendMessage(message);
}

void QuteConsole::scrollToEnd()
{
	//qDebug() << "QuteConsole::refresh()";
	static_cast<ConsoleWidget *>(m_widget)->scrollToEnd();
}

void QuteConsole::setWidgetGeometry(int x,int y,int width,int height)
{
	QuteWidget::setWidgetGeometry(x,y,width, height);
	static_cast<ConsoleWidget *>(m_widget)->setWidgetGeometry(x,y,width, height);
}

QString QuteConsole::getCabbageLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	QString line = "csoundoutput ";
	line += QString("bounds(%1,%2,%3,%4), ").arg(x()).arg(y()).arg(width()).arg(height());
	line += "text(\"Csound output\")";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

