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

#include "dockhelp.h"
#include "ui_dockhelp.h"

#include <QtWidgets>


DockHelp::DockHelp(QWidget *parent)
	: QDockWidget(parent), ui(new Ui::DockHelp)
{
	ui->setupUi(this);
    findFlags = QTextDocument::FindFlags();
    setWindowTitle("Help"); // titlebar and overall layout
	setMinimumSize(400,200);

    ui->backButton->setIcon(QIcon(":/themes/breeze/br_prev.png"));
    ui->forwardButton->setIcon(QIcon(":/themes/breeze/br_next.png"));

    connect(ui->toggleFindButton, SIGNAL(toggled(bool)), this, SLOT(toggleFindBarVisible(bool)));
    connect(ui->backButton, SIGNAL(released()), this, SLOT(browseBack()));
	connect(ui->forwardButton, SIGNAL(released()), this, SLOT(browseForward()));
    // connect(ui->opcodesToolButton, SIGNAL(released()), this, SLOT(showOverview()));
	connect(ui->homeToolButton, SIGNAL(released()), this, SLOT(showManual()));
	connect(ui->findLine,SIGNAL(returnPressed()),this,SLOT(onReturnPressed()));
	connect(ui->findLine,SIGNAL(textEdited(QString)),this,SLOT(onTextChanged()));
	ui->findPreviousAct->setShortcut(QKeySequence::FindPrevious);
	ui->nextFindAct->setShortcut(QKeySequence::FindNext);
	connect(ui->findPreviousAct,SIGNAL(triggered()),this,SLOT(onPreviousButtonPressed()));
	connect(ui->nextFindAct,SIGNAL(triggered()),this,SLOT(onNextButtonPressed()));
	ui->previousFindButton->setDefaultAction(ui->findPreviousAct);
	ui->nextFindButton->setDefaultAction(ui->nextFindAct);

	connect(ui->caseBox,SIGNAL(stateChanged(int)),this,SLOT(onCaseBoxChanged(int)));
	connect(ui->wholeWordBox,SIGNAL(stateChanged(int)),this,SLOT(onWholeWordBoxChanged(int)));
	connect(ui->text, SIGNAL(anchorClicked(QUrl)), this, SLOT(followLink(QUrl)));

    ui->toggleFindButton->setChecked(false);
    ui->findLine->setVisible(false);
    ui->caseBox->setVisible(false);
    ui->wholeWordBox->setVisible(false);
    ui->label->setVisible(false);
    ui->nextFindButton->setVisible(false);
    ui->previousFindButton->setVisible(false);


    styleSheetLight = (
      "body {                                                                      "
      "    background-color: #f8f8f8 !important;                                   "
      "    color: #101010;                                                         "
      "}                                                                           "
      "a { text-decoration: none; }                                                "
      "td.linenos { background-color: #f0f0f0; padding-right: 10px; }              "
      "span.lineno { background-color: #f0f0f0; padding: 0 5px 0 5px; }            "
      "pre  { line-height: 100%;                                                   "
      "       background-color: #f2f2f2 !important;                                "
      "       font-size: 92% !important;}                                          "
      "code {                                                                      "
      "    color: #202020 !important;                                              "
      "    font-size: 92% !important;                                              "
      "    border: none !important;                                                "
      "}                                                                           "
      "body .hll { background-color: #ffffcc }                                     "
      "body .c { color: #408080; font-style: italic } /* Comment */                "
      "body .err { border: 1px solid #FF0000 } /* Error */                         "
      "body .k { color: #008000; font-weight: bold } /* Keyword */                 "
      "body .o { color: #666666 } /* Operator */                                   "
      "body .ch { color: #408080; font-style: italic } /* Comment.Hashbang */      "
      "body .cm { color: #408080; font-style: italic } /* Comment.Multiline */     "
      "body .cp { color: #BC7A00 } /* Comment.Preproc */                           "
      "body .cpf { color: #408080; font-style: italic } /* Comment.PreprocFile */  "
      "body .c1 { color: #408080; font-style: italic } /* Comment.Single */        "
      "body .cs { color: #408080; font-style: italic } /* Comment.Special */       "
      "body .gd { color: #A00000 } /* Generic.Deleted */                           "
      "body .ge { font-style: italic } /* Generic.Emph */                          "
      "body .gr { color: #FF0000 } /* Generic.Error */                             "
      "body .gh { color: #000080; font-weight: bold } /* Generic.Heading */        "
      "body .gi { color: #00A000 } /* Generic.Inserted */                          "
      "body .go { color: #888888 } /* Generic.Output */                            "
      "body .gp { color: #000080; font-weight: bold } /* Generic.Prompt */         "
      "body .gs { font-weight: bold } /* Generic.Strong */                         "
      "body .gu { color: #800080; font-weight: bold } /* Generic.Subheading */     "
      "body .gt { color: #0044DD } /* Generic.Traceback */                         "
      "body .kc { color: #008000; font-weight: bold } /* Keyword.Constant */       "
      "body .kd { color: #008000; font-weight: bold } /* Keyword.Declaration */    "
      "body .kn { color: #008000; font-weight: bold } /* Keyword.Namespace */      "
      "body .kp { color: #008000 } /* Keyword.Pseudo */                            "
      "body .kr { color: #008000; font-weight: bold } /* Keyword.Reserved */       "
      "body .kt { color: #B00040 } /* Keyword.Type */                              "
      "body .m { color: #666666 } /* Literal.Number */                             "
      "body .s { color: #BA2121 } /* Literal.String */                             "
      "body .na { color: #7D9029 } /* Name.Attribute */                            "
      "body .nb { color: #008000 } /* Name.Builtin */                              "
      "body .nc { color: #0000FF; font-weight: bold } /* Name.Class */             "
      "body .no { color: #880000 } /* Name.Constant */                             "
      "body .nd { color: #AA22FF } /* Name.Decorator */                            "
      "body .ni { color: #999999; font-weight: bold } /* Name.Entity */            "
      "body .ne { color: #D2413A; font-weight: bold } /* Name.Exception */         "
      "body .nf { color: #0000FF } /* Name.Function */                             "
      "body .nl { color: #A0A000 } /* Name.Label */                                "
      "body .nn { color: #0000FF; font-weight: bold } /* Name.Namespace */         "
      "body .nt { color: #008000; font-weight: bold } /* Name.Tag */               "
      "body .nv { color: #19177C } /* Name.Variable */                             "
      "body .ow { color: #AA22FF; font-weight: bold } /* Operator.Word */          "
      "body .w { color: #bbbbbb } /* Text.Whitespace */                            "
      "body .mb { color: #666666 } /* Literal.Number.Bin */                        "
      "body .mf { color: #666666 } /* Literal.Number.Float */                      "
      "body .mh { color: #666666 } /* Literal.Number.Hex */                        "
      "body .mi { color: #666666 } /* Literal.Number.Integer */                    "
      "body .mo { color: #666666 } /* Literal.Number.Oct */                        "
      "body .sa { color: #BA2121 } /* Literal.String.Affix */                      "
      "body .sb { color: #BA2121 } /* Literal.String.Backtick */                   "
      "body .sc { color: #BA2121 } /* Literal.String.Char */                       "
      "body .dl { color: #BA2121 } /* Literal.String.Delimiter */                  "
      "body .sd { color: #BA2121; font-style: italic } /* Literal.String.Doc */    "
      "body .s2 { color: #BA2121 } /* Literal.String.Double */                     "
      "body .se { color: #BB6622; font-weight: bold } /* Literal.String.Escape */  "
      "body .sh { color: #BA2121 } /* Literal.String.Heredoc */                    "
      "body .si { color: #BB6688; font-weight: bold } /* Literal.String.Interpol */"
      "body .sx { color: #008000 } /* Literal.String.Other */                      "
      "body .sr { color: #BB6688 } /* Literal.String.Regex */                      "
      "body .s1 { color: #BA2121 } /* Literal.String.Single */                     "
      "body .ss { color: #19177C } /* Literal.String.Symbol */                     "
      "body .bp { color: #008000 } /* Name.Builtin.Pseudo */                       "
      "body .fm { color: #0000FF } /* Name.Function.Magic */                       "
      "body .vc { color: #19177C } /* Name.Variable.Class */                       "
      "body .vg { color: #19177C } /* Name.Variable.Global */                      "
      "body .vi { color: #19177C } /* Name.Variable.Instance */                    "
      "body .vm { color: #19177C } /* Name.Variable.Magic */                       "
      "body .il { color: #666666 } /* Literal.Number.Integer.Long */               "
    );
    ui->text->document()->setDefaultStyleSheet(styleSheetLight);
    ui->text->setStyleSheet(
        "QTextEdit {                      "
        "    background-color: #f0f0f0;   "
        "    color: #202020;              "
        "}                                "

    );
}

DockHelp::~DockHelp()
{
}

bool DockHelp::hasFocus()
{
    return QDockWidget::hasFocus()
           || ui->text->hasFocus()
           || ui->findLine->hasFocus();
}

void DockHelp::loadFile(QString fileName, QString anchor) {
    if(!QFile::exists(fileName)) {
        ui->text->setText(tr("Not Found! Make sure the documentation path is set in the Configuration Dialog."));
		return;
	}

//#ifdef Q_OS_WIN32
//	QStringList searchPaths;
//	searchPaths << docDir;
//	ui->text->setSearchPaths(searchPaths);
//	QTextStream in(&file);
//	in.setAutoDetectUnicode(true);
//	ui->text->setHtml(in.readAll());
//#else

 // seems that setSource work now for Windows too. Needs testing
    if (QFileInfo(fileName).suffix() == ".md") {
        QFile file(fileName);
        QTextStream md(&file);
#if QT_VERSION >= 0x051400
        ui->text->document()->setMarkdown(md.readAll());
        ui->text->setHtml(ui->text->document()->toHtml());
#else
        ui->text->document()->setPlainText(md.readAll());
#endif
    }
    else {
        QUrl url = QUrl::fromLocalFile(fileName);
        if(!anchor.isEmpty()) {
            url.setUrl(url.toString() + "#" + anchor);
        }
        // qDebug() << "url:" << url << url.toString();
        ui->text->setSource(url);
    }


//#endif

}

void DockHelp::setIconTheme(QString theme)
{
    ui->backButton->setIcon(QIcon(QString(":/themes/%1/browse-prev.png").arg(theme)));
    ui->forwardButton->setIcon(QIcon(QString(":/themes/%1/browse-next.png").arg(theme)));
    ui->homeToolButton->setIcon(QIcon(QString(":/themes/%1/home.png").arg(theme)));
    ui->toggleFindButton->setIcon(QIcon(QString(":/themes/%1/edit-find.png").arg(theme)));
    ui->previousFindButton->setIcon(QIcon(QString(":/themes/%1/browse-prev.png").arg(theme)));
    ui->nextFindButton->setIcon(QIcon(QString(":/themes/%1/browse-next.png").arg(theme)));
}

void DockHelp::changeFontSize(int change)
{
    QFont font = ui->text->font();
    int currentSize = font.pointSize();
    font.setPointSize(currentSize+change);
    ui->text->setFont(font);
}

void DockHelp::closeEvent(QCloseEvent * /*event*/)
{
	emit Close(false);
}

void DockHelp::showManual()
{
	this->setVisible(true);
	this->loadFile(docDir + "/index.html");
}


void DockHelp::showGen()
{
	this->setVisible(true);
	this->loadFile(docDir + "/ScoreGenRef.html");
}

void DockHelp::showOverview()
{
	this->setVisible(true);
	this->loadFile(docDir + "/PartOpcodesOverview.html");
}

void DockHelp::showOpcodeQuickRef()
{
	this->setVisible(true);
	this->loadFile(docDir + "/MiscQuickref.html");
}

void DockHelp::browseBack()
{
	ui->text->backward();
}

void DockHelp::browseForward()
{
	ui->text->forward();
}

void DockHelp::followLink(QUrl url)
{
	if (url.host() == "") {
		// Will not follow external links for safety, only local files
		if (url.toString().endsWith(".csd")) {
            QString csdFile = url.toLocalFile().isEmpty() ? url.toString() : url.toLocalFile(); // necessary for windows 10
            emit openManualExample(csdFile);
		}
		else {
			if (!url.toString().endsWith("indexframes.html") ) {
				ui->text->setSource(url);
			}
			else { // Don't do anything with frames version...
				// This could be fixed using the WebKit rendering engine
				QMessageBox::warning(this, tr("CsoundQt"),
									 tr("Frames version only available in external browser."));
                 emit requestExternalBrowser(url);
			}
		}
	}
	else {
//		QMessageBox::warning(this, tr("CsoundQt"),
//							 tr("External links can't be followed in help browser."));
        emit requestExternalBrowser(url); // connected to CsoundQt::openInExternalBrowser
	}
}

void DockHelp::copy()
{
	ui->text->copy();
}

void DockHelp::onTextChanged()
{
	QTextCursor tmpCursor = ui->text->textCursor();
	tmpCursor.setPosition(ui->text->textCursor().selectionStart());
	ui->text->setTextCursor(tmpCursor);
	findFlags &= 6; // first bit (FindBackward) to zero
	findText(ui->findLine->text());
}

void DockHelp::onReturnPressed()
{
	findFlags &= 6; // first bit (FindBackward) to zero
	findText(ui->findLine->text());
}

void DockHelp::onNextButtonPressed()
{
	findFlags &= 6; // first bit to zero
	findText(ui->findLine->text());
}

void DockHelp::onPreviousButtonPressed()
{
	findFlags |= QTextDocument::FindBackward;
	findText(ui->findLine->text());
}


void DockHelp::onCaseBoxChanged(int value)
{
	if (value)
		findFlags |=  QTextDocument::FindCaseSensitively;
	else
		findFlags &= 5; // set 2 bit to 0
}

void DockHelp::onWholeWordBoxChanged(int value)
{
	if (value)
		findFlags |=  QTextDocument::FindWholeWords;
	else
		findFlags &= 3; // set 3rd bit to 0
}

void DockHelp::findText(QString expr)
{
	QTextCursor tmpCursor = ui->text->textCursor();
	if (!ui->text->find(expr,findFlags)) { // if not found, try from start
		ui->text->moveCursor(QTextCursor::Start);
		if (!ui->text->find(ui->findLine->text(),findFlags)) {
			ui->text->setTextCursor(tmpCursor); // if not found at all, restore position
		}
	} else {
		int cursorY = ui->text->cursorRect().top();
		QScrollBar *vbar = ui->text->verticalScrollBar();
		vbar->setValue(vbar->value() + cursorY);
	}
}

void DockHelp::resizeEvent(QResizeEvent *e)
{
	QDockWidget::resizeEvent(e);
    // ui->backButton->move(frameGeometry().width()/2-25, 0);
    // ui->forwardButton->move(frameGeometry().width()/2, 0);
}


void DockHelp::focusText() {
    ui->text->setFocus();
}

void DockHelp::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Escape) {
        toggleFindBarVisible(false);
    }
}

void DockHelp::toggleFindBarVisible(bool show) {
    ui->findLine->setVisible(show);
    ui->label->setVisible(show);
    ui->caseBox->setVisible(show);
    ui->wholeWordBox->setVisible(show);
    ui->nextFindButton->setVisible(show);
    ui->previousFindButton->setVisible(show);
    if(show) {
        ui->findLine->setFocus();
    }
}
