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

#include "qutewidget.h"
#include "widgetlayout.h"

QuteWidget::QuteWidget(QWidget *parent):
	QWidget(parent), dialog(NULL)
{
	propertiesAct = new QAction(tr("&Properties"), this);
	propertiesAct->setStatusTip(tr("Open widget properties"));
	connect(propertiesAct, SIGNAL(triggered()), this, SLOT(openProperties()));

	addChn_kAct = new QAction(tr("Add chn_k to csd"),this);
	addChn_kAct->setStatusTip(tr("Add chn_k definitionto ;;channels section in editor"));
	connect(addChn_kAct, SIGNAL(triggered()), this, SLOT(addChn_k()));

	m_value = 0.0;
	m_value2 = 0.0;
	m_stringValue = "";
	m_valueChanged = false;
	m_value2Changed = false;
	m_locked = false;
    m_description = "";
    // used by all widgets which need access to the api (TableDisplay)
    // TODO: adapt Scope and Graph to use this instead of implementing their own
    m_csoundUserData = nullptr;

	this->setMinimumSize(2,2);
	this->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels

	setProperty("CSQT_x", 0);
	setProperty("CSQT_y", 0);
    //setProperty("width", 20);
    //setProperty("height", 20);
    setProperty("CSQT_width", 20);
    setProperty("CSQT_height", 20);
    setProperty("CSQT_uuid", QUuid::createUuid().toString());
	setProperty("CSQT_visible", true);
	setProperty("CSQT_midichan", 0);
	setProperty("CSQT_midicc", -3);
    setProperty("CSQT_description", "");
}

QuteWidget::~QuteWidget()
{
}

void QuteWidget::setWidgetGeometry(int x, int y, int w, int h)
{
	//  qDebug() << "QuteWidget::setWidgetGeometry" <<x<<y<<w<<h;
	Q_ASSERT(w > 0 && h > 0);
	//	Q_ASSERT(x > 0 && y > 0 and w > 0 && h > 0);
	this->setGeometry(QRect(x,y,w,h));
	m_widget->blockSignals(true);
	m_widget->setGeometry(QRect(0,0,w,h));
	m_widget->blockSignals(false);
	//  this->markChanged();  // It's better not to have geometry changes trigger markChanged as geometry changes can occur for various reasons (e.g. when calling applyInternalProperties)
}

void QuteWidget::setValue(double value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	m_value = value;
	m_valueChanged = true;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteWidget::setValue2(double value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	m_value2 = value;
	m_value2Changed = true;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteWidget::setValue(QString value)
{
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForWrite();
#endif
	m_stringValue = value;
	m_valueChanged = true;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteWidget::setMidiValue(int /* value */)
{
    qDebug() << "Not available for this widget." << this;
}

void QuteWidget::setMidiValue2(int /* value */)
{
    qDebug() << "Not available for this widget." << this;
}

void QuteWidget::widgetMessage(QString path, QString text)
{
    qDebug() << text;
	if (property(path.toLocal8Bit()).isValid()) {
		setProperty(path.toLocal8Bit(), text);
		//    applyInternalProperties();
	}
}

void QuteWidget::widgetMessage(QString path, double value)
{
    qDebug() << value;
	if (property(path.toLocal8Bit()).isValid()) {
		setProperty(path.toLocal8Bit(), value);
		//    applyInternalProperties();
	}
}

QString QuteWidget::getChannelName()
{
    return m_channel;
}

QString QuteWidget::getChannel2Name()
{
	//  widgetLock.lockForRead();
	QString name = m_channel2;
	//  widgetLock.unlock();
	return name;
}

QString QuteWidget::getCabbageLine()
{
	//Widgets return empty strings when not supported
	return QString("");
}

void QuteWidget::createXmlWriter(QXmlStreamWriter &s)
{
	s.setAutoFormatting(true);
	s.writeStartElement("bsbObject");
	s.writeAttribute("type", getWidgetType());

	s.writeAttribute("version", CSQT_CURRENT_XML_VERSION);  // Only for compatibility with blue (absolute values)

	s.writeTextElement("objectName", m_channel);
	s.writeTextElement("x", QString::number(x()));
	s.writeTextElement("y", QString::number(y()));
	s.writeTextElement("width", QString::number(width()));
	s.writeTextElement("height", QString::number(height()));
	s.writeTextElement("uuid", property("CSQT_uuid").toString());
	s.writeTextElement("visible", property("CSQT_visible").toBool() ? "true":"false");
	s.writeTextElement("midichan", QString::number(property("CSQT_midichan").toInt()));
	s.writeTextElement("midicc", QString::number(property("CSQT_midicc").toInt()));
    s.writeTextElement("description", m_description);
}

double QuteWidget::getValue()
{
    // When reimplementing this, remember to use the widget mutex to protect data,
    // as this can be called from many different places
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	double value = m_value;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return value;
}

double QuteWidget::getValue2()
{
	// When reimplementing this, remember to use the widget mutex to protect data, as this can be called from many different places
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	double value = m_value2;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return value;
}

QString QuteWidget::getStringValue()
{
	// When reimplementing this, remember to use the widget mutex to protect data, as this can be called from many different places
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	QString value = m_stringValue;
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	return value;
}

QString QuteWidget::getDescription()
{
    return m_description;
}

QString QuteWidget::getCsladspaLine()
{
	//Widgets return empty strings when not supported
	return QString("");
}

QString QuteWidget::getQml()
{
    //Widgets return empty strings when not supported
	return QString();
}

QString QuteWidget::getUuid()
{
	if (property("CSQT_uuid").isValid())
		return property("CSQT_uuid").toString();
	else
		return QString();
}

void QuteWidget::applyInternalProperties()
{
	//  qDebug() << "QuteWidget::applyInternalProperties()";
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	int x,y,width, height;
	x = property("CSQT_x").toInt();
	y = property("CSQT_y").toInt();
	width = property("CSQT_width").toInt();
	height = property("CSQT_height").toInt();
	setWidgetGeometry(x,y,width, height);
	m_channel = property("CSQT_objectName").toString();
    m_channel2 = property("CSQT_objectName2").toString();
	m_midicc = property("CSQT_midicc").toInt();
	m_midichan = property("CSQT_midichan").toInt();
	setVisible(property("CSQT_visible").toBool());
	m_valueChanged = true;
    m_description = property("CSQT_description").toString();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteWidget::markChanged()
{
	emit widgetChanged(this);
}

void QuteWidget::canFocus(bool can)
{
	if (can) {
		this->setFocusPolicy(Qt::StrongFocus);
		m_widget->setFocusPolicy(Qt::StrongFocus);
	}
	else {
		this->setFocusPolicy(Qt::NoFocus);
		m_widget->setFocusPolicy(Qt::NoFocus);
	}
}

void QuteWidget::updateDialogWindow(int cc, int channel) // to update values from midi Learn window to widget properties' dialog
{

	if (!dialog) {
		qDebug() << "Dialog window not careated";
		return;
	}

	if (dialog->isVisible() && acceptsMidi()) {
		midiccSpinBox->setValue(cc);
		midichanSpinBox->setValue(channel);
	}
}

void QuteWidget::contextMenuEvent(QContextMenuEvent *event)
{
	popUpMenu(event->globalPos());
}

void QuteWidget::popUpMenu(QPoint pos)
{
    if (m_locked) {
		return;
	}
	QMenu menu(this);
	menu.addAction(propertiesAct);
	menu.addSeparator();

	if (!m_channel.isEmpty() || !m_channel2.isEmpty()) {
		menu.addAction(addChn_kAct);
		menu.addSeparator();
	}

	if (acceptsMidi()) {
        menu.addAction(tr("MIDI learn"), this, SLOT(openMidiDialog()) );
		menu.addSeparator();
	}

	QList<QAction *> actionList = getParentActionList();

	for (int i = 0; i < actionList.size(); i++) {
        auto action = actionList[i];
        if(action == nullptr)
            menu.addSeparator();
        else
            menu.addAction(action);
	}

	menu.addSeparator();

    WidgetLayout *layout = static_cast<WidgetLayout *>(this->parentWidget());
    layout->setCurrentPosition(layout->mapFromGlobal(pos));

    menu.addAction(layout->storePresetAct);
	menu.addAction(layout->newPresetAct);
	menu.addAction(layout->recallPresetAct);

    menu.addSeparator();

    QMenu presetMenu(tr("Presets"), &menu);

	QList<int> list = layout->getPresetNums();
	for (int i = 0; i < list.size(); i++) {
		QAction *act = new QAction(layout->getPresetName(list[i]), &menu);
		act->setData(i);
		connect(act, SIGNAL(triggered()), layout, SLOT(loadPresetFromAction()));
		presetMenu.addAction(act);
	}

    /*
    menu.addSeparator();

    QMenu createMenu(tr("Create New", "Menu name in widget right-click menu"),&menu);
    createMenu.addAction(layout->createSliderAct);
    createMenu.addAction(layout->createLabelAct);
    createMenu.addAction(layout->createDisplayAct);
    createMenu.addAction(layout->createScrollNumberAct);
    createMenu.addAction(layout->createLineEditAct);
    createMenu.addAction(layout->createSpinBoxAct);
    createMenu.addAction(layout->createButtonAct);
    createMenu.addAction(layout->createKnobAct);
    createMenu.addAction(layout->createCheckBoxAct);
    createMenu.addAction(layout->createMenuAct);
    createMenu.addAction(layout->createMeterAct);
    createMenu.addAction(layout->createConsoleAct);
    createMenu.addAction(layout->createGraphAct);
    createMenu.addAction(layout->createScopeAct);

    menu.addMenu(&createMenu);
    */
	menu.exec(pos);
}

void QuteWidget::openProperties()
{
	createPropertiesDialog();

	connect(acceptButton, SIGNAL(released()), dialog, SLOT(accept()));
	connect(dialog, SIGNAL(accepted()), this, SLOT(apply()));
	connect(applyButton, SIGNAL(released()), this, SLOT(apply()));
	connect(cancelButton, SIGNAL(released()), dialog, SLOT(close()));
	if (acceptsMidi()) {
		connect(midiLearnButton, SIGNAL(released()),this, SLOT(openMidiDialog()));
	}
	dialog->exec();
	if (dialog->result() != QDialog::Accepted) {
		qDebug() << "QuteWidget::openProperties() dialog not accepted";
	}
	//  dialog->deleteLater();
	parentWidget()->setFocus(Qt::OtherFocusReason); // For some reason focus is grabbed away from the layout, but this doesn't solve the problem...
}


void QuteWidget::deleteWidget()
{
	//   qDebug("QuteWidget::deleteWidget()");
	emit(deleteThisWidget(this));
}

void QuteWidget::openMidiDialog()
{
	//createPropertiesDialog(); <- tryout for midi learn from context menu
	emit showMidiLearn(this);
}

void QuteWidget::addChn_k()
{
	//qDebug()<<Q_FUNC_INFO << m_channel << m_channel2;
	if (!m_channel.isEmpty()) {
		emit addChn_kSignal(m_channel);
	}
	if (!m_channel2.isEmpty()) {
		emit addChn_kSignal(m_channel2);
	}
}

void QuteWidget::createPropertiesDialog()
{
//    qDebug() << "QuteWidget::createPropertiesDialog()---Dynamic Properties:\n"
//             << dynamicPropertyNames ();
	int footerRow = 20;
	dialog = new QDialog(this);
    dialog->resize(360, 300);
	//  dialog->setModal(true);
	layout = new QGridLayout(dialog);
    QLabel *label;

    label = new QLabel("X =", dialog);
    layout->addWidget(label, 0, 0, Qt::AlignRight|Qt::AlignVCenter);

    xSpinBox = new QSpinBox(dialog);
    xSpinBox->unsetLocale();
	xSpinBox->setMaximum(9999);
	layout->addWidget(xSpinBox, 0, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel("Y =", dialog);
    layout->addWidget(label, 0, 2, Qt::AlignRight|Qt::AlignVCenter);

    ySpinBox = new QSpinBox(dialog);
    ySpinBox->unsetLocale();
	ySpinBox->setMaximum(9999);
	layout->addWidget(ySpinBox, 0, 3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(tr("Width ="), dialog);
    layout->addWidget(label, 1, 0, Qt::AlignRight|Qt::AlignVCenter);

    wSpinBox = new QSpinBox(dialog);
    wSpinBox->unsetLocale();
	wSpinBox->setMaximum(9999);
	layout->addWidget(wSpinBox, 1, 1, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(tr("Height ="), dialog);
    layout->addWidget(label, 1, 2, Qt::AlignRight|Qt::AlignVCenter);

    hSpinBox = new QSpinBox(dialog);
    hSpinBox->unsetLocale();
	hSpinBox->setMaximum(9999);
	layout->addWidget(hSpinBox, 1, 3, Qt::AlignLeft|Qt::AlignVCenter);

    channelLabel = new QLabel(tr("Channel ="), dialog);
    layout->addWidget(channelLabel, 3, 0, Qt::AlignRight|Qt::AlignVCenter);

    nameLineEdit = new QLineEdit(dialog);
	nameLineEdit->setFocus(Qt::OtherFocusReason);
	nameLineEdit->selectAll();
    layout->addWidget(nameLineEdit, 3, 1, 1, 3, Qt::AlignLeft|Qt::AlignVCenter);

    label = new QLabel(tr("Description ="), dialog);
    layout->addWidget(label, footerRow-2, 0, Qt::AlignRight|Qt::AlignVCenter);

    descriptionLineEdit = new QLineEdit(dialog);
    descriptionLineEdit->setMinimumWidth(300);
    layout->addWidget(descriptionLineEdit, footerRow-2, 1, 1, 4, Qt::AlignLeft|Qt::AlignVCenter);


    if (acceptsMidi()) { // only when MIDI-enabled widgets
        int midiRow = footerRow - 1;
        label = new QLabel("MIDI CC =", dialog);
        layout->addWidget(label, midiRow, 0, Qt::AlignRight|Qt::AlignVCenter);

        midiccSpinBox = new QSpinBox(dialog);
        midiccSpinBox->unsetLocale();
		midiccSpinBox->setRange(0,119);
        layout->addWidget(midiccSpinBox, midiRow, 1, Qt::AlignLeft|Qt::AlignVCenter);

        label = new QLabel("MIDI Channel =", dialog);
        layout->addWidget(label, midiRow, 2, Qt::AlignRight|Qt::AlignVCenter);

        midichanSpinBox = new QSpinBox(dialog);
        midichanSpinBox->unsetLocale();
		midichanSpinBox->setRange(0,127);
        layout->addWidget(midichanSpinBox, midiRow,3, Qt::AlignLeft|Qt::AlignVCenter);

        midiLearnButton = new QPushButton(tr("MIDI learn"));
		layout->addWidget(midiLearnButton, midiRow, 4, Qt::AlignLeft|Qt::AlignVCenter);
	}
	acceptButton = new QPushButton(tr("Ok"));
	acceptButton->setDefault(true);
    layout->addWidget(acceptButton, footerRow, 3, Qt::AlignCenter|Qt::AlignVCenter);

    applyButton = new QPushButton(tr("Apply"));
    layout->addWidget(applyButton, footerRow, 1, Qt::AlignCenter|Qt::AlignVCenter);
	cancelButton = new QPushButton(tr("Cancel"));
    layout->addWidget(cancelButton, footerRow, 2, Qt::AlignCenter|Qt::AlignVCenter);

#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	xSpinBox->setValue(this->x());
	ySpinBox->setValue(this->y());
	wSpinBox->setValue(this->width());
	hSpinBox->setValue(this->height());
	nameLineEdit->setText(getChannelName());
    descriptionLineEdit->setText(getDescription());
	if (acceptsMidi()) {
        midiccSpinBox->setValue(this->m_midicc);
        midichanSpinBox->setValue(this->m_midichan);
    }
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
}

void QuteWidget::applyProperties()
{
//	qDebug();
#ifdef  USE_WIDGET_MUTEX
	widgetLock.lockForRead();
#endif
	setProperty("CSQT_objectName", nameLineEdit->text());
	setProperty("CSQT_x", xSpinBox->value());
	setProperty("CSQT_y",ySpinBox->value());
	setProperty("CSQT_width", wSpinBox->value());
	setProperty("CSQT_height", hSpinBox->value());
	if (acceptsMidi()) {
		setProperty("CSQT_midicc", midiccSpinBox->value());
		setProperty("CSQT_midichan", midichanSpinBox->value());
	}
    setProperty("CSQT_description", descriptionLineEdit->text());
#ifdef  USE_WIDGET_MUTEX
	widgetLock.unlock();
#endif
	applyInternalProperties();
	//  setChannelName(nameLineEdit->text());
	//  setWidgetGeometry(xSpinBox->value(), ySpinBox->value(), wSpinBox->value(), hSpinBox->value());

	//  this->setMouseTracking(true); // Necessary to pass mouse tracking to widget panel for _MouseX channels
	emit(widgetChanged(this));
	emit propertiesAccepted();
	parentWidget()->setFocus(Qt::PopupFocusReason); // For some reason focus is grabbed away from the layout
	m_valueChanged = true;
}

QList<QAction *> QuteWidget::getParentActionList()
{
	QList<QAction *> actionList;
	// A bit of a kludge... Must get the Widget Panel, which is the parent to the widget which
	// holds the actual QuteWidgets
	WidgetLayout *layout = static_cast<WidgetLayout *>(this->parentWidget());
    actionList.append(layout->alignLeftAct);
	actionList.append(layout->alignRightAct);
	actionList.append(layout->alignTopAct);
	actionList.append(layout->alignBottomAct);
    actionList.append(nullptr);
    actionList.append(layout->alignCenterHorizontalAct);
	actionList.append(layout->alignCenterVerticalAct);
    actionList.append(nullptr);
    actionList.append(layout->sendToBackAct);
    actionList.append(layout->sendToFrontAct);
    actionList.append(nullptr);
    actionList.append(layout->distributeHorizontalAct);
    actionList.append(layout->distributeVerticalAct);
    actionList.append(nullptr);
    actionList.append(layout->copyAct);
    actionList.append(layout->pasteAct);
    actionList.append(layout->cutAct);
    actionList.append(layout->deleteAct);
    actionList.append(layout->duplicateAct);

    // FIXME put edit action in menu
	//  actionList.append(layout->editAct);
	return actionList;
}

void QuteWidget::apply()
{
	applyProperties();
}
