/*
	Copyright (C) 2009 Andres Cabrera
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

#include "inspector.h"
#include "types.h"
#include <chrono>
#include <QtGui>

Inspector::Inspector(QWidget *parent)
	: QDockWidget(parent)
{
	setWindowTitle(tr("Inspector"));
	m_treeWidget = new QTreeWidget(this);
//	m_treeWidget->setHeaderLabel(tr("Inspector"));
	m_treeWidget->setHeaderHidden(true);
	m_treeWidget->show();
	setWidget(m_treeWidget);
	//  connect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
	//          this, SLOT(itemActivated(QTreeWidgetItem*,int)));
	connect(m_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
			this, SLOT(itemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	//  connect(m_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
	//          this, SLOT(itemActivated(QTreeWidgetItem*,int)));
	treeItem1 = new TreeItem(m_treeWidget, QStringList(tr("Opcodes")));
	treeItem2 = new TreeItem(m_treeWidget, QStringList(tr("Macros")));
	treeItem3 = new TreeItem(m_treeWidget, QStringList(tr("Instruments")));
	treeItem4 = new TreeItem(m_treeWidget, QStringList(tr("F-tables")));
	treeItem5 = new TreeItem(m_treeWidget, QStringList(tr("Score")));

    m_treeWidget->expandItem(treeItem1);    // opcodes
    m_treeWidget->collapseItem(treeItem2);  // macros
    m_treeWidget->expandItem(treeItem3);    // instruments
    m_treeWidget->collapseItem(treeItem4);  // ftables
    m_treeWidget->collapseItem(treeItem5);    // score
    rxOpcode.setPattern("\\bopcode\\s+(\\w+),\\s*\\w+\\s*,\\s*\\w+");
    xinRx.setPattern("\\bxin\\b");
    // xinRx.setPattern("[akiS]\\w+[,\\w\\d\\s_]+\\bxin\\s*$");
    xoutRx.setPattern("\\bxout\\s+");
    // ftableRx1.setPattern("^f\\s*\\d");
    ftableRx2.setPattern("^\\w*\\s*ftgen");
    inspectLabels = false;
}


Inspector::~Inspector()
{
	delete m_treeWidget;
}


void Inspector::parseText(const QString &text)
{
    udosMap.clear();
    inspectorMutex.lock();

    // auto t1 = std::chrono::high_resolution_clock::now();

    bool treeItem1Expanded = true;
    bool treeItem2Expanded = true;
    bool treeItem3Expanded = true;
    bool treeItem4Expanded = true;
    bool treeItem5Expanded = true;
    bool insideOrc = false;
    if  (treeItem1 != 0) {
        treeItem1Expanded = treeItem1->isExpanded();
        treeItem2Expanded = treeItem2->isExpanded();
        treeItem3Expanded = treeItem3->isExpanded();
        treeItem4Expanded = (treeItem4 != 0 ? treeItem4->isExpanded() : false);
        treeItem5Expanded = (treeItem5 != 0 ? treeItem5->isExpanded() : false);
    }
    QHash<QString, bool> instrumentExpanded;  // Remember if instrument is expanded in menu
    for (int i = 0; i < treeItem3->childCount(); i++) {
        QTreeWidgetItem * instr = treeItem3->child(i);
        Q_ASSERT(instr->columnCount() > 0);
        instrumentExpanded[instr->text(0)] = instr->isExpanded();
    }

    m_treeWidget->clear();
    treeItem1 = new TreeItem(m_treeWidget, QStringList(tr("Opcodes")));
    treeItem1->setLine(-1);
    treeItem2 = new TreeItem(m_treeWidget, QStringList(tr("Preprocessor")));
    treeItem2->setLine(-1);
    treeItem3 = new TreeItem(m_treeWidget, QStringList(tr("Instruments")));
    treeItem3->setLine(-1);
    treeItem4 = new TreeItem(m_treeWidget, QStringList(tr("F-tables")));
    treeItem4->setLine(-1);
    treeItem5 = new TreeItem(m_treeWidget, QStringList(tr("Score")));
    treeItem5->setLine(-1);  // This might be overridden below
    TreeItem *currentInstrument = nullptr;
    TreeItem *currentItem = treeItem3;
    Opcode *currentOpcode = nullptr;
    int commentIndex = 0;
    bool partOfComment = false;
    int i = 0;
    auto lines = text.split('\n'); // was (old): splitRef
    // could be: auto lines = QStringView{text}.split(u'\n'); -  this requires a lot of changes below.
    // Since this function is not called so often, let's not overcomplicate.
    QRegularExpressionMatch match;
    static QRegularExpression orcStartRx("^\\s*<CsInstruments>");

    for(; i<lines.size(); i++) {
        if(orcStartRx.match(lines[i]).hasMatch()) {
            i++;
            treeItem3->setLine(i);
            insideOrc = true;
            break;
        }
    }

    // Parsing orchestra

    if(insideOrc) {
        for (; i< lines.size(); i++) {
            auto line = lines[i].trimmed();
            if (line.isEmpty() || line[0] == ';' || (line[0] == '/' && line[1] == '/'))
                continue;

            if (partOfComment) {
                if (line.indexOf("*/") != -1)
                    partOfComment = false;
                continue;
            }
            if (!partOfComment && (commentIndex=line.indexOf("/*")) != -1) {
                partOfComment = line.indexOf("*/", commentIndex) == -1;
                continue;
            }
            if (line[0] == '<') {
                if (!line.startsWith("</CsInstruments>"))
                    QDEBUG << "Malformed orchestra, tag" << line << "is invalid";
                break;
            }
            if(currentOpcode == nullptr && currentInstrument == nullptr) {
                // we are at instr 0
                if (line.startsWith("instr ")) {
                    auto instrline = line.mid(6).trimmed();
                    auto newItem = new TreeItem(treeItem3, QStringList(instrline));
                    newItem->setLine(i + 1);
                    currentInstrument = newItem;
                    currentItem = newItem;
                }
                else if(line.startsWith("opcode ") && (match=rxOpcode.match(line)).hasMatch()) {
                    auto opcodeName = match.captured(1);
                    auto itemtext = line.mid(7);
                    QStringList columnslist(itemtext);
                    if (treeItem1->childCount() == 0) { // set line for element to the first one found
                        treeItem1->setLine(i + 1);
                    }
                    TreeItem *newItem = new TreeItem(treeItem1, columnslist);
                    newItem->setLine(i + 1);
                    currentItem = newItem;
                    udosMap.insert(opcodeName, Opcode(opcodeName));
                    currentOpcode = &udosMap[opcodeName];
                }
                else if (line[0] == '#') {
                    if (line.startsWith("#define")) {
                        QString item = line.mid(8);
                        if (treeItem2->childCount() == 0) { // set line for element to the first one found
                            treeItem2->setLine(i + 1);
                        }
                        TreeItem *newItem = new TreeItem(treeItem2, QStringList(item));
                        newItem->setLine(i + 1);
                    } else if (line.startsWith("#include")) {
                        // TODO: parse include file
                        if (treeItem2->childCount() == 0) { // set line for element to the first one found
                            treeItem2->setLine(i + 1);
                        }
                        TreeItem *newItem = new TreeItem(treeItem2, QStringList(line));
                        newItem->setLine(i + 1);

                    }
                }
                else if(ftableRx2.match(line).hasMatch()) {
                    QStringList columnslist(line);
                    if (treeItem4->childCount() == 0) { // set line for element to the first one found
                        treeItem4->setLine(i + 1);
                    }
                    TreeItem *newItem = new TreeItem(treeItem4, columnslist);
                    newItem->setLine(i + 1);
                }
            }
            else if(currentOpcode != nullptr) {
                if (line == "endop") {
                    if(currentOpcode != nullptr) {
                        currentOpcode = nullptr;
                        currentItem = treeItem3;
                    }
                    else {
                        qDebug() << "endop found outside opcode definition, line" << (i+1);
                    }
                }
                else if(currentOpcode->inArgs.isEmpty() && (match=xinRx.match(line)).hasMatch()) {
                    currentOpcode->inArgs = line.mid(0, match.capturedStart()).simplified();
                    QStringList columnslist(currentOpcode->inArgs + " xin");
                    TreeItem *newItem = new TreeItem(currentItem, columnslist);
                    newItem->setLine(i + 1);
                }
                else if(currentOpcode->outArgs.isEmpty() && (match=xoutRx.match(line)).hasMatch()) {
                    currentOpcode->outArgs = line.mid(match.capturedEnd()).simplified();
                    QStringList columnslist("xout " + currentOpcode->outArgs);
                    TreeItem *newItem = new TreeItem(currentItem, columnslist);
                    newItem->setLine(i + 1);
                }
            }
            else if (line == "endin") {
                // everything between instruments is placed in the main instrument menu
                currentInstrument = nullptr;
                currentItem = treeItem3;
            }
        }
    }


    static QRegularExpression rxCsScore("^\\s*<CsScore>");
    for(; i< lines.size(); i++) {
        if (rxCsScore.match(lines[i]).hasMatch()) {
            treeItem5->setLine(i + 1);
            currentItem = treeItem5;
            break;
        }
    }

    treeItem1->setExpanded(treeItem1Expanded);
    treeItem2->setExpanded(treeItem2Expanded);
    treeItem3->setExpanded(treeItem3Expanded);
    treeItem4->setExpanded(treeItem4Expanded);
    treeItem5->setExpanded(treeItem5Expanded);

    for (int i = 0; i < treeItem3->childCount(); i++) {
        QTreeWidgetItem * instr = treeItem3->child(i);
        if (instrumentExpanded.contains(instr->text(0))) {
            instr->setExpanded(instrumentExpanded[instr->text(0)]);
        }
    }
    // std::chrono::duration<double, std::milli> ms_double = std::chrono::high_resolution_clock::now() - t1;
    // qDebug() << "Inspector::parseText: "<< ms_double.count() << "ms\n";
    inspectorMutex.unlock();

}

void Inspector::parsePythonText(const QString &text)
{
	//  qDebug() << "Inspector:parseText";
	inspectorMutex.lock();
	m_treeWidget->clear();
	treeItem1 = 0;
	treeItem1 = new TreeItem(m_treeWidget, QStringList(tr("Imports")));
	treeItem1->setLine(-1);
	treeItem2 = new TreeItem(m_treeWidget, QStringList(tr("Classes")));
	treeItem2->setLine(-1);
	treeItem3 = new TreeItem(m_treeWidget, QStringList(tr("Functions")));
	treeItem3->setLine(-1);
	treeItem4 = 0;
	treeItem5 = 0;
	TreeItem *currentParent = 0;
    static const QRegularExpression newLineRegex("[\\n\\r]");
    static const QRegularExpression defRegex("[\\s]+def ");
    static const QRegularExpression importRegex("\\bimport\\b");
    QStringList lines = text.split(newLineRegex);
	for (int i = 0; i< lines.size(); i++) {
		if (lines[i].trimmed().startsWith("class ")) {
			QStringList columnslist(lines[i].simplified());
			TreeItem *newItem = new TreeItem(treeItem2, columnslist);
			currentParent = newItem;
			m_treeWidget->expandItem(newItem);
            QFont itemFont = newItem->font(0);
            itemFont.setBold(true);
            newItem->setFont(1, itemFont);
			newItem->setLine(i + 1);
		}
        else if (lines[i].contains(defRegex)) {
			QStringList columnslist(lines[i].simplified());
			if (currentParent != 0) {
				TreeItem *newItem = new TreeItem(currentParent, columnslist);
				newItem->setLine(i + 1);
			}
		}
        else if (lines[i].trimmed().contains(importRegex)) {
			QStringList columnslist(lines[i].simplified());
			TreeItem *newItem = new TreeItem(treeItem1, columnslist);
			newItem->setLine(i + 1);
		}
		else if (lines[i].trimmed().startsWith("def ")) {
			QStringList columnslist(lines[i].simplified());
			TreeItem *newItem = new TreeItem(treeItem3, columnslist);
			newItem->setLine(i + 1);
		}
		else if (lines[i].contains("##")) {
			QStringList columnslist(lines[i].simplified());
			TreeItem *newItem = new TreeItem(treeItem3, columnslist);
            // newItem->setForeground (0, QBrush(Qt::darkGreen) );
			newItem->setLine(i + 1);
		}
	}
	m_treeWidget->expandItem(treeItem1);
	m_treeWidget->expandItem(treeItem2);
	m_treeWidget->expandItem(treeItem3);
	inspectorMutex.unlock();
}

void Inspector::focusInEvent (QFocusEvent * event)
{
	QWidget::focusInEvent(event);
	//  parseText();
}

void Inspector::closeEvent(QCloseEvent * /*event*/)
{
	//  qDebug() << "Inspector::closeEvent";
	emit Close(false);
}

void Inspector::itemActivated(QTreeWidgetItem * item, int /*column*/)
{
	int line = static_cast<TreeItem *>(item)->getLine();
	if (line >= 0) {
		emit jumpToLine(line);
	}
}

void Inspector::itemChanged(QTreeWidgetItem * newItem, QTreeWidgetItem * /*oldItem*/)
{
	if (newItem == 0) {
		return;
	}
	int line = static_cast<TreeItem *>(newItem)->getLine();
	if (line >= 0) {
		emit jumpToLine(line);
	}
}
