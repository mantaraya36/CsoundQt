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

#ifndef QUTEWIDGET_H
#define QUTEWIDGET_H

#include <QtWidgets>
#include <QtXml>

#define CSQT_CURRENT_XML_VERSION "2"
#define CSQT_CURVE_BUFFER_SIZE 256
//#define USE_WIDGET_MUTEX

#include "csoundengine.h"

enum QuteWidgetType { UNKNOWN=0, SPINBOX=1, LINEEDIT, CHECKBOX, SLIDER, KNOB, SCROLLNUMBER,
                      BUTTON, DROPDOWN, CONTROLLER, GRAPH, SCOPE, CONSOLE,
                      TABLEDISPLAY };

class QuteWidget : public QWidget
{
	Q_OBJECT
public:
	QuteWidget(QWidget* parent);
	~QuteWidget();

	virtual void setWidgetGeometry(int x, int y, int w, int h);
	virtual void setValue(double);  // These should only be reimplemented if something special needs to be done with the value.
	virtual void setValue2(double);
	virtual void setValue(QString);
	virtual void setMidiValue(int value);
	virtual void setMidiValue2(int value);
	virtual bool acceptsMidi() {return false;}
	virtual void setLocked(bool locked) {m_locked = locked;}

	virtual void widgetMessage(QString path, QString text);
	virtual void widgetMessage(QString path, double value);

	virtual QString getWidgetLine() = 0;
	virtual QString getCabbageLine();
	virtual QString getCsladspaLine();
    virtual QString getQml();
	virtual QString getWidgetXmlText() = 0;
	virtual QString getChannelName();
	virtual QString getChannel2Name();
	virtual double getValue();
	virtual double getValue2();
	virtual QString getStringValue();
    virtual QString getDescription();

	QString getUuid();
	virtual QString getWidgetType() = 0;
	virtual void refreshWidget() { ;}

	virtual void applyInternalProperties();

	void createXmlWriter(QXmlStreamWriter &s);
	void markChanged();
	void canFocus(bool can);
	void updateDialogWindow(int cc, int channel);
    virtual void setCsoundUserData(CsoundUserData *ud) { m_csoundUserData = ud; }
    double getSr(double defaultSr=0.0) {
        if(m_csoundUserData == nullptr) {
            QDEBUG << "csoundUserData: null";
            return defaultSr;
        }
        auto engine = m_csoundUserData->csEngine;
        if(engine == nullptr) {
            QDEBUG << "engine null";
            return defaultSr;
        }
        if(!engine->isRunning()) {
            QDEBUG << "not running";
            return defaultSr;
        }
        return csoundGetSr(engine->getCsound());
    }

	bool m_valueChanged;
	bool m_value2Changed;


public slots:
	void popUpMenu(QPoint pos);
	void openProperties();

protected:
	QSpinBox *xSpinBox;
	QSpinBox *ySpinBox;
	QSpinBox *wSpinBox;
	QSpinBox *hSpinBox;
	QLabel *channelLabel;
	QLineEdit *nameLineEdit;
    QLineEdit *descriptionLineEdit;

	QSpinBox *midiccSpinBox;
	QSpinBox *midichanSpinBox;
	QPushButton *midiLearnButton;
	QWidget *m_widget;
	QDialog *dialog;
	QGridLayout *layout;  // For preference dialog
	double m_value, m_value2;
	QString m_stringValue;
	QString m_channel, m_channel2;
	int m_midicc, m_midichan;
	bool m_locked; // Allow modification of widget (properties, alignment, etc.)
    CsoundUserData *m_csoundUserData;
    QString m_description;


#ifdef  USE_WIDGET_MUTEX
	QReadWriteLock widgetLock;
#endif
	QString xmlText;

	virtual void contextMenuEvent(QContextMenuEvent *event);

	virtual void createPropertiesDialog();
	virtual void applyProperties();

	QList<QAction *> getParentActionList();
	QList<QAction *> getParentPresetList();

protected slots:
	void apply();
	void deleteWidget();
	void openMidiDialog();

private:

	QAction *propertiesAct;
	QAction *addChn_kAct;


	QPushButton *applyButton;
	QPushButton *cancelButton;
	QPushButton *acceptButton;

private slots:
    void addChn_k();

signals:
	void newValue(QPair<QString,double> channelValue);
	void newValue(QPair<QString,QString> channelValue);
	void widgetChanged(QuteWidget* widget);
	void deleteThisWidget(QuteWidget *thisWidget);
	void propertiesAccepted();
	void showMidiLearn(QuteWidget* widget);
	void addChn_kSignal(QString channel);

};

#endif
