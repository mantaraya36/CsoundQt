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

#ifndef QUTECSOUND_H
#define QUTECSOUND_H

#include <QtWidgets>
#include <QQuickWidget>
#include <QQuickItem>


#include <QLocalServer>
#include <QLocalSocket>

#include "types.h"
#include "configlists.h"

#include "risset.h"

#ifdef MACOSX_PRE_SNOW
// Needed to be able to grab menus back from FLTK
#include <Carbon/Carbon.h>
#endif

#define QCS_USE_NEW_ICON

class QAction;
class QMenu;
class QTextEdit;

class DockHelp;
class WidgetPanel;
class Inspector;
#ifdef QCS_PYTHONQT
class PythonConsole;
#endif
class DockConsole;
class DebugPanel;
class OpEntryParser;
class Options;
class ConfigLists;
class DocumentPage;
class UtilitiesDialog;
class Curve;
class GraphicWindow;
class KeyboardShortcuts;
class EventDispatcher;
class EventSheet;
class CsoundEngine;
class MidiHandler;
class MidiLearnDialog;
class WidgetLayout;

#if defined(QCS_QTHTML)
class CsoundHtmlView;
#endif

class CsoundQt:public QMainWindow
{
	Q_OBJECT

	friend class WidgetPanel; //to pass ud
	friend class FileOpenEater; //to pass curPage
public:
    CsoundQt(QStringList fileNames);
	~CsoundQt();
	static void utilitiesMessageCallback(CSOUND *csound,
										 int /*attr*/,
										 const char *fmt,
										 va_list args);

	// Editor
	QString setDocument(int index); // Returns document name
	void insertText(QString text, int index = -1, int section = -1);
	void setCsd(QString text, int index = -1);
	void setFullText(QString text, int index = -1);
	void setOrc(QString text, int index = -1);
	void setSco(QString text, int index = -1);
	void setWidgetsText(QString text, int index = -1);
	void setPresetsText(QString text, int index = -1);
	void setOptionsText(QString text, int index = -1);
	int getDocument(QString name = ""); // Returns document index. -1 if not current open
    QString getTheme();
    QString getSelectedText(int index = -1, int section = 0);
	QString getCsd(int index);
	QString getFullText(int index);
	QString getOrc(int index);
	QString getSco(int index);
	QString getWidgetsText(int index);
	QString getSelectedWidgetsText(int index);
	QString getPresetsText(int index);
	QString getOptionsText(int index);
	QString getFileName(int index);
	QString getFilePath(int index);
	// Widgets
	void setChannelValue(QString channel, double value, int index = -1);
	double getChannelValue(QString channel, int index = -1);
	void setChannelString(QString channel, QString value, int index = -1);
	QString getChannelString(QString channel, int index = -1);
	void setWidgetProperty(QString widgetid, QString property, QVariant value, int index= -1);
	QVariant getWidgetProperty(QString widgetid, QString property, int index= -1);
	QString createNewLabel(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewDisplay(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewScrollNumber(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewLineEdit(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewSpinBox(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewSlider(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewButton(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewKnob(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewCheckBox(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewMenu(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewMeter(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewConsole(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewGraph(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	QString createNewScope(int x = -1, int y = -1, QString channel = QString(), int index = -1);
	//    int popKeyPressEvent(); // return ASCII code of key press event for Csound or -1 if no event
	//    int popKeyReleaseEvent(); // return ASCII code of key release event for Csound -1 if no
	QStringList getWidgetUuids(int index = -1);
	QStringList listWidgetProperties(QString widgetid, int index = -1); // widgetid can be eihter uuid (prefered) or channel
	bool destroyWidget(QString widgetid, int  index = -1);
	void loadPreset(int preSetIndex, int index);
	//Live Event Sheets
	EventSheet* getSheet(int index = -1, int sheetIndex = -1);
	EventSheet* getSheet(int index = -1, QString sheetName = QString());
	// Engine
	CsoundEngine *getEngine(int index = -1);
	OpEntryParser *m_opcodeTree;
    Risset *risset;
	void stkCheck();
	// localServer
	bool startServer();
#if defined(QCS_QTHTML)
	void updateHtmlView();
#endif
    void changeFontSize(int change);

public slots:
	int loadFile(QString fileName, bool runNow = false);
	int loadFileFromSystem(QString fileName); // checks for m_options->autoPlay, if the function is called from other class
	void newFile();
	bool closeTab(bool forceCloseApp = false, int index = -1);
	bool saveFile(const QString &fileName, bool saveWidgets = true);
	void play(bool realtime = true, int index = -1);
	void runInTerm(bool realtime = true);
	void pause(int index = -1);
	void stop(int index = -1);
	void stopAll();
	void stopAllOthers();
	void markStopped();
	void perfEnded();
	void render();
	void record(bool);
	void record(bool, int index);
	void sendEvent(QString eventLine, double delay = 0);
	void sendEvent(int index, QString line, double delay = 0);
	void changePage(int index);
	void pageLeft();
	void pageRight();
	void setWidgetTooltipsVisible(bool visible);
	void closeExtraPanels(); // to close help and console panels when esc is pressed in the editor

	//    void updateWidgets();
	void openExample();
	void openTemplate();
	void logMessage(QString msg);
    void statusBarMessage(QString message);
	//    void registerLiveEvent(QWidget *e);
	void evaluateCsound(QString code = QString());
	void breakpointReached();
    void setParsedUDOs();
    void openExternalBrowser(QUrl url = QUrl());

    void increaseFontSize();
    void decreaseFontSize();


protected:
	virtual void closeEvent(QCloseEvent *event);
	//    virtual void keyPressEvent(QKeyEvent *event);
private slots:
	void open();
	void reload();
	void openFromAction();
	void openFromAction(QString fileName);
	void runScriptFromAction();
	void runScript(QString fileName);
	void createCodeGraph();
	void closeGraph();
	bool save();
	bool saveAs();
	void createApp();
	bool saveNoWidgets();
	void info();
	// Edition
	void copy();
	void cut();
	void paste();
	void undo();
	void redo();
	void evaluateSection();
	void evaluate(QString code = QString());
	void evaluatePython(QString code = QString());
	void evaluateString(QString evalCode);
	void setScratchPadMode(bool csdMode);
	void setWidgetEditMode(bool);  // This is not necessary as the action is passed and connected in the widget layout
	void duplicate();
	void print();
	void findReplace();  // Direct to current Page
	void findString();  // Direct to current Page
	bool join(bool ask = true);
	void showUtilities(bool);
	void getToIn();
	void inToGet();
	void insertMidiControlInstrument();
	void updateCsladspaText();
	void updateCabbageText();
    void saveWidgetsToQml();
	void setCurrentAudioFile(const QString fileName);
	void openExternalEditor();
	void openExternalPlayer();
	void setEditorFocus();
	void setHelpEntry();
    void helpForEntry(QString text, bool external=false);
    void setFullScreen(bool full);
    void checkFullScreen();
    void setEditorFullScreen(bool full);
    void setHtmlFullScreen(bool full);
    void setHelpFullScreen(bool full);
    void setWidgetsFullScreen(bool full);
    void showDebugger(bool show);
	void showVirtualKeyboard(bool show);
	void showTableEditor(bool show);
	void virtualKeyboardActOff(QObject *parent=0);
	void tableEditorActOff(QObject *parent=0);
	void showHtml5Gui(bool show);
	void splitView(bool split);
	void showMidiLearn();
	void virtualMidiIn(QVariant on, QVariant note, QVariant channel, QVariant velocity);
	void virtualCCIn(int channel, int cc, int value);
	void handleTableSyntax(QString syntax);
	void openManualExample(QString fileName);
    //void openExternalBrowser(QUrl url = QUrl()); // moved to public slots to connect with helpPanel
	void openPdfFile(QString name);
	void openFLOSSManual();
	void openQuickRef();
	void openOnlineDocumentation();
	void showManualOnline();
	void resetPreferences();
	void reportBug();
    void reportCsoundBug();
    void openShortcutDialog();
	void downloadManual();
	void about();
	void donate();
    void documentWasModified(bool status=true);
    void configure();
	void applySettings();
	void setCurrentOptionsForPage(DocumentPage *p);
	void runUtility(QString flags);
	//     void widgetDockLocationChanged(Qt::DockWidgetArea area);
	void updateInspector();
    void updateCurrentPageTask();
    void markInspectorUpdate(); // Notification that inspector needs update
	void setDefaultKeyboardShortcuts();
	void showNoPythonQtWarning();
	void showOrc(bool);
	void showScore(bool);
	void showOptions(bool);
	void showFileB(bool);
	void showOther(bool);
	void showOtherCsd(bool);
	void showWidgetEdit(bool);
	void toggleLineArea();
	void toggleParameterMode();
    void autocomplete();
//	void showParametersInEditor();
	void onNewConnection(); // when new client is connecting to localServer
	void onReadyRead(); // when message comes in
	void disableInternalRtMidi();
    void focusToTab(int tab);
    void ambiguosShortcut();
    void testAudioSetup();
    void checkSyntaxMenuAction();
    void tabMoved(int to, int from);
    void openExamplesFolder();
    void setLineAndColumn(int line, int column);

    DocumentPage *getCurrentDocumentPage() {
        if(curPage >= documentPages.size())
            return nullptr;
        return documentPages[curPage];
    }
    void gotoLineDialog();

#ifdef QCS_DEBUGGER
	void runDebugger();
	void stopDebugger();
	void pauseDebugger();
	void continueDebugger();
	void addBreakpoint(int line, int instr, int skip);
	void addInstrumentBreakpoint(double instr, int skip);
	void removeBreakpoint(int line, int instr);
	void removeInstrumentBreakpoint(double instr);
#endif
private:
	void createActions();
	void setKeyboardShortcutsList();
	void connectActions();
	QString getExamplePath(QString dir);
	void createMenus();
	void fillExampleMenu();
    void fillExampleSubmenu(QDir dir, QMenu *m, int depth);
	void fillFileMenu();
	void fillFavoriteMenu();
	void fillFavoriteSubMenu(QDir dir, QMenu *m, int depth);
	void fillScriptsMenu();
	void fillScriptsSubMenu(QDir dir, QMenu *m, int depth);
	void fillEditScriptsSubMenu(QDir dir, QMenu *m, int depth);
	void createToolBars();
    void createStatusBar();
	void readSettings();
	void storeSettings();
	void writeSettings(QStringList openFiles=QStringList(), int lastIndex = 0);
	void clearSettings();
    void setToolbarIconSize(int size);
    int execute(QString executable, QString options);
    int startProcess(QString executable, const QStringList &args);
    //    bool saveCurrent();
	bool makeNewPage(QString fileName, QString text);
	bool loadCompanionFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	QString strippedName(const QString &fullFileName);
	QString generateScript(bool realtime = true, QString tempFileName = "", QString executable = "");
	void getCompanionFileName();
	void setWidgetPanelGeometry();
	int isOpen(QString fileName);  // Returns index of document if open -1 if not open
	//    void markErrorLine();
	QString getSaveFileName();
	void createQuickRefPdf();
	void deleteTab(int index = -1);
	void openLogFile();
	void showNewFormatWarning();
	void setupEnvironment();
    void openHtmlHelp(QString fileName, QString entry, bool external);

    ConfigLists m_configlists;
	QTabWidget *documentTabs;
	GraphicWindow *m_graphic;  // To display the code graph images
	QVector<DocumentPage *> documentPages;
	Options *m_options;
	DockConsole *m_console;
	DockHelp *helpPanel;
	WidgetPanel *widgetPanel;  // Dock widget, for containing the widget layout
	QDockWidget *m_scratchPad;

	QQuickWidget *m_virtualKeyboard, *m_tableEditor;
	QPointer <QQuickWidget> m_virtualKeyboardPointer, m_tableEditorPointer; // to control, if the object is deleted

#if defined(QCS_QTHTML)
public: CsoundHtmlView *csoundHtmlView;
#endif
private:
	//    QString m_widgetClipboard;
	Inspector *m_inspector;
#ifdef QCS_DEBUGGER
	DebugPanel *m_debugPanel;
	CsoundEngine *m_debugEngine;
#endif
#ifdef QCS_PYTHONQT
	PythonConsole *m_pythonConsole;
#endif
	MidiHandler *midiHandler;
	MidiLearnDialog *m_midiLearn;
	QFile logFile;
	QVector<QAction *> m_keyActions; //Actions which have keyboard shortcuts
	QMenu *fileMenu;
	QMenu *recentMenu;
	QMenu *templateMenu;
	QMenu *editMenu;
	QMenu *controlMenu;
	QMenu *viewMenu;
	QMenu *favoriteMenu;
	QMenu *scriptsMenu;
	QMenu *helpMenu;
//	QToolBar *fileToolBar;
//	QToolBar *editToolBar;
	QToolBar *controlToolBar;
	QToolBar *configureToolBar;
	QAction *newAct;
	QAction *openAct;
	QAction *reloadAct;
	QAction *saveAct;
	QAction *saveAsAct;
	QAction *createAppAct;
	QAction *saveNoWidgetsAct;
	QAction *closeTabAct;
	QAction *printAct;
	QAction *createCodeGraphAct;
	QAction *infoAct;
	QAction *exitAct;
	QList<QAction *> openRecentAct;
	QAction *undoAct;
	QAction *redoAct;
	QAction *cutAct;
	QAction *copyAct;
	QAction *pasteAct;
	QAction *duplicateAct;
	QAction *joinAct;
	QAction *evaluateAct;
	QAction *evaluateSectionAct;
	QAction *scratchPadCsdModeAct;
	QAction *getToInAct;
	QAction *inToGetAct;
	QAction *midiControlAct;
	QAction *csladspaAct;
	QAction *cabbageAct;
    QAction *qmlAct;
    QAction *findAct;
	QAction *findAgainAct;
	QAction *configureAct;
	QAction *setShortcutsAct;
	QAction *editAct;
	QAction *runAct;
    QAction *testAudioSetupAct;
    QAction *checkSyntaxAct;
	QAction *runTermAct;
	QAction *pauseAct;
	QAction *stopAct;
	QAction *stopAllAct;
	QAction *recAct;
	QAction *renderAct;
	QAction *externalEditorAct;
	QAction *externalPlayerAct;
	QAction *focusEditorAct;
	QAction *showHelpAct;
    QAction *raiseHelpAct;
	QAction *showManualAct;
	QAction *showManualOnlineAct;
	QAction *downloadManualAct;
	QAction *showGenAct;
	QAction *showOverviewAct;
	QAction *showOpcodeQuickRefAct;
	QAction *showConsoleAct;
    QAction *raiseConsoleAct;
    QAction *viewFullScreenAct;
    QAction *viewEditorFullScreenAct;
    QAction *viewHtmlFullScreenAct;
    QAction *viewHelpFullScreenAct;
    QAction *viewWidgetsFullScreenAct;
#ifdef QCS_DEBUGGER
	QAction *showDebugAct;
#endif
	QAction *showVirtualKeyboardAct;
	QAction *showTableEditorAct;
#if defined(QCS_QTHTML)
	QAction *showHtml5Act;
    QAction *raiseHtml5Act;
#endif
	QAction *midiLearnAct;
	QAction *splitViewAct;
	QAction *showOrcAct;
	QAction *showScoreAct;
	QAction *showOptionsAct;
	QAction *showFileBAct;
	QAction *showOtherAct;
	QAction *showOtherCsdAct;
	QAction *showWidgetEditAct;
	QAction *setHelpEntryAct;
	QAction *browseBackAct;
	QAction *browseForwardAct;
	QAction *externalBrowserAct;
	QAction *openQuickRefAct;
	QAction *openDocumentationAct;
	QAction *showUtilitiesAct;
	QAction *showWidgetsAct;
    QAction *raiseWidgetsAct;

	QAction *showInspectorAct;
    QAction *raiseInspectorAct;
	QAction *showLiveEventsAct;
	QAction *showPythonConsoleAct;
    QAction *raisePythonConsoleAct;
	QAction *showScratchPadAct;
    QAction *raiseScratchPadAct;
	QAction *commentAct;
	//    QAction *uncommentAct;
	QAction *indentAct;
	QAction *unindentAct;
	QAction *killLineAct;
	QAction *killToEndAct;
    QAction *gotoLineAct;
    QAction *goBackAct;
	QAction *aboutAct;
	QAction *donateAct;
	//    QAction *aboutQtAct;
	QAction *resetPreferencesAct;
	QAction *reportBugAct;
    QAction *reportCsoundBugAct;
    QAction *lineNumbersAct;
	QAction *parameterModeAct;
    QAction *autocompleteAct;
//	QAction *showParametersAct;
    QAction *openExamplesFolderAct;
    QSignalMapper *focusMapper;
	int curPage;
	int curCsdPage;  // To recall last csd visited
	int configureTab; // Tab in last configure dialog accepted
	QString lastUsedDir;
	QString lastFileDir;
	QString quickRefFileName;
	QStringList recentFiles;
	QStringList lastFiles;
	QStringList tempScriptFiles; //Remember temp files to delete them later
	int lastTabIndex;
	bool m_resetPrefs; // Flag to reset preferences to default when closing
	bool m_inspectorNeedsUpdate;
	bool m_closing; // CsoundQt is closing (to inform timer threads)
	UtilitiesDialog *utilitiesDialog;
	QIcon modIcon;
	QString currentAudioFile;
	QString initialDir;
	QMutex closemutex;
	QLocalServer * m_server; // for receiving 'open file' messages from other instances
#ifdef MACOSX_PRE_SNOW
	MenuBarHandle menuBarHandle;
#endif
    QByteArray m_preFullScreenState;
    QString m_fullScreenComponent;
    QDir m_rissetDataPath;
    bool isDarkPalette;
    QStringList m_longOptions = {
        "--syntax-check-only", "--control-rate=", "--messagelevel=",
        "--env:", "--dither", "--sched", "--omacro:", "--smacro:",
        "--verbose", "--sample-accurate", "--realtime",
        "--nchnls=", "--nchnls_i=", "--sinesize", "--daemon",
        "--port=", "--use-system-sr", "--ksmps=",
        "--limiter", "--udp-echo", "--opcode-dir=", "-+rtmidi", "-+rtaudio",
        "--format"
    };
    QLabel *lineAndColumnLabel;
};

class FileOpenEater : public QObject
{
	Q_OBJECT
public:
	FileOpenEater() {m_mw = 0;}
	void setMainWindow(CsoundQt *mainWindow) {
		m_mw = mainWindow;
		while (!fileEventQueue.isEmpty()) {
			QString fileName = fileEventQueue.takeFirst();
            qDebug() << "FileOpenEater::setMainWindow  opening file" << fileName << ENDL; // use macro for Qt version compatibility
			m_mw->loadFileFromSystem(fileName);
		}
	}
	QStringList fileEventQueue;
protected:
	bool eventFilter(QObject *obj, QEvent *event)
	{
		if (event->type() == QEvent::FileOpen) {
			QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent*>(event);
			if (m_mw == 0) {
				fileEventQueue << fileEvent->file();
			}
			else {
				m_mw->loadFileFromSystem(fileEvent->file());
			}
			qDebug() << "FileOpenEater::eventFilter " << fileEvent->file();
			return true;
		} else {
			// standard event processing
			return QObject::eventFilter(obj, event);
		}
	}

	CsoundQt *m_mw;
};

#endif
