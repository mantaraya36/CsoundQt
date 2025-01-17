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

#include "widgetpanel.h"
#include "widgetlayout.h"

#include "csoundqt.h"

WidgetPanel::WidgetPanel(QWidget *parent)
	: QDockWidget(parent)
{
	setWindowTitle("Widgets");
	setMinimumSize(200, 140);
	//   connect(this,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(dockLocationChanged(Qt::DockWidgetArea)));
	//connect(this,SIGNAL(topLevelChanged(bool)), this, SLOT(dockStateChanged(bool)));

	m_stack = new QStackedWidget(this);
	m_stack->show();
	setWidget(m_stack);

	setMouseTracking(true);
}

WidgetPanel::~WidgetPanel()
{
}

void WidgetPanel::addWidgetLayout(WidgetLayout *w)
{
	QScrollArea *scrollArea;
	scrollArea = new QScrollArea(this);
	scrollArea->setWidget(w);
	m_stack->addWidget(scrollArea);
	m_stack->setFocusProxy(w);
	scrollArea->setFocusProxy(w);
	//    QHBoxLayout *l = new QHBoxLayout(this);
	//    l->addWidget(scrollArea);
	//    stack->setLayout(l);
	connect(scrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)),
			this, SLOT(scrollBarMoved(int)));
	connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
			this, SLOT(scrollBarMoved(int)));
	w->setContained(true);
	QRect outer = w->getOuterGeometry();
	
	this->setGeometry(outer);
	w->blockSignals(true);
	w->adjustLayoutSize();
	w->show();
	scrollArea->show();
	w->blockSignals(false);
}

WidgetLayout * WidgetPanel::takeWidgetLayout(QRect outerGeometry)
{
	//  qDebug() << "WidgetPanel::takeWidgetLayout()";
	disconnect(this,SIGNAL(topLevelChanged(bool)));
	QScrollArea *s = (QScrollArea*) m_stack->currentWidget();
	if (!s) // scroll area is sometimes null during startup and shutdown
        return nullptr;
    // This transfers ownership of the widget to the caller
    WidgetLayout *w = (WidgetLayout *) s->takeWidget();
    if (w != nullptr) {
		w->setOuterGeometry(outerGeometry);
		w->setContained(false);  // Must set before removing from container to get background
		disconnect(w, SIGNAL(selection(QRect)), 0, 0);
	}
	m_stack->removeWidget(s);
	delete s;
	return w;
}

QRect WidgetPanel::getOuterGeometry()
{
	QRect g;
	QScrollArea *s = (QScrollArea*) m_stack->currentWidget();
	if (s){
		if (x() >= 0 && y() >= 0) {
			g = this->geometry();
		}
	}
	return g;
}

void WidgetPanel::closeEvent(QCloseEvent * /*event*/)
{
	emit Close(false);
}

void WidgetPanel::contextMenuEvent(QContextMenuEvent *event)
{
	QScrollArea *s = (QScrollArea*) m_stack->currentWidget();
    Q_ASSERT(s != nullptr);
	static_cast<WidgetLayout *>(s->widget())->createContextMenu(event);
}

void WidgetPanel::mousePressEvent(QMouseEvent * event)
{
	QScrollArea *s = (QScrollArea*) m_stack->currentWidget();
	if (!s) // scroll area is sometimes null during startup and shutdown
		return;
	static_cast<WidgetLayout *>(s->widget())->mousePressEventParent(event);
}

void WidgetPanel::mouseReleaseEvent(QMouseEvent * event)
{
	QScrollArea *s = (QScrollArea*) m_stack->currentWidget();
	if (!s) // scroll area is sometimes null during startup and shutdown
		return;
	static_cast<WidgetLayout *>(s->widget())->mouseReleaseEventParent(event);
}

void WidgetPanel::mouseMoveEvent(QMouseEvent * event)
{
	QScrollArea *s = (QScrollArea*) m_stack->currentWidget();
	if (!s) // scroll area is sometimes null during startup and shutdown
		return;
	WidgetLayout *w  = static_cast<WidgetLayout *>(s->widget());
    if (w != nullptr) {
		w->mouseMoveEventParent(event);
	}
}

void WidgetPanel::scrollBarMoved(int /*value*/)
{
	QScrollArea *s = (QScrollArea*) m_stack->currentWidget();
	if (!s) // scroll area is sometimes null during startup and shutdown
		return;
	int v = s->verticalScrollBar()->value();
	int h = s->horizontalScrollBar()->value();
	static_cast<WidgetLayout *>(s->widget())->setMouseOffset(h, v);
}
