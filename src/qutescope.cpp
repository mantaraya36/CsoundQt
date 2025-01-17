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

#include "qutescope.h"
#include <cmath>
#include "types.h"  //necessary for the userdata struct
#include "csoundqt.h"  //necessary for the userdata struct


QuteScope::QuteScope(QWidget *parent) : QuteWidget(parent)
{
	QGraphicsScene *m_scene = new QGraphicsScene(this);
    // m_scene->setBackgroundBrush(QBrush(Qt::black));
    m_scene->setBackgroundBrush(QBrush(QColor("#161616")));
	m_widget = new ScopeWidget(this);
	m_widget->show();
	m_widget->setAutoFillBackground(true);
	m_widget->setContextMenuPolicy(Qt::NoContextMenu);
    // Necessary to pass mouse tracking to widget panel for _MouseX channels
    m_widget->setMouseTracking(true);
	//  m_widget->setWindowFlags(Qt::WindowStaysOnTopHint);
	canFocus(false);
	static_cast<ScopeWidget *>(m_widget)->setScene(m_scene);
	static_cast<ScopeWidget *>(m_widget)->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    // static_cast<ScopeWidget *>(m_widget)->setRenderHints(QPainter::Antialiasing);
    m_label = new QLabel(this);
	QPalette palette = m_widget->palette();
    palette.setColor(QPalette::WindowText, QColor(196, 196, 196));
	m_label->setPalette(palette);
    auto font = m_label->font();
    font.setPixelSize(11);
    m_label->setFont(font);
	m_label->setText("Scope");
    m_label->move(10, 0);
	m_label->resize(500, 25);

    m_params = new ScopeParams(nullptr,
                               m_scene,
                               static_cast<ScopeWidget *>(m_widget),
                               &scopeLock,
                               this->width(),
                               this->height());
    m_params->triggerMode = TriggerMode::NoTrigger;

    m_scopeData    = new ScopeData(m_params);
	m_lissajouData = new LissajouData(m_params);
	m_poincareData = new PoincareData(m_params);

    m_dataDisplay = (DataDisplay *)m_scopeData;
	m_dataDisplay->show();

	// Default properties
	setProperty("QCS_type", "scope");
	setProperty("QCS_zoomx", 1.0);
	setProperty("QCS_zoomy", 1.0);
	setProperty("QCS_dispx", 1.0);
	setProperty("QCS_dispy", 1.0);
	setProperty("QCS_mode", "lin");
    setProperty("QCS_triggermode", "NoTrigger");
}

QuteScope::~QuteScope()
{
	delete m_poincareData;
	delete m_lissajouData;
	delete m_scopeData;
	delete m_params;
}

TriggerMode triggerNameToMode(QString s) {
    if(s == "NoTrigger")
        return TriggerMode::NoTrigger;
    else if(s =="TriggerUp")
        return TriggerMode::TriggerUp;
    else
        return TriggerMode::NoTrigger;
}

QString triggerModeToName(TriggerMode t) {
    if(t == TriggerMode::NoTrigger)
        return "NoTrigger";
    else if(t == TriggerMode::TriggerUp)
        return "TriggerUp";
    else
        return QString();
}


QString QuteScope::getWidgetLine()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString line = "ioGraph {" + QString::number(x()) + ", " + QString::number(y()) + "} ";
	line += "{"+ QString::number(width()) +", "+ QString::number(height()) +"} ";
	line += property("QCS_type").toString() + " " + QString::number(property("QCS_zoomx").toDouble(), 'f', 6) + " ";
	line += QString::number((int) m_value) + " ";
	line += m_channel;
	//   qDebug("QuteScope::getWidgetLine() %s", line.toStdString().c_str());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return line;
}

QString QuteScope::getWidgetXmlText()
{
	xmlText = "";
	QXmlStreamWriter s(&xmlText);
	createXmlWriter(s);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif

	s.writeTextElement("value", QString::number(m_value, 'f', 8));
	s.writeTextElement("type", property("QCS_type").toString());
	s.writeTextElement("zoomx", QString::number(property("QCS_zoomx").toDouble(), 'f', 8));
	s.writeTextElement("zoomy", QString::number(property("QCS_zoomy").toDouble(), 'f', 8));
	s.writeTextElement("dispx", QString::number(property("QCS_dispx").toDouble(), 'f', 8));
	s.writeTextElement("dispy", QString::number(property("QCS_dispy").toDouble(), 'f', 8));
    s.writeTextElement("mode",  QString::number(property("QCS_mode").toDouble(), 'f', 8));
    s.writeTextElement("triggermode", property("QCS_triggermode").toString());
	s.writeEndElement();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return xmlText;
}

QString QuteScope::getWidgetType()
{
	return QString("BSBScope");
}

void QuteScope::setType(QString type)
{
	updateLabel();
	m_dataDisplay->hide();
	if (type == "scope") {
		m_dataDisplay = (DataDisplay *)m_scopeData;
	}
	else if (type == "lissajou") {
		m_dataDisplay = (DataDisplay *)m_lissajouData;
	}
	else if (type == "poincare") {
		m_dataDisplay = (DataDisplay *)m_poincareData;
	}
	m_dataDisplay->show();
}

void QuteScope::setValue(double value)
{
	QuteWidget::setValue(value);
	updateLabel();
}

void QuteScope::setUd(CsoundUserData *ud)
{
	m_params->ud = ud;
}

void QuteScope::updateLabel()
{
	QString chan;
	if ((int) m_value < 0) {
        chan = tr("all", "meaning 'all' channels in scope, 4 letter max");
	}
	else if ((int) m_value <= 0) {
        chan = tr("None", "meaning 'no' channels in scope, 4 letter max");
	}
	else {
		chan =  QString::number((int) m_value );
	}
	m_label->setText(tr("Scope ch:") + chan);
}


void QuteScope::applyInternalProperties()
{
	QuteWidget::applyInternalProperties();
	setType(property("QCS_type").toString());
	setValue(property("QCS_value").toDouble());
    m_params->triggerMode = triggerNameToMode(property("QCS_triggermode").toString());
}

void QuteScope::createPropertiesDialog()
{
	QuteWidget::createPropertiesDialog();
	dialog->setWindowTitle("Scope");
	//   channelLabel->hide();
	//   nameLineEdit->hide();
	QLabel *label = new QLabel(dialog);
	label->setText("Type");
	layout->addWidget(label, 6, 0, Qt::AlignRight|Qt::AlignVCenter);
	typeComboBox = new QComboBox(dialog);
	typeComboBox->addItem("Oscilloscope", QVariant(QString("scope")));
	typeComboBox->addItem("Lissajou curve", QVariant(QString("lissajou")));
	typeComboBox->addItem("Poincare map", QVariant(QString("poincare")));
	//   typeComboBox->addItem("Spectrogram", QVariant(QString("fft")));
	layout->addWidget(typeComboBox, 6, 1, Qt::AlignLeft|Qt::AlignVCenter);
	label = new QLabel(dialog);
	label->setText("Channel");
	layout->addWidget(label, 6, 2, Qt::AlignRight|Qt::AlignVCenter);
	channelBox = new QComboBox(dialog);
	channelBox->addItem("all", QVariant((int) -255));
    int maxChannels = 16;
    if(m_params->ud != nullptr && m_params->ud->csEngine->isRunning()) {
        maxChannels = m_params->ud->numChnls;
    }
    for(int i=1; i <= maxChannels; i++) {
        channelBox->addItem(QString::number(i), QVariant((int) i));
    }
    channelBox->addItem("none", QVariant((int) 0));
	layout->addWidget(channelBox, 6, 3, Qt::AlignLeft|Qt::AlignVCenter);
	label = new QLabel(dialog);
	label->setText("Zoom X");
	layout->addWidget(label, 8, 0, Qt::AlignRight|Qt::AlignVCenter);
	zoomxBox = new QDoubleSpinBox(dialog);
	zoomxBox->setRange(1, 20);
	layout->addWidget(zoomxBox, 8, 1, Qt::AlignLeft|Qt::AlignVCenter);
	label = new QLabel(dialog);
	label->setText("Zoom Y");
	layout->addWidget(label, 8, 2, Qt::AlignRight|Qt::AlignVCenter);
	zoomyBox = new QDoubleSpinBox(dialog);
	zoomyBox->setRange(1, 20);
	layout->addWidget(zoomyBox, 8, 3, Qt::AlignLeft|Qt::AlignVCenter);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	typeComboBox->setCurrentIndex(typeComboBox->findData(QVariant(property("QCS_type").toString())));
	channelBox->setCurrentIndex(channelBox->findData(QVariant((int) m_value)));
	zoomxBox->setValue(property("QCS_zoomx").toDouble());
	zoomyBox->setValue(property("QCS_zoomy").toDouble());

    label = new QLabel("Trigger");
    layout->addWidget(label, 9, 0, Qt::AlignRight|Qt::AlignVCenter);
    triggerBox = new QComboBox(dialog);
    triggerBox->addItem("No Trigger", "NoTrigger");
    triggerBox->addItem("Trigger Up", "TriggerUp");
    triggerBox->setCurrentIndex(triggerBox->findData(property("QCS_triggermode").toString()));
    layout->addWidget(triggerBox, 9, 1, Qt::AlignLeft|Qt::AlignVCenter);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteScope::applyProperties()
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	setProperty("QCS_type", typeComboBox->itemData(typeComboBox->currentIndex()).toString());
	setProperty("QCS_zoomx", zoomxBox->value());
	setProperty("QCS_zoomy", zoomyBox->value());
	setProperty("QCS_value", channelBox->itemData(channelBox->currentIndex()).toInt());
    auto triggerModeStr = triggerBox->currentData().toString();
    setProperty("QCS_triggermode", triggerModeStr);
    m_params->triggerMode = triggerNameToMode(triggerModeStr);
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
    //Must be last to make sure the widgetChanged signal is last
    QuteWidget::applyProperties();
}

void QuteScope::resizeEvent(QResizeEvent * event)
{
	QuteWidget::resizeEvent(event);
	m_params->setWidth(this->width());
	m_params->setHeight(this->height());
	m_scopeData->resize();
	m_lissajouData->resize();
	m_poincareData->resize();
	//   QGraphicsScene *m_scene = static_cast<ScopeWidget *>(m_widget)->scene();
	//   m_scene->setSceneRect(-m_ud->zerodBFS, m_ud->zerodBFS, width() - 5, m_ud->zerodBFS *2);
	//   static_cast<ScopeWidget *>(m_widget)->setSceneRect(-m_ud->zerodBFS , m_ud->zerodBFS, width() - 5, m_ud->zerodBFS *2);
}

void QuteScope::updateData()
{
    m_dataDisplay->updateData((int) m_value,
                              property("QCS_zoomx").toDouble(),
                              property("QCS_zoomy").toDouble(),
                              static_cast<ScopeWidget *>(m_widget)->freeze);
}

ScopeItem::ScopeItem(int width, int height)
{
	m_width = width;
	m_height = height;
}

void ScopeItem::paint(QPainter *p,
                      const QStyleOptionGraphicsItem */*option*/,
                      QWidget */*widget*/)
{
	p->setPen(m_pen);
	p->drawPoints(m_polygon);
}

void ScopeItem::setPen(const QPen & pen)
{
	m_pen = pen;
}

void ScopeItem::setPolygon(const QPolygonF & polygon)
{
	m_polygon = polygon;
	update(boundingRect());
}

void ScopeItem::setSize(int width, int height)
{
	m_width = width;
	m_height = height;
	prepareGeometryChange();
}

ScopeData::ScopeData(ScopeParams *params) : DataDisplay(params)
{
	curveData.resize(m_params->width + 2);
	curve = new QGraphicsPolygonItem(/*&curveData*/);
    curve->setPen(QPen(Qt::green, 0));
    curve->setPen(QPen(QColor("#40FF40"), 0));

	curve->hide();
	m_params->scene->addItem(curve);
}

void ScopeData::resize()
{
	curveData.resize(m_params->width + 2);
}

void ScopeData::updateData(int channel, double zoomx, double zoomy, bool freeze)
{
	CsoundUserData *ud = m_params->ud;
	int width = m_params->width;
	int height = m_params->height;
    if (ud == 0 || !ud->csEngine->isRunning() )
		return;
	if (freeze)
		return;
	double value;
	int numChnls = ud->numChnls;
	MYFLT newValue;
    if (channel == 0 || channel > numChnls ) {
        return;
	}
	channel = (channel < 0 ? -1: channel - 1);
#ifdef  USE_WIDGET_MUTEX
    //FIXME is this locking needed, or should a separate locking mechanism be implemented?
    QReadWriteLock *mutex = m_params->mutex;
	mutex->lockForWrite();
#endif
    // FIXME how to make sure the buffer is read before it is flushed when recorded?
    // Have another buffer?
	RingBuffer *buffer = &ud->audioOutputBuffer;
	buffer->lock();
	QList<MYFLT> list = buffer->buffer;
	buffer->unlock();
	long listSize = list.size();
    long offset = buffer->currentPos;
    long dataToRead = width;
    // search for trig
    long trigOffset = 0;
    double lastValue = 1.0;
    long maxFrames = width < dataToRead ? width: dataToRead;

    if(m_params->triggerMode == TriggerMode::TriggerUp) {
        if(channel >= 0) {
            for(int i=0; i < maxFrames; i++) {
                int idx = (int)((offset + (int)(i*numChnls*zoomx) + channel) % listSize);
                double value = list[idx];
                if(value >= 0 && lastValue < 0) {
                    trigOffset = idx - offset - channel;
                    break;
                }
                lastValue = value;
            }
        }
        else {
            for(int i=0; i < maxFrames; i++) {
                int baseidx = (int)((offset + (int)(i*numChnls*zoomx)) % listSize);
                double value = 0;
                for(int chan = 0; chan < numChnls; chan++) {
                    double newValue = list[baseidx+chan];
                    if(fabs(newValue) > fabs(value))
                        value = newValue;
                }
                if(value >= 0 && lastValue < 0) {
                    trigOffset = baseidx - offset;
                    break;
                }
                lastValue = value;
            }

        }
        offset += trigOffset;
    }
    double factor = numChnls * zoomx;
    int halfheight = height/2;
    if(channel >= 0) {
        for(int i = 0; i < maxFrames; i++) {
            int idx = (int)((offset+channel+ (int)(i*factor)) % listSize);
            value = (double) list[idx];
            curveData[i+1] = QPoint(i, -zoomy*value*halfheight);
        }
    } else {
        for(int i = 0; i < maxFrames; i++) {
            value = 0;
            for(int chan = 0; chan < numChnls; chan++) {
                int idx = (int)((offset+chan+ (int)(i*factor)) % listSize);
                newValue = (double) list[idx];
                if(fabs(newValue) > fabs(value))
                    value = newValue;
            }
            curveData[i+1] = QPoint(i, -zoomy*value*halfheight);
        }

    }


    /*
    for (int i = 0; i < width; i++) {
		value = 0;
        for (int j = 0; j < (int) zoomx; j++) {
			if (channel == -1) {
                // all channels
				newValue = 0;
				for (int k = 0; k < numChnls; k++) {
                    int bufIdx = (int)((((i*zoomx)+j)*numChnls) + offset + k) % listSize;
                    newValue += list[bufIdx];
				}
				newValue /= numChnls;
				if (fabs(newValue) > fabs(value))
					value = -(double) newValue;
			}
			else {
                int bufIdx = (int)((((i*zoomx)+j)*numChnls) + offset + channel) % listSize;
                if (fabs(list[bufIdx]) > fabs(value))
                    value = (double) -list[bufIdx];
			}
		}
        curveData[i+1] = QPoint(i, zoomy*value*height/2);
	}
    */
    // buffer->currentReadPos += width;
    buffer->currentReadPos = (offset + dataToRead) % buffer->size;
	m_params->widget->setSceneRect(0, -height/2, width, height );
	curveData.last() = QPoint(width-4, 0);
	curveData.first() = QPoint(0, 0);
	curve->setPolygon(curveData);
#ifdef  USE_WIDGET_MUTEX
	mutex->unlock();
#endif
}

void ScopeData::show()
{
	curve->show();
}

void ScopeData::hide()
{
	curve->hide();
}

LissajouData::LissajouData(ScopeParams *params) : DataDisplay(params)
{
	curveData.resize(m_params->width);
	curve = new ScopeItem(m_params->width, m_params->height);
    auto pen = QPen(Qt::green);
    pen.setCosmetic(true);
    curve->setPen(pen);
	curve->hide();
	m_params->scene->addItem(curve);
}

void LissajouData::resize()
{
	// We take 8 times the display width points for each pass
	// to have a smooth animation
	curveData.resize(m_params->width * 8);
	curve->setSize(m_params->width, m_params->height);
}

void LissajouData::updateData(int channel, double zoomx, double zoomy, bool freeze)
{
	// The decimation factor (zoom) is not used here
	CsoundUserData *ud = m_params->ud;
	int width = m_params->width;
	int height = m_params->height;
    if (ud == nullptr || !ud->csEngine->isRunning())
		return;
	if (freeze)
		return;
	double x, y;
	int numChnls = ud->numChnls;
	// We take two consecutives channels, the first one for abscissas and
	// the second one for ordinates
    if (channel == 0 || channel >= numChnls || numChnls < 2) {
        return;
	}
	channel = (channel < 0 ? 0 : channel - 1);
#ifdef  USE_WIDGET_MUTEX
	QReadWriteLock *mutex = m_params->mutex;
	mutex->lockForWrite();
#endif
	RingBuffer *buffer = &ud->audioOutputBuffer;
	buffer->lock();
	QList<MYFLT> list = buffer->buffer;
	buffer->unlock();
	long listSize = list.size();
	long offset = buffer->currentPos;
	for (int i = 0; i < curveData.size(); i++) {
		int bufferIndex = (int)((i*numChnls) + offset + channel) % listSize;
		x = (double)list[bufferIndex];
        bufferIndex = (bufferIndex + 1) % listSize;
        y = (double) -list[bufferIndex];
		curveData[i] = QPoint(x*width*zoomx/4, y*height*zoomy/4);
	}
	m_params->widget->setSceneRect(-width/2, -height/2, width, height );
	curve->setPolygon(curveData);
#ifdef  USE_WIDGET_MUTEX
	mutex->unlock();
#endif
}

void LissajouData::show()
{
	curve->show();
}

void LissajouData::hide()
{
	curve->hide();
}

PoincareData::PoincareData(ScopeParams *params) : DataDisplay(params)
{
	curveData.resize(m_params->width);
	curve = new ScopeItem(m_params->width, m_params->height);
    curve->setPen(QPen(Qt::green, 0));
	curve->hide();
	lastValue = 0.0;
	m_params->scene->addItem(curve);
}

void PoincareData::resize()
{
	// We take 8 times the display width points for each pass
	// to have a smooth animation
	curveData.resize(m_params->width * 8);
	curve->setSize(m_params->width, m_params->height);
}

void PoincareData::updateData(int channel, double zoomx, double zoomy, bool freeze)
{
	CsoundUserData *ud = m_params->ud;
	int width = m_params->width;
	int height = m_params->height;
    if (ud == 0 || !ud->csEngine->isRunning() )
		return;
	if (freeze)
		return;
	double value;
	int numChnls = ud->numChnls;
    if (channel == 0 || channel > numChnls) {
        return;
	}
	channel = (channel < 0 ? 0 :  channel - 1);
#ifdef  USE_WIDGET_MUTEX
	QReadWriteLock *mutex = m_params->mutex;
	mutex->lockForWrite();
#endif
	RingBuffer *buffer = &ud->audioOutputBuffer;
	buffer->lock();
	QList<MYFLT> list = buffer->buffer;
	buffer->unlock();
	long listSize = list.size();
	long offset = buffer->currentPos;
	for (int i = 0; i < curveData.size(); i++) {
		int bufferIndex = (int)((i*zoomx*numChnls) + offset + channel) % listSize;
		value = (double)list[bufferIndex];
		curveData[i] = QPoint(lastValue*width*zoomx/2, -value*height*zoomy/2);
		lastValue = value;
	}
	m_params->widget->setSceneRect(-width/2, -height/2, width, height );
	curve->setPolygon(curveData);
#ifdef  USE_WIDGET_MUTEX
	mutex->unlock();
#endif
}

void PoincareData::show()
{
	curve->show();
}

void PoincareData::hide()
{
	curve->hide();
}

