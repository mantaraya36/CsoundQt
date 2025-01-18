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

#include "configdialog.h"
#include "console.h"
#include "dockhelp.h"
#include "documentpage.h"
#include "highlighter.h"
#include "inspector.h"
#include "opentryparser.h"
#include "options.h"
#include "csoundqt.h"
#include "widgetpanel.h"
#include "utilitiesdialog.h"
#include "graphicwindow.h"
#include "keyboardshortcuts.h"
#include "about.h"
#include "appwizard.h"
#include "midihandler.h"
#include "midilearndialog.h"
#include "livecodeeditor.h"
#include "csoundhtmlview.h"
#include "risset.h"
#include <thread>


#include <csound_compiler.h> // csound7: needed for TREE structure


#ifdef Q_OS_WIN
#include <ole2.h> // for OleInitialize() FLTK bug workaround
#endif

#ifdef QCS_DEBUGGER
#include "debugpanel.h"
#endif

// One day remove these from here for nicer abstraction....
#include "csoundengine.h"
#include "documentview.h"
#include "widgetlayout.h"

#ifdef QCS_RTMIDI
#include "RtMidi.h"
#endif

#ifdef Q_OS_WIN32
static const QString SCRIPT_NAME = "csoundqt_run_script-XXXXXX.bat";
#else
static const QString SCRIPT_NAME = "csoundqt_run_script-XXXXXX.sh";
#endif

#define MAX_THREAD_COUNT 32 // to enable up to MAX_THREAD_COUNT documents/consoles have messageDispatchers

#define INSPECTOR_UPDATE_PERIOD_MS 3000


CsoundQt::CsoundQt(QStringList fileNames)
{
    m_closing = false;
    m_resetPrefs = false;
    utilitiesDialog = NULL;
    curCsdPage = -1;
    configureTab = 0;
    //	initialDir = QDir::current().path();
    initialDir = QCoreApplication::applicationDirPath();
    setWindowTitle("CsoundQt[*]");
    // resize(780,550);
	m_fullScreenComponent = QString();
    setWindowIcon(QIcon(":/images/csoundqt.png"));
    //Does this take care of the decimal separator for different locales?
    QLocale::setDefault(QLocale::system());
    curPage = -1;
    m_options = new Options(&m_configlists);

#ifdef Q_OS_MAC
    // this->setUnifiedTitleAndToolBarOnMac(true);
    // The unified toolbar has rendering problems if opengl is used, which happens
    // when any QtQuick widget is in use - that is the case with the MIDI keyboard widgetu
#endif


    // Create GUI panels

    helpPanel = new DockHelp(this);
    helpPanel->setAllowedAreas(Qt::RightDockWidgetArea |
                               Qt::BottomDockWidgetArea |
                               Qt::LeftDockWidgetArea);
    helpPanel->setObjectName("helpPanel");

    // QLabel *helpTitle = new QLabel("Help", helpPanel);
    // helpTitle->setStyleSheet("qproperty-alignment: AlignCenter; padding: 3px; font-size: 9pt; ");
    // helpPanel->setTitleBarWidget(helpTitle);

    helpPanel->show();
    addDockWidget(Qt::RightDockWidgetArea, helpPanel);

#ifdef Q_OS_WIN
    // Call OleInitialize  to enable clipboard together with FLTK libraries
    HRESULT result = OleInitialize(NULL);
    if (result) {
        qDebug()<<"Problem with OleInitialization" << result;
    }
#endif

    widgetPanel = new WidgetPanel(this);
    widgetPanel->setFocusPolicy(Qt::NoFocus);
    widgetPanel->setAllowedAreas(Qt::RightDockWidgetArea |
                                Qt::BottomDockWidgetArea |
                                Qt::LeftDockWidgetArea);
    widgetPanel->setObjectName("widgetPanel");
    // Hide until CsoundQt has finished loading
    widgetPanel->hide();
    // widgetPanel->show();

    addDockWidget(Qt::RightDockWidgetArea, widgetPanel);
    tabifyDockWidget(helpPanel, widgetPanel);

    m_console = new DockConsole(this);
    m_console->setObjectName("m_console");
    m_console->setFeatures(QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetClosable);
    QLabel *consoleTitle = new QLabel("Console", m_console);
    consoleTitle->setStyleSheet("qproperty-alignment: AlignCenter; padding: 3px; font-size: 9pt; ");
    m_console->setTitleBarWidget(consoleTitle);

    //   m_console->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    // addDockWidget(Qt::BottomDockWidgetArea, m_console);
    addDockWidget(Qt::RightDockWidgetArea, m_console);

    m_inspector = new Inspector(this);
    m_inspector->setObjectName("Inspector");
    m_inspector->parseText(QString());
    addDockWidget(Qt::LeftDockWidgetArea, m_inspector);
    m_inspector->hide();

#ifdef QCS_DEBUGGER
    m_debugPanel = new DebugPanel(this);
    m_debugPanel->setObjectName("Debug Panel");
    m_debugPanel->hide();
    connect(m_debugPanel, SIGNAL(runSignal()), this, SLOT(runDebugger()));
    connect(m_debugPanel, SIGNAL(pauseSignal()), this, SLOT(pauseDebugger()));
    connect(m_debugPanel, SIGNAL(continueSignal()), this, SLOT(continueDebugger()));
    connect(m_debugPanel, SIGNAL(addInstrumentBreakpoint(double, int)),
            this, SLOT(addInstrumentBreakpoint(double, int)));
    connect(m_debugPanel, SIGNAL(removeInstrumentBreakpoint(double)),
            this, SLOT(removeInstrumentBreakpoint(double)));
    connect(m_debugPanel, SIGNAL(addBreakpoint(int, int, int)),
            this, SLOT(addBreakpoint(int, int, int)));
    connect(m_debugPanel, SIGNAL(removeBreakpoint(int, int)),
            this, SLOT(removeBreakpoint(int, int)));
    addDockWidget(Qt::RightDockWidgetArea, m_debugPanel);
    m_debugEngine = NULL;
#endif

    m_scratchPad = new QDockWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_scratchPad);
    m_scratchPad->setObjectName("Interactive Code Pad");
    m_scratchPad->setWindowTitle(tr("Interactive Code Pad"));
    m_scratchPad->hide();
    connect(helpPanel, SIGNAL(openManualExample(QString)), this, SLOT(openManualExample(QString)));

    QSettings settings("csoundqt", "csoundqt");
    settings.beginGroup("GUI");
    m_options->theme = settings.value("theme", "breeze").toString();
    helpPanel->setIconTheme(m_options->theme);

    if(settings.contains("windowState")) {
        restoreState(settings.value("windowState").toByteArray());
    }
    settings.endGroup();

    settings.beginGroup("Options");
    settings.beginGroup("Editor");
    // necessary to get it before htmlview is created
    m_options->debugPort = settings.value("debugPort", 34711).toInt();
    m_options->highlightingTheme = settings.value("higlightingTheme", "light").toString();
    settings.endGroup();

    m_server = new QLocalServer();
    connect(m_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

#if defined(QCS_QTHTML)
#ifdef USE_WEBENGINE	// set the remote debugging port for chromium based web browser here
    //TODO: change it when user changes
    if (m_options->debugPort) {
        qDebug()<<"Set port "<< m_options->debugPort << " for remote html debugging";
        qputenv("QTWEBENGINE_REMOTE_DEBUGGING",
                QString::number(m_options->debugPort).toLocal8Bit().data() );
    }

#endif
    csoundHtmlView = new CsoundHtmlView(this);
    csoundHtmlView->setFocusPolicy(Qt::NoFocus);
    csoundHtmlView->setAllowedAreas(Qt::RightDockWidgetArea |
                                   Qt::BottomDockWidgetArea |
                                   Qt::LeftDockWidgetArea);
    csoundHtmlView->setObjectName("csoundHtmlView");
    csoundHtmlView->setWindowTitle(tr("HTML View"));
    csoundHtmlView->setOptions(m_options);
    addDockWidget(Qt::LeftDockWidgetArea, csoundHtmlView);
    csoundHtmlView->hide();
#endif

    focusMapper = new QSignalMapper(this);
    createActions(); // Must be before readSettings: this sets default shortcuts
    createStatusBar();
    createToolBars();
    readSettings();
    createMenus(); // creating menu must be after readSettings. probably create Status- and toolbars, too?
    this->setToolbarIconSize(m_options->toolbarIconSize);

    // this section was above before, check that it does not create problems...
    // Later: find a way to recreate Midi Handler, if API jack/alsa or similar is changed
    midiHandler = new MidiHandler(m_options->rtMidiApi,  this);
    m_midiLearn = new MidiLearnDialog(this);
    m_midiLearn->setModal(false);
    midiHandler->setMidiLearner(m_midiLearn);

    // Must be after readSettings() to save last state // was: isVisible()
    // in some reason reported always false
    bool widgetsVisible = !widgetPanel->isHidden();
    // To avoid showing and reshowing panels during initial load
    showWidgetsAct->setChecked(false);
    // Must be after readSettings() to save last state
    bool scratchPadVisible = !m_scratchPad->isHidden();
    if (scratchPadVisible)
        m_scratchPad->hide();  // Hide until CsoundQt has finished loading
    documentTabs = new QTabWidget (this);
    documentTabs->setTabsClosable(true);
    documentTabs->setMovable(true);
    connect(documentTabs, SIGNAL(currentChanged(int)), this, SLOT(changePage(int)));
    // To force changing to clicked tab before closing
    connect(documentTabs, SIGNAL(tabCloseRequested(int)),
            documentTabs, SLOT(setCurrentIndex(int)));
    connect(documentTabs, SIGNAL(tabCloseRequested(int)), closeTabAct, SLOT(trigger()));
    connect(documentTabs->tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int,int)));

    setCentralWidget(documentTabs);
    documentTabs->setDocumentMode(true);
    modIcon.addFile(":/images/modIcon2.png", QSize(), QIcon::Normal);
    modIcon.addFile(":/images/modIcon.png", QSize(), QIcon::Disabled);
    documentTabs->setMinimumSize(QSize(500, 500));

    // set shortcuts to change between tabs,Alt +<tab no>
    // example from: https://stackoverflow.com/questions/10160232/qt-designer-shortcut-to-another-tab
    // Setup a signal mapper to avoid creating custom slots for each tab
    // it is possible to switch it out from Cinfigure->Editor
    if (m_options->tabShortcutActive) {
        QSignalMapper *mapper = new QSignalMapper(this);
        // Setup the shortcut for tabs 1..10
        for (int i=0; i<10; i++) {
            int key = (i==9) ? 0 : i+1;
#ifdef Q_OS_MACOS
            QShortcut *shortcut = new  QShortcut(QKeySequence(Qt::META | (Qt::Key_0 + key)), this);
#else
            QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ALT | (Qt::Key_0 + key)), this);
#endif
            connect(shortcut, SIGNAL(activated()), mapper, SLOT(map()));
            mapper->setMapping(shortcut, i); // tab 0 -> Alt+1, tab 1 -> Alt + 2 etc tab 9 -> Alt + 0
        }
        // Wire the signal mapper to the tab widget index change slot
        connect(mapper, SIGNAL(mappedInt(int)), documentTabs, SLOT(setCurrentIndex(int)));
#ifdef Q_OS_MACOS
        QShortcut *tabLeft = new QShortcut(QKeySequence(Qt::META | Qt::Key_Left), this);
        QShortcut *tabRight = new QShortcut(QKeySequence(Qt::META | Qt::Key_Right), this);
#else
        QShortcut *tabLeft = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Left), this);
        QShortcut *tabRight = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Right), this);
#endif
        connect(tabLeft, SIGNAL(activated()), this, SLOT(pageLeft()));
        connect(tabRight, SIGNAL(activated()), this, SLOT(pageRight()));
    }

    fillFileMenu();     // Must be placed after readSettings to include recent Files
    fillFavoriteMenu(); // Must be placed after readSettings to know directory
    risset = new Risset(m_options->pythonExecutable);
    /*
#if defined(Q_OS_LINUX)
    m_rissetDataPath.setPath(QDir::home().filePath(".local/share/risset"));
#elif defined(Q_OS_MACOS)
    m_rissetDataPath.setPath(QDir::home().filePath("Library/Application Support/risset"));
#elif defined(Q_OS_WIN)
    m_rissetDataPath.setPath(QDir(QProcessEnvironment::systemEnvironment().value("LOCALAPPDATA")).filePath("risset"));
#endif
    */
    m_opcodeTree = new OpEntryParser(":/opcodes.xml");
    if(risset->isInstalled) {
        QString rissetOpcodesXml = risset->rissetOpcodesXml;
        if(!QFile::exists(rissetOpcodesXml)) {
            QDEBUG << "opcodes.xml not found, searched: " << rissetOpcodesXml;
            QDEBUG << "Calling risset to generate opcodes.xml";
            auto error = risset->generateOpcodesXml();
            if(error != RissetError::Ok) {
                QDEBUG << "Could not generate opcodes.xml";
                rissetOpcodesXml = "";
            }
        }
        if(!rissetOpcodesXml.isEmpty() && QFile::exists(rissetOpcodesXml)) {
            QDEBUG << "Parsing risset's opcodes.xml:" << rissetOpcodesXml;
            m_opcodeTree->parseOpcodesXml(rissetOpcodesXml);
            m_opcodeTree->sortOpcodes();
            risset->markOpcodeTree(m_opcodeTree);
        } else {
            QDEBUG << "Risset's opcodes.xml not found: " << rissetOpcodesXml;
        }
    }

    m_opcodeTree->setUdos(m_inspector->getUdosMap());

    LiveCodeEditor *liveeditor = new LiveCodeEditor(m_scratchPad, m_opcodeTree);
    liveeditor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    connect(liveeditor, SIGNAL(evaluate(QString)), this, SLOT(evaluate(QString)));
    connect(liveeditor, SIGNAL(enableCsdMode(bool)),
            scratchPadCsdModeAct, SLOT(setChecked(bool)));
    //	connect(scratchPadCsdModeAct, SIGNAL(toggled(bool)),
    //			liveeditor, SLOT(setCsdMode(bool)));
    m_scratchPad->setWidget(liveeditor);
    m_scratchPad->setFocusProxy(liveeditor->getDocumentView());
    scratchPadCsdModeAct->setChecked(true);

    // Open files passed in the command line. Here to make sure they are the active tab.
    foreach (QString fileName, fileNames) {
        if (QFile::exists(fileName)) {
            loadFile(fileName, m_options->autoPlay);
        }
        else {
            qDebug() << "CsoundQt::CsoundQt could not open file:" << fileName;
        }
    }

    if (documentPages.size() == 0 ) { // No files yet open. Open default
        QDEBUG << "No open files, opening default";
        newFile();
    }

    // try to find directory for manual, if not set
    QString docDir = QString(); //m_options->csdocdir;
    QString index = docDir + QString("/index.html");
    QStringList possibleDirectories;
#ifdef Q_OS_LINUX
    possibleDirectories  << "/usr/share/doc/csound-manual/html/"
                         << "/usr/share/doc/csound-doc/html/"
                         << QCoreApplication::applicationDirPath() + "/../share/doc/csound-manual/html/"   // for transportable apps like AppImage and perhas others
                         << QCoreApplication::applicationDirPath() + "/../share/doc/csound-doc/html/"   ;
#endif
#ifdef Q_OS_WIN
        QString programFilesPath = QDir::fromNativeSeparators(getenv("PROGRAMFILES"));
        QString programFilesPathx86 = QDir::fromNativeSeparators(getenv("PROGRAMFILES(X86)"));
        possibleDirectories << initialDir +"/doc/manual/" << programFilesPath + "/Csound6/doc/manual/" << programFilesPathx86 + "/Csound6/doc/manual/" <<  programFilesPath + "/Csound6_x64/doc/manual/" <<  programFilesPath + "/csound/doc/manual/";
#endif
#ifdef Q_OS_MACOS
     possibleDirectories <<  initialDir + QString("/../Frameworks/CsoundLib64.framework/Resources/Manual/") <<  "/Library/Frameworks/CsoundLib64.framework/Resources/Manual/";
#endif
     if (m_options->csdocdir.isEmpty() ||
            !QFile::exists(m_options->csdocdir+"/index.html") ) {
        foreach (QString dir, possibleDirectories) {
            qDebug() << "Looking manual in: " << dir;
            if (QFile::exists(dir+"/index.html")) {
                docDir = dir;
                qDebug()<<"Found manual in: "<<docDir;
                break;
            }
        }
    } else {
        docDir = m_options->csdocdir;
    }
    helpPanel->docDir = docDir;
    helpPanel->loadFile(docDir + "/index.html");

    applySettings();
    // createQuickRefPdf();

#ifdef Q_OS_MACOS // workaround to set resotre window size for Mac. Does not work within readSettings()
    QSettings settings2("csoundqt", "csoundqt");
    QSize size = settings2.value("GUI/size", QSize(800, 600)).toSize();
    resize(size);
#endif

    //openLogFile();

    // FIXME is there still need for no atexit?
    int init = csoundInitialize(0);
    if (init < 0) {
        qDebug("CsoundEngine::CsoundEngine() Error initializing Csound!\n"
               "CsoundQt will probably crash if you try to run Csound.");
    }
    // To finish settling dock widgets and other stuff before messing with
    // them (does it actually work?)
    qApp->processEvents();

    // This is a bad idea, breaks lots of subtle things, like background color
    // on files loaded from command line
#ifndef  Q_OS_MACOS // a workaround for showing close buttons on close NB! disable later
    auto palette = qApp->palette();
    // the palette is dark (dark "theme") if the background is darker than the text
    isDarkPalette = palette.text().color().lightness() > palette.window().color().lightness();

    auto originalStyleSheet = qApp->styleSheet();
    QFile file;
    if (isDarkPalette) {
        QDEBUG << "Using dark palette";
        file.setFileName(":/appstyle-dark.css");
    }
    else {
        QDEBUG << "Using light palette";
        file.setFileName(":/appstyle-white.css");
    }
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    // originalStyleSheet += styleSheet;
    // qApp->setStyleSheet(originalStyleSheet);
    qApp->setStyleSheet(styleSheet);

#endif

    m_closing = false;
    updateInspector(); //Starts update inspector thread

    // Starts updating things like a list of udos for the current page, etc.
    updateCurrentPageTask();

    showWidgetsAct->setChecked(widgetsVisible);
    if (!m_options->widgetsIndependent) {
        // FIXME: for some reason this produces a move event for widgetlayout with pos (0,0)
        widgetPanel->setVisible(widgetsVisible);
    }

    if (scratchPadVisible) { // Reshow scratch panel if necessary
        m_scratchPad->show();
    }
    // Initialize buttons to current state of panels
    showConsoleAct->setChecked(!m_console->isHidden());
    showHelpAct->setChecked(!helpPanel->isHidden());
    showInspectorAct->setChecked(!m_inspector->isHidden());
    showScratchPadAct->setChecked(!m_scratchPad->isHidden());

    //qDebug()<<"Max thread count: "<< QThreadPool::globalInstance()->maxThreadCount();
    QThreadPool::globalInstance()->setMaxThreadCount(MAX_THREAD_COUNT);

    // Open files saved from last session
    if (!lastFiles.isEmpty()) {
		// close the first, default page.
		//qDebug() << " First page: " << documentPages[0]->getFileName() << lastFiles.count();
		if (documentPages[0]->getFileName().isEmpty() && lastFiles.count()>0) { // first empty page
			deleteTab(0);
		}
        foreach (QString lastFile, lastFiles) {
            if (lastFile!="" && !lastFile.startsWith("untitled")) {
                loadFile(lastFile);
            }
        }
    }

    // restore last index after opening the documents
    if (lastTabIndex < documentPages.size() &&
            documentTabs->currentIndex() != lastTabIndex) {
        changePage(lastTabIndex);
        documentTabs->setCurrentIndex(lastTabIndex);
    }
    else {
        changePage(documentTabs->currentIndex());
    }

    // Make sure that we do have an open file
    if(documentPages.size() < 1)
        newFile();

/*
#ifdef Q_OS_LINUX
    // ---- this is workaround for problem reported by Renè that on first run ival = 16.0/3
    // gets rounded... Did not find the real reasound
    // Csound must be started and stopped once, then it works:
    int oldPage = documentTabs->currentIndex();
	QString tmp1 = lastUsedDir;
	QString tmp2 = lastFileDir;
    makeNewPage(QDir::tempPath()+"/tmp.csd",  QCS_DEFAULT_TEMPLATE);
    save();
    play(true, -1); // problem, if pulse/alsa in settings but jack has been started
    QThread::msleep(100); // wait a tiny bit
    stop();
    deleteTab(curPage);
    documentTabs->setCurrentIndex(oldPage);
	lastUsedDir = tmp1;
	lastFileDir = tmp2;
#endif
*/
}


CsoundQt::~CsoundQt()
{
    qDebug() << "CsoundQt::~CsoundQt()";
    // This function is not called... see closeEvent()
}

void CsoundQt::utilitiesMessageCallback(CSOUND *csound,
                                        int /*attr*/,
                                        const char *fmt,
                                        va_list args)
{
    DockConsole *console = (DockConsole *) csoundGetHostData(csound);
    QString msg;
    msg = msg.vasprintf(fmt, args);
    //  qDebug() << msg;
    console->appendMessage(msg);
}

void CsoundQt::changePage(int index)
{
    // Previous page has already been destroyed here (if it was closed).
    // Remember this is called when opening, closing or switching tabs (including loading).
    // First thing to do is blank the HTML page to prevent misfired API calls.
    if (index < 0) { // No tabs left
        qDebug() << "CsoundQt::changePage index < 0";
        return;
    }
    if (documentPages.size() > curPage
            && documentPages.size() > 0
            && documentPages[curPage]) {
#ifdef QCS_DEBUGGER
        if (documentPages[curPage]->getEngine()->m_debugging) {
            stop(); // TODO How to better handle this rather than forcing stop?
        }
#endif
        disconnect(documentPages[curPage], SIGNAL(stopSignal()),0,0);
        auto page = documentPages[curPage];
        page->hideWidgets();
        if (!m_options->widgetsIndependent) {
            QRect panelGeometry = widgetPanel->geometry();
            if (!widgetPanel->isFloating()) {
                panelGeometry.setX(-1);
                panelGeometry.setY(-1);
            }
            widgetPanel->takeWidgetLayout(panelGeometry);
        } else {
            page->setWidgetLayoutOuterGeometry(QRect());
        }
    }
    curPage = index;
    if (curPage >= 0 && curPage < documentPages.size() && documentPages[curPage] != NULL) {
        auto page = documentPages[curPage];
        setCurrentFile(page->getFileName());
        connectActions();
        //    documentPages[curPage]->passWidgetClipboard(m_widgetClipboard);
        if (!m_options->widgetsIndependent) {
            WidgetLayout *w = page->getWidgetLayout();
            widgetPanel->addWidgetLayout(w);
            setWidgetPanelGeometry();
        } else {
            WidgetLayout *w = page->getWidgetLayout();
            w->adjustWidgetSize();
        }
        if (!page->getFileName().endsWith(".csd") && !page->getFileName().isEmpty()) {
            widgetPanel->hide();
        }
        else {
            if (!m_options->widgetsIndependent) {
                page->showWidgets();
                widgetPanel->setVisible(showWidgetsAct->isChecked());
            }
            else {
                page->showWidgets(showWidgetsAct->isChecked());
            }
        }
        m_console->setWidget(page->getConsole());
        runAct->setChecked(page->isRunning());
        recAct->setChecked(page->isRecording());
        splitViewAct->setChecked(page->getViewMode() > 1);
        if (page->getFileName().endsWith(".csd")) {
            curCsdPage = curPage;
            // force parsing
            page->parseUdos(true);
        }
    }
#if defined(QCS_QTHTML)
    if (!documentPages.isEmpty()) {
        updateHtmlView();
    }
#endif
    m_inspectorNeedsUpdate = true;

	// set acceptsMidiCC for pages -  either all true, or only currrentPage true
	for (int i=0; i<documentPages.size(); i++ ) {
		if ( m_options->midiCcToCurrentPageOnly ) {
			if (i==curPage) {
				documentPages[i]->acceptsMidiCC = true;
			} else {
				documentPages[i]->acceptsMidiCC = false;
			}
		} else {
			documentPages[i]->acceptsMidiCC = true;
		}
	}
}

void CsoundQt::pageLeft()
{
    if (curPage >= 1) {
        //changePage(curPage-1);
        documentTabs->setCurrentIndex(curPage-1);

    }
}

void CsoundQt::pageRight()
{
    if (curPage < documentPages.count()-1) {
        //changePage(curPage+1);
        documentTabs->setCurrentIndex(curPage+1);

    }
}

void CsoundQt::setWidgetTooltipsVisible(bool visible)
{
    documentPages[curPage]->showWidgetTooltips(visible);
}

void CsoundQt::closeExtraPanels()
{
    if (m_console->isVisible()) {
        m_console->hide();
        showConsoleAct->setChecked(false);
    } else if (helpPanel->isVisible()) {
        helpPanel->hide();
        showHelpAct->setChecked(false);
    }
}

void CsoundQt::openExample()
{
    QObject *sender = QObject::sender();
    if (sender == 0)
        return;
    QAction *action = static_cast<QAction *>(sender);
    loadFile(action->data().toString());
    //   saveAs();
}

void CsoundQt::openTemplate()
{
    QObject *sender = QObject::sender();
    if (sender == 0)
        return;
    QAction *action = static_cast<QAction *>(sender);

    QString dir = lastUsedDir;
    QString templateName = action->data().toString();
    dir += templateName.mid(templateName.lastIndexOf("/") + 1);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Template As"), dir,
        tr("Known Files (*.csd *.html);;Csound Files (*.csd);;Html files (*.html);;All Files (*)",
        "Rename and save the template"));
    if (!fileName.isEmpty()) {


        if (QFile::exists(fileName))
        {
            QFile::remove(fileName);
        }
        bool result = QFile::copy(templateName, fileName); // copy or overwrite
        if (!result) {
            qDebug() << "Could not copy the template to " << fileName;
            return;
        }
        loadFile(fileName);
    } else {
        qDebug() << "No file selected";
    }

}

void CsoundQt::logMessage(QString msg)
{
    if (logFile.isOpen()) {
        logFile.write(msg.toLatin1());
    }
}

void CsoundQt::statusBarMessage(QString message)
{
    statusBar()->showMessage(message.replace("<br />", " "));
}

void CsoundQt::closeEvent(QCloseEvent *event)
{
    qDebug() ;
    m_closing = true;
    // this->showNormal();  // Don't store full screen size in preferences
    qApp->processEvents();
    storeSettings();

    if (!m_virtualKeyboardPointer.isNull() && m_virtualKeyboard->isVisible()) {
        showVirtualKeyboard(false);
    }
    if (!m_tableEditorPointer.isNull() && m_tableEditor->isVisible()) {
        showTableEditor(false);
    }

    // These two give faster shutdown times as the panels don't have to be
    // called up as the tabs close
    showWidgetsAct->setChecked(false);
    // Using this block this causes HTML5 performance to leave a zombie,
    // not using it causes a crash on exit.
    while (!documentPages.isEmpty()) {
        if (!closeTab(true)) { // Don't ask for closing app
            event->ignore();
            return;
        }
    }
    foreach (QString tempFile, tempScriptFiles) {
        QDir().remove(tempFile);
    }
    if (logFile.isOpen()) {
        logFile.close();
    }
    showUtilities(false);  // Close utilities dialog if open
    delete helpPanel;
    //  delete closeTabButton;
    delete m_options;
    m_closing = true;
    widgetPanel->close();
    m_inspector->close();
    m_console->close();
    documentTabs->close();
    m_console->close();
    delete m_opcodeTree;
    m_opcodeTree = nullptr;
    close();
}

void CsoundQt::newFile()
{
    if (loadFile(":/default.csd") < 0) {
        return;
    }
    documentPages[curPage]->loadTextString(m_options->csdTemplate);
    documentPages[curPage]->setFileName("");
    setWindowModified(false);
    // documentTabs->setTabIcon(curPage, modIcon);
    documentTabs->setTabText(curPage, "default.csd");
    //   documentPages[curPage]->setTabStopWidth(m_options->tabWidth);
    connectActions();
}

void CsoundQt::open()
{
    QStringList fileNames;
    bool widgetsVisible = widgetPanel->isVisible();
    if (widgetsVisible && widgetPanel->isFloating())
        widgetPanel->hide(); // Necessary for Mac, as widget Panel covers open dialog
    bool helpVisible = helpPanel->isVisible();
    if (helpVisible && helpPanel->isFloating())
        helpPanel->hide(); // Necessary for Mac, as widget Panel covers open dialog
    bool inspectorVisible = m_inspector->isVisible();
    if (inspectorVisible && m_inspector->isFloating())
        m_inspector->hide(); // Necessary for Mac, as widget Panel covers open dialog
    fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"), lastUsedDir ,
                                              tr("Known Files (*.csd *.orc *.sco *.py *.inc *.udo *.html);;Csound Files (*.csd *.orc *.sco *.inc *.udo *.CSD *.ORC *.SCO *.INC *.UDO);;Python Files (*.py);;HTML files (*.html);;All Files (*)",
                                                 "Be careful to respect spacing parenthesis and usage of punctuation"));
    //	if (widgetsVisible) {
    //		if (!m_options->widgetsIndependent) {
    //			//      widgetPanel->show();
    //		}
    //	}
    if (helpVisible)
        helpPanel->show();
    if (inspectorVisible)
        m_inspector->show();
    foreach (QString fileName, fileNames) {
        if (!fileName.isEmpty()) {
            loadFile(fileName, m_options->autoPlay);
        }
    }
}

void CsoundQt::reload()
{
    if (documentPages[curPage]->isModified()) {
        QString fileName = documentPages[curPage]->getFileName();
        deleteTab();
        loadFile(fileName);
        // auto doc = documentPages[curPage];

    } else {
        QMessageBox::information(nullptr, "Reload", "File was not modified");
    }
}

void CsoundQt::openFromAction()
{
    QString fileName = static_cast<QAction *>(sender())->data().toString();
    openFromAction(fileName);
}

void CsoundQt::openFromAction(QString fileName)
{
    if (!fileName.isEmpty()) {
        if ( (fileName.endsWith(".sco") || fileName.endsWith(".orc"))
             && m_options->autoJoin) {

        }
        else {
            loadCompanionFile(fileName);
            loadFile(fileName);
        }
    }
}

void CsoundQt::runScriptFromAction()
{
    QString fileName = static_cast<QAction *>(sender())->data().toString();
    runScript(fileName);
}

void CsoundQt::runScript(QString fileName)
{
    if (!fileName.isEmpty()) {
// this was for pythonconsole before probably not necessary any more
    }
}

void CsoundQt::createCodeGraph()
{
    QString command = m_options->dot + " -V";
#ifdef Q_OS_WIN32
    // add quotes surrounding dot command if it has spaces in it
    if (m_options->dot.contains(" "))
        command.replace(m_options->dot, "\"" + m_options->dot + "\"");
    // replace linux/mac style directory separators with windows style separators
    command.replace("/", "\\");
#endif

    int ret = system(command.toLatin1());
    if (ret != 0) {
        QMessageBox::warning(this, tr("CsoundQt"),
                             tr("Dot executable not found.\n"
                                "Please install graphviz from\n"
                                "www.graphviz.org"));
        return;
    }
    QString dotText = documentPages[curPage]->getDotText();
    if (dotText.isEmpty()) {
        qDebug() << "Empty dot text.";
        return;
    }
    qDebug() << dotText;
    QTemporaryFile file(QDir::tempPath() + QDir::separator() + "CsoundQt-GraphXXXXXX.dot");
    QTemporaryFile pngFile(QDir::tempPath() + QDir::separator() + "CsoundQt-GraphXXXXXX.png");
    if (!file.open() || !pngFile.open()) {
        QMessageBox::warning(this, tr("CsoundQt"),
                             tr("Cannot create temp dot/png file."));
        return;
    }

    QTextStream out(&file);
    out << dotText;
    file.close();
    file.open();

    command = "\"" + m_options->dot + "\"" + " -Tpng -o \"" + pngFile.fileName() + "\" \"" + file.fileName() + "\"";

#ifdef Q_OS_WIN32
    // remove quotes surrounding dot command if it doesn't have spaces in it
    if (!m_options->dot.contains(" "))
        command.replace("\"" + m_options->dot + "\"", m_options->dot);
    // replace linux/mac style directory separators with windows style separators
    command.replace("/", "\\");
    command.prepend("\"");
    command.append("\"");
#endif
    //   qDebug() << command;
    ret = system(command.toLatin1());
    if (ret != 0) {
        qDebug() << "CsoundQt::createCodeGraph() Error running dot";
    }
    m_graphic = new GraphicWindow(this);
    m_graphic->show();
    m_graphic->openPng(pngFile.fileName());
    connect(m_graphic, SIGNAL(destroyed()), this, SLOT(closeGraph()));
}

void CsoundQt::closeGraph()
{
    qDebug("CsoundQt::closeGraph()");
}

bool CsoundQt::save()
{
    QString fileName = documentPages[curPage]->getFileName();
    if (fileName.isEmpty() || fileName.startsWith(":/examples/", Qt::CaseInsensitive)) {
        return saveAs();
    }
    else if (documentPages[curPage]->readOnly){
        if (saveAs()) {
            documentPages[curPage]->readOnly = false;
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return saveFile(fileName);
    }
}

void CsoundQt::copy()
{
    if (helpPanel->hasFocus()) {
        helpPanel->copy();
    }
    else if (m_console->widgetHasFocus()) {
        m_console->copy();
    }
    else {
        documentPages[curPage]->copy();
    }
}

void CsoundQt::cut()
{
    documentPages[curPage]->cut();
}

void CsoundQt::paste()
{
    documentPages[curPage]->paste();
}

void CsoundQt::undo()
{
    //  qDebug() << "CsoundQt::undo()";
    documentPages[curPage]->undo();
}

void CsoundQt::redo()
{
    documentPages[curPage]->redo();
}

void CsoundQt::evaluateSection()
{
    QString text;
    if (static_cast<LiveCodeEditor *>(m_scratchPad->widget())->getDocumentView()->hasFocus()) {
        text = static_cast<LiveCodeEditor *>(m_scratchPad->widget())->getDocumentView()->getActiveText();
    }
    else {
        text = documentPages[curPage]->getActiveSection();
    }
    evaluateString(text);
}

void CsoundQt::evaluate(QString code)
{
    QString evalCode;
    if (code.isEmpty()) { //evaluate current selection in current document
        if (static_cast<LiveCodeEditor *>(m_scratchPad->widget())->getDocumentView()->hasFocus()) {
            evalCode = static_cast<LiveCodeEditor *>(m_scratchPad->widget())->getDocumentView()->getActiveText();
        }
        else {
            evalCode = documentPages[curPage]->getActiveText();
            if (evalCode.count("\n") <= 1) {
                documentPages[curPage]->gotoNextRow();
            }
        }
    }
    else {
        evalCode = code;
    }
    if (evalCode.size() > 1) { // evalCode can be \n and that crashes csound...
        evaluateString(evalCode);
    }
}


void CsoundQt::evaluateCsound(QString code)
{
    documentPages[curPage]->sendCodeToEngine(code);
}

void CsoundQt::breakpointReached()
{
#ifdef QCS_DEBUGGER
    Q_ASSERT(m_debugEngine);
    m_debugPanel->setVariableList(m_debugEngine->getVaribleList());
    m_debugPanel->setInstrumentList(m_debugEngine->getInstrumentList());
    documentPages[curPage]->getView()->m_mainEditor->setCurrentDebugLine(m_debugEngine->getCurrentLine());
#endif
}


void CsoundQt::evaluateString(QString evalCode)
{
    TREE *testTree = NULL;
    if  (scratchPadCsdModeAct->isChecked()) {
        // first check if it is a scoreline, then if it is csound code, if that also that fails, try with python
        if (QRegularExpression("[if]\\s*-*[0-9]+\\s+[0-9]+\\s+[0-9]+.*\\n").match(evalCode).hasMatch()) {
            sendEvent(evalCode);
            return;
        }
        CSOUND *csound = getEngine(curPage)->getCsound();
        if (csound!=NULL) {
            testTree = csoundParseOrc(csound,evalCode.toLocal8Bit()); // return not NULL, if the code is valid
            if (testTree == NULL) {
                qDebug() << "Not Csound code or cannot compile";
            }
        }
        if (testTree) { // when the code is csound code, but with errors, it will be sent to python interpreter too
            evaluateCsound(evalCode);
            return;
        }
    }
}

void CsoundQt::setScratchPadMode(bool csdMode)
{
    static_cast<LiveCodeEditor *>(m_scratchPad->widget())->setCsdMode(csdMode);
}

void CsoundQt::setWidgetEditMode(bool active)
{
    for (int i = 0; i < documentPages.size(); i++) {
        documentPages[i]->setWidgetEditMode(active);
    }
}

void CsoundQt::duplicate()
{
    qDebug() << "CsoundQt::duplicate()";
    documentPages[curPage]->duplicateWidgets();
}

void CsoundQt::testAudioSetup()
{
    qDebug() << "CsoundQt::testAudioSetup";
    loadFile(":/AudioMidiTest.csd");
    widgetPanel->setVisible(true);
    widgetPanel->setFocus();
    play();
}

void CsoundQt::checkSyntaxMenuAction()
{
    bool prev = m_options->checkSyntaxOnly;
    m_options->checkSyntaxOnly = true;
    play();
    m_options->checkSyntaxOnly = prev;
}

void CsoundQt::tabMoved(int to, int from)  // arguments should be from, but probably the other (counter-)moving tab is reported here, not the one that is dragged
{
    qDebug() << "Tab moved from " << from << " to: " << to;
    if (to<0 || to>=documentPages.count() || from<0 || from>=documentPages.count() ) {
        qDebug() << "Tab index out of range";
        return;
    }
    std::swap( documentPages[to], documentPages[from] );
    curPage = to;
    changePage(to);
}

void CsoundQt::openExamplesFolder()
{
    QUrl examplePath = QUrl::fromLocalFile(QFileInfo(getExamplePath("")).absoluteFilePath());
    qDebug() << "Trying to open Examples folder" << examplePath;
    QDesktopServices::openUrl(examplePath);
}

void CsoundQt::setLineAndColumn(int line, int column)
{
    QString labelText = QString("%1 : %2, %3 : ").arg(tr("Line")).arg(line).arg(tr("Column"));
    if (column>80) {
        QString color = "red"; //isDarkPalette ? "#FF4C4C" : "red";
        labelText += QString("<span style='color:%1;'><b>%2</b></span>").arg(color).arg(column);
    } else {
        labelText +=  QString::number(column);
    }
    lineAndColumnLabel->setText(labelText);
}

QString CsoundQt::getSaveFileName()
{
    bool widgetsVisible = widgetPanel->isVisible();
    if (widgetsVisible && widgetPanel->isFloating())
        widgetPanel->hide(); // Necessary for Mac, as widget Panel covers open dialog
    bool helpVisible = helpPanel->isVisible();
    if (helpVisible && helpPanel->isFloating())
        helpPanel->hide(); // Necessary for Mac, as widget Panel covers open dialog
    bool inspectorVisible = m_inspector->isVisible();
    if (inspectorVisible && m_inspector->isFloating())
        m_inspector->hide(); // Necessary for Mac, as widget Panel covers open dialog
    QString dir = lastUsedDir;
    QString name = documentPages[curPage]->getFileName();
    dir += name.mid(name.lastIndexOf("/") + 1);
    QString fileName = QFileDialog::getSaveFileName(
                this, tr("Save File As"), dir,
                tr("Known Files (*.csd *.orc *.sco *.py *.udo *.html);;"
                   "Csound Files (*.csd *.orc *.sco *.udo *.inc *.CSD *.ORC"
                   "*.SCO *.UDO *.INC);;Python Files (*.py);;"
                   "Html files (*.html);;All Files (*)",
                   "Be careful to respect spacing parenthesis and usage of punctuation"));
    if (widgetsVisible) {
        if (!m_options->widgetsIndependent) {
            widgetPanel->show(); // Necessary for Mac, as widget Panel covers open dialog
        }
    }
    if (helpVisible)
        helpPanel->show(); // Necessary for Mac, as widget Panel covers open dialog
    if (inspectorVisible)
        m_inspector->show(); // Necessary for Mac, as widget Panel covers open dialog
    if (fileName.isEmpty())
        return QString("");
    if (isOpen(fileName) != -1 && isOpen(fileName) != curPage) {
        QMessageBox::critical(this, tr("CsoundQt"),
                              tr("The file is already open in another tab.\nFile not saved!"),
                              QMessageBox::Ok | QMessageBox::Default);
        return QString("");
    }
    //  if (!fileName.endsWith(".csd",Qt::CaseInsensitive) && !fileName.endsWith(".orc",Qt::CaseInsensitive)
    //    && !fileName.endsWith(".sco",Qt::CaseInsensitive) && !fileName.endsWith(".txt",Qt::CaseInsensitive)
    //    && !fileName.endsWith(".udo",Qt::CaseInsensitive))
    //    fileName += ".csd";
    if (fileName.isEmpty()) {
        fileName = name;
    }
    if (!fileName.contains(".")) {
        fileName += ".csd";
    }
    return fileName;
}

void CsoundQt::createQuickRefPdf()
{
    QString tempFileName(QDir::tempPath() + QDir::separator() +
                         "QuteCsound Quick Reference.pdf");
    QString internalFileName = ":/doc/QuteCsound Quick Reference (0.4)-";
    internalFileName += m_configlists.languageCodes[m_options->language];
    internalFileName += ".pdf";
    if (!QFile::exists(internalFileName)) {
        internalFileName = ":/doc/QuteCsound Quick Reference (0.4).pdf";
    }
    //  qDebug() << "CsoundQt::createQuickRefPdf() Opening " << internalFileName;
    QFile file(internalFileName);
    file.open(QIODevice::ReadOnly);
    QFile quickRefFile(tempFileName);
    quickRefFile.open(QFile::WriteOnly);
    QDataStream quickRefIn(&quickRefFile);
    quickRefIn << file.readAll();
    quickRefFileName = tempFileName;
}

void CsoundQt::deleteTab(int index)
{
    if (index == -1) {
        index = curPage;
    }
    DocumentPage *d = documentPages[index];
    d->stop();
    midiHandler->removeListener(d);
    if (!m_options->widgetsIndependent) {
        QRect panelGeometry = widgetPanel->geometry();
        if (!widgetPanel->isFloating()) {
            panelGeometry.setX(-1);
            panelGeometry.setY(-1);
        }
        widgetPanel->takeWidgetLayout(panelGeometry);
    }
    documentPages.remove(index);
    documentTabs->removeTab(index);
    delete  d;
    if (curPage >= documentPages.size()) {
        curPage = documentPages.size() - 1;
    }
    if (curPage < 0)
        curPage = 0; // deleting the document page decreases curPage, so must check
}

void CsoundQt::openLogFile()
{
    if (logFile.isOpen()) {
        logFile.close();
    }
    logFile.setFileName(m_options->logFile);
    if (m_options->logFile.isEmpty())
        return;
    if (logFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        logFile.readAll();
        QString text = "--**-- " + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")
                + " CsoundQt Logging Started: "
                + "\n";
        logFile.write(text.toLatin1());
    }
    else {
        qDebug() << "CsoundQt::openLogFile() Error. Could not open log file! NO logging. " << logFile.fileName();
    }
}

void CsoundQt::showNewFormatWarning()
{
    QMessageBox::warning(this, tr("New widget format"),
                         tr("This version of CsoundQt implements a new format for storing"
                            " widgets, which enables many of the new widget features you "
                            "will find now.\n"
                            "The old format is still read and saved, so you will be able "
                            "to open files in older versions but some of the features will "
                            "not be passed to older versions.\n"),
                         QMessageBox::Ok);
}

void CsoundQt::setupEnvironment()
{
    if (m_options->sadirActive){
        int ret = csoundSetGlobalEnv("SADIR", m_options->sadir.toLocal8Bit().constData());
        if (ret != 0) {
            qDebug() << "CsoundEngine::runCsound() Error setting SADIR";
        }
    }
    else {
        csoundSetGlobalEnv("SADIR", "");
    }
    if (m_options->ssdirActive){
        int ret = csoundSetGlobalEnv("SSDIR", m_options->ssdir.toLocal8Bit().constData());
        if (ret != 0) {
            qDebug() << "CsoundEngine::runCsound() Error setting SSDIR";
        }
    }
    else {
        csoundSetGlobalEnv("SSDIR", "");
    }
    if (m_options->sfdirActive){
        int ret = csoundSetGlobalEnv("SFDIR", m_options->sfdir.toLocal8Bit().constData());
        if (ret != 0) {
            qDebug() << "CsoundEngine::runCsound() Error setting SFDIR";
        }
    }
    else {
        csoundSetGlobalEnv("SFDIR", "");
    }
    if (m_options->incdirActive){
        int ret = csoundSetGlobalEnv("INCDIR", m_options->incdir.toLocal8Bit().constData());
        if (ret != 0) {
            qDebug() << "CsoundEngine::runCsound() Error setting INCDIR";
        }
    }
    else {
        csoundSetGlobalEnv("INCDIR", "");
    }
    // set rawWavePath for stk opcodes
    QString rawWavePath = m_options->rawWaveActive ? m_options->rawWave : QString(getenv("RAWWAVE_PATH"));
    if (rawWavePath.isNull()) {
        rawWavePath=QString(""); // NULL string may crash csoundInitilaize
    }
    if ( csoundSetGlobalEnv("RAWWAVE_PATH", rawWavePath.toLocal8Bit().constData()) ) {
        qDebug() << "CsoundEngine::runCsound() Error setting RAWWAVE_PATH";
    }
#ifdef Q_OS_UNIX
    // make sure the the environment variable is set for stk opcodes
    setenv("RAWWAVE_PATH",rawWavePath.toLocal8Bit(),1);
#endif
#ifdef Q_OS_WIN
    QString envString = "RAWWAVE_PATH="+rawWavePath;
    _putenv(envString.toLocal8Bit());
#endif
    // csoundGetEnv must be called after Compile or Precompile,
    // But I need to set OPCODEDIR before compile.... So I can't know keep the old OPCODEDIR
    if (m_options->opcode7dir64Active) {
        csoundSetGlobalEnv("OPCODE7DIR64", m_options->opcode7dir64.toLatin1().constData());
    }
#ifdef Q_OS_WIN32
	// if opcodes are in the same directory or in ./plugins64, then set OPCODE7DIR64 to the bundled plugins
	// no need to support 32-opcodes any more, set only OPCODE7DIR64
	QString opcodedir;
	if (QFile::exists(initialDir+"/rtpa.dll" )) {
		opcodedir = initialDir;
	} else if (QFile::exists(initialDir+"/plugins64/rtpa.dll" )) {
		opcodedir = initialDir+"/plugins64/";
	} else {
		opcodedir = QString();
	}
	if (!opcodedir.isEmpty()) {
        qDebug() << "Setting OPCODE7DIR64 to: " << opcodedir;
        csoundSetGlobalEnv("OPCODE7DIR64", opcodedir.toLocal8Bit().constData());
	}
#endif
#ifdef Q_OS_MACOS
    // Use bundled opcodes if available
    QString opcodedir = initialDir + "/../Frameworks/CsoundLib64.framework/Resources/Opcodes64";
    if (QFile::exists(opcodedir)) {
        csoundSetGlobalEnv("OPCODE7DIR64", opcodedir.toLocal8Bit().constData());
    }
#endif
}

void CsoundQt::onNewConnection()
{
    qDebug()<<"NEW connection";
    QLocalSocket *client = m_server->nextPendingConnection();
    connect(client, SIGNAL(disconnected()), client, SLOT(deleteLater()));
    connect(client, SIGNAL(readyRead()), this, SLOT(onReadyRead()) );
}

void CsoundQt::onReadyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket || !socket->bytesAvailable())
        return;
    QByteArray ba = socket->readAll();
    if (ba.isEmpty())
        return;
    else {
        QStringList messageParts = QString(ba).split("***");
        if (messageParts[0].contains("open")) {
            messageParts.removeAt(0); // other parts should be filenames
            foreach (QString fileName, messageParts) {
                qDebug() << "Call from other instance. Opening "<<fileName;
                loadFile(fileName);
                //to bring window to front:
                this->raise();
                this->activateWindow();
                this->showNormal();
            }
        }
    }
}

void CsoundQt::disableInternalRtMidi()
{
#ifdef QCS_RTMIDI
    midiHandler->closeMidiInPort();
    midiHandler->closeMidiOutPort();
#endif
}

void CsoundQt::focusToTab(int tab) {
    QDockWidget *panel = nullptr;
    QAction * action = nullptr;
    qDebug()<<tab;
    switch (tab) {
    case 1:
        qDebug()<<"Raise widgets";
        if(m_options->widgetsIndependent) {
            QDEBUG << "Widget panel is an independent window";
            auto doc = this->documentPages[this->curPage];
            auto wl = doc->getWidgetLayout();
            if(wl->isVisible()) {
                wl->setVisible(false);
                showWidgetsAct->setChecked(false);
            } else {
                // not visible
                wl->setVisible(true);
                showWidgetsAct->setChecked(true);
            }
            return;
        } else {
           panel = widgetPanel;
           action = showWidgetsAct;
           break;
        }
    case 2:
        qDebug()<<"Raise help";
        panel = helpPanel;
        action = showHelpAct;
        break;
    case 3:
        qDebug()<<"Raise console";
        panel = m_console;
        action = showConsoleAct;
        break;
    case 4:
        qDebug()<<"Raise html panel";
    #ifdef QCS_QTHTML
        panel = csoundHtmlView;
        action = showHtml5Act;
    #endif
        break;
    case 5:
        qDebug()<<"Raise inspector";
        panel = m_inspector;
        action = showInspectorAct;
        break;
    // 6 is  Live events that is independent window

    case 7:
        qDebug()<<"Raise Python Console";
        // TODO: remove later
        break;

    case 8:
        qDebug()<<"Raise Code Pad";
        panel = m_scratchPad;
        action = showScratchPadAct;
        break;
    default:
        qDebug() << "focusToTab: unknoun tab" << tab;
        return;
    }

    if(!panel)
        return;

    if (panel->isFloating() && panel->isVisible()) {
        // if as separate window, close it shortcut activated again
        QDEBUG << "panel is floating, hiding";
        panel->setVisible(false);
        action->setChecked(false);
        return;
    }
    if (!panel->isVisible()) {
        panel->show();
        action->setChecked(true);
    }
    panel->raise();
}

void CsoundQt::ambiguosShortcut()
{
    QMessageBox::warning(this, tr("Shortcuts changed"),
        tr("Ambiguous shortcut. In version 0.9.5 some shortcuts changed, please configure "
           "the shourtcuts or set to defaults "
           "(Edit/Configure shortcuts/Restore Defaults) "));

}

bool CsoundQt::saveAs()
{
    QString fileName = getSaveFileName();
    if (!fileName.isEmpty() && saveFile(fileName)) {
        return true;
    }
    return false;
}

void CsoundQt::createApp()
{
    QString opcodeDir;
    if (!documentPages[curPage]->getFileName().endsWith(".csd")) {
        QMessageBox::critical(this, tr("Error"),
                              tr("You can only create an app with a csd file."));
        return;
    }
    if (documentPages[curPage]->isModified() ||
            documentPages[curPage]->getFileName().startsWith(":/")) {
        auto but = QMessageBox::question(
                    this, tr("Save"),
                    tr("Do you want to save before creating app?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (but == QMessageBox::Yes) {
            bool ret = save();
            if (!ret) {
                qDebug() << "CsoundQt::createApp() Error saving file";
                return;
            }
        } else {
            QMessageBox::critical(this, tr("Abort"),
                                  tr("You must save the csd before creating the App!"));
            return;
        }
    }

    if (m_options->opcode7dir64Active) {
        // FIXME allow for OPCODEDIR64 if built for doubles
        opcodeDir = m_options->opcode7dir64.toLocal8Bit();
    }
    else {
#ifdef USE_DOUBLE
#ifdef Q_OS_LINUX
#endif
#ifdef Q_OS_SOLARIS
#endif
#ifdef Q_OS_WIN32
#endif
#ifdef Q_OS_MACOS
        opcodeDir = "/Library/Frameworks/CsoundLib64.framework/Resources/Opcodes";
        //    opcodeDir = initialDir + "/CsoundQt.app/Contents/Frameworks/CsoundLib64.framework/Resources/Opcodes";
#endif
#else

#ifdef Q_OS_LINUX
#endif
#ifdef Q_OS_SOLARS
#endif
#ifdef Q_OS_WIN32
#endif
#ifdef Q_OS_MACOS
    opcodeDir = "/Library/Frameworks/CsoundLib64.framework/Resources/Opcodes";
    //    opcodeDir = initialDir + "/CsoundQt.app/Contents/Frameworks/CsoundLib.framework/Resources/Opcodes";
#endif


#endif
    }
    QString fullPath = documentPages[curPage]->getFileName();
    AppWizard wizard(this, opcodeDir, fullPath, m_options->sdkDir);
    AppProperties existingProperties = documentPages[curPage]->getAppProperties();
    if (existingProperties.used) {
        wizard.setField("appName", QVariant(existingProperties.appName));
        wizard.setField("targetDir", QVariant(existingProperties.targetDir));
        wizard.setField("author", QVariant(existingProperties.author));
        wizard.setField("version", QVariant(existingProperties.version));
        wizard.setField("email", QVariant(existingProperties.email));
        wizard.setField("website", QVariant(existingProperties.website));
        wizard.setField("instructions", QVariant(existingProperties.instructions));
        wizard.setField("autorun", QVariant(existingProperties.autorun));
        wizard.setField("showRun", QVariant(existingProperties.showRun));
        wizard.setField("saveState", QVariant(existingProperties.saveState));
        wizard.setField("runMode", QVariant(existingProperties.runMode));
        wizard.setField("newParser", QVariant(existingProperties.newParser));
        wizard.setField("useSdk", QVariant(existingProperties.useSdk ? 1 : 0));
        wizard.setField("useCustomPaths", QVariant(existingProperties.useCustomPaths));
        wizard.setField("libDir", QVariant(existingProperties.libDir));
        wizard.setField("opcodeDir", QVariant(existingProperties.opcodeDir));
    } else { // Put in default values
        QString appDir = fullPath.left(fullPath.lastIndexOf(QDir::separator()) );
        if (appDir.startsWith(":/")) { // For embedded examples
            appDir = QDir::homePath();
        }
        QString appName = fullPath.mid(fullPath.lastIndexOf(QDir::separator()) + 1);
        appName = appName.remove(".csd");
        wizard.setField("appName", appName);
        wizard.setField("targetDir", appDir);
        if (m_options->sdkDir.isEmpty()) { // No sdk,
            wizard.setField("customPaths", true);
#if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
            wizard.setField("libDir", "/usr/lib");
            if (opcodeDir.isEmpty()) {
                wizard.setField("opcodeDir", "/usr/lib/csound/plugins");
            }
#endif
#ifdef Q_OS_WIN32
            wizard.setField("libDir", "");
            if (opcodeDir.isEmpty()) {
                wizard.setField("opcodeDir", "");
            }
#endif
#ifdef Q_OS_MACOS
            wizard.setField("libDir", "/Library/Frameworks");
#endif
        }
    }
    wizard.exec();
    if (wizard.result() == QDialog::Accepted) {
        AppProperties properties;
        properties.used = true;
        properties.appName = wizard.field("appName").toString();
        properties.targetDir = wizard.field("targetDir").toString();
        properties.author = wizard.field("author").toString();
        properties.version = wizard.field("version").toString();
        properties.date = QDateTime::currentDateTime().toString("MMMM d yyyy");
        properties.email = wizard.field("email").toString();
        properties.website = wizard.field("website").toString();
        properties.instructions = wizard.field("instructions").toString();
        properties.autorun = wizard.field("autorun").toBool();
        properties.showRun = wizard.field("showRun").toBool();
        properties.saveState = wizard.field("saveState").toBool();
        properties.runMode = wizard.field("runMode").toInt();
        properties.newParser = wizard.field("newParser").toBool();
        properties.useSdk = wizard.field("useSdk").toBool();
        properties.useCustomPaths = wizard.field("useCustomPaths").toBool();
        properties.libDir = wizard.field("libDir").toString();
        properties.opcodeDir = wizard.field("opcodeDir").toString();

        documentPages[curPage]->setAppProperties(properties);
        bool ret = save(); // Must save to apply properties to file on disk.
        if (!ret) {
            qDebug() << "CsoundQt::createApp() Error saving file";
            return;
        }
        wizard.makeApp();
    }
}

bool CsoundQt::saveNoWidgets()
{
    QString fileName = getSaveFileName();
    if (fileName != "")
        return saveFile(fileName, false);
    else
        return false;
}

void CsoundQt::info()
{
    QString text = tr("Full Path:") + " " + documentPages[curPage]->getFileName() + "\n\n";
    text += tr("Number of lines (Csound Text):") + " " +
            QString::number(documentPages[curPage]->lineCount(true)) + "\n";
    text += tr("Number of characters (Csound Text):") + " " +
            QString::number(documentPages[curPage]->characterCount(true)) + "\n";
    text += tr("Number of lines (total):") + " " +
            QString::number(documentPages[curPage]->lineCount()) + "\n";
    text += tr("Number of characters (total):") + " " +
            QString::number(documentPages[curPage]->characterCount()) + "\n";
    text += tr("Number of instruments:") + " " +
            QString::number(documentPages[curPage]->instrumentCount()) + "\n";
    text += tr("Number of UDOs:") + " " +
            QString::number(documentPages[curPage]->udoCount()) + "\n";
    text += tr("Number of Widgets:") + " " +
            QString::number(documentPages[curPage]->widgetCount()) + "\n";
    text += tr("Embedded Files:") + " " +
            documentPages[curPage]->embeddedFiles() + "\n";
    QMessageBox::information(this, tr("File Information"),
                             text,
                             QMessageBox::Ok,
                             QMessageBox::Ok);
}

bool CsoundQt::closeTab(bool forceCloseApp, int index)
{
    if (index == -1) {
        index = curPage;
    }
    //   qDebug("CsoundQt::closeTab() curPage = %i documentPages.size()=%i", curPage, documentPages.size());
    if (documentPages.size() > 0 && documentPages[index]->isModified()) {
        QString message = tr("The document ")
                + (documentPages[index]->getFileName() != ""
                ? documentPages[index]->getFileName(): "untitled.csd")
                + tr("\nhas been modified.\nDo you want to save the changes before closing?");
        int ret = QMessageBox::warning(this, tr("CsoundQt"),
                                       message,
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No,
                                       QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Cancel)
            return false;
        else if (ret == QMessageBox::Yes) {
            if (!save())
                return false;
        }
    }
    if (!forceCloseApp) {
        if (documentPages.size() <= 1) {
            if (QMessageBox::warning(this, tr("CsoundQt"),
                                     tr("Do you want to exit CsoundQt?"),
                                     QMessageBox::Yes | QMessageBox::Default,
                                     QMessageBox::No) == QMessageBox::Yes)
            {
                close();
                return false;
            }
            else {
                newFile();
                curPage = 0;
            }
        }
    }
    deleteTab(index);
    changePage(curPage);
    // storeSettings(); // do not store here, since all tabs are closed on exit
    return true;
}

void CsoundQt::print()
{
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(tr("Print Document"));
    //   if (editor->textCursor().hasSelection())
    //     dialog->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    if (dialog->exec() != QDialog::Accepted)
        return;
    documentPages[curPage]->print(&printer);
}

void CsoundQt::findReplace()
{
    if(this->helpPanel->hasFocus()) {
        // TODO : call the help panel find
        this->helpPanel->toggleFindBarVisible(true);
        return;
    }
    documentPages[curPage]->findReplace();
}

void CsoundQt::findString()
{
    documentPages[curPage]->findString();
}

bool CsoundQt::join(bool ask)
{
    QDialog dialog(this);
    dialog.resize(700, 350);
    dialog.setModal(true);
    QPushButton *okButton = new QPushButton(tr("Ok"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));

    connect(okButton, SIGNAL(released()), &dialog, SLOT(accept()));
    connect(cancelButton, SIGNAL(released()), &dialog, SLOT(reject()));

    QGridLayout *layout = new QGridLayout(&dialog);
    QListWidget *list1 = new QListWidget(&dialog);
    QListWidget *list2 = new QListWidget(&dialog);
    layout->addWidget(list1, 0, 0);
    layout->addWidget(list2, 0, 1);
    layout->addWidget(okButton, 1,0);
    layout->addWidget(cancelButton, 1,1);
    //   layout->resize(400, 200);

    for (int i = 0; i < documentPages.size(); i++) {
        QString name = documentPages[i]->getFileName();
        if (name.endsWith(".orc"))
            list1->addItem(name);
        else if (name.endsWith(".sco"))
            list2->addItem(name);
    }
    QString name = documentPages[curPage]->getFileName();
    QList<QListWidgetItem *> itemList = list1->findItems(name, Qt::MatchExactly);
    if (itemList.size() == 0) {
        itemList = list1->findItems(name.replace(".sco", ".orc"),Qt::MatchExactly);
    }
    if (itemList.size() > 0) {
        list1->setCurrentItem(itemList[0]);
    }
    QList<QListWidgetItem *> itemList2 = list2->findItems(name.replace(".orc", ".sco"),
                                                          Qt::MatchExactly);
    if (itemList2.size() == 0) {
        itemList2 = list1->findItems(name,Qt::MatchExactly);
    }
    if (itemList2.size() > 0)
        list2->setCurrentItem(itemList2[0]);
    if (itemList.size() == 0 || itemList.size() == 0) {
        if (!ask) {
            QMessageBox::warning(this, tr("Join"),
                                 tr("Please open the orc and sco files in CsoundQt first!"));
        }
        return false;
    }
    if (!ask || dialog.exec() == QDialog::Accepted) {
        QString orcText = "";
        QString scoText = "";
        for (int i = 0; i < documentPages.size(); i++) {
            QString name = documentPages[i]->getFileName();
            if (name == list1->currentItem()->text())
                orcText = documentPages[i]->getFullText();
            else if (name == list2->currentItem()->text())
                scoText = documentPages[i]->getFullText();
        }
        QString text = "<CsoundSynthesizer>\n<CsOptions>\n</CsOptions>\n<CsInstruments>\n";
        text += orcText;
        text += "</CsInstruments>\n<CsScore>\n";
        text += scoText;
        text += "</CsScore>\n</CsoundSynthesizer>\n";
        newFile();
        documentPages[curPage]->loadTextString(text);
        return true;
    }
    return false;
}

void CsoundQt::showUtilities(bool show)
{
    if (!show) {
        if (utilitiesDialog != nullptr && utilitiesDialog->close()) {
            utilitiesDialog = nullptr;
        }
    }
    else {
        utilitiesDialog = new UtilitiesDialog(this, m_options/*, _configlists*/);
        connect(utilitiesDialog, SIGNAL(Close(bool)), showUtilitiesAct, SLOT(setChecked(bool)));
        connect(utilitiesDialog, SIGNAL(runUtility(QString)), this, SLOT(runUtility(QString)));
        utilitiesDialog->setModal(false);
        utilitiesDialog->show();
    }
}

void CsoundQt::inToGet()
{
	documentPages[curPage]->inToGet();
}

void CsoundQt::insertMidiControlInstrument()
{

	QString instrument = documentPages[curPage]->getMidiControllerInstrument();
	qDebug() << instrument;
	int ret = QMessageBox::information(nullptr, tr("Insert"),
		tr("This function generates a Csound instrument from the MIDI bindings of your widgets (if thre is any) and inserts to the text editor at the current cursor position\n"
		"It can be useful if you want to run the csd outside of CsoundQt with your MIDI controller.\n"				"Rembember to add -Ma or similar MIDI options to the CsOptions."),   QMessageBox::Cancel |QMessageBox::Ok);
	if (ret==QMessageBox::Ok) {
		insertText("\n" + instrument + "\n");
	}

}

void CsoundQt::getToIn()
{
    documentPages[curPage]->getToIn();
}

void CsoundQt::updateCsladspaText()
{
    documentPages[curPage]->updateCsLadspaText();
}

void CsoundQt::updateCabbageText()
{
    documentPages[curPage]->updateCabbageText();
    // check if necessary options for Cabbage
    QString optionsText=documentPages[curPage]->getOptionsText().trimmed();
    if (!(optionsText.contains("-d") && optionsText.contains("-n"))) {
        int answer = QMessageBox::question(this, tr("Insert Cabbage options?"),
                              tr("There seems to be no Cabbage specific options in <CsOptions>.\n Do you want to insert it?\nNB! Comment your old options out, if necessary!"), QMessageBox::Yes|QMessageBox::No );
        if (answer==QMessageBox::Yes) {
            if (!optionsText.simplified().isEmpty()) {
                optionsText += "\n"; // add newline if there is some text
            }
            optionsText += "-n -d -+rtmidi=NULL -M0 --midi-key-cps=4 --midi-velocity-amp=5";
            documentPages[curPage]->setOptionsText(optionsText);
        }
    }
    QString scoreText = documentPages[curPage]->getSco();
    if (scoreText.simplified().isEmpty()) {
        int answer = QMessageBox::question(this, tr("Insert scorelines for Cabbage?"),
                                           tr("The score is empty. Cabbage needs some lines "
                                              "to work.\n Do you want to insert it?"),
                                           QMessageBox::Yes|QMessageBox::No );
        if (answer==QMessageBox::Yes) {
            documentPages[curPage]->setSco("f 0 3600\n"
                                           ";i 1 0 3600 ; don't forget to start your instrument if you are using it as an effect!");
        }
    }
}

void CsoundQt::saveWidgetsToQml()
{
    QString qml = documentPages[curPage]->getQml();

    //QString dir = lastUsedDir;
    QString name = documentPages[curPage]->getFileName();
    name.replace(".csd", ".qml");
    //dir += name.mid(name.lastIndexOf("/") + 1);
    //TODO: get dir from name -  extract path
    QString fileName = QFileDialog::getSaveFileName(
                this, tr("Save Qml File"), name,
                tr("QML Files (*.qml *.QML);;All Files (*)",
                "Export Widgets to QML"));
    if ( !fileName.isEmpty()) { // save to file
        QFile file(fileName);

        if(file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream stream(&file);
            stream << qml;
            file.close();
        }
    }
}

void CsoundQt::setCurrentAudioFile(const QString fileName)
{
    currentAudioFile = fileName;
}

void CsoundQt::play(bool realtime, int index)
{
    qDebug()  << "CsoundQt::play...";
    // TODO make csound pause if it is already running
    int oldPage = curPage;
    if (index == -1) {
        index = curPage;
    }
    if (index < 0 && index >= documentPages.size()) {
        qDebug() << "CsoundQt::play index out of range " << index;
        return;
    }
    curPage = index;
    auto page = documentPages[curPage];

    QFileInfo fileInfo(QFileInfo(page->getFileName()).path());

    if (page->getFileName().isEmpty()) {
        int answer;
        if(!m_options->askIfTemporary)
            answer= QMessageBox::Ok;
        else
            answer = QMessageBox::question(this, tr("Run as temporary file?"),
                tr("press <b>OK</b>, if you don't care about this file in "
                   "future.\n Press <b>Save</b>, if you want to save it with "
                   "given name.\n You can always save the file with <b>Save As</b> "
                   "also later, if you use the temporary file."),
                QMessageBox::Ok|QMessageBox::Save|QMessageBox::Cancel );
        bool saved = false;
        if (answer==QMessageBox::Save) {
            saved = saveAs();
        } else if (answer==QMessageBox::Ok) {
            auto tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            if (!tempPath.isEmpty() && saveFile(tempPath + "/csoundqt-temp.csd")) {
                saved = true;
            } else {
                QMessageBox::critical(this,tr("Save error"), tr("Could not save temporary file"));
            }
        }
        if (!saved) {
            if (curPage == oldPage) {
                runAct->setChecked(false);
            }
            curPage = oldPage;
            return;
        }
    }
    //else if (page->isModified()) {
    else if (m_options->saveChanges && fileInfo.isWritable() && !page->getFileName().startsWith(":/")) { // is modified returns sometimes wrongly false. save anyway when asked TODO: degub DocumentPage::isModified()
        if (!save()) {
            if (curPage == oldPage) {
                runAct->setChecked(false);
            }
            curPage = oldPage;
            return;
        }
    }
    QString fileName = page->getFileName();
    QString fileName2;
    QString msg = "__**__ " + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
    msg += " Play: " + fileName + "\n";
    logMessage(msg);
    m_options->csdPath = "";
    if (fileName.contains('/')) {
        //FIXME is it necessary to set the csdPath here?
        //    m_options->csdPath =
        //        documentPages[curPage]->fileName.left(documentPages[curPage]->fileName.lastIndexOf('/'));
        QDir::setCurrent(fileName.left(fileName.lastIndexOf('/')));
    }

    if (fileName.endsWith(".html",Qt::CaseInsensitive)) {
        QMessageBox::warning(this,
                             tr("CsoundQt"),
                             tr("Csound in html files should be started from html code."),
                             QMessageBox::Ok);
        runAct->setChecked(false);
        curPage = documentTabs->currentIndex();
        return;
    }
    if (!fileName.endsWith(".csd",Qt::CaseInsensitive)
            && !fileName.endsWith(".py",Qt::CaseInsensitive))  {
        if (page->askForFile)
            getCompanionFileName();
        // FIXME run orc file when sco companion is currently active
        if (fileName.endsWith(".sco",Qt::CaseInsensitive)) {
            //Must switch filename order when open file is a sco file
            fileName2 = fileName;
            fileName = page->getCompanionFileName();
        } else if (fileName.endsWith(".orc",Qt::CaseInsensitive) &&
                   page->getCompanionFileName().isEmpty()) {
            //TODO: create empty score file
        }
        else
            fileName2 = page->getCompanionFileName();
    }
    QString runFileName1, runFileName2;
    QTemporaryFile csdFile, csdFile2; // TODO add support for orc/sco pairs
    runFileName1 = fileName;
    if(fileName.startsWith(":/", Qt::CaseInsensitive) || !m_options->saveChanges || !fileInfo.isWritable() ) {
        QDEBUG << "***** Using temporary file for filename" << fileName;
        QString tmpFileName = QDir::tempPath();
        if (!tmpFileName.endsWith("/") && !tmpFileName.endsWith("\\")) {
            tmpFileName += QDir::separator();
        }
        if (fileName.endsWith(".csd",Qt::CaseInsensitive)) {
            tmpFileName += QString("csound-tmpXXXXXXXX.csd");
            csdFile.setFileTemplate(tmpFileName);
            if (!csdFile.open()) {
                qDebug() << "Error creating temporary file " << tmpFileName;
                QMessageBox::critical(this, tr("CsoundQt"), tr("Error creating temporary file."),
                                      QMessageBox::Ok);
                return;
            }
            // If example, just copy, since readonly anyway, otherwise get contents from editor.
            // Necessary since examples may contain <CsFileB> section with data.
            if (!fileName.startsWith(":/examples/", Qt::CaseInsensitive) ) {
                csdFile.write(page->getBasicText().toLatin1());
            } else {
                auto fullText = page->getView()->getFullText();
                if(!fullText.contains("<CsFileB")) {
                    csdFile.write(fullText.toLatin1());
                } else {
                    qDebug() << "File has embedded elements via <CsFileB> tag";
                    QFile file(fileName);
                    if(!file.open(QFile::ReadOnly)) {
                        qDebug() << "Could not open file " << fileName;
                        return;
                    }
                    int result = csdFile.write(file.readAll());
                    file.close();
                    if (result<=0) {
                        qDebug()<< "*** ERROR: Failed to copy to example to temporary "
                                   "location ***";
                        return;
                    }
                }
            }
            csdFile.flush();
            runFileName1 = csdFile.fileName();
        }
    }
    runFileName2 = page->getCompanionFileName();
    m_options->docName = fileName;
    m_options->fileName1 = runFileName1;
    m_options->fileName2 = runFileName2;
    m_options->rt = realtime;
    if (!m_options->simultaneousRun) {
        stopAllOthers();
#if defined(QCS_QTHTML)
        csoundHtmlView->stop();
#endif
    }
    if (curPage == oldPage) {
        runAct->setChecked(true);  // In case the call comes from a button
    }
    if (page->usesFltk() && m_options->terminalFLTK) {
        runInTerm();
        curPage = oldPage;
        return;
    }
    int ret = page->play(m_options);
    if (ret == -1) {
        QMessageBox::critical(this, "CsoundQt", tr("Internal error running Csound."),
                              QMessageBox::Ok);
    } else if (ret == -2) {
        // Error creating temporary file
        runAct->setChecked(false);
        qDebug() << "CsoundQt::play - Error creating temporary file";
    } else if (ret == -3) {
        // Csound compilation failed
        runAct->setChecked(false);
    } else if (ret == 0) {
		// No problem:
		// set playing icon on tab
        documentTabs->setTabIcon(index, QIcon(QString(":/themes/%1/media-play.png").arg(m_options->theme )));

		// enable widgets
        if(m_options->checkSyntaxOnly) {
            return;
        }
        if (m_options->enableWidgets && m_options->showWidgetsOnRun && fileName.endsWith(".csd")) {
            if(page->usesFltk()) {
                // Don't bring up widget panel if there's an FLTK panel
                qDebug() << "Page uses FLTK, CsoundQt's widgets will not be shown";
            } else {
                if (!m_options->widgetsIndependent) {
                    if (widgetPanel->isFloating()) {
                        widgetPanel->setVisible(true);
                        showWidgetsAct->setChecked(true);
                    }
                }
                else {
                    page->widgetsVisible(true);
                    showWidgetsAct->setChecked(true);
                }
                widgetPanel->setFocus(Qt::OtherFocusReason);
                page->focusWidgets();
            }
        }
    } else {
        qDebug() << "play: Return value not understood!";
        return;
    }
#if defined(QCS_QTHTML)
    if (!documentPages.isEmpty() && !documentPages[curPage]->getHtmlText().isEmpty()) {
        if ( !m_options->saveChanges ) { // otherwise the htmlview gets updated on save
            updateHtmlView();
            qDebug()<<"update html on run";
        }
        DocumentPage *documentPage = documentPages[curPage];
        if (!documentPage->getHtmlText().isEmpty()) {
            CsoundEngine *engine = documentPage->getEngine();
            csoundHtmlView->setCsoundEngine(engine);
        }
    }
#endif
    curPage = oldPage;
}

void CsoundQt::runInTerm(bool realtime)
{
    QString fileName = documentPages[curPage]->getFileName();
    QTemporaryFile tempFile(QDir::tempPath() + QDir::separator() + "CsoundQtExample-XXXXXX.csd");
    tempFile.setAutoRemove(false);
    if (fileName.startsWith(":/")) {
        if (!tempFile.open()) {
            QMessageBox::critical(this, tr("Error running terminal"),
                                  tr("Error creating temp file"));
            runAct->setChecked(false);
            return;
        }
        QString csdText = documentPages[curPage]->getBasicText();
        fileName = tempFile.fileName();
        tempFile.write(csdText.toLatin1());
        tempFile.flush();
        if (!tempScriptFiles.contains(fileName))
            tempScriptFiles << fileName;
    }
    QString executable = fileName.endsWith(".py") ? m_options->pythonExecutable : "";
    QString script = generateScript(realtime, fileName, executable);
    QTemporaryFile scriptFile(QDir::tempPath() + QDir::separator() + SCRIPT_NAME);
    scriptFile.setAutoRemove(false);
    if (!scriptFile.open()) {
        runAct->setChecked(false);
        QMessageBox::critical(this, tr("Error running terminal"),
                              tr("Error generating the script to be run at the terminal"));
        return;
    }
    QTextStream out(&scriptFile);
    out << script;
    scriptFile.close();
    scriptFile.setPermissions(QFile::ExeOwner| QFile::WriteOwner| QFile::ReadOwner);
    QString scriptFileName = scriptFile.fileName();
    QStringList args;
    QString termexe = m_options->terminal;
    // QString options;
#ifdef Q_OS_LINUX
    args << "-e" << scriptFileName;
    // options = "-e " + scriptFileName;
#endif
#ifdef Q_OS_SOLARIS
    args << "-e" << scriptFileName;
    // options = "-e " + scriptFileName;
#endif
#ifdef Q_OS_MACOS
    args << "-a" << m_options->terminal << scriptFileName;
    termexe = "open";
    // options = scriptFileName;
#endif
#ifdef Q_OS_WIN32
    args << scriptFileName;
    // options = scriptFileName;
    // qDebug() << "m_options->terminal == " << m_options->terminal;
#endif
    if(startProcess(termexe, args)) {
        QMessageBox::critical(this, tr("Error running terminal"),
                              tr("Could not run terminal program: '%1'\nArgs: %2\n"
                                 "Check environment tab in preferences.").arg(termexe, args.join(" ")));
    }
    runAct->setChecked(false);
    if (!tempScriptFiles.contains(scriptFileName))
        tempScriptFiles << scriptFileName;
}

void CsoundQt::pause(int index)
{
    int docIndex = index;
    if (docIndex == -1) {
        docIndex = curPage;
    }
    if (docIndex >= 0 && docIndex < documentPages.size()) {
        documentPages[docIndex]->pause();
		if (documentPages[docIndex]->getEngine()->isPaused() ) {
            documentTabs->setTabIcon(docIndex, QIcon(QString(":/themes/%1/media-pause.png").arg(m_options->theme )));
		} else {
            documentTabs->setTabIcon(docIndex, QIcon(QString(":/themes/%1/media-play.png").arg(m_options->theme )));
		}
    }
}

void CsoundQt::stop(int index)
{
    // Must guarantee that csound has stopped when it returns
    int docIndex = index;
    if (docIndex == -1) {
        docIndex = curPage;
    }
    if (curPage >= documentPages.size()) {
        return; // A bit of a hack to avoid crashing when documents are deleted very quickly...
    }
    Q_ASSERT(docIndex >= 0);
    if (docIndex >= documentPages.size()) {
        qDebug() << "CsoundQt::stop : document index" << docIndex
                 << "out of range (max=" << documentPages.size() << ")";
        markStopped();
		return;
    }
    if (documentPages[docIndex]->isRunning()) {
        documentPages[docIndex]->stop();
		documentTabs->setTabIcon(docIndex, QIcon());

#ifdef Q_OS_WIN
        // necessary since sometimes fltk plugin closes the OLE/COM connection on csoundCleanup
        HRESULT result = OleInitialize(NULL);
        if (result) {
            qDebug()<<"Problem with OleInitialization" << result;
        }
#endif

    }
    documentTabs->setTabIcon(docIndex, QIcon());
    markStopped();
}

void CsoundQt::stopAll()
{
    for (int i = 0; i < documentPages.size(); i++) {
        documentPages[i]->stop();
		documentTabs->setTabIcon(i, QIcon());
    }
    markStopped();
}

void CsoundQt::stopAllOthers()
{
    for (int i = 0; i < documentPages.size(); i++) {
        if (i != curPage) {
            DocumentPage *documentPage = documentPages[i];
            documentPage->stop();
			documentTabs->setTabIcon(i, QIcon());
        }
    }
    //	markStopped();
}

void CsoundQt::markStopped()
{
    DocumentPage * senderPage = dynamic_cast<DocumentPage *>(sender());
    if (senderPage) {
        int index = documentPages.indexOf(senderPage);
        //qDebug() << "Stop was sent from: " << index;
        documentTabs->setTabIcon(index, QIcon());
    }
    documentTabs->setTabIcon(curPage, QIcon());
    runAct->setChecked(false);
    recAct->setChecked(false);
#ifdef QCS_DEBUGGER
    m_debugPanel->stopDebug();
#endif
}

void CsoundQt::perfEnded()
{
    runAct->setChecked(false);
}

void CsoundQt::record(bool rec) {
	record(rec, curPage);
}

void CsoundQt::record(bool rec, int index=-1)
{
	if (index==-1) index=curPage;
	if (rec) {
		if (!documentPages[index]->isRunning()) {
            play();
        }
		int ret = documentPages[index]->record(m_options->sampleFormat);
        documentTabs->setTabIcon(index, QIcon(QString(":/themes/%1/media-record.png").arg(m_options->theme )));
		if (ret != 0) {
            recAct->setChecked(false);
			documentTabs->setTabIcon(index, QIcon());
        }
    }
    else {
		documentPages[index]->stopRecording();
        QMessageBox::information(nullptr, tr("Record"), tr("Recorded to audiofile ") + this->currentAudioFile);
		// documentTabs->setTabIcon(curPage, QIcon());
    }
}


void CsoundQt::sendEvent(QString eventLine, double delay)
{
    sendEvent(curCsdPage, eventLine, delay);
}

void CsoundQt::sendEvent(int index, QString eventLine, double delay)
{
    int docIndex = index;
    if (docIndex == -1) {
        docIndex = curPage;
    }
    if (docIndex >= 0 && docIndex < documentPages.size()) {
        documentPages[docIndex]->queueEvent(eventLine, delay);
    }
}

void CsoundQt::render()
{
    if (m_options->fileAskFilename) {
        QString defaultFile;
        if (m_options->fileOutputFilenameActive) {
            defaultFile = m_options->fileOutputFilename;
        }
        else {
            defaultFile = lastFileDir;
        }
        QFileDialog dialog(this,tr("Output Filename"),defaultFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        //    dialog.setConfirmOverwrite(false);
        auto extensions = m_configlists.fileTypeExtensions[m_options->fileFileType];
        QString filter = QString(m_configlists.fileTypeLongNames[m_options->fileFileType] +
                " Files (" + extensions + ")");
        dialog.selectNameFilter(filter);
        if (dialog.exec() == QDialog::Accepted) {
            QString extension = extensions.left(extensions.indexOf(";"));
            // Remove the '*' from the extension
            extension.remove(0,1);
            m_options->fileOutputFilename = dialog.selectedFiles()[0];
            if (!m_options->fileOutputFilename.endsWith(".wav")
                    && !m_options->fileOutputFilename.endsWith(".aif")
                    && !m_options->fileOutputFilename.endsWith(extension) ) {
                m_options->fileOutputFilename += extension;
            }
            auto outfile = m_options->fileOutputFilename;
            if (QFile::exists(outfile)) {
                int ret = QMessageBox::warning(this, tr("CsoundQt"),
                            tr("The file %1 already exists.\n Do you want to overwrite it?")
                               .arg(outfile),
                            QMessageBox::Save|QMessageBox::Cancel, QMessageBox::Save);
                if (ret == QMessageBox::Cancel)
                    return;
            }
            lastFileDir = dialog.directory().path();
        }
        else {
            return;
        }
    }
    QString outName = m_options->fileOutputFilename;
#ifdef Q_OS_WIN32
    outName = outName.replace('\\', '/');
#endif
    if (outName.isEmpty()) {
        outName = "test.wav";
    }
    setCurrentAudioFile(outName);
    play(false);
}

void CsoundQt::openExternalEditor()
{
    QString name = "";
    if (m_options->sfdirActive) {
        name = m_options->sfdir + (m_options->sfdir.endsWith("/") ? "" : "/");
    }
    name += currentAudioFile;
    QString optionsText = documentPages[curPage]->getOptionsText();
    if (currentAudioFile == "") {
        if (!optionsText.contains(QRegularExpression("\\W-o"))) {
            name += "test.wav";
        }
        else {
            optionsText = optionsText.mid(optionsText.indexOf(QRegularExpression("\\W-o")) + 3);
            optionsText = optionsText.left(optionsText.indexOf("\n")).trimmed();
            optionsText = optionsText.left(optionsText.indexOf("-")).trimmed();
            if (!optionsText.startsWith("dac"))
                name += optionsText;
        }
    }
    if (!QFile::exists(name)) {
        QMessageBox::critical(this, tr("Render not available"),
                              tr("Could not find rendered file. Please render before calling"
                                 " external editor."));
    }
    if (!m_options->waveeditor.isEmpty()) {
        // name = "\"" + name + "\"";
        QDEBUG << "Opening rendered audiofile: " << name;
        execute(m_options->waveeditor, name);
    }
    else {
        QDesktopServices::openUrl(QUrl::fromLocalFile (name));
    }
}

void CsoundQt::openExternalPlayer()
{
    QString name = "";
    if (m_options->sfdirActive) {
        name = m_options->sfdir + (m_options->sfdir.endsWith("/") ? "" : "/");
    }
    name += currentAudioFile;
    QString optionsText = documentPages[curPage]->getOptionsText();
    if (currentAudioFile == "") {
        if (!optionsText.contains(QRegularExpression("\\W-o"))) {
            name += "test.wav";
        }
        else {
            optionsText = optionsText.mid(optionsText.indexOf(QRegularExpression("\\W-o")) + 3);
            optionsText = optionsText.left(optionsText.indexOf("\n")).trimmed();
            optionsText = optionsText.left(optionsText.indexOf(QRegularExpression("-"))).trimmed();
            if (!optionsText.startsWith("dac"))
                name += optionsText;
        }
    }
    if (!QFile::exists(name)) {
        QMessageBox::critical(this, tr("Render not available"),
                              tr("Could not find rendered file. Please render before calling"
                                 " external player."));
    }
    if (!m_options->waveplayer.isEmpty()) {
        name = "\"" + name + "\"";
        execute(m_options->waveplayer, name);
    }
    else {
        QDesktopServices::openUrl(QUrl::fromLocalFile (name));
    }
}

void CsoundQt::setEditorFocus()
{
    documentPages[curPage]->setEditorFocus();
    this->raise();
}


/**
 * @brief CsoundQt::helpForEntry open help for given entry
 * @param entry - The entry to find help for (normally an opcode)
 * @param external - If true, open in an external browser (default=false)
 */
void CsoundQt::helpForEntry(QString entry, bool external) {
    QDEBUG << "searching help for" << entry;
    if (entry.startsWith('#')) { // For #define and friends
        entry.remove(0,1);
    }
    QString dir = m_options->csdocdir.isEmpty() ? helpPanel->docDir : m_options->csdocdir ;
    if (entry.startsWith("http://")) {
        openExternalBrowser(QUrl(entry));
        return;
    }
    QString errmsg;

    if(risset->isInstalled && risset->opcodeToPlugin.contains(entry)) {
        // Check external help sources
        QDEBUG << "Found an opcode from an external plugin: " << entry;
        auto pluginName = risset->opcodeToPlugin[entry];
        if(!risset->pluginInstalled[pluginName]) {
            QDEBUG << "The opcode " << entry << "is defined in the plugin " << pluginName << ", but this plugin is not installed";
            QMessageBox::information(this, "Risset",
                                     QString("The plugin '%1' containing the opcode '%2' is not installed "
                                             "You can install it via risset: 'risset install %1'").arg(pluginName, entry));
        }
        QString fileName = risset->htmlManpage(entry);
        if(!fileName.isEmpty()) {
            openHtmlHelp(fileName, entry, external);
        }
        else {
            QDEBUG << "Did not find html page for opcode" << entry;
            auto reply = QMessageBox::question(this, "Risset",
                                               "Risset documentation not found. Do you want to fetch it? "
                                               " (it may take a few seconds)",
                                               QMessageBox::Yes|QMessageBox::No);
            if(reply == QMessageBox::Yes) {
                risset->generateDocumentation([=](int exitCode) {
                    if(exitCode==0) {
                        QMessageBox::information(this, "Risset", "Fetched documentation");
                        helpForEntry(entry, external);
                    }
                    else {
                        qDebug() << "Generate documentation exit code: " << exitCode;
                        QMessageBox::warning(this, "Risset",
                                             "Failed to fetch documentation");
                    }
                });
            }
            return;
        }
    }
    else if (dir.isEmpty()) {
        QMessageBox::critical(this, tr("Error"),
            tr("HTML Documentation directory not set!\n"
               "Please go to Edit->Options->Environment and select directory\n"));
        return;
    }
    else if(entry.startsWith("--") || entry.startsWith("-+")) {
        openHtmlHelp(dir+"/CommandFlagsCategory.html", "", external);
    }
    else {
        helpPanel->docDir = dir;
        QString fileName;
        if (entry == "0dbfs")
            fileName = dir + "/Zerodbfs.html";
        else if (entry.contains("CsOptions"))
            fileName = dir + "/CommandUnifile.html";
        else if (entry.startsWith("chn_"))
            fileName = dir + "/chn.html";
        else if(QFile::exists(dir + "/" + entry + ".html")) {
            fileName = dir + "/" + entry + ".html";
        }
        else
            return;
        openHtmlHelp(fileName, entry, external);
    }
}


void CsoundQt::openHtmlHelp(QString fileName, QString entry, bool external) {
    if(external) {
        openExternalBrowser(fileName);
    } else {
        helpPanel->loadFile(fileName, entry);
        helpPanel->show();
        helpPanel->raise();
        helpPanel->focusText();
    }
}


void CsoundQt::setHelpEntry()
{
    QString line = documentPages[curPage]->lineUnderCursor();
    auto match = QRegularExpression("-[-\\+][a-zA-Z-_]+").match(line);
    if(match.hasMatch()) {
        auto capt = match.captured();
        if(m_longOptions.contains(capt)) {
            return helpForEntry(capt);
        }
    }
    QString word = documentPages[curPage]->wordUnderCursor();
    return helpForEntry(word);
}


void CsoundQt::setFullScreen(bool full)
{
	if (full) {
		checkFullScreen();
        this->showFullScreen();
		m_fullScreenComponent = "mainwindow";
    }
    else {
        checkFullScreen(); // if something is still fullscreen by error, reset it;
        this->showNormal();
		m_fullScreenComponent = "";
    }
}

void CsoundQt::checkFullScreen() // checks if some component is already fullscreen and resets it
{
    //qDebug()<< " fullScreenComponent: " << m_fullScreenComponent << "state bytes: " << m_preFullScreenState.size();
	if ( !m_fullScreenComponent.isEmpty() ) {
		if (m_fullScreenComponent == "mainwindow" ) {
			viewFullScreenAct->setChecked(false);
		} else if (m_fullScreenComponent == "help" ) {
			viewHelpFullScreenAct->setChecked(false);
		} else if (m_fullScreenComponent == "widgets") {
			viewWidgetsFullScreenAct->setChecked(false);
		} else if ( m_fullScreenComponent == "editor" ) {
		} else if ( m_fullScreenComponent == "html" ) {
			viewHtmlFullScreenAct->setChecked(false);
		}
    }
}

void CsoundQt::setEditorFullScreen(bool full)
{
	if (full) {
		checkFullScreen();
		m_preFullScreenState = this->saveState();
        QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
        foreach (QDockWidget *dockWidget, dockWidgets) {
            dockWidget->hide();
        }
        this->showFullScreen();
		m_fullScreenComponent = "editor";
    }
    else {
		this->restoreState(m_preFullScreenState);
        this->showNormal(); // to restore titlebar etc
		m_fullScreenComponent = "";
    }
}


void CsoundQt::setHtmlFullScreen(bool full)
{
#ifdef QCS_QTHTML
	if (full) {
		checkFullScreen();
		m_preFullScreenState = this->saveState();
		this->csoundHtmlView->setFloating(true);
#ifndef Q_OS_WIN // this was Q_OS_MACOS before -  try now showMaximized for all expect Windows
        this->csoundHtmlView->showMaximized();
#else
		this->csoundHtmlView->showFullScreen();
#endif
		m_fullScreenComponent = "html";
    }
    else {
		this->restoreState(m_preFullScreenState);
        this->csoundHtmlView->setFloating(false);
		//this->showNormal();
		m_fullScreenComponent = "";
    }
# else
    Q_UNUSED(full);
#endif
}

void CsoundQt::setHelpFullScreen(bool full)
{
	if (full) {
		checkFullScreen();
		m_preFullScreenState = this->saveState();
        this->helpPanel->setFloating(true);
#ifndef Q_OS_WIN
        this->helpPanel->showMaximized();
#else
        this->helpPanel->showFullScreen();
#endif
        m_fullScreenComponent = "help";
    }
    else {
		this->restoreState(m_preFullScreenState);
		//this->showNormal();
		m_fullScreenComponent = "";
    }
}

void CsoundQt::setWidgetsFullScreen(bool full)
{
	if (full) {
		checkFullScreen();
		m_preFullScreenState = this->saveState();
		if (m_options->widgetsIndependent) {
            auto doc = getCurrentDocumentPage();
            if(doc == nullptr)
                return;
            auto wl = doc->getWidgetLayout();
            wl->setVisible(true);
#ifndef Q_OS_WIN
            wl->showMaximized();
#else
            wl->showFullScreen();
#endif
        } else {
            this->widgetPanel->setFloating(true);
#ifndef Q_OS_WIN
        this->widgetPanel->showMaximized();
#else
            this->widgetPanel->showFullScreen();
#endif
        }
		m_fullScreenComponent = "widgets";
    }
    else {
        if(m_options->widgetsIndependent) {
            auto doc = getCurrentDocumentPage();
            if(doc == nullptr)
                return;
            doc->getWidgetLayout()->showNormal();
			m_fullScreenComponent = "";
        } else {
			this->restoreState(m_preFullScreenState);
			//this->showNormal();
			m_fullScreenComponent = "";
        }

    }
}


void CsoundQt::showDebugger(bool show)
{
#ifdef QCS_DEBUGGER
    m_debugPanel->setVisible(show);
#else
    (void) show;
#endif
}

void CsoundQt::showVirtualKeyboard(bool show)
{

    if (show) {
        // create here to be able to be able to delete in on close and catch destroyed()
        // to uncheck action button
        m_virtualKeyboard = new QQuickWidget(this);
        m_virtualKeyboard->setAttribute(Qt::WA_DeleteOnClose);
        m_virtualKeyboardPointer = m_virtualKeyboard;  // guarded pointer to check if object is  alive
        m_virtualKeyboard->setWindowTitle(tr("CsoundQt Virtual Keyboard"));
        m_virtualKeyboard->setWindowFlags(Qt::Window);
        m_virtualKeyboard->setSource(QUrl("qrc:/QML/VirtualKeyboard.qml"));
        QObject *rootObject = m_virtualKeyboard->rootObject();
        m_virtualKeyboard->setFocus();
        connect(rootObject, SIGNAL(genNote(QVariant, QVariant, QVariant, QVariant)),
                this, SLOT(virtualMidiIn(QVariant, QVariant, QVariant, QVariant)));
        connect(rootObject, SIGNAL(newCCvalue(int,int,int)), this, SLOT(virtualCCIn(int, int, int)));
        m_virtualKeyboard->setVisible(true);
        connect(m_virtualKeyboard, SIGNAL(destroyed(QObject*)),
                this, SLOT(virtualKeyboardActOff(QObject*)));
    } else if (!m_virtualKeyboardPointer.isNull()) { // check if object still existing (i.e not on exit)
        m_virtualKeyboard->setVisible(false);
        m_virtualKeyboard->close();
    }

}

void CsoundQt::showTableEditor(bool show)
{

    if (show) {
        m_tableEditor = new QQuickWidget(this); // create here to be able to
        m_tableEditor->setAttribute(Qt::WA_DeleteOnClose); // ... be able to delete in on close and catch destroyed() to uncheck action button
        m_tableEditorPointer = m_tableEditor;  // guarded pointer to check if object is  alive
        m_tableEditor->setWindowTitle(tr("CsoundQt table editor"));
        m_tableEditor->setWindowFlags(Qt::Window);
        m_tableEditor->setSource(QUrl("qrc:/QML/TableEditor.qml"));
        m_tableEditor->setResizeMode(QQuickWidget::SizeRootObjectToView);
        QObject *rootObject = m_tableEditor->rootObject();
        m_tableEditor->setFocus();
        connect(rootObject, SIGNAL(newSyntax(QString)), this, SLOT(handleTableSyntax(QString)));
        // see if selected text contains a ftgen definition, then set in the editor
        QString selectedText = getSelectedText();
        if (selectedText.contains("ftgen") && selectedText.split(",")[3].simplified()=="7") {
            qDebug()<<"This is a ftgen 7 definition: " << selectedText;
            QObject *syntaxField = rootObject->findChild<QObject*>("syntaxField");
            syntaxField->setProperty("text",QVariant(selectedText.simplified())); // display teh text in QML window
            QMetaObject::invokeMethod(rootObject, "syntax2graph",
                    Q_ARG(QVariant, selectedText.simplified() ),
                    Q_ARG(QVariant, 0)); // if 0, finds max from the table parameters
        }

        m_tableEditor->setVisible(true);
        connect(m_tableEditor, SIGNAL(destroyed(QObject*)), this, SLOT(tableEditorActOff(QObject*)));
    } else if (!m_tableEditorPointer.isNull()) { // check if object still existing (i.e not on exit)
        m_tableEditor->setVisible(false);
        m_tableEditor->close();
    }


}

void CsoundQt::virtualKeyboardActOff(QObject *parent)
{
    (void) parent;

    //qDebug()<<"VirtualKeyboard destroyed";
    if (!m_virtualKeyboardPointer.isNull()) {
        // check if object still existing (i.e application not exiting)
        if (showVirtualKeyboardAct->isChecked()) {
            showVirtualKeyboardAct->setChecked(false);
        }
    }

}

void CsoundQt::tableEditorActOff(QObject *parent)
{
    (void) parent;

    //qDebug()<<"VirtualKeyboard destroyed";
    if (!m_tableEditorPointer.isNull()) { // check if object still existing (i.e application not exiting)
        if (showTableEditorAct->isChecked()) {
            showTableEditorAct->setChecked(false);
        }
    }

}


void CsoundQt::showHtml5Gui(bool show)
{
#if defined(QCS_QTHTML)
    csoundHtmlView->setVisible(show);
#else
    Q_UNUSED(show);
#endif
}

void CsoundQt::splitView(bool split)
{
    if (split) {
        int mode = showOrcAct->isChecked() ? 2 : 0 ;
        mode |= showScoreAct->isChecked() ? 4 : 0 ;
        mode |= showOptionsAct->isChecked() ? 8 : 0 ;
        mode |= showFileBAct->isChecked() ? 16 : 0 ;
        mode |= showOtherAct->isChecked() ? 32 : 0 ;
        mode |= showOtherCsdAct->isChecked() ? 64 : 0 ;
        mode |= showWidgetEditAct->isChecked() ? 128 : 0 ;
        if (mode == 0) {
            mode = 2+4;
        }
        documentPages[curPage]->setViewMode(mode);
    }
    else {
        documentPages[curPage]->setViewMode(0);
    }
}

void CsoundQt::showMidiLearn()
{
    m_midiLearn->show();
}

void CsoundQt::virtualMidiIn(QVariant on, QVariant note, QVariant channel, QVariant velocity)
{
    std::vector<unsigned char> message;
    unsigned char status = (8 << 4) | on.toInt() << 4 | (channel.toInt() -1);
    message.push_back(status);
    message.push_back(note.toInt());
    message.push_back(velocity.toInt());
    documentPages[curPage]->queueVirtualMidiIn(message);
}

void CsoundQt::virtualCCIn(int channel, int cc, int value)
{
    //qDebug()<<"CC event: "<<channel<< cc<<value;
    std::vector<unsigned char> message;
    unsigned char status = (11 << 4 | (channel - 1));
    message.push_back(status);
    message.push_back(cc);
    message.push_back(value);
    documentPages[curPage]->queueVirtualMidiIn(message);
}

void CsoundQt::handleTableSyntax(QString syntax)
{
    qDebug() << syntax;
    insertText(syntax+"\n");
}

void CsoundQt::openManualExample(QString fileName)
{
#ifdef Q_OS_WIN // on windows poper path is not forwarded. Add it if necessary.
    if (!fileName.startsWith(helpPanel->docDir)) {
        fileName =  helpPanel->docDir + fileName;
    }
#endif
    loadFile(fileName);
}

void CsoundQt::openExternalBrowser(QUrl url)
{
    QString test = url.toString();
    if (!m_options->browser.isEmpty() && QFile::exists(m_options->browser)) {
        //execute(m_options->browser, "\"" + url.toString() + "\"");
        startProcess(m_options->browser, QStringList(url.toString()));
        // execute(m_options->browser, url.toString()); // remove quotes, otherwise wrong with changed QProcess
    }
    else {
//        QMessageBox::critical(this, tr("Error"),
//                              tr("Could not open external browser:\n%1\n"
//                                 "Please check preferences.").arg(m_options->browser));
        qDebug() << "No browser found from option, trying default system browser";
        QDesktopServices::openUrl(url);
    }
    //QDesktopServices::openUrl(url); // this opens it twice
}

void CsoundQt::increaseFontSize()
{
    changeFontSize(1);
}

void CsoundQt::decreaseFontSize()
{
    changeFontSize(-1);
}

void CsoundQt::changeFontSize(int change)
{
    if (helpPanel->hasFocus() ) {
        // QDEBUG << "change helpPanel size" << change;
        helpPanel->changeFontSize(change);
    }  else {
        m_options->fontPointSize += change; // this may affect the pad's font size as well...
        // QDEBUG << "main Editor size" << m_options->fontPointSize;
        for (int i = 0; i < documentPages.size(); i++) {
            documentPages[i]->setTextFont(QFont(m_options->font, (int) m_options->fontPointSize));
        }
        // apply the same size to code pad (maybe should be separate but so far it has used the same size.
        DocumentView *pad = static_cast<LiveCodeEditor*>(m_scratchPad->widget())->getDocumentView();
        pad->setFont(QFont(m_options->font, (int) m_options->fontPointSize));
    }
    // console live changing not supported now, possible only through Configuration Dialog
}

void CsoundQt::openPdfFile(QString name)
{
    if (!m_options->pdfviewer.isEmpty()) {
#ifndef Q_OS_MACOS
        if (!QFile::exists(m_options->pdfviewer)) {
            QMessageBox::critical(this,
                                  tr("Error"),
                                  tr("PDF viewer not found!\n"
                                     "Please go to Edit->Options->Environment and select directory\n"));
        }
#endif
        QString arg = "\"" + name + "\"";
        execute(m_options->pdfviewer, arg);
    }
    else {
        QDesktopServices::openUrl(QUrl::fromLocalFile (name));
    }
}

void CsoundQt::openFLOSSManual()
{
    openExternalBrowser(QUrl("https://flossmanual.csound.com/"));
}

void CsoundQt::openQuickRef()
{
    openPdfFile(quickRefFileName);
}

void CsoundQt::openOnlineDocumentation()
{
     openExternalBrowser(QUrl("http://csoundqt.github.io/pages/documentation.html"));
}


void CsoundQt::showManualOnline()
{
	openExternalBrowser(QUrl("http://csound.com/manual/index.html"));
}

void CsoundQt::resetPreferences()
{
    int ret = QMessageBox::question (this, tr("Reset Preferences"),
                                     tr("Are you sure you want to revert CsoundQt's "
                                        "preferences\nto their initial default values? "),
                                     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
    if (ret ==  QMessageBox::Ok) {
        m_resetPrefs = true;
        storeSettings();
        QMessageBox::information(this, tr("Reset Preferences"),
                                 tr("Preferences have been reset.\nYou must restart CsoundQt."),
                                 QMessageBox::Ok, QMessageBox::Ok);
    }
}

void CsoundQt::reportBug()
{
    openExternalBrowser(QUrl("https://github.com/CsoundQt/CsoundQt/issues/new"));
}

void CsoundQt::reportCsoundBug()
{
    openExternalBrowser(QUrl("https://github.com/csound/csound/issues/new"));
}

void CsoundQt::openShortcutDialog()
{
    KeyboardShortcuts dialog(this, m_keyActions);
    connect(&dialog, SIGNAL(restoreDefaultShortcuts()), this, SLOT(setDefaultKeyboardShortcuts()));
    dialog.exec();
    if(dialog.needsSave()) {
        writeSettings(QStringList(), 0);
    }
}

void CsoundQt::downloadManual()
{
    // NB! must be updated when new manual comes out!
    openExternalBrowser(QUrl("https://github.com/csound/manual/releases/download/6.18.0/Csound6.18.0_manual_html.zip"));
    QMessageBox::information(this, tr("Set manual path"),
                             tr("Unzip the manual to any location and set that path"
                                " in Configure/Enviromnent/Html doc directory"));
}

void CsoundQt::about()
{
    About *msgBox = new About(this);
    msgBox->setWindowFlags(msgBox->windowFlags() | Qt::FramelessWindowHint);

    QString text ="<h2>";
    text += tr("Authors: Andres Cabrera, Tarmo Johannes, Eduardo Moguillansky and others") + "</h2><h2>";
    text += tr("Version %1").arg(QCS_VERSION) + "</h2>";
    text += "<h3>" + tr("Released under the LGPLv2 or GPLv3") + "</h3>";
    text += tr("Using Csound version: <strong>%1</strong>, precision: <strong>%2</strong><br/>").arg(
                QString::number(csoundGetVersion()),
                csoundGetSizeOfMYFLT() == 8 ? "double (64-bit)" : "float (32-bit)");
    if(risset->isInstalled) {
        text += "<hr>";
        text += "<h3>Risset</h3>";
        text += tr("Risset package manager found, version: <strong>%1</strong><br/>").arg(risset->rissetVersion);
        text += tr("Plugins installed:<br/>");
        if(!risset->installedPlugins.isEmpty()) {
            text += "<ul>";
            for(auto pluginName : risset->installedPlugins) {
                auto opcodesInPlugin = risset->pluginOpcodes[pluginName];
                auto opcodesStr = opcodesInPlugin.join(", ");

                text += QString("<li><strong>%1</strong>: %2</li>").arg(pluginName, opcodesStr);
            }
            text += "</ul>";
        } else {
            text += "No plugins installed<br/>";
        }

    } else {
        text += tr("Risset package manager not found. See <center><a href=\"https://github.com/csound-plugins/risset\">github.com/csound-plugins/risset</a></center>");
    }
    text += "<hr>";

#ifdef USE_WEBENGINE
    text += tr("Html5 support based on QtWebEngine")+"<br />";
#endif
#ifdef USE_WEBKIT
    text += tr("Html5 support based on QtWebkit")+"<br />";
#endif
    text += "<br />";
    text += tr("Spanish translation: Andrés Cabrera and Guillermo Senna") + "<br />";
    text += tr("French translation: Fran&ccedil;ois Pinot") + "<br />";
    text += tr("German translation: Joachim Heintz") + "<br />";
    text += tr("Portuguese translation: Victor Lazzarini") + "<br />";
    text += tr("Italian translation: Francesco") + "<br />";
    text += tr("Turkish translation: Ali Isciler") + "<br />";
    text += tr("Finnish translation: Niko Humalam&auml;ki") + "<br />";
    text += tr("Russian translation: Gleb Rogozinski") + "<br />";
    text += tr("Persian translation: Amin Khoshsabk") + "<br />";
    text += tr("Korean translation: Jieun Jun") + "<br />";
    text += QString("<center><a href=\"http://csoundqt.github.io\">csoundqt.github.io</a></center>");
    text += QString("<center><a href=\"mailto:mantaraya36@gmail.com\">mantaraya36@gmail.com</a><br /> <a href=\"mailto:trmjhnns@gmail.com\">trmjhnns@gmail.com</a> </center><br />");
    text += tr("If you find CsoundQt useful, please consider donating to the project:");
    text += "<center><a href=\"http://sourceforge.net/donate/index.php?group_id=227265\">Support this project!</a></center><br>";

    text += tr("Please file bug reports and feature suggestions in the ");
    text += "<a href=\"https://github.com/CsoundQt/CsoundQt/issues\">";
    text += tr("CsoundQt tracker") + "</a>.<br><br />";

    text +=tr("Mailing Lists:");
    text += "<br /><a href=\"http://lists.sourceforge.net/lists/listinfo/qutecsound-users\">Join/Read CsoundQt Mailing List</a><br />";
        text += "<a href=\"http://lists.sourceforge.net/lists/listinfo/qutecsound-devel\">Join/Read CsoundQt Developer List</a><br />";
    text += "<a href=\"https://listserv.heanet.ie/cgi-bin/wa?A0=CSOUND\">Join/Read Csound Mailing List</a><br />";
    text += "<a href=\"https://listserv.heanet.ie/cgi-bin/wa?A0=CSOUND-DEV\"> Join/Read Csound Developer List</a><br />";

    text += "<br />"+ tr("Other Resources:") + "<br />";
    text += "<a href=\"http://csound.github.io\">Csound official homepage</a><br />";
    text += "<a href=\"http://www.csoundjournal.com/\">Csound Journal</a><br />";
    text += "<a href=\"http://csound.noisepages.com/\">The Csound Blog</a><br />";
    text +=  "<br />" + tr("Supported by:") +"<br />";
    text +=  "<a href=\"http://www.incontri.hmtm-hannover.de/\">Incontri - HMTM Hannover</a><br />";
    text += "<a href=\"http://sourceforge.net/project/project_donations.php?group_id=227265\">";
    text +=  tr("And other generous users.") + "</a><br />";

    msgBox->getTextEdit()->setOpenLinks(false);
    msgBox->setHtmlText(text);
    connect(msgBox->getTextEdit(), SIGNAL(anchorClicked(QUrl)), this, SLOT(openExternalBrowser(QUrl)));
    msgBox->exec();
    delete msgBox;
}

void CsoundQt::donate()
{
    openExternalBrowser(QUrl("http://sourceforge.net/donate/index.php?group_id=227265"));
}

void CsoundQt::documentWasModified(bool status)
{
    if(status) {
        setWindowModified(true);
        documentTabs->setTabIcon(curPage, modIcon);
    }
    else {
        setWindowModified(false);
        documentTabs->setTabIcon(curPage, QIcon());
    }
}

void CsoundQt::configure()
{
    ConfigDialog dialog(this,  m_options, &m_configlists);
    dialog.setCurrentTab(configureTab);
    // dialog.newParserCheckBox->setEnabled(csoundGetVersion() > 5125);
    dialog.multicoreCheckBox->setEnabled(csoundGetVersion() > 5125);
    dialog.numThreadsSpinBox->setEnabled(csoundGetVersion() > 5125);
    connect(dialog.applyButton, SIGNAL(released()),
            this, SLOT(applySettings()));
    connect(&dialog, SIGNAL(disableInternalRtMidi()),
            this, SLOT(disableInternalRtMidi()) );
    connect(dialog.testAudioSetupButton, SIGNAL(released()),
            this, SLOT(testAudioSetup()));
    if (dialog.exec() == QDialog::Accepted) {
        applySettings();
        storeSettings();
    }
    configureTab = dialog.currentTab();
}

void CsoundQt::applySettings()
{
    // This is called at initialization, when clicking "apply" in the settings dialog
    // and when closing it with "OK"
    for (int i = 0; i < documentPages.size(); i++) {
        setCurrentOptionsForPage(documentPages[i]);
    }
    auto toolButtonStyle = (m_options->iconText ? Qt::ToolButtonTextUnderIcon :
                                                  Qt::ToolButtonIconOnly);

    //	setUnifiedTitleAndToolBarOnMac(m_options->iconText);
    //	fileToolBar->setToolButtonStyle(toolButtonStyle);
    //	editToolBar->setToolButtonStyle(toolButtonStyle);
    controlToolBar->setToolButtonStyle(toolButtonStyle);
    configureToolBar->setToolButtonStyle(toolButtonStyle);
    //	fileToolBar->setVisible(m_options->showToolbar);
    //	editToolBar->setVisible(m_options->showToolbar);
    controlToolBar->setVisible(m_options->showToolbar);
    configureToolBar->setVisible(m_options->showToolbar);
    controlToolBar->setMovable(!(m_options->lockToolbar));
    configureToolBar->setMovable(!(m_options->lockToolbar));

    QString currentOptions = (m_options->useAPI ? tr("API") : tr("Console")) + " ";
    //  if (m_options->useAPI) {
    //    currentOptions +=  (m_options->thread ? tr("Thread") : tr("NoThread")) + " ";
    //  }

    // Display a summary of options on the status bar
    currentOptions += (m_options->saveWidgets ? tr("SaveWidgets") : tr("DontSaveWidgets")) + " ";
    QString playOptions = " (Audio:" + m_options->rtAudioModule + " ";
    playOptions += "MIDI:" +  m_options->rtMidiModule + ")";
    playOptions += " (" + (m_options->rtUseOptions ? tr("UseCsoundQtOptions"):
                                                     tr("DiscardCsoundQtOptions"));
    playOptions += " " + (m_options->rtOverrideOptions? tr("OverrideCsOptions"): tr("")) + ") ";
    playOptions += currentOptions;
    QString renderOptions =
            " ("
            + (m_options->fileUseOptions? tr("UseCsoundQtOptions"): tr("DiscardCsoundQtOptions"))
            + " ";
    renderOptions +=  "" + (m_options->fileOverrideOptions? tr("OverrideCsOptions"): tr("")) + ") ";
    renderOptions += currentOptions;
    runAct->setStatusTip(tr("Play") + playOptions);
    renderAct->setStatusTip(tr("Render to file") + renderOptions);

    if (! m_options->useCsoundMidi) {
        QString interfaceNotFoundMessage;
        // find midi interface by name; if not found, set to None
        // returns 9999 if not found
        m_options->midiInterface = midiHandler->findMidiInPortByName(m_options->midiInterfaceName);
        midiHandler->setMidiInterface(m_options->midiInterface);
        // close port, if 9999
        if (m_options->midiInterface==9999 && !m_options->midiInterfaceName.contains("None")) {
            qDebug()<<"MIDI In interface "<< m_options->midiInterfaceName << " not found!";
            interfaceNotFoundMessage = tr("MIDI In interface ") +
                    m_options->midiInterfaceName +
                    tr(" not found!\n Switching to None.\n");
        }

        m_options->midiOutInterface = midiHandler->findMidiOutPortByName(m_options->midiOutInterfaceName);
        midiHandler->setMidiOutInterface(m_options->midiOutInterface);
        if (m_options->midiOutInterface == 9999 && !m_options->midiOutInterfaceName.contains("None")) {
            qDebug()<<"MIDI Out interface "<< m_options->midiOutInterfaceName << " not found!";
            interfaceNotFoundMessage += tr("MIDI Out interface ") +
                    m_options->midiOutInterfaceName +
                    tr(" not found!\n Switching to None.");
        }
        //	if (!interfaceNotFoundMessage.isEmpty()) { // probably messagebox alwais is a bit too disturbing. Keep it quiet.
        //		QMessageBox::warning(this, tr("MIDI interface not found"),
        //							 interfaceNotFoundMessage);
        //	}
    }

    fillFavoriteMenu();
    DocumentView *pad = static_cast<LiveCodeEditor*>(m_scratchPad->widget())->getDocumentView();
    pad->setFont(QFont(m_options->font, (int) m_options->fontPointSize));
    if (csoundGetVersion() < 5140) {
        m_options->newParser = -1; // Don't use new parser flags
    }
    setupEnvironment();

    // set editorBgColor also for inspector, codepad and python console.
    // Maybe the latter should use the same color as console?
    // auto bgColor= m_options->editorBgColor.name();
    // m_inspector->setStyleSheet(QString("QTreeWidget { background-color: %1; }").arg(bgColor));
    // m_scratchPad->setStyleSheet(QString("QTextEdit { background-color: %1; }").arg(bgColor));
    this->setToolbarIconSize(m_options->toolbarIconSize);

    //storeSettings(); // save always when something new is changed

    this->helpPanel->setIconTheme(m_options->theme);
}

void CsoundQt::setCurrentOptionsForPage(DocumentPage *p)
{
    p->setColorVariables(m_options->colorVariables);
    p->setTabStopWidth(m_options->tabWidth);
    p->setTabIndents(m_options->tabIndents);
    p->setLineWrapMode(m_options->wrapLines ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
    p->setAutoComplete(m_options->autoComplete, m_options->autoCompleteDelay);
    p->setAutoParameterMode(m_options->autoParameterMode);
    p->setWidgetEnabled(m_options->enableWidgets);
    p->showWidgetTooltips(m_options->showTooltips);
    p->setKeyRepeatMode(m_options->keyRepeat);
    p->setOpenProperties(m_options->openProperties);
    p->setFontOffset(m_options->fontOffset);
    p->setFontScaling(m_options->fontScaling);
    p->setGraphUpdateRate(m_options->graphUpdateRate);
    p->setTextFont(QFont(m_options->font,
                         (int) m_options->fontPointSize));
    p->setLineEnding(m_options->lineEnding);
    p->setConsoleFont(QFont(m_options->consoleFont,
                            (int) m_options->consoleFontPointSize));
	p->setConsoleColors(m_options->consoleFontColor,
	                    m_options->consoleBgColor);
    p->setPythonExecutable(m_options->pythonExecutable);
    p->useOldFormat(m_options->oldFormat);
    p->setConsoleBufferSize(m_options->consoleBufferSize);
    p->showLineNumbers(m_options->showLineNumberArea);
    p->setHighlightingTheme(m_options->highlightingTheme);
    p->enableScoreSyntaxHighlighting(m_options->highlightScore);

    int flags = m_options->noBuffer ? QCS_NO_COPY_BUFFER : 0;
    flags |= m_options->noPython ? QCS_NO_PYTHON_CALLBACK : 0;
    flags |= m_options->noMessages ? QCS_NO_CONSOLE_MESSAGES : 0;
    flags |= m_options->noEvents ? QCS_NO_RT_EVENTS : 0;
    p->setFlags(flags);

}

void CsoundQt::runUtility(QString flags)
{
    qDebug("CsoundQt::runUtility");
    if (m_options->useAPI) {
#ifdef MACOSX_PRE_SNOW
        //Remember menu bar to set it after FLTK grabs it
        menuBarHandle = GetMenuBar();
#endif
        //    m_console->reset();
        static char *argv[33];
        QString name = "";
        QString fileFlags = flags.mid(flags.indexOf("\""));
        flags.remove(fileFlags);
        QStringList indFlags= flags.split(" ", SKIP_EMPTY_PARTS);
        QStringList files = fileFlags.split("\"", SKIP_EMPTY_PARTS);
        if (indFlags.size() < 2) {
            qDebug("CsoundQt::runUtility: Error: empty flags");
            return;
        }
        if (indFlags[0] == "-U") {
            indFlags.removeAt(0);
            name = indFlags[0];
            indFlags.removeAt(0);
        }
        else {
            qDebug("CsoundQt::runUtility: Error: unexpected flag!");
            return;
        }
        int index = 0;
        foreach (QString flag, indFlags) {
            argv[index] = (char *) calloc(flag.size()+1, sizeof(char));
            strcpy(argv[index], flag.toLocal8Bit());
            index++;
        }
        argv[index] = (char *) calloc(files[0].size()+1, sizeof(char));
        strcpy(argv[index++], files[0].toLocal8Bit());
        argv[index] = (char *) calloc(files[2].size()+1, sizeof(char));
        strcpy(argv[index++],files[2].toLocal8Bit());
        int argc = index;
        CSOUND *csoundU;
        csoundU=csoundCreate(nullptr, nullptr);
        csoundSetHostData(csoundU, (void *) m_console);
        csoundSetMessageCallback(csoundU, &CsoundQt::utilitiesMessageCallback);
        // Utilities always run in the same thread as CsoundQt
        csoundRunUtility(csoundU, name.toLocal8Bit(), argc, argv);
        csoundDestroy(csoundU);
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
#ifdef MACOSX_PRE_SNOW
        // Put menu bar back
        SetMenuBar(menuBarHandle);
#endif
    }
    else {
        QString script;
#ifdef Q_OS_WIN32
        script = "";
#ifdef USE_DOUBLE
        if (m_options->opcode7dir64Active)
            script += "set OPCODE7DIR64=" + m_options->opcode7dir64 + "\n";
#else
        if (m_options->opcodedirActive)
            script += "set OPCODEDIR=" + m_options->opcodedir + "\n";
#endif
        // Only OPCODEDIR left here as it must be present before csound initializes

        script += "cd " + QFileInfo(documentPages[curPage]->getFileName()).absolutePath() + "\n";
        script += "csound " + flags + "\n";
#else
        script = "#!/bin/sh\n";

        if (m_options->opcode7dir64Active)
            script += "set OPCODE7DIR64=" + m_options->opcode7dir64 + "\n";

        // Only OPCODEDIR left here as it must be present before csound initializes

        script += "cd " + QFileInfo(documentPages[curPage]->getFileName()).absolutePath() + "\n";
#ifdef Q_OS_MACOS
        script += "/usr/local/bin/csound " + flags + "\n";
#else
        script += "csound " + flags + "\n";
#endif
        script += "echo \"\nPress return to continue\"\n";
        script += "dummy_var=\"\"\n";
        script += "read dummy_var\n";
        script += "rm $0\n";
#endif
        QFile file(SCRIPT_NAME);
        if (!file.open(QIODevice::WriteOnly))
            return;

        QTextStream out(&file);
        out << script;
        file.flush();
        file.close();
        file.setPermissions (QFile::ExeOwner| QFile::WriteOwner| QFile::ReadOwner);

        QString options;
#ifdef Q_OS_LINUX
        options = "-e " + SCRIPT_NAME;
#endif
#ifdef Q_OS_SOLARIS
        options = "-e " + SCRIPT_NAME;
#endif
#ifdef Q_OS_MACOS
        options = SCRIPT_NAME;
#endif
#ifdef Q_OS_WIN32
        options = SCRIPT_NAME;
#endif
        execute(m_options->terminal, options);
    }
}


void CsoundQt::updateCurrentPageTask() {
    if (m_closing || curPage >= documentPages.size() ) {
        return;  // And don't call this again from the timer
    }
    Q_ASSERT(documentPages.size() > curPage);
    auto currentPage = documentPages[curPage];
    if ( !currentPage->getFileName().toLower().endsWith(".udo")  ) { // otherwise this marks an unedited .udo file as edited
        currentPage->parseUdos();
    }
    QTimer::singleShot(INSPECTOR_UPDATE_PERIOD_MS, this, SLOT(updateCurrentPageTask()));
}


void CsoundQt::updateInspector()
{
    if (m_closing  || curPage >= documentPages.size()) {
        return;  // And don't call this again from the timer
    }
    Q_ASSERT(documentPages.size() > curPage);
    if (!m_inspectorNeedsUpdate) {
        QTimer::singleShot(INSPECTOR_UPDATE_PERIOD_MS, this, SLOT(updateInspector()));
        return; // Retrigger timer, but do no update
    }
    if (!documentPages[curPage]->getFileName().endsWith(".py")) {
        m_inspector->parseText(documentPages[curPage]->getBasicText());
    }
    else {
        m_inspector->parsePythonText(documentPages[curPage]->getBasicText());
        // m_inspector->parsePythonText(documentPages[curPage]->getFullText());
    }
    m_inspectorNeedsUpdate = false;
    QTimer::singleShot(INSPECTOR_UPDATE_PERIOD_MS, this, SLOT(updateInspector()));
}

void CsoundQt::markInspectorUpdate()
{
    //  qDebug() << "CsoundQt::markInspectorUpdate()";
    m_inspectorNeedsUpdate = true;
}

void CsoundQt::setDefaultKeyboardShortcuts()
{
    //   m_keyActions.append(createCodeGraphAct);
    newAct->setShortcut(tr("Ctrl+N"));
    openAct->setShortcut(tr("Ctrl+O"));
    reloadAct->setShortcut(tr("Alt+R"));
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAsAct->setShortcut(tr("Shift+Ctrl+S"));
    createAppAct->setShortcut(tr(""));
    closeTabAct->setShortcut(tr("Ctrl+W"));

    printAct->setShortcut(tr("Ctrl+P"));
    exitAct->setShortcut(tr("Ctrl+Q"));

    undoAct->setShortcut(tr("Ctrl+Z"));
    redoAct->setShortcut(tr("Shift+Ctrl+Z"));

    cutAct->setShortcut(tr("Ctrl+X"));
    copyAct->setShortcut(tr("Ctrl+C"));
    pasteAct->setShortcut(tr("Ctrl+V"));
    duplicateAct->setShortcut(tr("Ctrl+D"));
    joinAct->setShortcut(tr(""));
    inToGetAct->setShortcut(tr(""));
    getToInAct->setShortcut(tr(""));
	midiControlAct->setShortcut(tr(""));
	csladspaAct->setShortcut(tr(""));
    findAct->setShortcut(tr("Ctrl+F"));
    findAgainAct->setShortcut(tr("Ctrl+G"));
    configureAct->setShortcut(tr("Ctrl+,"));
    editAct->setShortcut(tr("CTRL+E"));
    runAct->setShortcut(tr("CTRL+R"));
    runTermAct->setShortcut(QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_T));
    pauseAct->setShortcut(tr("Ctrl+Shift+M"));

    stopAct->setShortcut(tr("Ctrl+."));
    stopAllAct->setShortcut(QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_Period));

    recAct->setShortcut(tr("Ctrl+Space"));
    renderAct->setShortcut(tr("Alt+F"));
    externalPlayerAct->setShortcut(tr(""));
    externalEditorAct->setShortcut(tr(""));
    focusEditorAct->setShortcut(tr("Ctrl+0"));
    raiseHelpAct->setShortcut(tr("Ctrl+2"));
    raiseWidgetsAct->setShortcut(tr("Ctrl+1"));
    showGenAct->setShortcut(tr(""));
    showOverviewAct->setShortcut(tr(""));
    raiseConsoleAct->setShortcut(tr("Ctrl+3"));
    autocompleteAct->setShortcut(tr("Ctrl+M"));
#ifdef Q_OS_MACOS
    viewFullScreenAct->setShortcut(tr("Ctrl+Shift+F"));
#else
    viewFullScreenAct->setShortcut(tr("F11"));
#endif

    viewEditorFullScreenAct->setShortcut(tr("Alt+Ctrl+0"));
#ifdef QCS_QTHTML
    viewHtmlFullScreenAct->setShortcut(tr("Alt+Ctrl+4"));  // was H
#endif
    viewHelpFullScreenAct->setShortcut(tr("Alt+Ctrl+2")); // was vice versa
    viewWidgetsFullScreenAct->setShortcut(tr("Alt+Ctrl+1"));
#ifdef QCS_DEBUGGER
    showDebugAct->setShortcut(tr("F5"));
#endif
    showVirtualKeyboardAct->setShortcut(tr("Ctrl+Shift+V"));
    showTableEditorAct->setShortcut(tr(""));
    splitViewAct->setShortcut(tr("Ctrl+Shift+A"));
    midiLearnAct->setShortcut(tr("Ctrl+Shift+M"));
    createCodeGraphAct->setShortcut(tr("")); // was Ctr +4 save it for html view
    raiseInspectorAct->setShortcut(tr("Ctrl+5"));

#ifdef QCS_QTHTML
    raiseHtml5Act->setShortcut(tr("Ctrl+4")); // Alt-4 was before for Code graph
#endif
    openDocumentationAct->setShortcut(tr("F1"));
    showUtilitiesAct->setShortcut(tr("Ctrl+9"));

    setHelpEntryAct->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F1));
    externalBrowserAct->setShortcut(tr("Shift+Alt+F1"));
    showInspectorAct->setShortcut(tr("F5"));

    browseBackAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    browseForwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    openQuickRefAct->setShortcut(tr(""));
    commentAct->setShortcut(tr("Ctrl+/"));
    //  uncommentAct->setShortcut(tr("Shift+Ctrl+/"));
    indentAct->setShortcut(tr("Ctrl+I"));
    unindentAct->setShortcut(tr("Shift+Ctrl+I"));
    evaluateAct->setShortcut(tr("Shift+Ctrl+E"));
    evaluateSectionAct->setShortcut(tr("Shift+Ctrl+W"));
    scratchPadCsdModeAct->setShortcut(tr("Shift+Alt+S"));
    raiseScratchPadAct->setShortcut(tr("Ctrl+8"));
    killLineAct->setShortcut(tr("Ctrl+K"));
    killToEndAct->setShortcut(tr("Shift+Alt+K"));
    gotoLineAct->setShortcut(tr("Ctrl+L"));
    goBackAct->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
    showOrcAct->setShortcut(tr("Shift+Alt+1"));
    showScoreAct->setShortcut(tr("Shift+Alt+2"));
    showOptionsAct->setShortcut(tr("Shift+Alt+3"));
    showFileBAct->setShortcut(tr("Shift+Alt+4"));
    showOtherAct->setShortcut(tr("Shift+Alt+5"));
    showOtherCsdAct->setShortcut(tr("Shift+Alt+6"));
    showWidgetEditAct->setShortcut(tr("Shift+Alt+7"));
    lineNumbersAct->setShortcut(tr("Shift+Alt+L"));
    parameterModeAct->setShortcut(tr("Shift+Alt+P"));
    cabbageAct->setShortcut(tr("Shift+Ctrl+C"));
    //	showParametersAct->setShortcut(tr("Alt+P"));

    checkSyntaxAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));

    storeSettings();
}

void CsoundQt::showNoPythonQtWarning()
{
    QMessageBox::warning(this, tr("No PythonQt support"),
                         tr("This version of CsoundQt has been compiled without PythonQt support.\n"
                            "Extended Python features are not available"));
    qDebug() << "CsoundQt::showNoPythonQtWarning()";
}

void CsoundQt::showOrc(bool show)
{
    documentPages[curPage]->showOrc(show);
}

void CsoundQt::showScore(bool show)
{
    documentPages[curPage]->showScore(show);
}

void CsoundQt::showOptions(bool show)
{
    documentPages[curPage]->showOptions(show);
}

void CsoundQt::showFileB(bool show)
{
    documentPages[curPage]->showFileB(show);
}

void CsoundQt::showOther(bool show)
{
    documentPages[curPage]->showOther(show);
}

void CsoundQt::showOtherCsd(bool show)
{
    documentPages[curPage]->showOtherCsd(show);
}

void CsoundQt::showWidgetEdit(bool show)
{
    documentPages[curPage]->showWidgetEdit(show);
}

void CsoundQt::toggleLineArea()
{
    documentPages[curPage]->toggleLineArea();
}

void CsoundQt::toggleParameterMode()
{
    documentPages[curPage]->toggleParameterMode();
}

void CsoundQt::autocomplete()
{
    DocumentPage* page = documentPages[curPage];
    page->autocomplete();
}

#ifdef QCS_DEBUGGER

void CsoundQt::runDebugger()
{
    m_debugEngine = documentPages[curPage]->getEngine();
    m_debugEngine->setDebug();

    m_debugPanel->setDebugFilename(documentPages[curPage]->getFileName());
    connect(m_debugEngine, SIGNAL(breakpointReached()),
            this, SLOT(breakpointReached()));
    connect(m_debugPanel, SIGNAL(stopSignal()),
            this, SLOT(stopDebugger()));
    m_debugEngine->setStartingBreakpoints(m_debugPanel->getBreakpoints());

    documentPages[curPage]->getView()->m_mainEditor->setCurrentDebugLine(-1);
    play();
}

void CsoundQt::stopDebugger()
{
    if (m_debugEngine) {
        disconnect(m_debugEngine, SIGNAL(breakpointReached()), 0, 0);
        disconnect(m_debugPanel, SIGNAL(stopSignal()), 0, 0);
        m_debugEngine->stopDebug();
        m_debugEngine = 0;
        documentPages[curPage]->getView()->m_mainEditor->setCurrentDebugLine(-1);
    }
    markStopped();
}

void CsoundQt::pauseDebugger()
{
    if(m_debugEngine) {
        m_debugEngine->pauseDebug();
    }
}

void CsoundQt::continueDebugger()
{
    if(m_debugEngine) {
        m_debugEngine->continueDebug();
        documentPages[curPage]->getView()->m_mainEditor->setCurrentDebugLine(-1);
    }
}

void CsoundQt::addBreakpoint(int line, int instr, int skip)
{
    if (instr == 0) {
        documentPages[curPage]->getView()->m_mainEditor->markDebugLine(line);
    }
    if(m_debugEngine) {
        m_debugEngine->addBreakpoint(line, instr, skip);
    }
}

void CsoundQt::addInstrumentBreakpoint(double instr, int skip)
{
    if(m_debugEngine) {
        m_debugEngine->addInstrumentBreakpoint(instr, skip);
    }
}

void CsoundQt::removeBreakpoint(int line, int instr)
{
    if (instr == 0) {
        documentPages[curPage]->getView()->m_mainEditor->unmarkDebugLine(line);
    }
    if(m_debugEngine) {
        m_debugEngine->removeBreakpoint(line, instr);
    }
}

void CsoundQt::removeInstrumentBreakpoint(double instr)
{
    if(m_debugEngine) {
        m_debugEngine->removeInstrumentBreakpoint(instr);
    }
}

#endif

//void CsoundQt::showParametersInEditor()
//{
//	documentPages[curPage]->showParametersInEditor();
//}

void CsoundQt::createActions()
{
    // Actions that are not connected here depend on the active document, so they are
    // connected with connectActions() and are changed when the document changes.
    QString theme = m_options->theme;
    QString prefix = ":/themes/" + theme + "/";
    newAct = new QAction(QIcon(prefix + "edit-new.png"), tr("&New"), this);
    newAct->setStatusTip(tr("Create a new file"));
    newAct->setIconText(tr("New"));
    newAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(prefix + "folder.png"), tr("&Open..."), this);
    openAct->setStatusTip(tr("Open an existing file"));
    openAct->setIconText(tr("Open"));
    openAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    reloadAct = new QAction(QIcon(prefix + "reload.png"), tr("Reload"), this);
    reloadAct->setStatusTip(tr("Reload file from disk, discarding changes"));
    //   reloadAct->setIconText(tr("Reload"));
    reloadAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(reloadAct, SIGNAL(triggered()), this, SLOT(reload()));

    saveAct = new QAction(QIcon(prefix + "floppy.png"), tr("&Save"), this);
    saveAct->setStatusTip(tr("Save the document to disk"));
    saveAct->setIconText(tr("Save"));
    saveAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    saveAsAct->setIconText(tr("Save as"));
    saveAsAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    createAppAct = new QAction(tr("Create App..."), this);
    createAppAct->setStatusTip(tr("Create Standalone application"));
    createAppAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(createAppAct, SIGNAL(triggered()), this, SLOT(createApp()));

    saveNoWidgetsAct = new QAction(tr("Export without widgets"), this);
    saveNoWidgetsAct->setStatusTip(tr("Save to new file without including widget sections"));
    //   saveNoWidgetsAct->setIconText(tr("Save as"));
    saveNoWidgetsAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveNoWidgetsAct, SIGNAL(triggered()), this, SLOT(saveNoWidgets()));

    closeTabAct = new QAction(tr("Close current tab"), this);
    closeTabAct->setStatusTip(tr("Close current tab"));
    //   closeTabAct->setIconText(tr("Close"));
    closeTabAct->setIcon(QIcon(prefix + "edit-close.png"));
    closeTabAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(closeTabAct, SIGNAL(triggered()), this, SLOT(closeTab()));

    printAct = new QAction(tr("Print"), this);
    printAct->setStatusTip(tr("Print current document"));
    //   printAct->setIconText(tr("Print"));
    //   closeTabAct->setIcon(QIcon(prefix + "gtk-close.png"));
    printAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));

    //  for (int i = 0; i < QCS_MAX_RECENT_FILES; i++) {
    //    QAction *newAction = new QAction(this);
    //    openRecentAct.append(newAction);
    //    connect(newAction, SIGNAL(triggered()), this, SLOT(openFromAction()));
    //  }

    infoAct = new QAction(tr("File Information"), this);
    infoAct->setStatusTip(tr("Show information for the current file"));
    //   exitAct->setIconText(tr("Exit"));
    infoAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(infoAct, SIGNAL(triggered()), this, SLOT(info()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setStatusTip(tr("Exit the application"));
    //   exitAct->setIconText(tr("Exit"));
    exitAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    createCodeGraphAct = new QAction(tr("View Code &Graph"), this);
    createCodeGraphAct->setStatusTip(tr("View Code Graph"));
    //   createCodeGraphAct->setIconText("Exit");
    createCodeGraphAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(createCodeGraphAct, SIGNAL(triggered()), this, SLOT(createCodeGraph()));

    undoAct = new QAction(QIcon(prefix + "edit-undo.png"), tr("Undo"), this);
    undoAct->setStatusTip(tr("Undo last action"));
    undoAct->setIconText(tr("Undo"));
    undoAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));

    redoAct = new QAction(QIcon(prefix + "edit-redo.png"), tr("Redo"), this);
    redoAct->setStatusTip(tr("Redo last action"));
    redoAct->setIconText(tr("Redo"));
    redoAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(redoAct, SIGNAL(triggered()), this, SLOT(redo()));

    cutAct = new QAction(QIcon(prefix + "edit-cut.png"), tr("Cu&t"), this);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    cutAct->setIconText(tr("Cut"));
    cutAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    copyAct = new QAction(QIcon(prefix + "edit-copy.png"), tr("&Copy"), this);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    copyAct->setIconText(tr("Copy"));
    copyAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAct = new QAction(QIcon(prefix + "edit-paste.png"), tr("&Paste"), this);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    pasteAct->setIconText(tr("Paste"));
    pasteAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

    joinAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("&Join orc/sco"), this);
    joinAct->setStatusTip(tr("Join orc/sco files in a single csd file"));
    //   joinAct->setIconText(tr("Join"));
    joinAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(joinAct, SIGNAL(triggered()), this, SLOT(join()));

    evaluateAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Evaluate selection"), this);
    evaluateAct->setStatusTip(tr("Evaluate selection in Python Console"));
    evaluateAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(evaluateAct, SIGNAL(triggered()), this, SLOT(evaluate()));

    evaluateSectionAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Evaluate section"), this);
    evaluateSectionAct->setStatusTip(tr("Evaluate current section in Python Console"));
    evaluateSectionAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(evaluateSectionAct, SIGNAL(triggered()), this, SLOT(evaluateSection()));

    scratchPadCsdModeAct = new QAction(tr("Code Pad in Csound Mode"), this);
    scratchPadCsdModeAct->setStatusTip(tr("Toggle the mode for the scratch pad between python and csound"));
    scratchPadCsdModeAct->setShortcutContext(Qt::ApplicationShortcut);
    scratchPadCsdModeAct->setCheckable(true);
    connect(scratchPadCsdModeAct, SIGNAL(toggled(bool)), this, SLOT(setScratchPadMode(bool)));

    inToGetAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Invalue->Chnget"), this);
    inToGetAct->setStatusTip(tr("Convert invalue/outvalue to chnget/chnset"));
    inToGetAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(inToGetAct, SIGNAL(triggered()), this, SLOT(inToGet()));

    getToInAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Chnget->Invalue"), this);
    getToInAct->setStatusTip(tr("Convert chnget/chnset to invalue/outvalue"));
    getToInAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(getToInAct, SIGNAL(triggered()), this, SLOT(getToIn()));

	midiControlAct = new QAction(tr("Insert MidiControl instrument"), this);
	midiControlAct->setStatusTip(tr("Generate instrument according to MIDI bindings of widgets"));
	midiControlAct->setShortcutContext(Qt::ApplicationShortcut);
	connect(midiControlAct, SIGNAL(triggered()), this, SLOT(insertMidiControlInstrument()));

    csladspaAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Insert/Update CsLADSPA text"), this);
    csladspaAct->setStatusTip(tr("Insert/Update CsLADSPA section to csd file"));
    csladspaAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(csladspaAct, SIGNAL(triggered()), this, SLOT(updateCsladspaText()));

    cabbageAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Insert/Update Cabbage text"), this);
    cabbageAct->setStatusTip(tr("Insert/Update Cabbage section to csd file"));
    cabbageAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(cabbageAct, SIGNAL(triggered()), this, SLOT(updateCabbageText()));

    qmlAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Export widgets to QML"), this);
    qmlAct->setStatusTip(tr("Export widgets in QML format"));
    qmlAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(qmlAct, SIGNAL(triggered()), this, SLOT(saveWidgetsToQml()));

    findAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("&Find and Replace"), this);
    findAct->setStatusTip(tr("Find and replace strings in file"));
    //   findAct->setIconText(tr("Find"));
    findAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(findAct, SIGNAL(triggered()), this, SLOT(findReplace()));

    findAgainAct = new QAction(/*QIcon(prefix + "gtk-paste.png"),*/ tr("Find again"), this);
    findAgainAct->setStatusTip(tr("Find next appearance of string"));
    //   findAct->setIconText(tr("Find"));
    findAgainAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(findAgainAct, SIGNAL(triggered()), this, SLOT(findString()));

    configureAct = new QAction(QIcon(prefix + "settings.png"), tr("Configuration"), this);
    configureAct->setStatusTip(tr("Open configuration dialog"));
    configureAct->setIconText(tr("Configure"));
    configureAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(configureAct, SIGNAL(triggered()), this, SLOT(configure()));

    editAct = new QAction(/*QIcon(prefix + "gtk-media-play-ltr.png"),*/ tr("Widget Edit Mode"), this);
    editAct->setStatusTip(tr("Activate Edit Mode for Widget Panel"));
    //   editAct->setIconText("Play");
    editAct->setCheckable(true);
    editAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(editAct, SIGNAL(triggered(bool)), this, SLOT(setWidgetEditMode(bool)));

    runAct = new QAction(QIcon(prefix + "media-play.png"), tr("Run Csound"), this);
    runAct->setStatusTip(tr("Run current file"));
    runAct->setIconText(tr("Run"));
    runAct->setCheckable(true);
    runAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(runAct, SIGNAL(triggered()), this, SLOT(play()));

    runTermAct = new QAction(QIcon(prefix + "terminal.png"), tr("Run in Terminal"), this);
    runTermAct->setStatusTip(tr("Run in external shell"));
    runTermAct->setIconText(tr("Run in Term"));
    runTermAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(runTermAct, SIGNAL(triggered()), this, SLOT(runInTerm()));

    stopAct = new QAction(QIcon(prefix + "media-stop.png"), tr("Stop"), this);
    stopAct->setStatusTip(tr("Stop"));
    stopAct->setIconText(tr("Stop"));
    stopAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(stopAct, SIGNAL(triggered()), this, SLOT(stop()));

    pauseAct = new QAction(QIcon(prefix + "media-pause.png"), tr("Pause"), this);
    pauseAct->setStatusTip(tr("Pause"));
    pauseAct->setIconText(tr("Pause"));
    pauseAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(pauseAct, SIGNAL(triggered()), this, SLOT(pause()));

    stopAllAct = new QAction(QIcon(prefix + "media-stop.png"), tr("Stop All"), this);
    stopAllAct->setStatusTip(tr("Stop all running documents"));
    stopAllAct->setIconText(tr("Stop All"));
    stopAllAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(stopAllAct, SIGNAL(triggered()), this, SLOT(stopAll()));

    recAct = new QAction(QIcon(prefix + "media-record.png"), tr("Record"), this);
    recAct->setStatusTip(tr("Record"));
    recAct->setIconText(tr("Record"));
    recAct->setCheckable(true);
    recAct->setShortcutContext(Qt::ApplicationShortcut);
    recAct->setChecked(false);
    connect(recAct, SIGNAL(toggled(bool)), this, SLOT(record(bool)));

    // renderAct = new QAction(QIcon(prefix + "render.png"), tr("Render to file"), this);
    renderAct = new QAction(QIcon(prefix + "render.svg"), tr("Render to file"), this);
    renderAct->setStatusTip(tr("Render to file"));
    renderAct->setIconText(tr("Render"));
    renderAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(renderAct, SIGNAL(triggered()), this, SLOT(render()));

    testAudioSetupAct = new QAction(QIcon(prefix+"hearing.svg"),
                                    tr("Test Audio Setup"), this);
    connect(testAudioSetupAct, SIGNAL(triggered(bool)), this, SLOT(testAudioSetup()));

    checkSyntaxAct = new QAction(QIcon(prefix + "scratchpad.png"), tr("Check Syntax"), this);
    checkSyntaxAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(checkSyntaxAct, SIGNAL(triggered()), this, SLOT(checkSyntaxMenuAction()));

    externalPlayerAct = new QAction(QIcon(prefix + "playfile.png"), tr("Play Rendered Audiofile"), this);
    externalPlayerAct->setStatusTip(tr("Play rendered audiofile in external application"));
    externalPlayerAct->setIconText(tr("Ext. Player"));
    externalPlayerAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(externalPlayerAct, SIGNAL(triggered()), this, SLOT(openExternalPlayer()));

    externalEditorAct = new QAction(QIcon(prefix + "editfile.png"), tr("Edit Rendered Audiofile"), this);
    externalEditorAct->setStatusTip(tr("Edit rendered audiofile in External Editor"));
    externalEditorAct->setIconText(tr("Ext. Editor"));
    externalEditorAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(externalEditorAct, SIGNAL(triggered()), this, SLOT(openExternalEditor()));

    showWidgetsAct = new QAction(QIcon(prefix + "widgets.png"),
                                 tr("Widgets"), this);
    showWidgetsAct->setCheckable(true);
    //showWidgetsAct->setChecked(true);
    showWidgetsAct->setStatusTip(tr("Show Realtime Widgets"));
    showWidgetsAct->setIconText(tr("Widgets"));
    showWidgetsAct->setShortcutContext(Qt::ApplicationShortcut);

    raiseWidgetsAct = new QAction(this);
    raiseWidgetsAct->setText(tr("Show/Raise Widgets Panel"));
    raiseWidgetsAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(raiseWidgetsAct, SIGNAL(triggered()), focusMapper, SLOT(map()));
    focusMapper->setMapping(raiseWidgetsAct, 1);
    this->addAction(raiseWidgetsAct);

    showInspectorAct = new QAction(QIcon(prefix + "edit-find.png"), tr("Inspector"), this);
    showInspectorAct->setCheckable(true);
    showInspectorAct->setStatusTip(tr("Show Inspector"));
    showInspectorAct->setIconText(tr("Inspector"));
    showInspectorAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showInspectorAct, SIGNAL(triggered(bool)), m_inspector, SLOT(setVisible(bool)));
    connect(m_inspector, SIGNAL(Close(bool)), showInspectorAct, SLOT(setChecked(bool)));

    raiseInspectorAct = new QAction(this);
    raiseInspectorAct->setText(tr("Show/Raise Inspector Panel"));
    raiseInspectorAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(raiseInspectorAct, SIGNAL(triggered()), focusMapper, SLOT(map()));
    focusMapper->setMapping(raiseInspectorAct, 5);
    this->addAction(raiseInspectorAct);

    focusEditorAct = new QAction(tr("Focus Text Editor", "Give keyboard focus to the text editor"), this);
    focusEditorAct->setStatusTip(tr("Give keyboard focus to the text editor"));
    focusEditorAct->setIconText(tr("Editor"));
    focusEditorAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(focusEditorAct, SIGNAL(triggered()), this, SLOT(setEditorFocus()));

    showHelpAct = new QAction(QIcon(prefix + "info.png"), tr("Help Panel"), this);
    showHelpAct->setCheckable(true);
    showHelpAct->setChecked(true);
    showHelpAct->setStatusTip(tr("Show the Csound Manual Panel"));
    showHelpAct->setIconText(tr("Manual"));
    showHelpAct->setShortcutContext(Qt::ApplicationShortcut);
    // connect(showHelpAct, SIGNAL(toggled(bool)), helpPanel, SLOT(setVisible(bool)));
    connect(showHelpAct, SIGNAL(toggled(bool)), helpPanel, SLOT(setVisibleAndRaise(bool)));
    connect(helpPanel, SIGNAL(Close(bool)), showHelpAct, SLOT(setChecked(bool)));
    connect(helpPanel, SIGNAL(requestExternalBrowser(QUrl)), this, SLOT(openExternalBrowser(QUrl)) );

    raiseHelpAct = new QAction(this);
    raiseHelpAct->setText(tr("Show/Raise help panel"));
    raiseHelpAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(raiseHelpAct, SIGNAL(triggered()), focusMapper, SLOT(map()));
    focusMapper->setMapping(raiseHelpAct, 2);
    connect(focusMapper, SIGNAL(mapped(int)), this, SLOT(focusToTab(int)));
    this->addAction(raiseHelpAct);

    showScratchPadAct = new QAction(QIcon(prefix + "scratchpad.png"), tr("CodePad"), this);
    showScratchPadAct->setCheckable(true);
    //  showPythonConsoleAct->setChecked(true);  // Unnecessary because it is set by options
    showScratchPadAct->setStatusTip(tr("Show Code Pad"));
    showScratchPadAct->setIconText(tr("CodePad"));
    showScratchPadAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showScratchPadAct, SIGNAL(triggered(bool)), m_scratchPad, SLOT(setVisible(bool)));
    connect(m_scratchPad, SIGNAL(visibilityChanged(bool)), showScratchPadAct, SLOT(setChecked(bool)));

    raiseScratchPadAct = new QAction(this);
    raiseScratchPadAct->setText(tr("Show/Raise Code Pad"));
    raiseScratchPadAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(raiseScratchPadAct, SIGNAL(triggered()), focusMapper, SLOT(map()));
    focusMapper->setMapping(raiseScratchPadAct, 8);
    this->addAction(raiseScratchPadAct);

    showManualAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */tr("Csound Manual"), this);
    showManualAct->setStatusTip(tr("Show the Csound manual in the help panel"));
    showManualAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showManualAct, SIGNAL(triggered()), helpPanel, SLOT(showManual()));

	showManualOnlineAct = new QAction(tr("Csound Manual Online"), this);
	showManualOnlineAct->setStatusTip(tr("Show the Csound manual online"));
	showManualOnlineAct->setShortcutContext(Qt::ApplicationShortcut);
	connect(showManualOnlineAct, SIGNAL(triggered()), this, SLOT(showManualOnline()));

    downloadManualAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */tr("Download Csound Manual"), this);
    downloadManualAct->setStatusTip(tr("Download latest Csound manual"));
    downloadManualAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(downloadManualAct, SIGNAL(triggered()), this, SLOT(downloadManual()));


    showGenAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */tr("GEN Routines"), this);
    showGenAct->setStatusTip(tr("Show the GEN Routines Manual page"));
    showGenAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showGenAct, SIGNAL(triggered()), helpPanel, SLOT(showGen()));

    showOverviewAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */tr("Opcode Overview"), this);
    showOverviewAct->setStatusTip(tr("Show opcode overview"));
    showOverviewAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showOverviewAct, SIGNAL(triggered()), helpPanel, SLOT(showOverview()));

    showOpcodeQuickRefAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */tr("Opcode Quick Reference"), this);
    showOpcodeQuickRefAct->setStatusTip(tr("Show opcode quick reference page"));
    showOpcodeQuickRefAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showOpcodeQuickRefAct, SIGNAL(triggered()), helpPanel, SLOT(showOpcodeQuickRef()));

    showConsoleAct = new QAction(QIcon(prefix + "terminal.png"), tr("Output Console"), this);
    showConsoleAct->setCheckable(true);
    showConsoleAct->setChecked(true);
    showConsoleAct->setStatusTip(tr("Show Csound's message console"));
    showConsoleAct->setIconText(tr("Console"));
    showConsoleAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showConsoleAct, SIGNAL(toggled(bool)), m_console, SLOT(setVisible(bool)));
    connect(m_console, SIGNAL(Close(bool)), showConsoleAct, SLOT(setChecked(bool)));

    raiseConsoleAct = new QAction(this);
    raiseConsoleAct->setText(tr("Show/Raise Console"));
    raiseConsoleAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(raiseConsoleAct, SIGNAL(triggered()), focusMapper, SLOT(map()));
    focusMapper->setMapping(raiseConsoleAct, 3);
    this->addAction(raiseConsoleAct);

    viewFullScreenAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("View Fullscreen"), this);
    viewFullScreenAct->setCheckable(true);
    viewFullScreenAct->setChecked(false);
    viewFullScreenAct->setStatusTip(tr("Have CsoundQt occupy all available screen space"));
    viewFullScreenAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(viewFullScreenAct, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));

    viewEditorFullScreenAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("View Editor Fullscreen"), this);
    viewEditorFullScreenAct->setCheckable(true);
    viewEditorFullScreenAct->setChecked(false);
    viewEditorFullScreenAct->setStatusTip(tr("Have the editor occupy all available screen space"));
    viewEditorFullScreenAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(viewEditorFullScreenAct, SIGNAL(toggled(bool)), this, SLOT(setEditorFullScreen(bool)));

#ifdef QCS_QTHTML
    viewHtmlFullScreenAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("View HTML Fullscreen"), this);
    viewHtmlFullScreenAct->setCheckable(true);
    viewHtmlFullScreenAct->setChecked(false);
    viewHtmlFullScreenAct->setStatusTip(tr("Have the HTML page occupy all available screen space"));
    viewHtmlFullScreenAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(viewHtmlFullScreenAct, SIGNAL(toggled(bool)), this, SLOT(setHtmlFullScreen(bool)));
#endif
    viewHelpFullScreenAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("View Help Fullscreen"), this);
    viewHelpFullScreenAct->setCheckable(true);
    viewHelpFullScreenAct->setChecked(false);
    viewHelpFullScreenAct->setStatusTip(tr("Have the help page occupy all available screen space"));
    viewHelpFullScreenAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(viewHelpFullScreenAct, SIGNAL(toggled(bool)), this, SLOT(setHelpFullScreen(bool)));

    viewWidgetsFullScreenAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("View Widgets Fullscreen"), this);
    viewWidgetsFullScreenAct->setCheckable(true);
    viewWidgetsFullScreenAct->setChecked(false);
    viewWidgetsFullScreenAct->setStatusTip(tr("Have the widgets panel occupy all available screen space"));
    viewWidgetsFullScreenAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(viewWidgetsFullScreenAct, SIGNAL(toggled(bool)), this, SLOT(setWidgetsFullScreen(bool)));

#ifdef QCS_DEBUGGER
    showDebugAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show debugger"), this);
    showDebugAct->setCheckable(true);
    showDebugAct->setChecked(false);
    showDebugAct->setStatusTip(tr("Show the Csound debugger"));
    showDebugAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showDebugAct, SIGNAL(toggled(bool)), this, SLOT(showDebugger(bool)));
#endif
    showVirtualKeyboardAct = new QAction(QIcon(prefix + "midi-keyboard.png"), tr("Show Virtual Keyboard"), this);
    showVirtualKeyboardAct->setCheckable(true);
    showVirtualKeyboardAct->setChecked(false);
    showVirtualKeyboardAct->setStatusTip(tr("Show the Virtual MIDI Keyboard"));
    showVirtualKeyboardAct->setIconText(tr("Keyboard"));
    showVirtualKeyboardAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showVirtualKeyboardAct, SIGNAL(toggled(bool)), this, SLOT(showVirtualKeyboard(bool)));

    showTableEditorAct = new QAction(tr("Show Table editor"), this);
    showTableEditorAct->setCheckable(true); // TODO: make it clickable, ie not checkable
    showTableEditorAct->setChecked(false);
    showTableEditorAct->setStatusTip(tr("Show Table editor"));
    showTableEditorAct->setIconText(tr("Table editor"));
    showTableEditorAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showTableEditorAct, SIGNAL(toggled(bool)), this, SLOT(showTableEditor(bool)));


#if defined(QCS_QTHTML)
    showHtml5Act = new QAction(QIcon(":/images/html5.png"), tr("HTML View"), this);
    showHtml5Act->setIconText(tr("HTML"));
    showHtml5Act->setCheckable(true);
    showHtml5Act->setChecked(true);
    showHtml5Act->setStatusTip(tr("Show the HTML view"));
    showHtml5Act->setShortcutContext(Qt::ApplicationShortcut);
    connect(showHtml5Act, SIGNAL(toggled(bool)), this, SLOT(showHtml5Gui(bool)));
    //connect(csoundHtmlView, SIGNAL(Close(bool)), showHtml5Act, SLOT(setChecked(bool)));

    raiseHtml5Act = new QAction(this);
    raiseHtml5Act->setText(tr("Show/Raise HtmlView"));
    raiseHtml5Act->setShortcutContext(Qt::ApplicationShortcut);
    connect(raiseHtml5Act, SIGNAL(triggered()), focusMapper, SLOT(map()));
    focusMapper->setMapping(raiseHtml5Act, 4);
    this->addAction(raiseHtml5Act);
#endif
    splitViewAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Split View"), this);
    splitViewAct->setCheckable(true);
    splitViewAct->setChecked(false);
    splitViewAct->setStatusTip(tr("Toggle between full csd and split text display"));
    splitViewAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(splitViewAct, SIGNAL(toggled(bool)), this, SLOT(splitView(bool)));

    midiLearnAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("MIDI Learn"), this);
    midiLearnAct->setStatusTip(tr("Show MIDI Learn Window for widgets"));
    midiLearnAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(midiLearnAct, SIGNAL(triggered()), this, SLOT(showMidiLearn()));

    showOrcAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show Orchestra"), this);
    showOrcAct->setCheckable(true);
    showOrcAct->setChecked(true);
    showOrcAct->setEnabled(false);
    showOrcAct->setStatusTip(tr("Show orchestra panel in split view"));
    showOrcAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showOrcAct, SIGNAL(toggled(bool)), this, SLOT(showOrc(bool)));
    connect(splitViewAct, SIGNAL(toggled(bool)), showOrcAct, SLOT(setEnabled(bool)));

    showScoreAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show Score"), this);
    showScoreAct->setCheckable(true);
    showScoreAct->setChecked(true);
    showScoreAct->setEnabled(false);
    showScoreAct->setStatusTip(tr("Show orchestra panel in split view"));
    showScoreAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showScoreAct, SIGNAL(toggled(bool)), this, SLOT(showScore(bool)));
    connect(splitViewAct, SIGNAL(toggled(bool)), showScoreAct, SLOT(setEnabled(bool)));

    showOptionsAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show CsOptions"), this);
    showOptionsAct->setCheckable(true);
    showOptionsAct->setChecked(true);
    showOptionsAct->setEnabled(false);
    showOptionsAct->setStatusTip(tr("Show CsOptions section panel in split view"));
    showOptionsAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showOptionsAct, SIGNAL(toggled(bool)), this, SLOT(showOptions(bool)));
    connect(splitViewAct, SIGNAL(toggled(bool)), showOptionsAct, SLOT(setEnabled(bool)));

    showFileBAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show Embedded files"), this);
    showFileBAct->setCheckable(true);
    showFileBAct->setChecked(false);
    showFileBAct->setEnabled(false);
    showFileBAct->setStatusTip(tr("Show Embedded files panel in split view"));
    showFileBAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showFileBAct, SIGNAL(toggled(bool)), this, SLOT(showFileB(bool)));
    connect(splitViewAct, SIGNAL(toggled(bool)), showFileBAct, SLOT(setEnabled(bool)));

    showOtherAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show Information Text"), this);
    showOtherAct->setCheckable(true);
    showOtherAct->setChecked(false);
    showOtherAct->setEnabled(false);
    showOtherAct->setStatusTip(tr("Show information text panel in split view"));
    showOtherAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showOtherAct, SIGNAL(toggled(bool)), this, SLOT(showOther(bool)));
    connect(splitViewAct, SIGNAL(toggled(bool)), showOtherAct, SLOT(setEnabled(bool)));

    showOtherCsdAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show Extra Tags"), this);
    showOtherCsdAct->setCheckable(true);
    showOtherCsdAct->setChecked(false);
    showOtherCsdAct->setEnabled(false);
    showOtherCsdAct->setStatusTip(tr("Show extra tags panel in split view"));
    showOtherCsdAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showOtherCsdAct, SIGNAL(toggled(bool)), this, SLOT(showOtherCsd(bool)));
    connect(splitViewAct, SIGNAL(toggled(bool)), showOtherCsdAct, SLOT(setEnabled(bool)));

    showWidgetEditAct = new QAction(/*QIcon(prefix + "gksu-root-terminal.png"),*/ tr("Show Widgets Text"), this);
    showWidgetEditAct->setCheckable(true);
    showWidgetEditAct->setChecked(false);
    showWidgetEditAct->setEnabled(false);
    showWidgetEditAct->setStatusTip(tr("Show Widgets text panel in split view"));
    showWidgetEditAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showWidgetEditAct, SIGNAL(toggled(bool)), this, SLOT(showWidgetEdit(bool)));
    connect(splitViewAct, SIGNAL(toggled(bool)), showWidgetEditAct, SLOT(setEnabled(bool)));

    setHelpEntryAct = new QAction(QIcon(prefix + "info.png"), tr("Opcode Entry"), this);
    setHelpEntryAct->setStatusTip(tr("Show Opcode Entry in help panel"));
    setHelpEntryAct->setIconText(tr("Manual for opcode"));
    setHelpEntryAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(setHelpEntryAct, SIGNAL(triggered()), this, SLOT(setHelpEntry()));

    browseBackAct = new QAction(tr("Help Back"), this);
    browseBackAct->setStatusTip(tr("Go back in help page"));
    browseBackAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(browseBackAct, SIGNAL(triggered()), helpPanel, SLOT(browseBack()));

    browseForwardAct = new QAction(tr("Help Forward"), this);
    browseForwardAct->setStatusTip(tr("Go forward in help page"));
    browseForwardAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(browseForwardAct, SIGNAL(triggered()), helpPanel, SLOT(browseForward()));

    externalBrowserAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */ tr("Opcode Entry in External Browser"), this);
    externalBrowserAct->setStatusTip(tr("Show Opcode Entry in external browser"));
    externalBrowserAct->setShortcutContext(Qt::ApplicationShortcut);

    connect(externalBrowserAct, &QAction::triggered, this,
            [this](){this->helpForEntry(documentPages[curPage]->wordUnderCursor(), true);});

    openDocumentationAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */ tr("CsoundQt Documentation (online)"), this);
    openDocumentationAct->setStatusTip(tr("open CsoundQt Documentation in browser"));
    openDocumentationAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(openDocumentationAct, SIGNAL(triggered()), this, SLOT(openOnlineDocumentation()));

    openQuickRefAct = new QAction(/*QIcon(prefix + "gtk-info.png"), */ tr("Quick Reference Guide"), this);
    openQuickRefAct->setStatusTip(tr("Open Quick Reference Guide in PDF viewer"));
    openQuickRefAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(openQuickRefAct, SIGNAL(triggered()), this, SLOT(openQuickRef()));

    showUtilitiesAct = new QAction(QIcon(prefix + "devel.png"), tr("Utilities"), this);
    showUtilitiesAct->setCheckable(true);
    showUtilitiesAct->setChecked(false);
    showUtilitiesAct->setStatusTip(tr("Show the Csound Utilities dialog"));
    showUtilitiesAct->setIconText(tr("Utilities"));
    showUtilitiesAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(showUtilitiesAct, SIGNAL(triggered(bool)), this, SLOT(showUtilities(bool)));

    setShortcutsAct = new QAction(tr("Keyboard Shortcuts"), this);
    setShortcutsAct->setStatusTip(tr("Set Keyboard Shortcuts"));
    setShortcutsAct->setIconText(tr("Set Shortcuts"));
    setShortcutsAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(setShortcutsAct, SIGNAL(triggered()), this, SLOT(openShortcutDialog()));

    commentAct = new QAction(tr("Comment/Uncomment"), this);
    commentAct->setStatusTip(tr("Comment/Uncomment selection"));
    commentAct->setShortcutContext(Qt::ApplicationShortcut);
    //  commentAct->setIconText(tr("Comment"));
    //  connect(commentAct, SIGNAL(triggered()), this, SLOT(controlD()));

    //  uncommentAct = new QAction(tr("Uncomment"), this);
    //  uncommentAct->setStatusTip(tr("Uncomment selection"));
    //  uncommentAct->setShortcutContext(Qt::ApplicationShortcut);
    //   uncommentAct->setIconText(tr("Uncomment"));
    //   connect(uncommentAct, SIGNAL(triggered()), this, SLOT(uncomment()));

    indentAct = new QAction(tr("Indent"), this);
    indentAct->setStatusTip(tr("Indent selection"));
    indentAct->setShortcutContext(Qt::ApplicationShortcut);
    //   indentAct->setIconText(tr("Indent"));
    //   connect(indentAct, SIGNAL(triggered()), this, SLOT(indent()));

    unindentAct = new QAction(tr("Unindent"), this);
    unindentAct->setStatusTip(tr("Unindent selection"));
    unindentAct->setShortcutContext(Qt::ApplicationShortcut);
    //   unindentAct->setIconText(tr("Unindent"));
    //   connect(unindentAct, SIGNAL(triggered()), this, SLOT(unindent()));

    killLineAct = new QAction(tr("Kill Line"), this);
    killLineAct->setStatusTip(tr("Completely delete current line"));
    killLineAct->setShortcutContext(Qt::ApplicationShortcut);

    killToEndAct = new QAction(tr("Kill to End of Line"), this);
    killToEndAct->setStatusTip(tr("Delete everything from cursor to the end of the current line"));
    killToEndAct->setShortcutContext(Qt::ApplicationShortcut);

    gotoLineAct = new QAction(tr("Goto Line"), this);
    gotoLineAct->setStatusTip(tr("Go to a given line"));
    gotoLineAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(gotoLineAct, SIGNAL(triggered()), this, SLOT(gotoLineDialog()));

    goBackAct = new QAction(tr("Go Back"), this);
    goBackAct->setStatusTip(tr("Go back to previous position"));
    goBackAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(goBackAct, &QAction::triggered, [this]() { this->getCurrentDocumentPage()->goBackToPreviousPosition(); });

    aboutAct = new QAction(tr("&About CsoundQt"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    //   aboutAct->setIconText(tr("About"));
    aboutAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    donateAct = new QAction(tr("Donate to CsoundQt"), this);
    donateAct->setStatusTip(tr("Donate to support development of CsoundQt"));
    //   aboutAct->setIconText(tr("About"));
    donateAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(donateAct, SIGNAL(triggered()), this, SLOT(donate()));

    //  aboutQtAct = new QAction(tr("About &Qt"), this);
    //  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    ////   aboutQtAct->setIconText(tr("About Qt"));
    //  aboutQtAct->setShortcutContext(Qt::ApplicationShortcut);
    //  connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    resetPreferencesAct = new QAction(tr("Reset Preferences"), this);
    resetPreferencesAct->setStatusTip(tr("Reset CsoundQt's preferences to their original default state"));
    resetPreferencesAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(resetPreferencesAct, SIGNAL(triggered()), this, SLOT(resetPreferences()));

    reportBugAct = new QAction(tr("Report a CsoundQt Bug"), this);
    reportBugAct->setStatusTip(tr("Report a CsoundQt bug that is clearly related to the front-end"));
    reportBugAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(reportBugAct, SIGNAL(triggered()), this, SLOT(reportBug()));

    reportCsoundBugAct = new QAction(tr("Report a Csound Bug"), this);
    reportCsoundBugAct->setStatusTip(tr("Report a bug that is caused by the core Csound -  when the same problem occurs also with \"Run in Terminal\""));
    reportCsoundBugAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(reportCsoundBugAct, SIGNAL(triggered()), this, SLOT(reportCsoundBug()));

    duplicateAct = new QAction(tr("Duplicate Widgets"), this);
    duplicateAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(duplicateAct, SIGNAL(triggered()), this, SLOT(duplicate()));

    lineNumbersAct = new QAction(tr("Show/hide line number area"),this);
    lineNumbersAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(lineNumbersAct,SIGNAL(triggered()), this, SLOT(toggleLineArea()));

    parameterModeAct = new QAction(tr("Toggle parameter mode"),this);
    connect(parameterModeAct,SIGNAL(triggered()), this, SLOT(toggleParameterMode()));

    autocompleteAct = new QAction(tr("Autocomplete"), this);
    autocompleteAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(autocompleteAct, SIGNAL(triggered()), this, SLOT(autocomplete()));

    //	showParametersAct = new QAction(tr("Show available parameters"),this);
    //	connect(showParametersAct,SIGNAL(triggered()), this, SLOT(showParametersInEditor()));

    // font size ZoomIn, ZoomOut
    QAction *increaseFontAction = new QAction(this);
    QAction *decreaseFontAction = new QAction(this);
    increaseFontAction->setShortcut(QKeySequence::ZoomIn);
    decreaseFontAction->setShortcut(QKeySequence::ZoomOut);
    connect(increaseFontAction, &QAction::triggered, this, &CsoundQt::increaseFontSize );
    connect(decreaseFontAction, &QAction::triggered, this, &CsoundQt::decreaseFontSize);
    addAction(increaseFontAction);
    addAction(decreaseFontAction);

    setKeyboardShortcutsList();
    setDefaultKeyboardShortcuts();
}

void CsoundQt::setKeyboardShortcutsList()
{
    m_keyActions.append(newAct);
    m_keyActions.append(openAct);
    m_keyActions.append(reloadAct);
    m_keyActions.append(saveAct);
    m_keyActions.append(saveAsAct);
    m_keyActions.append(createAppAct);
    m_keyActions.append(closeTabAct);
    m_keyActions.append(printAct);
    m_keyActions.append(exitAct);
    m_keyActions.append(createCodeGraphAct);
    m_keyActions.append(undoAct);
    m_keyActions.append(redoAct);
    m_keyActions.append(cutAct);
    m_keyActions.append(copyAct);
    m_keyActions.append(pasteAct);
    m_keyActions.append(duplicateAct);
    m_keyActions.append(joinAct);
    m_keyActions.append(inToGetAct);
    m_keyActions.append(getToInAct);
    m_keyActions.append(csladspaAct);
    m_keyActions.append(cabbageAct);
    m_keyActions.append(findAct);
    m_keyActions.append(findAgainAct);
    m_keyActions.append(configureAct);
    m_keyActions.append(editAct);
    m_keyActions.append(runAct);
    m_keyActions.append(runTermAct);
    m_keyActions.append(stopAct);
    m_keyActions.append(pauseAct);
    m_keyActions.append(stopAllAct);
    m_keyActions.append(recAct);
    m_keyActions.append(renderAct);
    m_keyActions.append(commentAct);
    m_keyActions.append(indentAct);
    m_keyActions.append(unindentAct);
    m_keyActions.append(externalPlayerAct);
    m_keyActions.append(externalEditorAct);
    m_keyActions.append(focusEditorAct);
    m_keyActions.append(raiseWidgetsAct);
    m_keyActions.append(showInspectorAct);
    m_keyActions.append(raiseHelpAct);
    m_keyActions.append(showGenAct);
    m_keyActions.append(showOverviewAct);
    m_keyActions.append(raiseConsoleAct);
    m_keyActions.append(raiseInspectorAct);
    m_keyActions.append(raiseScratchPadAct);
    m_keyActions.append(showUtilitiesAct);
    m_keyActions.append(showVirtualKeyboardAct);
    m_keyActions.append(showTableEditorAct);
    m_keyActions.append(checkSyntaxAct);
    m_keyActions.append(autocompleteAct);
#if defined(QCS_QTHTML)
    m_keyActions.append(raiseHtml5Act);
#endif
    m_keyActions.append(setHelpEntryAct);
    m_keyActions.append(browseBackAct);
    m_keyActions.append(browseForwardAct);
    m_keyActions.append(externalBrowserAct);
    m_keyActions.append(openQuickRefAct);
    m_keyActions.append(openDocumentationAct);
    // m_keyActions.append(showOpcodeQuickRefAct);
    m_keyActions.append(infoAct);
    m_keyActions.append(viewFullScreenAct);
    m_keyActions.append(viewEditorFullScreenAct);
#if defined(QCS_QTHTML)
    m_keyActions.append(viewHtmlFullScreenAct);
#endif
    m_keyActions.append(viewHelpFullScreenAct);
    m_keyActions.append(viewWidgetsFullScreenAct);
#ifdef QCS_DEBUGGER
    m_keyActions.append(showDebugAct);
#endif
    m_keyActions.append(splitViewAct);
    m_keyActions.append(midiLearnAct);
    m_keyActions.append(killLineAct);
    m_keyActions.append(killToEndAct);
    m_keyActions.append(gotoLineAct);
    m_keyActions.append(goBackAct);
    m_keyActions.append(evaluateAct);
    m_keyActions.append(evaluateSectionAct);
    m_keyActions.append(scratchPadCsdModeAct);
    m_keyActions.append(showOrcAct);
    m_keyActions.append(showScoreAct);
    m_keyActions.append(showOptionsAct);
    m_keyActions.append(showFileBAct);
    m_keyActions.append(showOtherAct);
    m_keyActions.append(showOtherCsdAct);
    m_keyActions.append(showWidgetEditAct);
    m_keyActions.append(lineNumbersAct);
    m_keyActions.append(parameterModeAct);
    //	m_keyActions.append(showParametersAct);
}

void CsoundQt::connectActions()
{
    DocumentPage * doc = documentPages[curPage];

    disconnect(commentAct, 0, 0, 0);
    //  disconnect(uncommentAct, 0, 0, 0);
    disconnect(indentAct, 0, 0, 0);
    disconnect(unindentAct, 0, 0, 0);
    disconnect(killLineAct, 0, 0, 0);
    disconnect(killToEndAct, 0, 0, 0);
    // disconnect(gotoLineAct, 0, 0, 0);
    //  disconnect(findAct, 0, 0, 0);
    //  disconnect(findAgainAct, 0, 0, 0);
    connect(commentAct, SIGNAL(triggered()), doc, SLOT(comment()));
    //  connect(uncommentAct, SIGNAL(triggered()), doc, SLOT(uncomment()));
    connect(indentAct, SIGNAL(triggered()), doc, SLOT(indent()));
    connect(unindentAct, SIGNAL(triggered()), doc, SLOT(unindent()));
    connect(killLineAct, SIGNAL(triggered()), doc, SLOT(killLine()));
    connect(killToEndAct, SIGNAL(triggered()), doc, SLOT(killToEnd()));

    // line and column number connection
    connect(doc->getView(), SIGNAL(newLineAndColumn(int, int)), this, SLOT(setLineAndColumn(int,int)));

    // connect(gotoLineAct, SIGNAL(triggered()), doc, SLOT(gotoLineDialog()));
    //  disconnect(doc, SIGNAL(copyAvailable(bool)), 0, 0);
    //  disconnect(doc, SIGNAL(copyAvailable(bool)), 0, 0);
    //TODO put these back
    //   connect(doc, SIGNAL(copyAvailable(bool)),
    //           cutAct, SLOT(setEnabled(bool)));
    //   connect(doc, SIGNAL(copyAvailable(bool)),
    //           copyAct, SLOT(setEnabled(bool)));

    //  disconnect(doc, SIGNAL(textChanged()), 0, 0);
    //  disconnect(doc, SIGNAL(cursorPositionChanged()), 0, 0);

    //  disconnect(widgetPanel, SIGNAL(widgetsChanged(QString)),0,0);
    //   connect(widgetPanel, SIGNAL(widgetsChanged(QString)),
    //           doc, SLOT(setMacWidgetsText(QString)) );

    // Connect inspector actions to document
    disconnect(m_inspector, 0, 0, 0);
    connect(m_inspector, SIGNAL(jumpToLine(int)),
            doc, SLOT(jumpToLine(int)));
    disconnect(m_inspector, 0, 0, 0);
    connect(m_inspector, SIGNAL(jumpToLine(int)),
            doc, SLOT(jumpToLine(int)));
    connect(m_inspector, SIGNAL(Close(bool)), showInspectorAct, SLOT(setChecked(bool)));

    disconnect(doc, 0,0,0);
    connect(doc, SIGNAL(stopSignal()), this, SLOT(markStopped()));
    connect(doc, SIGNAL(setHelpSignal()), this, SLOT(setHelpEntry()));
    connect(doc, SIGNAL(currentTextUpdated()), this, SLOT(markInspectorUpdate()));

    connect(doc->getView(), SIGNAL(opcodeSyntaxSignal(QString)),
            this, SLOT(statusBarMessage(QString)));

    connect(doc, SIGNAL(modified()), this, SLOT(documentWasModified()));
    connect(doc, &DocumentPage::unmodified, [this](){this->documentWasModified(false);});

    //  connect(documentPages[curPage], SIGNAL(setWidgetClipboardSignal(QString)),
    //          this, SLOT(setWidgetClipboard(QString)));
    connect(doc, SIGNAL(setCurrentAudioFile(QString)),
            this, SLOT(setCurrentAudioFile(QString)));
    // connect(doc, SIGNAL(evaluatePythonSignal(QString)),
    //         this, SLOT(evaluateString(QString)));

    disconnect(showWidgetsAct, 0,0,0);
    if (m_options->widgetsIndependent) {
        connect(showWidgetsAct, SIGNAL(triggered(bool)), doc, SLOT(showWidgets(bool)));
        auto wl = doc->getWidgetLayout();
        connect(wl, SIGNAL(windowStatus(bool)), showWidgetsAct, SLOT(setChecked(bool)));
        //    connect(widgetPanel, SIGNAL(Close(bool)), showWidgetsAct, SLOT(setChecked(bool)));
    }
    else {
        // connect(showWidgetsAct, SIGNAL(triggered(bool)), widgetPanel, SLOT(setVisible(bool)));
        connect(showWidgetsAct, SIGNAL(triggered(bool)), widgetPanel, SLOT(showAndRaise(bool)));

        connect(widgetPanel, SIGNAL( Close(bool)), showWidgetsAct, SLOT(setChecked(bool)));
    }
}

QString CsoundQt::getExamplePath(QString dir)
{
    QString examplePath;

    if (!m_options->examplePath.isEmpty() && QDir(m_options->examplePath + dir).exists()) {
        examplePath = m_options->examplePath + dir;
        return examplePath;
    }

#ifdef Q_OS_WIN32
    examplePath = qApp->applicationDirPath() + "/Examples/" + dir;
    if (!QDir(examplePath).exists()) {
        QString programFilesPath= QDir::fromNativeSeparators(getenv("PROGRAMFILES"));
        examplePath =  programFilesPath + "/Csound6/bin/Examples/" + dir; // NB! with csound6.0.6 no Floss/mCcurdy/Stria examples there. Copy manually
        //qDebug()<<"Windows examplepath: "<<examplePath;
    }
#endif
#ifdef Q_OS_MAC
    examplePath = qApp->applicationDirPath() + "/../Resources/Examples/" + dir;
    qDebug() << examplePath;
#endif
#ifdef Q_OS_LINUX
    examplePath = QString(); //qApp->applicationDirPath() + "/Examples/" + dir;
    QStringList possiblePaths;
    possiblePaths << qApp->applicationDirPath() + "/Examples/"
                  << qApp->applicationDirPath() + "/../src/Examples/"
                  << qApp->applicationDirPath() + "/../../csoundqt/src/Examples/"
                  << qApp->applicationDirPath() + "/../../CsoundQt/src/Examples/"
                  << qApp->applicationDirPath() + "/../share/csoundqt/Examples/"
                  <<  "/../../csoundq/src/Examples/"
                  << "~/.local/share/csoundq/Examples/" << "/usr/share/qutecsound/Examples/"
                  << "~/.local/share/csoundqt/Examples/"
                  << "/usr/share/csoundqt/Examples/";

    foreach (QString path, possiblePaths) {
        path += dir;
        if (QDir(path).exists()) {
            examplePath = path;
            break;
        }
    }
    if (examplePath.isEmpty()) {
        qDebug() << "Path to extended examples not found!";
    } else {
        qDebug() << "ExamplePath: " << examplePath;
    }

#endif
#ifdef Q_OS_SOLARIS
    examplePath = qApp->applicationDirPath() + "/Examples/" + dir;
    if (!QDir(examplePath).exists()) {
        examplePath = "/usr/share/csoundq/Examples/" + dir;
    }
    if (!QDir(examplePath).exists()) {
        examplePath = qApp->applicationDirPath() + "/../src/Examples/" + dir;
    }
#endif
    return examplePath;
}



void CsoundQt::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(saveNoWidgetsAct);
    fileMenu->addAction(qmlAct);
    //  fileMenu->addAction(createAppAct);
    fileMenu->addAction(reloadAct);
    fileMenu->addAction(closeTabAct);
    fileMenu->addAction(printAct);
    fileMenu->addAction(infoAct);
    fileMenu->addSeparator();
    recentMenu = fileMenu->addMenu(tr("Recent files"));
    templateMenu = fileMenu->addMenu(tr("Templates"));
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addAction(autocompleteAct);
    editMenu->addAction(evaluateAct);
    editMenu->addAction(evaluateSectionAct);
    editMenu->addAction(scratchPadCsdModeAct);

    editMenu->addSeparator();
    editMenu->addAction(findAct);
    editMenu->addAction(findAgainAct);
    editMenu->addAction(gotoLineAct);
    // editMenu->addAction(goBackAct);
    editMenu->addSeparator();
    editMenu->addAction(commentAct);
    //  editMenu->addAction(uncommentAct);
    editMenu->addAction(indentAct);
    editMenu->addAction(unindentAct);
    // editMenu->addAction(killLineAct);
    // editMenu->addAction(killToEndAct);
    editMenu->addAction(lineNumbersAct);
    editMenu->addAction(parameterModeAct);
    editMenu->addSeparator();
    editMenu->addAction(joinAct);
    editMenu->addAction(inToGetAct);
    editMenu->addAction(getToInAct);
	editMenu->addAction(midiControlAct);
    // editMenu->addAction(csladspaAct);
    editMenu->addAction(cabbageAct);
    editMenu->addSeparator();
    editMenu->addAction(editAct);
    editMenu->addSeparator();
    editMenu->addAction(configureAct);
    editMenu->addAction(setShortcutsAct);

    controlMenu = menuBar()->addMenu(tr("Control"));
    controlMenu->addAction(runAct);
    controlMenu->addAction(runTermAct);
    controlMenu->addAction(pauseAct);
    controlMenu->addAction(renderAct);
    controlMenu->addAction(recAct);
    controlMenu->addAction(stopAct);
    controlMenu->addAction(stopAllAct);
    controlMenu->addSeparator();
    controlMenu->addAction(externalEditorAct);
    controlMenu->addAction(externalPlayerAct);
    controlMenu->addAction(checkSyntaxAct);
    controlMenu->addSeparator();
    controlMenu->addAction(testAudioSetupAct);


    viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(focusEditorAct);
    viewMenu->addAction(showWidgetsAct);
    viewMenu->addAction(showHelpAct);
    viewMenu->addAction(showConsoleAct);
    viewMenu->addAction(showUtilitiesAct);
    viewMenu->addAction(createCodeGraphAct);
    viewMenu->addAction(showInspectorAct);
    viewMenu->addAction(showScratchPadAct);
#if defined(QCS_QTHTML)
    viewMenu->addAction(showHtml5Act);
#endif
    viewMenu->addAction(showUtilitiesAct);
#ifdef QCS_DEBUGGER
    viewMenu->addAction(showDebugAct);
#endif
    viewMenu->addAction(midiLearnAct);
    viewMenu->addAction(showVirtualKeyboardAct);
    viewMenu->addAction(showTableEditorAct);
    viewMenu->addSeparator();
    viewMenu->addAction(viewFullScreenAct);
    viewMenu->addAction(viewEditorFullScreenAct);
#if defined(QCS_QTHTML)
    viewMenu->addAction(viewHtmlFullScreenAct);
#endif
    viewMenu->addAction(viewHelpFullScreenAct);
    viewMenu->addAction(viewWidgetsFullScreenAct);
    viewMenu->addSeparator();
    viewMenu->addAction(splitViewAct);
    viewMenu->addSeparator();
    viewMenu->addAction(showOrcAct);
    viewMenu->addAction(showScoreAct);
    viewMenu->addAction(showOptionsAct);
    viewMenu->addAction(showFileBAct);
    viewMenu->addAction(showOtherAct);
    viewMenu->addAction(showOtherCsdAct);
    viewMenu->addAction(showWidgetEditAct);

    fillExampleMenu();

    favoriteMenu = menuBar()->addMenu(tr("Favorites"));

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(openDocumentationAct);
    helpMenu->addAction(setHelpEntryAct);
    helpMenu->addAction(externalBrowserAct);
    // helpMenu->addSeparator();
    // helpMenu->addAction(browseBackAct);
    // helpMenu->addAction(browseForwardAct);
    helpMenu->addSeparator();
    helpMenu->addAction(showManualAct);
	helpMenu->addAction(showManualOnlineAct);
    helpMenu->addAction(downloadManualAct);
    helpMenu->addAction(showOverviewAct);
    // helpMenu->addAction(showOpcodeQuickRefAct);
    helpMenu->addAction(showGenAct);
    // helpMenu->addAction(openQuickRefAct);
    helpMenu->addSeparator();
    helpMenu->addAction(resetPreferencesAct);
    helpMenu->addSeparator();
    helpMenu->addAction(reportBugAct);
    helpMenu->addAction(reportCsoundBugAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);
    // helpMenu->addAction(donateAct);
    // uhelpMenu->addAction(aboutQtAct);

}


void CsoundQt::fillExampleMenu()
{
    QString examplePath = getExamplePath("");
    qDebug() << examplePath;
    if (examplePath.isEmpty() ) {
        qDebug() << "examplePath not set";
        return;
    }

    QMenu *examplesMenu = menuBar()->addMenu(tr("Examples"));

    QDir dir(examplePath);
    if (dir.count() > 0) {
        fillExampleSubmenu(dir.absolutePath(), examplesMenu, 0);
    }

    if (!examplePath.isEmpty()) {
        examplesMenu->addSeparator();
        openExamplesFolderAct = new QAction(tr("Open Examples Folder"), this);
        openExamplesFolderAct->setStatusTip(tr("Save the document under a new name"));
        openExamplesFolderAct->setShortcutContext(Qt::ApplicationShortcut);
        connect(openExamplesFolderAct, SIGNAL(triggered()), this, SLOT(openExamplesFolder()));
        examplesMenu->addAction(openExamplesFolderAct);
    }


}

void CsoundQt::fillExampleSubmenu(QDir dir, QMenu *m, int depth)
{
    QString test = dir.dirName();
    //qDebug() << "dirName: " << test;
    if (depth > m_options->menuDepth) {
        return;
    }

    // add extra entry for FLOSS manual
    if (dir.dirName().startsWith("FLOSS")) {
        m->addAction(tr("Read FLOSS Manual Online"),this, SLOT(openFLOSSManual()));
        m->addSeparator();
    }

    QStringList filters;
    filters << "*.csd" << "*.pdf" << "*.html";
    dir.setNameFilters(filters);
    QStringList files = dir.entryList(QDir::Files,QDir::Name);
    QStringList dirs = dir.entryList(QDir::AllDirs,QDir::Name);
    for (int i = 0; i < dirs.size() && i < 256; i++) {
        QDir newDir(dir.absolutePath() + "/" + dirs[i]);
        newDir.setNameFilters(filters);
        QStringList newFiles = dir.entryList(QDir::Files,QDir::Name);
        QStringList newDirs = dir.entryList(QDir::AllDirs,QDir::Name);
        if (newFiles.size() > 0 ||  newDirs.size() > 0) {
            if (dirs[i] != "." && dirs[i] != ".." && dirs[i]!="SourceMaterials") {
                QMenu *menu = m->addMenu(dirs[i]);
                fillExampleSubmenu(newDir.absolutePath(), menu, depth + 1);
            }
        }
    }
    for (int i = 0; i < files.size() &&  i < 256; i++) {
        QAction *newAction = m->addAction(files[i],
                                          this, SLOT(openExample()));
        newAction->setData(dir.absoluteFilePath(files[i]));
    }
}


void CsoundQt::fillFileMenu()
{
    recentMenu->clear();
    for (int i = 0; i < recentFiles.size(); i++) {
        if (i < recentFiles.size() && recentFiles[i] != "") {
            QAction *a = recentMenu->addAction(recentFiles[i], this, SLOT(openFromAction()));
            a->setData(recentFiles[i]);
        }
    }
    templateMenu->clear();
    QString templatePath = m_options->templateDir;
    if (templatePath.isEmpty() || !QDir(templatePath).exists()) {
#ifdef Q_OS_WIN32
        templatePath = qApp->applicationDirPath() + "/templates/";
#endif
#ifdef Q_OS_MAC
        templatePath = qApp->applicationDirPath() + "/../templates/";
        qDebug() << templatePath;
#endif
#ifdef Q_OS_LINUX
        templatePath = qApp->applicationDirPath() + "/templates/";
        if (!QDir(templatePath).exists()) {
            templatePath = qApp->applicationDirPath() + "/../templates/";
        }
        if (!QDir(templatePath).exists()) { // for out of tree builds
            templatePath = qApp->applicationDirPath() + "/../../csoundqt/templates/";
        }
        if (!QDir(templatePath).exists()) { // for out of tree builds
            templatePath = qApp->applicationDirPath() + "/../../CsoundQt/templates/";
        }
        if (!QDir(templatePath).exists()) {
            templatePath = "/usr/share/csoundqt/templates/";
        }
#endif
#ifdef Q_OS_SOLARIS
        templatePath = qApp->applicationDirPath() + "/templates/";
        if (!QDir(templatePath).exists()) {
            templatePath = "/usr/share/qutecsound/templates/";
        }
        if (!QDir(templatePath).exists()) {
            templatePath = qApp->applicationDirPath() + "/../src/templates/";
        }
#endif
    }
    QStringList filters;
    filters << "*.csd"<<"*.html";
    QStringList templateFiles = QDir(templatePath).entryList(filters,QDir::Files);
    foreach (QString fileName, templateFiles) {
        QAction *newAction = templateMenu->addAction(fileName, this,
                                                     SLOT(openTemplate()));
        newAction->setData(templatePath + QDir::separator() + fileName);
    }
}

void CsoundQt::fillFavoriteMenu()
{
    favoriteMenu->clear();
    if (!m_options->favoriteDir.isEmpty()) {
        QDir dir(m_options->favoriteDir);
        fillFavoriteSubMenu(dir.absolutePath(), favoriteMenu, 0);
    }
    else {
        favoriteMenu->addAction(tr("Set the Favourites folder in the Configuration Window"),
                                this, SLOT(configure()));
    }
}

void CsoundQt::fillFavoriteSubMenu(QDir dir, QMenu *m, int depth)
{
    QStringList filters;
    filters << "*.csd" << "*.orc" << "*.sco" << "*.udo" << "*.inc" << "*.py";
    dir.setNameFilters(filters);
    QStringList files = dir.entryList(QDir::Files,QDir::Name);
    QStringList dirs = dir.entryList(QDir::AllDirs,QDir::Name);
    if (depth > m_options->menuDepth)
        return;
    for (int i = 0; i < dirs.size() && i < 64; i++) {
        QDir newDir(dir.absolutePath() + "/" + dirs[i]);
        newDir.setNameFilters(filters);
        QStringList newFiles = dir.entryList(QDir::Files,QDir::Name);
        QStringList newDirs = dir.entryList(QDir::AllDirs,QDir::Name);
        if (newFiles.size() > 0 ||  newDirs.size() > 0) {
            if (dirs[i] != "." && dirs[i] != "..") {
                QMenu *menu = m->addMenu(dirs[i]);
                fillFavoriteSubMenu(newDir.absolutePath(), menu, depth + 1);
            }
        }
    }
    for (int i = 0; i < files.size() &&  i < 64; i++) {
        QAction *newAction = m->addAction(files[i],
                                          this, SLOT(openFromAction()));
        newAction->setData(dir.absoluteFilePath(files[i]));
    }
}


void CsoundQt::fillEditScriptsSubMenu(QDir dir, QMenu *m, int depth)
{
    QStringList filters;
    filters << "*.py";
    dir.setNameFilters(filters);
    QStringList files = dir.entryList(QDir::Files,QDir::Name);
    QStringList dirs = dir.entryList(QDir::AllDirs,QDir::Name);
    if (depth > m_options->menuDepth)
        return;
    for (int i = 0; i < dirs.size() && i < 64; i++) {
        QDir newDir(dir.absolutePath() + "/" + dirs[i]);
        newDir.setNameFilters(filters);
        QStringList newFiles = dir.entryList(QDir::Files,QDir::Name);
        QStringList newDirs = dir.entryList(QDir::AllDirs,QDir::Name);
        if (newFiles.size() > 0 ||  newDirs.size() > 0) {
            if (dirs[i] != "." && dirs[i] != "..") {
                QMenu *menu = m->addMenu(dirs[i]);
                fillEditScriptsSubMenu(newDir.absolutePath(), menu, depth + 1);
            }
        }
    }
    for (int i = 0; i < files.size() &&  i < 64; i++) {
        QAction *newAction = m->addAction(files[i],
                                          this, SLOT(openFromAction()));
        newAction->setData(dir.absoluteFilePath(files[i]));
    }
}

//#include "flowlayout.h"

void CsoundQt::createToolBars()
{
    //	fileToolBar = addToolBar(tr("File"));
    //	fileToolBar->setObjectName("fileToolBar");
    //	fileToolBar->addAction(newAct);
    //	fileToolBar->addAction(openAct);
    //	fileToolBar->addAction(saveAct);

    //	editToolBar = addToolBar(tr("Edit"));
    //	editToolBar->setObjectName("editToolBar");
    //	editToolBar->addAction(undoAct);
    //	editToolBar->addAction(redoAct);
    //	editToolBar->addAction(cutAct);
    //	editToolBar->addAction(copyAct);
    //	editToolBar->addAction(pasteAct);

    //	QDockWidget *toolWidget = new QDockWidget(tr("Tools"), this);
    //	QWidget *w = new QWidget(toolWidget);
    //	FlowLayout * flowlayout = new FlowLayout(w);
    //	toolWidget->setWidget(w);
    //	w->setLayout(flowlayout);
    //	QVector<QAction *> actions;
    //	actions << runAct << pauseAct << stopAct << recAct << runTermAct << renderAct;
    //	foreach (QAction * act, actions) {
    //		QToolButton *b = new QToolButton(w);
    //		b->setDefaultAction(act);
    //		flowlayout->addWidget(b);
    //	}
    //	addDockWidget(Qt::AllDockWidgetAreas, toolWidget);
    //	toolWidget->show();
    const int iconSize = this->m_options->toolbarIconSize;
    controlToolBar = addToolBar(tr("Control"));
    controlToolBar->setObjectName("controlToolBar");
    controlToolBar->addAction(runAct);
    controlToolBar->addAction(pauseAct);
    controlToolBar->addAction(stopAct);
    controlToolBar->addAction(recAct);
    controlToolBar->addSeparator();
    controlToolBar->addAction(runTermAct);
    controlToolBar->addAction(renderAct);
    // controlToolBar->addAction(externalEditorAct);
    // controlToolBar->addAction(externalPlayerAct);
    controlToolBar->addAction(configureAct);
    controlToolBar->setIconSize(QSize(iconSize, iconSize));
    controlToolBar->setFloatable(false);

    configureToolBar = addToolBar(tr("Panels"));
    configureToolBar->setObjectName("panelToolBar");
#if defined(QCS_QTHTML)
    // configureToolBar->addAction(showHtml5Act);
#endif
    configureToolBar->addAction(showWidgetsAct);
    configureToolBar->addAction(showHelpAct);
    configureToolBar->addAction(showConsoleAct);
    configureToolBar->addAction(showInspectorAct);
    // configureToolBar->addAction(showLiveEventsAct);
    configureToolBar->addAction(showVirtualKeyboardAct);
    //configureToolBar->addAction(showTableEditorAct);

    // configureToolBar->addAction(showScratchPadAct);
    configureToolBar->addAction(showUtilitiesAct);
    configureToolBar->setFloatable(false);

    Qt::ToolButtonStyle toolButtonStyle = (m_options->iconText?
                                           Qt::ToolButtonTextUnderIcon: Qt::ToolButtonIconOnly);
    //	fileToolBar->setToolButtonStyle(toolButtonStyle);
    //	editToolBar->setToolButtonStyle(toolButtonStyle);
    controlToolBar->setToolButtonStyle(toolButtonStyle);
    configureToolBar->setToolButtonStyle(toolButtonStyle);
    configureToolBar->setIconSize(QSize(iconSize, iconSize));
    // test Mac
#ifdef Q_OS_MAC

    if (m_options->theme=="breeze-dark") {
        QColor textColor = QGuiApplication::palette().color(QPalette::Text);
        qDebug() << "Textcolor: " << textColor;
        QString styleString = QString("color: %1").arg(textColor.name());
        controlToolBar->setStyleSheet(styleString);
        configureToolBar->setStyleSheet(styleString);
    }
#endif
}

void CsoundQt::setToolbarIconSize(int size) {
    controlToolBar->setIconSize(QSize(size, size));
    controlToolBar->setStyleSheet("QToolBar { padding: 0 3px }");
    configureToolBar->setIconSize(QSize(size, size));
}

void CsoundQt::createStatusBar()
{
    auto statusbar = statusBar();
    statusbar->showMessage(tr("Ready"));

    lineAndColumnLabel = new QLabel();
    setLineAndColumn(0,0);
    statusbar->addPermanentWidget(lineAndColumnLabel);
}

void CsoundQt::readSettings()
{
    QSettings settings("csoundqt", "csoundqt");
	int settingsVersion = settings.value("settingsVersion", 4).toInt();
    // Version 1 to remove "-d" from additional command line flags
    // Version 2 to save default keyboard shortcuts (weren't saved previously)
    // Version 2 to add "*" to jack client name
    // version 4 to signal that many shotcuts have been changed
    if (settingsVersion>0 && settingsVersion<4) {
        QMessageBox::warning(this, tr("Settings changed"),
                             tr("In this version the shortcuts for showing panels changed. "
                                "See ... for more information. Please Use "
                                "'Edit/Keyboard shortcuts/Restore Defaults' to activate it."));
    }

    settings.beginGroup("GUI");
    m_options->theme = settings.value("theme", "breeze").toString();
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(600, 500)).toSize();
    resize(size); // does not work here for MacOS Mojave
    move(pos);
    if (settings.contains("dockstate")) {
        restoreState(settings.value("dockstate").toByteArray());
    }
    lastUsedDir = settings.value("lastuseddir", "").toString();
    lastFileDir = settings.value("lastfiledir", "").toString();
    //  showLiveEventsAct->setChecked(settings.value("liveEventsActive", true).toBool());
    m_options->language = m_configlists.languageCodes.indexOf(settings.value("language", QLocale::system().name()).toString());
    if (m_options->language < 0)
        m_options->language = 0;
    recentFiles.clear();
    recentFiles = settings.value("recentFiles").toStringList();
    setDefaultKeyboardShortcuts();
    QHash<QString, QVariant> actionList = settings.value("shortcuts").toHash();

    if (actionList.count() != 0) {
        QHashIterator<QString, QVariant> i(actionList);
        while (i.hasNext()) {
            i.next();
            QString shortcut = i.value().toString();
            foreach (QAction *act, m_keyActions) {
                if (act->text().remove("&") == i.key()) {
                    act->setShortcut(shortcut);
                    break;
                }
            }
        }
    }
    // else { // No shortcuts are stored
    //     setDefaultKeyboardShortcuts();
    // }
    settings.endGroup();
    settings.beginGroup("Options");
    settings.beginGroup("Editor");
#ifdef Q_OS_MACOS
    m_options->font = settings.value("font", "Menlo").toString();
    m_options->fontPointSize = settings.value("fontsize", 12).toDouble();
    m_options->consoleFont = settings.value("consolefont", "Menlo").toString();
    m_options->consoleFontPointSize = settings.value("consolefontsize", 10).toDouble();
#endif
#ifdef Q_OS_WIN
    m_options->font = settings.value("font", "Consolas").toString();
    m_options->fontPointSize = settings.value("fontsize", 11).toDouble();
    m_options->consoleFont = settings.value("consolefont", "Consolas").toString();
    m_options->consoleFontPointSize = settings.value("consolefontsize", 10).toDouble();
#endif
#ifdef Q_OS_LINUX
    m_options->font = settings.value("font", "Liberation Mono").toString();
    m_options->fontPointSize = settings.value("fontsize", 11).toDouble();
    m_options->consoleFont = settings.value("consolefont", "Liberation Mono").toString();
    m_options->consoleFontPointSize = settings.value("consolefontsize", 10).toDouble();
#endif
    m_options->showLineNumberArea = settings.value("showLineNumberArea", true).toBool();

#ifdef Q_OS_WIN
    m_options->lineEnding = settings.value("lineEnding", 1).toInt();
#else
    m_options->lineEnding = settings.value("lineEnding", 0).toInt();
#endif

    //m_options->consoleFont = settings.value("consolefont", "Courier").toString();  // platform specific
    //m_options->consoleFontPointSize = settings.value("consolefontsize", 10).toDouble();

    m_options->consoleFontColor = settings.value("consoleFontColor",
                                                 QVariant(QColor(Qt::black))).value<QColor>();
    m_options->consoleBgColor = settings.value("consoleBgColor",
                                               QVariant(QColor(Qt::white))).value<QColor>();
    // m_options->editorBgColor = settings.value("editorBgColor",
    //                                           QVariant(QColor(Qt::white))).value<QColor>();

    m_options->tabWidth = settings.value("tabWidth", 24).toInt();
    m_options->tabIndents = settings.value("tabIndents", false).toBool();
    // m_options->colorVariables = settings.value("colorvariables", true).toBool();
    m_options->highlightingTheme = settings.value("highlightingTheme", "light").toString();
    m_options->colorVariables = m_options->highlightingTheme != "none";
    m_options->autoPlay = settings.value("autoplay", false).toBool();
    m_options->autoJoin = settings.value("autoJoin", true).toBool();
    m_options->menuDepth = settings.value("menuDepth", 3).toInt();
    m_options->saveChanges = settings.value("savechanges", true).toBool();
    m_options->askIfTemporary = settings.value("askIfTemporary", false).toBool();
    m_options->rememberFile = settings.value("rememberfile", true).toBool();
    m_options->saveWidgets = settings.value("savewidgets", true).toBool();
	m_options->midiCcToCurrentPageOnly = settings.value("midiCcToActivePageOnly", false).toBool();
    m_options->widgetsIndependent = settings.value("widgetsIndependent", false).toBool();
    m_options->iconText = settings.value("iconText", false).toBool();
    m_options->showToolbar = settings.value("showToolbar", true).toBool();
    m_options->lockToolbar = settings.value("lockToolbar", true).toBool();
    m_options->toolbarIconSize = settings.value("toolbarIconSize", 20).toInt();
    m_options->wrapLines = settings.value("wrapLines", true).toBool();
    m_options->autoComplete = settings.value("autoComplete", true).toBool();
    m_options->autoCompleteDelay = settings.value("autoCompleteDelay", 150).toInt();
    m_options->autoParameterMode = settings.value("autoParameterMode", true).toBool();
    m_options->enableWidgets = settings.value("enableWidgets", true).toBool();
    m_options->showWidgetsOnRun = settings.value("showWidgetsOnRun", true).toBool();
    m_options->showTooltips = settings.value("showTooltips", true).toBool();
    m_options->enableFLTK = settings.value("enableFLTK", true).toBool();
    m_options->terminalFLTK = settings.value("terminalFLTK", true).toBool();
    m_options->oldFormat = settings.value("oldFormat", false).toBool();
    m_options->openProperties = settings.value("openProperties", true).toBool();
    m_options->fontOffset = settings.value("fontOffset", 0.0).toDouble();
    m_options->fontScaling = settings.value("fontScaling", 1.0).toDouble();
    m_options->graphUpdateRate = settings.value("graphUpdateRate", 30).toInt();
    lastFiles = settings.value("lastfiles", QStringList()).toStringList();
    lastTabIndex = settings.value("lasttabindex", "").toInt();
    m_options->debugPort = settings.value("debugPort",34711).toInt();
    m_options->tabShortcutActive = settings.value("tabShortcutActive", true).toBool();
    m_options->highlightScore = settings.value("highlightScore", false).toBool();


    settings.endGroup();
    settings.beginGroup("Run");
    m_options->useAPI = settings.value("useAPI", true).toBool();
    m_options->keyRepeat = settings.value("keyRepeat", false).toBool();
    m_options->debugLiveEvents = settings.value("debugLiveEvents", false).toBool();
    m_options->consoleBufferSize = settings.value("consoleBufferSize", 1024).toInt();
    m_options->checkSyntaxBeforeRun = settings.value("checkSyntaxBeforeRun", false).toBool();
    m_options->midiInterface = settings.value("midiInterface", 9999).toInt();
    m_options->midiInterfaceName = settings.value("midiInterfaceName", "None").toString();
    m_options->midiOutInterface = settings.value("midiOutInterface", 9999).toInt();
    m_options->midiOutInterfaceName = settings.value("midiOutInterfaceName", "None").toString();
    m_options->noBuffer = settings.value("noBuffer", false).toBool();
    m_options->noPython = settings.value("noPython", false).toBool();
    m_options->noMessages = settings.value("noMessages", false).toBool();
    m_options->noEvents = settings.value("noEvents", false).toBool();


    //experimental: enable setting internal RtMidi API in settings file. See RtMidi.h
#ifdef QCS_RTMIDI
    QString rtMidiApiString = settings.value("rtMidiApi","UNSPECIFIED").toString();
    if (rtMidiApiString.toUpper()=="LINUX_ALSA" )
        m_options->rtMidiApi = RtMidi::LINUX_ALSA;
    else if (rtMidiApiString.toUpper()=="UNIX_JACK" )
                m_options->rtMidiApi = RtMidi::UNIX_JACK;
    else if (rtMidiApiString.toUpper()=="MACOSX_CORE" )
                m_options->rtMidiApi = RtMidi::MACOSX_CORE;
    else if (rtMidiApiString.toUpper()=="WINDOWS_MM" )
        m_options->rtMidiApi = RtMidi::WINDOWS_MM;
    else
        m_options->rtMidiApi = RtMidi::UNSPECIFIED;
#endif

    m_options->bufferSize = settings.value("bufferSize", 1024).toInt();
    m_options->bufferSizeActive = settings.value("bufferSizeActive", false).toBool();
    m_options->HwBufferSize = settings.value("HwBufferSize", 1024).toInt();
    m_options->HwBufferSizeActive = settings.value("HwBufferSizeActive", false).toBool();
    m_options->dither = settings.value("dither", false).toBool();
    m_options->newParser = settings.value("newParser", false).toBool();
    m_options->multicore = settings.value("multicore", false).toBool();
    m_options->numThreads = settings.value("numThreads", 1).toInt();
    m_options->additionalFlags = settings.value("additionalFlags", "").toString();
    m_options->useSystemSamplerate = settings.value("useSystemSamplerate", false).toBool();
    m_options->samplerate = settings.value("overrideSamplerate", 0).toInt();
    m_options->overrideNumChannels = settings.value("overrideNumChannels", false).toBool();
    m_options->numChannels = settings.value("numChannels", 2).toInt();
    m_options->numInputChannels = settings.value("numInputChannels", 2).toInt();

    m_options->useLimiter = settings.value("useLimiter", csoundGetVersion()>=6160).toBool();
    m_options->limitValue = settings.value("limitValue", 1.0).toDouble();

    m_options->realtimeFlag = settings.value("realtimeFlag", false).toBool();
    m_options->sampleAccurateFlag = settings.value("sampleAccurateFlag", false).toBool();
    if (settingsVersion < 1)
        m_options->additionalFlags.remove("-d");  // remove old -d preference, as it is fixed now.
    m_options->additionalFlagsActive = settings.value("additionalFlagsActive", false).toBool();
    m_options->fileUseOptions = settings.value("fileUseOptions", true).toBool();
    m_options->fileOverrideOptions = settings.value("fileOverrideOptions", false).toBool();
    m_options->fileAskFilename = settings.value("fileAskFilename", false).toBool();
    m_options->filePlayFinished = settings.value("filePlayFinished", false).toBool();
    m_options->fileFileType = settings.value("fileFileType", 0).toInt();
    m_options->fileSampleFormat = settings.value("fileSampleFormat", 1).toInt();
    m_options->fileInputFilenameActive = settings.value("fileInputFilenameActive", false).toBool();
    m_options->fileInputFilename = settings.value("fileInputFilename", "").toString();
    m_options->fileOutputFilenameActive = settings.value("fileOutputFilenameActive", false).toBool();
    m_options->fileOutputFilename = settings.value("fileOutputFilename", "").toString();
    m_options->rtUseOptions = settings.value("rtUseOptions", true).toBool();
    m_options->rtOverrideOptions = settings.value("rtOverrideOptions", false).toBool();
    m_options->rtAudioModule = settings.value("rtAudioModule", "pa_cb").toString();
    if (m_options->rtAudioModule.isEmpty()) { m_options->rtAudioModule = "pa_cb"; }
    m_options->rtInputDevice = settings.value("rtInputDevice", "adc").toString();
    m_options->rtOutputDevice = settings.value("rtOutputDevice", "dac").toString();
    m_options->rtJackName = settings.value("rtJackName", "").toString();
    if (settingsVersion < 2) {
        if (!m_options->rtJackName.endsWith("*"))
            m_options->rtJackName.append("*");
    }
    m_options->rtMidiModule = settings.value("rtMidiModule", "portmidi").toString();
    if (m_options->rtMidiModule.isEmpty()) { m_options->rtMidiModule = "portmidi"; }
    m_options->rtMidiInputDevice = settings.value("rtMidiInputDevice", "0").toString();
    m_options->rtMidiOutputDevice = settings.value("rtMidiOutputDevice", "").toString();
    m_options->useCsoundMidi = settings.value("useCsoundMidi", false).toBool();
    m_options->simultaneousRun = settings.value("simultaneousRun", "").toBool();
    m_options->sampleFormat = settings.value("sampleFormat", 0).toInt();
    settings.endGroup();
    settings.beginGroup("Environment");
#ifdef Q_OS_MAC
    m_options->csdocdir = settings.value("csdocdir", DEFAULT_HTML_DIR).toString();
#else
    m_options->csdocdir = settings.value("csdocdir", "").toString();
#endif
    m_options->opcode7dir64 = settings.value("opcode7dir64","").toString();
    m_options->opcode7dir64Active = settings.value("opcode7dir64Active",false).toBool();
    m_options->sadir = settings.value("sadir","").toString();
    m_options->sadirActive = settings.value("sadirActive","").toBool();
    m_options->ssdir = settings.value("ssdir","").toString();
    m_options->ssdirActive = settings.value("ssdirActive","").toBool();
    m_options->sfdir = settings.value("sfdir","").toString();
    m_options->sfdirActive = settings.value("sfdirActive","").toBool();
    m_options->incdir = settings.value("incdir","").toString();
    m_options->incdirActive = settings.value("incdirActive","").toBool();
    m_options->rawWave = settings.value("rawWave","").toString();
    m_options->rawWaveActive = settings.value("rawWaveActive","").toBool();
    m_options->defaultCsd = settings.value("defaultCsd","").toString();
    m_options->defaultCsdActive = settings.value("defaultCsdActive","").toBool();
    m_options->favoriteDir = settings.value("favoriteDir","").toString();
    m_options->examplePath = settings.value("examplePath", "").toString();
    m_options->pythonDir = settings.value("pythonDir","").toString();
    m_options->pythonExecutable = settings.value("pythonExecutable","python").toString();
    m_options->csoundExecutable = settings.value("csoundExecutable","csound").toString();
    m_options->logFile = settings.value("logFile",DEFAULT_LOG_FILE).toString();
    m_options->sdkDir = settings.value("sdkDir","").toString();
    m_options->templateDir = settings.value("templateDir","").toString();
    m_options->opcodexmldir = settings.value("opcodexmldir", "").toString();
    m_options->opcodexmldirActive = settings.value("opcodexmldirActive","").toBool();
    settings.endGroup();
    settings.beginGroup("External");
    m_options->terminal = settings.value("terminal", DEFAULT_TERM_EXECUTABLE).toString();
    m_options->browser = settings.value("browser", DEFAULT_BROWSER_EXECUTABLE).toString();
    m_options->dot = settings.value("dot", DEFAULT_DOT_EXECUTABLE).toString();
    m_options->waveeditor = settings.value("waveeditor",
                                           DEFAULT_WAVEEDITOR_EXECUTABLE
                                           ).toString();
    m_options->waveplayer = settings.value("waveplayer",
                                           DEFAULT_WAVEPLAYER_EXECUTABLE
                                           ).toString();
    m_options->pdfviewer = settings.value("pdfviewer",
                                          DEFAULT_PDFVIEWER_EXECUTABLE
                                          ).toString();
#ifdef Q_OS_MACOS
    if (QOperatingSystemVersion::current() >  QOperatingSystemVersion::MacOSMojave )  { // fix path change to /System/Applications on MacOS Catalina

        QList <QString *> apps;
        apps << &m_options->terminal << &m_options->browser << &m_options->waveplayer << &m_options->pdfviewer;
        // not sure about  &m_options->waveeditor (Audacity) -  where is this installed on Catalina
        // no correctrion for waveeditor now

        foreach (QString * app, apps) {
            if (app->startsWith("/Applications")) {
                app->prepend("/System");
            }
        }

        if (m_options->browser.contains("Safary")) { // fix for typo in 0.9.8.1
            m_options->browser.replace("Safary", "Safari");
        }
    }
#endif
    settings.endGroup();
    settings.beginGroup("Template");
    m_options->csdTemplate = settings.value("csdTemplate", QCS_DEFAULT_TEMPLATE).toString();
    settings.endGroup();
    settings.endGroup();
    if (settingsVersion < 3 && settingsVersion > 0) {
        showNewFormatWarning();
        m_options->csdTemplate = QCS_DEFAULT_TEMPLATE;
    }
}

void CsoundQt::storeSettings()
{
    qDebug();
    QStringList files;
    if (m_options->rememberFile) {
        for (int i = 0; i < documentPages.size(); i++ ) {
            files.append(documentPages[i]->getFileName());
        }
    }
    // sometimes settings are stored in startup when there is no pages open
    if (documentPages.size() > 0) {
        writeSettings(files, documentTabs->currentIndex());
    } else {
        qDebug() << "No files open. Will not store settings (for any case - testing)";
    }
}

void CsoundQt::writeSettings(QStringList openFiles, int lastIndex)
{
    QSettings settings("csoundqt", "csoundqt");
    QDEBUG << "writing settings to csound/qutecsound";

    settings.setValue("settingsVersion", 4);

    QHash<QString, QVariant> shortcuts;
    foreach (QAction *act, m_keyActions) {
        shortcuts[act->text().remove("&")] = QVariant(act->shortcut().toString());
    }

    if(m_resetPrefs) {
        settings.beginGroup("GUI");
        settings.setValue("shortcuts", QVariant(shortcuts));
        settings.endGroup();
        settings.beginGroup("Options");
        for(auto groupName: {"Editor", "Run", "Environment", "External"}) {
            settings.beginGroup(groupName);
            settings.remove("");  // removes all settings in the section
            settings.endGroup();
        }
        settings.beginGroup("Template");
        settings.setValue("csdTemplate", m_options->csdTemplate);
        settings.endGroup();
        settings.endGroup();; // options
        m_resetPrefs = false;

    } else {
        settings.beginGroup("GUI");
        settings.setValue("shortcuts", QVariant(shortcuts));
        settings.setValue("pos", pos());
        settings.setValue("size", size());
        settings.setValue("dockstate", saveState());
        settings.setValue("lastuseddir", lastUsedDir);
        settings.setValue("lastfiledir", lastFileDir);
        settings.setValue("language", m_configlists.languageCodes[m_options->language]);
        //  settings.setValue("liveEventsActive", showLiveEventsAct->isChecked());
        settings.setValue("recentFiles", recentFiles);
        settings.setValue("theme", m_options->theme);
        settings.setValue("windowState", saveState());
        settings.setValue("windowGeometry", saveGeometry());
        settings.endGroup();

        settings.beginGroup("Options");
        settings.beginGroup("Editor");
        settings.setValue("font", m_options->font );
        settings.setValue("fontsize", m_options->fontPointSize);
        settings.setValue("showLineNumberArea", m_options->showLineNumberArea);
        settings.setValue("lineEnding", m_options->lineEnding);
        settings.setValue("consolefont", m_options->consoleFont );
        settings.setValue("consolefontsize", m_options->consoleFontPointSize);
        settings.setValue("consoleFontColor", QVariant(m_options->consoleFontColor));
        settings.setValue("consoleBgColor", QVariant(m_options->consoleBgColor));
        // settings.setValue("editorBgColor", QVariant(m_options->editorBgColor));
        settings.setValue("tabWidth", m_options->tabWidth );
        settings.setValue("tabIndents", m_options->tabIndents);
        settings.setValue("colorvariables", m_options->colorVariables);
        settings.setValue("highlightingTheme", m_options->highlightingTheme);
        settings.setValue("autoplay", m_options->autoPlay);
        settings.setValue("autoJoin", m_options->autoJoin);
        settings.setValue("menuDepth", m_options->menuDepth);
        settings.setValue("savechanges", m_options->saveChanges);
        settings.setValue("askIfTemporary", m_options->askIfTemporary);
        settings.setValue("rememberfile", m_options->rememberFile);
        settings.setValue("savewidgets", m_options->saveWidgets);
        settings.setValue("midiCcToActivePageOnly", m_options->midiCcToCurrentPageOnly);
        settings.setValue("widgetsIndependent", m_options->widgetsIndependent);
        settings.setValue("iconText", m_options->iconText);
        settings.setValue("showToolbar", m_options->showToolbar);
        settings.setValue("lockToolbar", m_options->lockToolbar);
        settings.setValue("toolbarIconSize", m_options->toolbarIconSize);
        settings.setValue("wrapLines", m_options->wrapLines);
        settings.setValue("autoComplete", m_options->autoComplete);
        settings.setValue("autoCompleteDelay", m_options->autoCompleteDelay);
        settings.setValue("autoParameterMode", m_options->autoParameterMode);
        settings.setValue("enableWidgets", m_options->enableWidgets);
        settings.setValue("showWidgetsOnRun", m_options->showWidgetsOnRun);
        settings.setValue("graphUpdateRate", m_options->graphUpdateRate);
        settings.setValue("showTooltips", m_options->showTooltips);
        settings.setValue("enableFLTK", m_options->enableFLTK);
        settings.setValue("terminalFLTK", m_options->terminalFLTK);
        settings.setValue("oldFormat", m_options->oldFormat);
        settings.setValue("openProperties", m_options->openProperties);
        settings.setValue("fontOffset", m_options->fontOffset);
        settings.setValue("fontScaling", m_options->fontScaling);
        settings.setValue("debugPort", m_options->debugPort);
        settings.setValue("tabShortcutActive", m_options->tabShortcutActive);
        settings.setValue("highlightScore", m_options->highlightScore);

        settings.setValue("lastfiles", openFiles);
        settings.setValue("lasttabindex", lastIndex);
        // if(openFiles.size() > 0 && lastIndex != 0) {
        if(openFiles.size() > 0) {
             settings.setValue("lastfiles", openFiles);
             if(lastIndex > 0) {
                settings.setValue("lasttabindex", lastIndex);
             }
        }
        settings.endGroup(); // Options/Editor

        settings.beginGroup("Run");
        settings.setValue("useAPI", m_options->useAPI);
        settings.setValue("keyRepeat", m_options->keyRepeat);
        settings.setValue("debugLiveEvents", m_options->debugLiveEvents);
        settings.setValue("consoleBufferSize", m_options->consoleBufferSize);
        settings.setValue("midiInterface", m_options->midiInterface);
        settings.setValue("midiInterfaceName", m_options->midiInterfaceName);
        settings.setValue("midiOutInterface", m_options->midiOutInterface);
        settings.setValue("midiOutInterfaceName", m_options->midiOutInterfaceName);
        settings.setValue("noBuffer", m_options->noBuffer);
        settings.setValue("noPython", m_options->noPython);
        settings.setValue("noMessages", m_options->noMessages);
        settings.setValue("noEvents", m_options->noEvents);
        settings.setValue("bufferSize", m_options->bufferSize);
        settings.setValue("bufferSizeActive", m_options->bufferSizeActive);
        settings.setValue("HwBufferSize",m_options->HwBufferSize);
        settings.setValue("HwBufferSizeActive", m_options->HwBufferSizeActive);
        settings.setValue("dither", m_options->dither);
        settings.setValue("realtimeFlag", m_options->realtimeFlag);
        settings.setValue("sampleAccurateFlag", m_options->sampleAccurateFlag);
        settings.setValue("newParser", m_options->newParser);
        settings.setValue("multicore", m_options->multicore);
        settings.setValue("numThreads", m_options->numThreads);
        settings.setValue("useSystemSamplerate", m_options->useSystemSamplerate);
        settings.setValue("overrideSamplerate", m_options->samplerate);
        settings.setValue("overrideNumChannels", m_options->overrideNumChannels);
        settings.setValue("numChannels", m_options->numChannels);
        settings.setValue("numInputChannels", m_options->numInputChannels);
        settings.setValue("useLimiter", m_options->useLimiter);
        settings.setValue("limitValue", m_options->limitValue);
        settings.setValue("additionalFlags", m_options->additionalFlags);
        settings.setValue("additionalFlagsActive", m_options->additionalFlagsActive);
        settings.setValue("fileUseOptions", m_options->fileUseOptions);
        settings.setValue("fileOverrideOptions", m_options->fileOverrideOptions);
        settings.setValue("fileAskFilename", m_options->fileAskFilename);
        settings.setValue("filePlayFinished", m_options->filePlayFinished);
        settings.setValue("fileFileType", m_options->fileFileType);
        settings.setValue("fileSampleFormat", m_options->fileSampleFormat);
        settings.setValue("fileInputFilenameActive", m_options->fileInputFilenameActive);
        settings.setValue("fileInputFilename", m_options->fileInputFilename);
        settings.setValue("fileOutputFilenameActive", m_options->fileOutputFilenameActive);
        settings.setValue("fileOutputFilename", m_options->fileOutputFilename);
        settings.setValue("rtUseOptions", m_options->rtUseOptions);
        settings.setValue("rtOverrideOptions", m_options->rtOverrideOptions);
        settings.setValue("rtAudioModule", m_options->rtAudioModule);
        settings.setValue("rtInputDevice", m_options->rtInputDevice);
        settings.setValue("rtOutputDevice", m_options->rtOutputDevice);
        settings.setValue("rtJackName", m_options->rtJackName);
        settings.setValue("rtMidiModule", m_options->rtMidiModule);
        settings.setValue("rtMidiInputDevice", m_options->rtMidiInputDevice);
        settings.setValue("rtMidiOutputDevice", m_options->rtMidiOutputDevice);
        settings.setValue("useCsoundMidi", m_options->useCsoundMidi);
        settings.setValue("simultaneousRun", m_options->simultaneousRun);
        settings.setValue("sampleFormat", m_options->sampleFormat);
        settings.setValue("checkSyntaxBeforeRun", m_options->checkSyntaxBeforeRun);
        settings.endGroup(); // Options/Run

        settings.beginGroup("Environment");
        settings.setValue("csdocdir", m_options->csdocdir);
        settings.setValue("opcode7dir64",m_options->opcode7dir64);
        settings.setValue("opcode7dir64Active",m_options->opcode7dir64Active);
        settings.setValue("sadir",m_options->sadir);
        settings.setValue("sadirActive",m_options->sadirActive);
        settings.setValue("ssdir",m_options->ssdir);
        settings.setValue("ssdirActive",m_options->ssdirActive);
        settings.setValue("sfdir",m_options->sfdir);
        settings.setValue("sfdirActive",m_options->sfdirActive);
        settings.setValue("incdir",m_options->incdir);
        settings.setValue("incdirActive",m_options->incdirActive);
        settings.setValue("rawWave",m_options->rawWave);
        settings.setValue("rawWaveActive",m_options->rawWaveActive);
        settings.setValue("defaultCsd",m_options->defaultCsd);
        settings.setValue("defaultCsdActive",m_options->defaultCsdActive);
        settings.setValue("favoriteDir",m_options->favoriteDir);
        settings.setValue("examplePath", m_options->examplePath);
        settings.setValue("pythonDir",m_options->pythonDir);
        settings.setValue("pythonExecutable",m_options->pythonExecutable);
        settings.setValue("csoundExecutable", m_options->csoundExecutable);
        settings.setValue("logFile",m_options->logFile);
        settings.setValue("sdkDir",m_options->sdkDir);
        settings.setValue("templateDir",m_options->templateDir);
        settings.setValue("opcodexmldir", m_options->opcodexmldir); // TEST can be that this is the reason for crash MacOS -  dir set wrong from corrupted plist file
        settings.setValue("opcodexmldirActive",m_options->opcodexmldirActive);
        settings.endGroup(); // options/environment

        settings.beginGroup("External");
        settings.setValue("terminal", m_options->terminal);
        settings.setValue("browser", m_options->browser);
        settings.setValue("dot", m_options->dot);
        settings.setValue("waveeditor", m_options->waveeditor);
        settings.setValue("waveplayer", m_options->waveplayer);
        settings.setValue("pdfviewer", m_options->pdfviewer);
        settings.endGroup(); // options/external

        settings.beginGroup("Template");
        settings.setValue("csdTemplate", m_options->csdTemplate);
        settings.endGroup(); // options/template

        settings.endGroup(); // options

    }
    settings.sync();
    return;

}

void CsoundQt::clearSettings()
{
    QSettings settings("csoundqt", "csoundqt");
    settings.remove("");
    settings.beginGroup("GUI");
    settings.beginGroup("Shortcuts");
    settings.remove("");
    settings.endGroup();
    settings.endGroup();
    settings.beginGroup("Options");
    settings.remove("");
    settings.beginGroup("Editor");
    settings.remove("");
    settings.endGroup();
    settings.beginGroup("Run");
    settings.remove("");
    settings.endGroup();
    settings.beginGroup("Environment");
    settings.remove("");
    settings.endGroup();
    settings.beginGroup("External");
    settings.remove("");
    settings.endGroup();
    settings.endGroup();

    settings.sync();
}

int CsoundQt::startProcess(QString executable, const QStringList &args) {
    QString path = documentPages[curPage]->getFilePath();
    if (path.startsWith(":/examples/", Qt::CaseInsensitive)) {
        // example or other embedded file
        path = QDir::tempPath();   // copy of example is saved there
    }
    auto p = new QProcess();
    p->setProgram(executable);
    p->setArguments(args);
    p->setWorkingDirectory(path);
    p->start();
    qDebug() << "Launched external program with id:" << p->processId() << ", executable: " << executable << ", args: " << args;
    return !p->waitForStarted() ? 1 : 0;
}

int CsoundQt::execute(QString executable, QString options)
{
    int ret = 0;

#ifdef Q_OS_MAC
    QString commandLine = "open -a \"" + executable + "\" " + options;
#endif
#ifdef Q_OS_LINUX
    // QString commandLine = "\"" + executable + "\" " + options;
    QString commandLine = executable + " " + options;
#endif
#ifdef Q_OS_HAIKU
    QString commandLine = "\"" + executable + "\" " + options;
#endif
#ifdef Q_OS_SOLARIS
    QString commandLine = "\"" + executable + "\" " + options;
#endif
#ifdef Q_OS_WIN32
    QString commandLine = "\"" + executable + "\" " + (executable.startsWith("cmd")? " /k " : " ") + options;
    auto command_line = commandLine.toUtf8();
    qDebug() << "command_line: " << command_line;
    std::thread system_thread([command_line]{std::system(command_line);});
    system_thread.detach();
#else
    QString path = documentPages[curPage]->getFilePath();
    if (path.startsWith(":/examples/", Qt::CaseInsensitive)) {
        // example or other embedded file
        path = QDir::tempPath();   // copy of example is saved there
    }
    qDebug() << "CsoundQt::execute " << commandLine;
    auto p = new QProcess();
    p->setProgram(executable);
    auto args = options.split(" ");
    p->setArguments(args);
    p->setWorkingDirectory(path);
    p->start();
    qDebug() << "Launched external program with id:" << p->processId();
    ret = !p->waitForStarted() ? 1 : 0;
#endif
    return ret;
}

int CsoundQt::loadFileFromSystem(QString fileName)
{
    return loadFile(fileName,m_options->autoPlay);
}

void changeNewLines(QByteArray &line) {
    while(line.contains("\r\n")) {
        line.replace("\r\n", "\n");
    }
    while (line.contains("\r")) {
        line.replace("\r", "\n");
    }
}

void fillCompanionSco(QString &fileName, QString &text) {
    auto companionFileName = fileName.replace(".orc", ".sco");
    if (!QFile::exists(companionFileName))
        return;
    text.prepend("<CsoundSynthesizer>\n<CsOptions>\n</CsOptions>\n<CsInstruments>\n");
    text.append("\n</CsInstruments>\n<CsScore>\n");
    QFile companionFile(companionFileName);
    if (!companionFile.open(QFile::ReadOnly))
        return;
    while (!companionFile.atEnd()) {
        QByteArray line = companionFile.readLine();
        changeNewLines(line);
        //QTextDecoder decoder(QTextCodec::codecForLocale());
        QStringDecoder decoder(QStringDecoder::System ); // TODO: TEST THIS ON WINDOWS!!!

        text = text + decoder(line);
        if (!line.endsWith("\n"))
            text.append("\n");
    }
    text.append("</CsScore>\n</CsoundSynthesizer>\n");
}

void fillCompanionOrc(QString &fileName, QString &text) {
    auto companionFileName = fileName.replace(".sco", ".orc");
    if (!QFile::exists(companionFileName))
        return;
    QFile companionFlle(companionFileName);
    QString orcText = "";
    if (!companionFlle.open(QFile::ReadOnly))
        return;
    while (!companionFlle.atEnd()) {
        QByteArray line = companionFlle.readLine();
        changeNewLines(line);
        //QTextDecoder decoder(QTextCodec::codecForLocale());
        QStringDecoder decoder(QStringDecoder::System); // TODO: TEST ON WINDOWS!
        orcText = orcText + decoder(line);
        if (!line.endsWith("\n"))
            orcText += "\n";
    }
    text.prepend("\n</CsInstruments>\n<CsScore>\n");
    text.prepend(orcText);
    text.prepend("<CsoundSynthesizer>\n<CsOptions>\n</CsOptions>\n<CsInstruments>\n");
    text += "</CsScore>\n</CsoundSynthesizer>\n";
}

int CsoundQt::loadFile(QString fileName, bool runNow)
{
    if (fileName.endsWith(".pdf")) {
        openPdfFile(fileName);
        return 0;
    }
    int index = isOpen(fileName);
    if (index != -1) {
        QDEBUG << "File " << fileName << "already open, switching to index " << index;
        documentTabs->setCurrentIndex(index);
        changePage(index);
        return index;
    }
    // QDEBUG << "loading file" << fileName;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("CsoundQt"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return -1;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString text;
    QStringList lines;
    bool inEncFile = false;
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line.contains("<CsFileB ")) {
            inEncFile = true;
        }
        if (!inEncFile) {
            changeNewLines(line);
            //QTextDecoder decoder(QTextCodec::codecForLocale());
            QStringDecoder decoder(QStringDecoder::System);
            // text = text + decoder.toUnicode(line);
            if (!line.endsWith("\n"))
                text += "\n";
            lines << text + decoder(line); // TEST ON WINDOWS!!!
        }
        else {
            // text += line;
            lines << line;
            if (line.contains("</CsFileB>" && !line.contains("<CsFileB " )) ) {
                inEncFile = false;
            }
        }
    }
    text = lines.join("");
    if (m_options->autoJoin) {
        // QString companionFileName = fileName;
        if (fileName.endsWith(".orc"))
            fillCompanionSco(fileName, text);
        else if (fileName.endsWith(".sco"))
            fillCompanionOrc(fileName, text);
    }

    if (fileName == ":/default.csd") {
        fileName = QString("");
    }

    if (!makeNewPage(fileName, text)) {
        QApplication::restoreOverrideCursor();
        return -1;
    }

    if (!m_options->autoJoin &&
            (fileName.endsWith(".sco") ||
             fileName.endsWith(".orc")) ) {
        // load companion, when the new page is made, otherwise isOpen works uncorrect
        loadCompanionFile(fileName);
    }

    if (m_options->autoJoin &&
            (fileName.endsWith(".sco") || fileName.endsWith(".orc"))) {
        fileName.chop(4);
        fileName +=  ".csd";
        documentPages[curPage]->setFileName(fileName);  // Must update internal name
        setCurrentFile(fileName);
        if (!QFile::exists(fileName))
            save();
        else
            saveAs();
    } else {
        documentPages[curPage]->setModified(false);
        setWindowModified(false);
        documentTabs->setTabIcon(curPage, QIcon());
    }

    if (fileName.startsWith(m_options->csdocdir) && !m_options->csdocdir.isEmpty()) {
        documentPages[curPage]->readOnly = true;
    }
    QApplication::restoreOverrideCursor();

    // FIXME put back
    //  widgetPanel->clearHistory();
    if (runNow) {
        play();
    }
    //QDEBUG << "loadFile" << fileName << "finished, curPage:" << curPage;
    return curPage;
}

bool CsoundQt::makeNewPage(QString fileName, QString text)
{
    if (documentPages.size() >= MAX_THREAD_COUNT) {
        QMessageBox::warning(this, tr("Document number limit"),
                             tr("Please close a document before opening another."));
        return false;
    }
    auto t0 = std::chrono::high_resolution_clock::now();
    DocumentPage *newPage = new DocumentPage(this, m_opcodeTree, &m_configlists, m_midiLearn);
    int insertPoint = curPage + 1;
    curPage += 1;
    if (documentPages.size() == 0) {
        insertPoint = 0;
        curPage = 0;
    }
    documentPages.insert(insertPoint, newPage);
    //  documentPages[curPage]->setOpcodeNameList(m_opcodeTree->opcodeNameList());
    setCurrentOptionsForPage(documentPages[curPage]);

    // Must set before sending text to set highlighting mode
    documentPages[curPage]->setFileName(fileName);

    connectActions();
    //	connect(documentPages[curPage], SIGNAL(currentTextUpdated()), this, SLOT(markInspectorUpdate()));
    //	connect(documentPages[curPage], SIGNAL(modified()), this, SLOT(documentWasModified()));
    //	//  connect(documentPages[curPage], SIGNAL(setWidgetClipboardSignal(QString)),
    //	//          this, SLOT(setWidgetClipboard(QString)));
    //	connect(documentPages[curPage], SIGNAL(setCurrentAudioFile(QString)),
    //			this, SLOT(setCurrentAudioFile(QString)));
    //	connect(documentPages[curPage], SIGNAL(evaluatePythonSignal(QString)),
    //			this, SLOT(evaluatePython(QString)));
    auto page = documentPages[curPage];
    page->loadTextString(text);
    if (m_options->widgetsIndependent) {
        //		documentPages[curPage]->setWidgetLayoutOuterGeometry(documentPages[curPage]->getWidgetLayoutOuterGeometry());
        page->getWidgetLayout()->setGeometry(page->getWidgetLayoutOuterGeometry());
    }
    //	setWidgetPanelGeometry();

    if (!fileName.startsWith(":/")) {  // Don't store internal examples in recents menu
        lastUsedDir = fileName;
        lastUsedDir.resize(fileName.lastIndexOf(QRegularExpression("[/]")) + 1);
    }
    if (recentFiles.count(fileName) == 0 && fileName!="" && !fileName.startsWith(":/")) {
        recentFiles.prepend(fileName);
        if (recentFiles.size() > QCS_MAX_RECENT_FILES)
            recentFiles.removeLast();
        fillFileMenu();
    }
    documentTabs->insertTab(curPage, page->getView(),"");
    documentTabs->setCurrentIndex(curPage);

    midiHandler->addListener(page);
    page->getEngine()->setMidiHandler(midiHandler);

    setCurrentOptionsForPage(page); // Redundant but does the trick of setting the font properly now that stylesheets are being used...
    auto t1 = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(t1-t0).count();
    QDEBUG << "makeNewPage finished in (ms)" << elapsed;
    return true;
}

bool CsoundQt::loadCompanionFile(const QString &fileName)
{
    QString companionFileName = fileName;
    if (fileName.endsWith(".orc")) {
        companionFileName.replace(".orc", ".sco");
    }
    else if (fileName.endsWith(".sco")) {
        companionFileName.replace(".sco", ".orc");
    }
    else {
        return false;
    }
    if (QFile::exists(companionFileName)) {
        return (loadFile(companionFileName) != -1);
    }
    return false;
}

bool CsoundQt::saveFile(const QString &fileName, bool saveWidgets)
{
    //  qDebug("CsoundQt::saveFile");
    // update htmlview on Save
#if defined(QCS_QTHTML)
    if (!documentPages.isEmpty() && !documentPages[curPage]->getHtmlText().isEmpty()) {
        qDebug()<<"Update html on save";
        updateHtmlView();
    }
#endif
    QString text;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (!m_options->widgetsIndependent) { // Update outer geometry information for writing
        QWidget *s = widgetPanel->widget();
        //			widgetPanel->applySize(); //Store size of outer panel as size of widget
        if (s != 0) {
            QRect panelGeometry = widgetPanel->geometry();
            if (!widgetPanel->isFloating()) {
                panelGeometry.setX(-1);
                panelGeometry.setY(-1);
            }
            documentPages[curPage]->setWidgetLayoutOuterGeometry(panelGeometry);
        } else {
            documentPages[curPage]->setWidgetLayoutOuterGeometry(QRect());
        }
    }
    if (m_options->saveWidgets && saveWidgets)
        text = documentPages[curPage]->getFullText();
    else
        text = documentPages[curPage]->getBasicText();
    QApplication::restoreOverrideCursor();

    if (fileName != documentPages[curPage]->getFileName()) {
        documentPages[curPage]->setFileName(fileName);
        setCurrentFile(fileName);
    }
    lastUsedDir = fileName;
    lastUsedDir.resize(fileName.lastIndexOf("/") + 1);
    if (recentFiles.count(fileName) == 0) {
        recentFiles.prepend(fileName);
        recentFiles.removeLast();
        fillFileMenu();
    }
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << text;
    documentPages[curPage]->setModified(false);
    setWindowModified(false);
    documentTabs->setTabIcon(curPage, QIcon());
    return true;
}

void CsoundQt::setCurrentFile(const QString &fileName)
{
    QString shownName;
    if (fileName.isEmpty())
        shownName = "untitled.csd";
    else
        shownName = strippedName(fileName);

    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("CsoundQt")));
    documentTabs->setTabText(curPage, shownName);
    //  updateWidgets();
}

QString CsoundQt::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

QString CsoundQt::generateScript(bool realtime, QString tempFileName, QString executable)
{
#ifndef Q_OS_WIN32
    QString script = "#!/bin/sh\n";
#else
    QString script = "";
#endif
    QString cmdLine = "";
    // Only OPCODEDIR left here as it must be present before csound initializes
    // The problem is that it can't be passed when using the API...

    if (m_options->opcode7dir64Active)
        script += "export OPCODE7DIR64=" + m_options->opcode7dir64 + "\n";

#ifndef Q_OS_WIN32
    script += "cd " + QFileInfo(documentPages[curPage]->getFileName()).absolutePath() + "\n";
#else
    QString script_cd = "@pushd " + QFileInfo(documentPages[curPage]->getFileName()).absolutePath() + "\n";
    script_cd.replace("/", "\\");
    script += script_cd;
#endif

    if (executable.isEmpty()) {

		//test if csound is present in the same folder as CsoundQt, then Csound is bundled. Try to copy it to temporary location and run it from there
		// TODO: condition on csound.exe for windows, don't copy but set
#ifdef Q_OS_WIN32
		QString csoundExecutable = "/csound.exe";
#else
		QString csoundExecutable = "/csound";
#endif
		if (QFile::exists(initialDir+csoundExecutable) && !initialDir.startsWith("/usr")) { // if starts with /usr then probably normal install on Linux
			qDebug() << "Using bundled Csound to run in terminal";
			cmdLine = "\"" + initialDir+csoundExecutable + "\" ";
		} else {
			cmdLine = m_options->csoundExecutable+ " ";
		}
		qDebug()<<"Command line: " << cmdLine;
        m_options->rt = (realtime && m_options->rtUseOptions)
                || (!realtime && m_options->fileUseOptions);
        cmdLine += m_options->generateCmdLineFlags() + " ";
    }
    else {
        cmdLine = executable + " ";
    }

    if (tempFileName == ""){
        QString fileName = documentPages[curPage]->getFileName();
        if (documentPages[curPage]->getCompanionFileName() != "") {
            QString companionFile = documentPages[curPage]->getCompanionFileName();
            if (fileName.endsWith(".orc"))
                cmdLine += "\""  + fileName
                        + "\" \""+ companionFile + "\" ";
            else
                cmdLine += "\""  + companionFile
                        + "\" \""+ fileName + "\" ";
        }
        else if (fileName.endsWith(".csd",Qt::CaseInsensitive)) {
            cmdLine += "\""  + fileName + "\" ";
        }
    }
    else {
        cmdLine += "\""  + tempFileName + "\" ";
    }
    //  m_options->rt = (realtime and m_options->rtUseOptions)
    //                  or (!realtime and m_options->fileUseOptions);
    //  cmdLine += m_options->generateCmdLineFlags();
    script += "echo \"" + cmdLine + "\"\n";
    script += cmdLine + "\n";

#ifndef Q_OS_WIN32
    script += "echo \"\nPress return to continue\"\n";
    script += "dummy_var=\"\"\n";
    script += "read dummy_var\n";
    script += "rm $0\n";
#else
    script += "@echo.\n";
    script += "@pause\n";
    script += "@exit\n";
#endif
    return script;
}

void CsoundQt::getCompanionFileName()
{
    QString fileName = "";
    QDialog dialog(this);
    dialog.resize(400, 200);
    dialog.setModal(true);
    QPushButton *button = new QPushButton(tr("Ok"));

    connect(button, SIGNAL(released()), &dialog, SLOT(accept()));

    QSplitter *splitter = new QSplitter(&dialog);
    QListWidget *list = new QListWidget(&dialog);
    QCheckBox *checkbox = new QCheckBox(tr("Do not ask again"), &dialog);
    splitter->addWidget(list);
    splitter->addWidget(checkbox);
    splitter->addWidget(button);
    splitter->resize(400, 200);
    splitter->setOrientation(Qt::Vertical);
    QString extensionComplement = "";
    if (documentPages[curPage]->getFileName().endsWith(".orc"))
        extensionComplement = ".sco";
    else if (documentPages[curPage]->getFileName().endsWith(".sco"))
        extensionComplement = ".orc";
    else if (documentPages[curPage]->getFileName().endsWith(".udo"))
        extensionComplement = ".csd";
    else if (documentPages[curPage]->getFileName().endsWith(".inc"))
        extensionComplement = ".csd";

    for (int i = 0; i < documentPages.size(); i++) {
        QString name = documentPages[i]->getFileName();
        if (name.endsWith(extensionComplement))
            list->addItem(name);
    }
    QList<QListWidgetItem *> itemList = list->findItems(
                documentPages[curPage]->getCompanionFileName(),
                Qt::MatchExactly);
    if (itemList.size() > 0)
        list->setCurrentItem(itemList[0]);
    dialog.exec();
    QListWidgetItem *item = list->currentItem();
    if (!item) {
        qDebug() << "Empty list. Create empty score file";
        QString companion = "";
        if (documentPages[curPage]->getFileName().endsWith(".orc")) {
            // create an empty score file to run the orchestra scorelessly
			companion = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/empty.sco";
            QFile f(companion); // does it create it here
            f.open(QIODevice::ReadWrite | QIODevice::Text);
            f.close();
            qDebug() << "Created empty score file as companion: " << companion;
        }
        documentPages[curPage]->setCompanionFileName(companion);
        return;
    }
    QString itemText = item->text();
    if (checkbox->isChecked())
        documentPages[curPage]->askForFile = false;
    documentPages[curPage]->setCompanionFileName(itemText);
    for (int i = 0; i < documentPages.size(); i++) {
        if (documentPages[i]->getFileName() == documentPages[curPage]->getCompanionFileName()
                && !documentPages[i]->getFileName().endsWith(".csd")) {
            documentPages[i]->setCompanionFileName(documentPages[curPage]->getFileName());
            documentPages[i]->askForFile = documentPages[curPage]->askForFile;
            break;
        }
    }
}

void CsoundQt::setWidgetPanelGeometry()
{
    QRect geometry = documentPages[curPage]->getWidgetLayoutOuterGeometry();
    if (geometry.width() <= 0 || geometry.width() > 4096) {
        geometry.setWidth(400);
        qDebug() << "Warning: width invalid.";
    }
    if (geometry.height() <= 0 || geometry.height() > 4096) {
        geometry.setHeight(300);
        qDebug() << "Warning: height invalid.";
    }
    if (geometry.x() < 0 || geometry.x() > 4096) {
        geometry.setX(20);
        qDebug() << "Warning: X position invalid.";
    }
    if (geometry.y() < 0 || geometry.y() > 4096) {
        geometry.setY(0);
        qDebug() << "Warning: Y position invalid.";
    }
    //	qDebug() << "geom " << widgetPanel->geometry() << " frame " << widgetPanel->frameGeometry();
    widgetPanel->setGeometry(geometry);
}

int CsoundQt::isOpen(QString fileName)
{
    int open = -1;
    int i = 0;
    for (i = 0; i < documentPages.size(); i++) {
        if (documentPages[i]->getFileName() == fileName) {
            open = i;
            break;
        }
    }
    return open;
}


QString CsoundQt::setDocument(int index)
{
    QString name = QString();
    if (index < documentTabs->count() && index >= 0) {
        documentTabs->setCurrentIndex(index);
        name = documentPages[index]->getFileName();
    }
    return name;
}

void CsoundQt::insertText(QString text, int index, int section)
{
    if (index == -1) {
        index = curPage;
    }
    if (section == -1) {
        if (index < documentTabs->count() && index >= 0) {
            documentPages[index]->insertText(text);
        }
    }
    else {
        qDebug() << "Not implemented for sections";
    }
}

void CsoundQt::setCsd(QString text, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setBasicText(text);
    }
}

void CsoundQt::setFullText(QString text, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setFullText(text);
    }
}

void CsoundQt::setOrc(QString text, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setOrc(text);
    }
}

void CsoundQt::setSco(QString text, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setSco(text);
    }
}

void CsoundQt::setWidgetsText(QString text, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setWidgetsText(text);
    }
}

void CsoundQt::setPresetsText(QString text, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setPresetsText(text);
    }
}

void CsoundQt::setOptionsText(QString text, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setOptionsText(text);
    }
}

int CsoundQt::getDocument(QString name)
{
    int index = curPage;
    if (!name.isEmpty()) {
        index = -1;
        for (int i = 0; i < documentPages.size(); i++) {
            QString fileName = documentPages[i]->getFileName();
            QString relName = fileName.mid(fileName.lastIndexOf("/")+1);
            if (name == fileName  || name == relName  ) {
                index = i;
                break;
            }
        }
    }
    return index;
}

QString CsoundQt::getTheme()
{
    return m_options->theme;
}

QString CsoundQt::getSelectedText(int index, int section)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getSelectedText(section);
    }
    return text;
}

QString CsoundQt::getCsd(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getBasicText();
    }
    return text;
}

QString CsoundQt::getFullText(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getFullText();
    }
    return text;
}

QString CsoundQt::getOrc(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getOrc();
    }
    return text;
}

QString CsoundQt::getSco(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getSco();
    }
    return text;
}

QString CsoundQt::getWidgetsText(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getWidgetsText();
    }
    return text;
}

QString CsoundQt::getSelectedWidgetsText(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getSelectedWidgetsText();
    }
    return text;
}

QString CsoundQt::getPresetsText(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getPresetsText();
    }
    return text;
}

QString CsoundQt::getOptionsText(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getOptionsText();
    }
    return text;
}

QString CsoundQt::getFileName(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getFileName();
    }
    return text;
}

QString CsoundQt::getFilePath(int index)
{
    QString text = QString();
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        text = documentPages[index]->getFilePath();
    }
    return text;
}



void CsoundQt::setChannelValue(QString channel, double value, int index)
{

    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setChannelValue(channel, value);
    }
}

double CsoundQt::getChannelValue(QString channel, int index)
{
    double value = 0.0;
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        value = documentPages[index]->getChannelValue(channel);
    }
    return value;
}

void CsoundQt::setChannelString(QString channel, QString value, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setChannelString(channel, value);
    }
}

QString CsoundQt::getChannelString(QString channel, int index)
{
    QString value = "";
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        value = documentPages[index]->getChannelString(channel);
    }
    return value;
}

void CsoundQt::setWidgetProperty(QString widgetid, QString property, QVariant value, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        documentPages[index]->setWidgetProperty(widgetid, property, value);
    }
}

QVariant CsoundQt::getWidgetProperty(QString widgetid, QString property, int index)
{
    QVariant value;
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        value = documentPages[index]->getWidgetProperty(widgetid, property);
    }
    return value;
}


QString CsoundQt::createNewLabel(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewLabel(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewDisplay(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewDisplay(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewScrollNumber(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewScrollNumber(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewLineEdit(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewLineEdit(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewSpinBox(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewSpinBox(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewSlider(int x, int y, QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewSlider(x,y, channel);
    }
    return QString();
}

QString CsoundQt::createNewButton(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewButton(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewKnob(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewKnob(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewCheckBox(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewCheckBox(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewMenu(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewMenu(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewMeter(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewMeter(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewConsole(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewConsole(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewGraph(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewGraph(x,y,channel);
    }
    return QString();
}

QString CsoundQt::createNewScope(int x , int y , QString channel, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->createNewScope(x,y,channel);
    }
    return QString();
}

QStringList CsoundQt::getWidgetUuids(int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->getWidgetUuids();
    }
    return QStringList();
}

QStringList CsoundQt::listWidgetProperties(QString widgetid, int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->listWidgetProperties(widgetid);
    }
    return QStringList();
}

bool CsoundQt::destroyWidget(QString widgetid,int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->destroyWidget(widgetid);
    }
    return false;
}


CsoundEngine *CsoundQt::getEngine(int index)
{
    if (index == -1) {
        index = curPage;
    }
    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->getEngine();
    }
    else {
        return NULL;
    }
}

bool CsoundQt::startServer()
{
    m_server->removeServer("csoundqt"); // for any case, if socket was not cleard due crash before
    return m_server->listen("csoundqt");
}

#if defined(QCS_QTHTML)
void CsoundQt::updateHtmlView()
{
    QString htmlText = documentPages[curPage]->getHtmlText();
    if (!htmlText.isEmpty()) {
        ///csoundHtmlView->setCsoundEngine(getEngine(curPage));
        csoundHtmlView->load(documentPages[curPage]);
    } else {
        csoundHtmlView->clear();
    }
}
#endif


void CsoundQt::loadPreset(int preSetIndex, int index) {
    if (index == -1) {
        index = curPage;
    }

    if (index < documentTabs->count() && index >= 0) {
        return documentPages[index]->loadPreset(preSetIndex);
    }
    else {
        return;
    }
}

void CsoundQt::setParsedUDOs() {
    auto udos = m_inspector->getParsedUDOs();
    auto doc = documentPages[curPage];
    doc->setParsedUDOs(udos);
}

void CsoundQt::gotoLineDialog() {
    auto doc = this->getCurrentDocumentPage();
    doc->gotoLineDialog();
}
