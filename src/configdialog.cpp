
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

#include <QDir>
//#include <QOperatingSystemVersion> // not present before Qt 5.9
#include <QSysInfo>

#include "qutecsound.h"
#include "configdialog.h"
#include "options.h"
#include "types.h"
#include "configlists.h"


typedef QPair<QString, QString> QStringPair;

#ifdef QCS_RTMIDI
#include "RtMidi.h"
#endif

ConfigDialog::ConfigDialog(CsoundQt *parent, Options *options, ConfigLists *configlists)
	: QDialog(parent), m_parent(parent), m_options(options), m_configlists(configlists)
{
	setupUi(this);
    // don't expand over screen, leave some room for panels
    setMaximumSize(  QGuiApplication::primaryScreen()->geometry().width()  * 0.9,
                   QGuiApplication::primaryScreen()->geometry().height() * 0.9);

	m_configlists->refreshModules();

	if(m_configlists->rtAudioNames.size() == 1) {
		QMessageBox::warning(this, tr("No Audio Modules"),
                             tr("No real-time audio modules were found.\n"
                                "Make sure OPCODE7DIR64 is set properly in your system"
                                "or the configuration dialog."));
	}
	QHash<QString, QString> audioModNames;
    audioModNames["pa_cb"] = "portaudio (callback)";
    audioModNames["pa_bl"] = "portaudio (blocking)";
    audioModNames["auhal"] = "coreaudio (auhal)";

    QHash<QString, int> audioPriorities;
    audioPriorities["jack"] = 0;
    audioPriorities["pa_cb"] = 1;
    audioPriorities["auhal"] = 2;
    std::sort(m_configlists->rtAudioNames.begin(),
              m_configlists->rtAudioNames.end(),
              [&audioPriorities](const QString &mod1, const QString &mod2) {
        return audioPriorities.value(mod1, 10) < audioPriorities.value(mod2, 10);
    });

    if(m_configlists->rtAudioNames.contains("jack")) {
        if(!m_configlists->isJackRunning()) {
            audioModNames["jack"] = "jack (not running)";
            JackNameLineEdit->setEnabled(false);
        }
    } else {
        if(RtUseOptionsCheckBox->checkState() == Qt::Checked)
            JackNameLineEdit->setEnabled(false);
    }
	foreach (QString item, m_configlists->fileTypeLongNames) {
		FileTypeComboBox->addItem(item);
	}
	foreach (QString item, m_configlists->fileFormatNames) {
		FileFormatComboBox->addItem(item);
	}
	foreach (QString item, m_configlists->rtAudioNames) {
		if (audioModNames.contains(item)) {
            RtModuleComboBox->addItem(audioModNames[item], item);
		} else {
			RtModuleComboBox->addItem(item, item);
		}
	}
	foreach (QString item, m_configlists->rtMidiNames) {
		RtMidiModuleComboBox->addItem(item, item);
	}
	for (int i = 0; i < m_configlists->languages.size(); i++) {
        languageComboBox->addItem(m_configlists->languages[i],
                                  QVariant(m_configlists->languageCodes[i]));
	}
	midiInterfaceComboBox->clear();
	midiOutInterfaceComboBox->clear();

#ifdef QCS_RTMIDI
	try {
		RtMidiIn midiin((RtMidi::Api) m_options->rtMidiApi); // later: should be possible also with other APIs UNIX_JACK etc
		for (int i = 0; i < (int) midiin.getPortCount(); i++) {
			midiInterfaceComboBox->addItem(QString::fromStdString(midiin.getPortName(i)), QVariant(i));
		}
	}

#ifdef QCS_OLD_RTMIDI
	catch (RtError &error) {
#else
	catch (RtMidiError &error) {
#endif
		// Handle the exception here
		error.printMessage();
	}
	try {
		RtMidiOut midiout((RtMidi::Api) m_options->rtMidiApi);
		for (int i = 0; i < (int) midiout.getPortCount(); i++) {
			midiOutInterfaceComboBox->addItem(QString::fromStdString(midiout.getPortName(i)), QVariant(i));
		}
	}
#ifdef QCS_OLD_RTMIDI
	catch (RtError &error) {
#else
	catch (RtMidiError &error) {
#endif
		// Handle the exception here
		error.printMessage();
	}
#else
	midiInterfaceComboBox->addItem(tr("No RtMidi support"));
#endif

	midiInterfaceComboBox->addItem(QString(tr("None", "No MIDI In interface")), QVariant(9999));
	//match interface by nameby name, not by index
	int ifIndex = midiInterfaceComboBox->findText(m_options->midiInterfaceName);
	if (ifIndex>=0) {
		midiInterfaceComboBox->setCurrentIndex(ifIndex);
	} else {
        qDebug()<< m_options->midiInterfaceName << "not found. Setting MIDI In to None";
        // set to none if not found
        midiInterfaceComboBox->setCurrentIndex(midiInterfaceComboBox->findData(9999));
	}
	midiOutInterfaceComboBox->addItem(QString(tr(" None", "No MIDI Out interface")), QVariant(9999));

	ifIndex = midiOutInterfaceComboBox->findText(m_options->midiOutInterfaceName);
	if (ifIndex>=0) {
		midiOutInterfaceComboBox->setCurrentIndex(ifIndex);
	} else {
        qDebug()<< m_options->midiOutInterfaceName << " not found. Setting MIDI Out to None";
		midiOutInterfaceComboBox->setCurrentIndex(midiOutInterfaceComboBox->findData(9999));
	}

	themeComboBox->setCurrentIndex(themeComboBox->findText(m_options->theme));
	fontComboBox->setCurrentIndex(fontComboBox->findText(m_options->font) );
	fontSizeComboBox->setCurrentIndex(fontSizeComboBox->findText(QString::number((int) m_options->fontPointSize)));
	lineNumbersCheckBox->setChecked(m_options->showLineNumberArea);

	lineEndingComboBox->setCurrentIndex(m_options->lineEnding);
	consoleFontComboBox->setCurrentIndex(consoleFontComboBox->findText(m_options->consoleFont) );
	consoleFontSizeComboBox->setCurrentIndex(consoleFontSizeComboBox->findText(QString::number((int) m_options->consoleFontPointSize)));
	QPixmap pixmap(64,64);
	pixmap.fill(m_options->consoleFontColor);
	consoleFontColorPushButton->setIcon(pixmap);
	QPalette palette(m_options->consoleFontColor);
	consoleFontColorPushButton->setPalette(palette);

	pixmap.fill(m_options->consoleBgColor);
	consoleBgColorPushButton->setIcon(pixmap);
	palette = QPalette(m_options->consoleBgColor);
	consoleBgColorPushButton->setPalette(palette);

	pixmap.fill(m_options->editorBgColor);
    // editorBgColorButton->setIcon(pixmap);
	palette = QPalette(m_options->editorBgColor);
    // editorBgColorButton->setPalette(palette);

	tabWidthSpinBox->setValue(m_options->tabWidth);
	tabIndentCheckBox->setChecked(m_options->tabIndents);
    highlightingThemeComboBox->setCurrentText(m_options->highlightingTheme);
	autoplayCheckBox->setChecked(m_options->autoPlay);
	autoJoinCheckBox->setChecked(m_options->autoJoin);
	menusDepthSpinBox->setValue(m_options->menuDepth);
	saveChangesCheckBox->setChecked(m_options->saveChanges);
    askIfTemporaryCheckBox->setChecked(m_options->askIfTemporary);
	rememberFileCheckBox->setChecked(m_options->rememberFile);
	saveWidgetsCheckBox->setChecked(m_options->saveWidgets);
	widgetsIndependentCheckBox->setChecked(m_options->widgetsIndependent);
	iconTextCheckBox->setChecked(m_options->iconText);
	toolbarCheckBox->setChecked(m_options->showToolbar);
    lockToolbarCheckBox->setChecked(m_options->lockToolbar);
    toolbarIconSizeSpinBox->setValue(m_options->toolbarIconSize);
	wrapLinesCheckBox->setChecked(m_options->wrapLines);
	autoCompleteCheckBox->setChecked(m_options->autoComplete);
    autoCompleteDelaySpinBox->setValue(m_options->autoCompleteDelay);

	autoParameterModeCheckBox->setChecked(m_options->autoParameterMode);
	widgetsCheckBox->setChecked(m_options->enableWidgets);
	midiCcToCurrentPageOnlyCheckBox->setChecked(m_options->midiCcToCurrentPageOnly);
	showWidgetsOnRunCheckBox->setChecked(m_options->showWidgetsOnRun);
	showTooltipsCheckBox->setChecked(m_options->showTooltips);
    graphUpdateRateSpinBox->setValue(m_options->graphUpdateRate);
	enableFLTKCheckBox->setChecked(m_options->enableFLTK);
	terminalFLTKCheckBox->setChecked(m_options->terminalFLTK);
	terminalFLTKCheckBox->setEnabled(m_options->enableFLTK);
	oldFormatCheckBox->setChecked(m_options->oldFormat);
	openPropertiesCheckBox->setChecked(m_options->openProperties);
	fontScalingSpinBox->setValue(m_options->fontScaling);
	fontOffsetSpinBox->setValue(m_options->fontOffset);
    tabShortcutActiveCheckBox->setChecked(m_options->tabShortcutActive);
    highlightScoreCheckBox->setChecked(m_options->highlightScore);

	if (m_options->useAPI)
		ApiRadioButton->setChecked(true);
	else
		ExternalRadioButton->setChecked(true);

	noMessagesCheckBox->setChecked(m_options->noMessages);
	noBufferCheckBox->setChecked(m_options->noBuffer);
	noPythonCheckBox->setChecked(m_options->noPython);
	noEventsCheckBox->setChecked(m_options->noEvents);

	//  threadCheckBox->setChecked(m_options->thread);
	//  threadCheckBox->setEnabled(ApiRadioButton->isChecked());
	keyRepeatCheckBox->setChecked(m_options->keyRepeat);
	debugLiveEventsCheckBox->setChecked(m_options->debugLiveEvents);
	int bufferIndex = consoleBufferComboBox->findText(QString::number(m_options->consoleBufferSize));
	if (bufferIndex < 0) {
		bufferIndex = consoleBufferComboBox->count() - 1;
	}
	consoleBufferComboBox->setCurrentIndex(bufferIndex);

    checkSyntaxBeforeRunCheckBox->setChecked(m_options->checkSyntaxBeforeRun);

	BufferSizeLineEdit->setText(QString::number(m_options->bufferSize));
	BufferSizeCheckBox->setChecked(m_options->bufferSizeActive);
	BufferSizeLineEdit->setEnabled(m_options->bufferSizeActive);
	HwBufferSizeLineEdit->setText(QString::number(m_options->HwBufferSize));
	HwBufferSizeCheckBox->setChecked(m_options->HwBufferSizeActive);
	HwBufferSizeLineEdit->setEnabled(m_options->HwBufferSizeActive);
    // DitherCheckBox->setChecked(m_options->dither);
    // newParserCheckBox->setChecked(m_options->newParser);
	multicoreCheckBox->setChecked(m_options->multicore);
	numThreadsSpinBox->setValue(m_options->numThreads);
	AdditionalFlagsCheckBox->setChecked(m_options->additionalFlagsActive);
	AdditionalFlagsLineEdit->setText(m_options->additionalFlags);
	AdditionalFlagsLineEdit->setEnabled(m_options->additionalFlagsActive);
	FileUseOptionsCheckBox->setChecked(m_options->fileUseOptions);
	FileOverrideCheckBox->setChecked(m_options->fileOverrideOptions);
	AskFilenameCheckBox->setChecked(m_options->fileAskFilename);
	FilePlayFinishedCheckBox->setChecked(m_options->filePlayFinished);
	FileTypeComboBox->setCurrentIndex(m_options->fileFileType);
	FileFormatComboBox->setCurrentIndex(m_options->fileSampleFormat);
	InputFilenameCheckBox->setChecked(m_options->fileInputFilenameActive);
	InputFilenameLineEdit->setText(m_options->fileInputFilename);
	InputFilenameLineEdit->setEnabled(m_options->fileInputFilenameActive);
	OutputFilenameCheckBox->setChecked(m_options->fileOutputFilenameActive);
	OutputFilenameLineEdit->setText(m_options->fileOutputFilename);
	OutputFilenameLineEdit->setEnabled(m_options->fileOutputFilenameActive);
	RtUseOptionsCheckBox->setChecked(m_options->rtUseOptions);
	RtOverrideCheckBox->setChecked(m_options->rtOverrideOptions);
	int index = RtModuleComboBox->findData(m_options->rtAudioModule);
	if (index < 0 || index >= RtModuleComboBox->count()) {
		index = 0;
	}
    RtModuleComboBox->setCurrentIndex(index);
    connect(RtModuleComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onRtModuleComboBoxChanged(int)) );

	RtInputLineEdit->setText(m_options->rtInputDevice);
	RtOutputLineEdit->setText(m_options->rtOutputDevice);
    // useSystemSamplerateCheckBox->setChecked(m_options->useSystemSamplerate);

    if(m_options->useSystemSamplerate) {
        if(RtModuleComboBox->currentText() == "pulse") {
            // pulseaudio backend has no system sample rate, so we default to the orchestra
            samplerateComboBox->setCurrentText("Orchestra");
        } else {
            samplerateComboBox->setCurrentText("System");
        }
    } else if (m_options->samplerate == 0) {
        samplerateComboBox->setCurrentText("Orchestra");
    } else {
        samplerateComboBox->setCurrentText(QString::number(m_options->samplerate));
    }

    numChannelsCheckBox->setChecked(m_options->overrideNumChannels);
    numChannelsSpinBox->setValue(m_options->numChannels);
    numChannelsSpinBox->setMinimum(1);
    numChannelsSpinBox->setMaximum(64);

    numInputChannelsSpinBox->setValue(m_options->numInputChannels);
    numInputChannelsSpinBox->setMinimum(1);
    numInputChannelsSpinBox->setMaximum(64);

    realtimeCheckBox->setChecked(m_options->realtimeFlag);
    sampleAccurateCheckBox->setChecked(m_options->sampleAccurateFlag);

    //limiter
    bool limiterAvailable = csoundGetVersion()>=6160;
    limiterCheckBox->setEnabled(limiterAvailable);
    limiterSpinBox->setEnabled(limiterAvailable);
    if (limiterAvailable) {
        limiterCheckBox->setChecked(m_options->useLimiter);
        limiterSpinBox->setValue(m_options->limitValue);
    }

	JackNameLineEdit->setText(m_options->rtJackName);
	RtMidiModuleComboBox->setCurrentIndex(RtMidiModuleComboBox->findData(m_options->rtMidiModule));
	RtMidiInputLineEdit->setText(m_options->rtMidiInputDevice);
	RtMidiOutputLineEdit->setText(m_options->rtMidiOutputDevice);
	csoundMidiCheckBox->setChecked(m_options->useCsoundMidi);
	simultaneousCheckBox->setChecked(m_options->simultaneousRun);

	sampleFormatComboBox->setCurrentIndex(m_options->sampleFormat);
	debugPortSpinBox->setValue(m_options->debugPort);

	CsdocdirLineEdit->setText(m_options->csdocdir);
    Opcode7dir64LineEdit->setText(m_options->opcode7dir64);
	SadirCheckBox->setChecked(m_options->sadirActive);
	SadirLineEdit->setText(m_options->sadir);
	SsdirCheckBox->setChecked(m_options->ssdirActive);
	SsdirLineEdit->setText(m_options->ssdir);
	SfdirCheckBox->setChecked(m_options->sfdirActive);
	SfdirLineEdit->setText(m_options->sfdir);
	rawWaveCheckBox->setChecked(m_options->rawWaveActive);
	rawWaveLineEdit->setText(m_options->rawWave);
	IncdirCheckBox->setChecked(m_options->incdirActive);
	IncdirLineEdit->setText(m_options->incdir);
	//  defaultCsdCheckBox->setChecked(m_options->defaultCsdActive);
	//  defaultCsdLineEdit->setText(m_options->defaultCsd);
	//  defaultCsdLineEdit->setEnabled(m_options->defaultCsdActive);
	favoriteLineEdit->setText(m_options->favoriteDir);
    examplePathlineEdit->setText(m_options->examplePath);
	pythonDirLineEdit->setText(m_options->pythonDir);
	pythonExecutableLineEdit->setText(m_options->pythonExecutable);
	csoundExecutableLineEdit->setText(m_options->csoundExecutable);
	logFileLineEdit->setText(m_options->logFile);
	sdkLineEdit->setText(m_options->sdkDir);
	templateLineEdit->setText(m_options->templateDir);

	TerminalLineEdit->setText(m_options->terminal);
	browserLineEdit->setText(m_options->browser);
	dotLineEdit->setText(m_options->dot);
	WaveEditorLineEdit->setText(m_options->waveeditor);
	WavePlayerLineEdit->setText(m_options->waveplayer);
	pdfViewerLineEdit->setText(m_options->pdfviewer);
	languageComboBox->setCurrentIndex(m_options->language);

	templateTextEdit->setPlainText(m_options->csdTemplate);

    connect(inputFilenameToolButton, SIGNAL(clicked()), this, SLOT(browseInputFilename()));
	connect(outputFilenameToolButton, SIGNAL(clicked()), this, SLOT(browseOutputFilename()));
	connect(csdocdirToolButton, SIGNAL(clicked()), this, SLOT(browseCsdocdir()));
    connect(opcode7dir64ToolButton, SIGNAL(clicked()), this, SLOT(browseOpcode7dir64())); // was commented out - why?
	connect(sadirToolButton, SIGNAL(clicked()), this, SLOT(browseSadir()));
	connect(ssdirToolButton, SIGNAL(clicked()), this, SLOT(browseSsdir()));
	connect(sfdirToolButton, SIGNAL(clicked()), this, SLOT(browseSfdir()));
	connect(incdirToolButton, SIGNAL(clicked()), this, SLOT(browseIncdir()));
	connect(rawWaveToolButton, SIGNAL(clicked()), this, SLOT(browseRawWaveDir()));
	//  connect(defaultCsdToolButton, SIGNAL(clicked()), this, SLOT(browseDefaultCsd()));
	connect(favoriteToolButton, SIGNAL(clicked()), this, SLOT(browseFavorite()));
    connect(examplePathToolButton, SIGNAL(clicked()), this, SLOT(browseExamplePath()) );
    connect(terminalToolButton, SIGNAL(clicked()), this, SLOT(browseTerminal()));
	connect(browserToolButton, SIGNAL(clicked()), this, SLOT(browseBrowser()));
	connect(dotToolButton, SIGNAL(clicked()), this, SLOT(browseDot()));
	connect(waveEditorToolButton, SIGNAL(clicked()), this, SLOT(browseWaveEditor()));
	connect(wavePlayerToolButton, SIGNAL(clicked()), this, SLOT(browseWavePlayer()));
	connect(pdfViewerToolButton, SIGNAL(clicked()), this, SLOT(browsePdfViewer()));
	connect(pythonDirToolButton, SIGNAL(clicked()), this, SLOT(browsePythonDir()));
	connect(logFileToolButton, SIGNAL(clicked()), this, SLOT(browseLogFile()));
	connect(sdkToolButton, SIGNAL(clicked()), this, SLOT(browseSdkDir()));
	connect(templateToolButton, SIGNAL(clicked()), this, SLOT(browseTemplateDir()));

	//  connect(this, SIGNAL(changeFont()), parent, SLOT(changeFont()));
	connect(audioInputToolButton, SIGNAL(released()), this, SLOT(selectAudioInput()));
	connect(audioOutputToolButton, SIGNAL(released()), this, SLOT(selectAudioOutput()));
	connect(midiInputToolButton, SIGNAL(released()), this, SLOT(selectMidiInput()));
	connect(midiOutputToolButton, SIGNAL(released()), this, SLOT(selectMidiOutput()));
	connect(consoleFontColorPushButton, SIGNAL(released()), this, SLOT(selectTextColor()));
	connect(consoleBgColorPushButton, SIGNAL(released()), this, SLOT(selectBgColor()));
    // connect(editorBgColorButton, SIGNAL(released()), this, SLOT(selectEditorBgColor()));

	connect(clearTemplatePushButton,SIGNAL(released()), this, SLOT(clearTemplate()));
	connect(defaultTemplatePushButton,SIGNAL(released()), this, SLOT(defaultTemplate()));

    // connect(OpcodedirCheckBox, SIGNAL(toggled(bool)), this, SLOT(warnOpcodeDir(bool)));
    // connect(Opcodedir64CheckBox, SIGNAL(toggled(bool)), this, SLOT(warnOpcodeDir(bool)));
    // connect(Opcode6dir64CheckBox, SIGNAL(toggled(bool)), this, SLOT(uDir(bool)));

	connect(csoundExecutableToolButton,SIGNAL(clicked()),this, SLOT(browseCsoundExecutable()));
    connect(pythonExecutableToolButton,SIGNAL(clicked()),this, SLOT(browsePythonExecutable()));

    connect(testAudioSetupButton, SIGNAL(released()), this, SLOT(testAudioSetup()));
    // connect(envirRecommendButton, SIGNAL(clicked()), this, SLOT(recommendEnvironmentSettings()));

	//connect(RtMidiModuleComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(checkRtMidiModule(QString)) );


#ifndef QCS_PYTHONQT
	pythonDirLineEdit->setEnabled(false);
	pythonDirToolButton->setEnabled(false);
	noPythonCheckBox->setEnabled(false);
	noPythonCheckBox->setChecked(false);
#endif

    m_selectedOutputDeviceIndex = -1;
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::onRtModuleComboBoxChanged(int index) {
    Q_UNUSED(index);
    // Todo: remember the settings for each backend in a hash table, and set it to the last
    // known value when changed back. Otherwise set it to the default adc/dac
    auto currentText = this->RtModuleComboBox->currentText();
    //auto currentOperatingSystem = QOperatingSystemVersion::current();
    //qDebug() << "module: " << currentText << "Op.system type is: " << QSysInfo::productType() ;
    if(currentText == "null") {
        RtInputLineEdit->setText("");
        RtOutputLineEdit->setText("");
    } else if (currentText == "jack") {
#ifdef Q_OS_LINUX
        RtInputLineEdit->setText("adc:system:capture_");
        RtOutputLineEdit->setText("dac:system:playback_");
#else
        RtInputLineEdit->setText("adc");
        RtOutputLineEdit->setText("dac");
#endif
    } else if (currentText == "pulse") {
        RtInputLineEdit->setText("adc");
        RtOutputLineEdit->setText("dac");
        if(m_options->useSystemSamplerate || this->samplerateComboBox->currentText() == "System") {
            this->samplerateComboBox->setCurrentText("Orchestra");
            m_options->useSystemSamplerate = false;
            m_options->samplerate = 0;
        }
    } else if (currentText.startsWith("portaudio") && QSysInfo::productType()=="osx" ) { // on newer Mac's the internal microphone has 1 channel and that causes problems for portaudio. Disable input by default
			qDebug() << "Set audio input to none for portaudio on MacOS";
			RtInputLineEdit->setText("");
			RtOutputLineEdit->setText("dac");
	} else {
        RtInputLineEdit->setText("adc");
        RtOutputLineEdit->setText("dac");
    }
}

int ConfigDialog::currentTab()
{
	return editorTabWidget->currentIndex();
}

void ConfigDialog::warnOpcodeDir(bool on)
{
	if (on) {
		QMessageBox::warning(this, "CsoundQt",
							 tr("Please note that OPCODEDIR and OPCODEDIR64 will overwrite current settings so you will need to restart CsoundQt to return to default.")
							 );
	}
}

void ConfigDialog::setCurrentTab(int index)
{
	if (index >= 0 && index < 4) {
		editorTabWidget->setCurrentIndex(index);
	}
}

void ConfigDialog::accept()
{
	m_options->theme = themeComboBox->currentText();
	m_options->font = fontComboBox->currentText();
	m_options->fontPointSize = fontSizeComboBox->currentText().toDouble();
	m_options->showLineNumberArea = lineNumbersCheckBox->isChecked();
	m_options->lineEnding = lineEndingComboBox->currentIndex();
	m_options->consoleFont = consoleFontComboBox->currentText();
	m_options->consoleFontPointSize = consoleFontSizeComboBox->currentText().toDouble();
	m_options->tabWidth = tabWidthSpinBox->value();
	m_options->tabIndents = tabIndentCheckBox->isChecked();
    m_options->highlightingTheme = highlightingThemeComboBox->currentText();
    m_options->colorVariables = m_options->highlightingTheme != "none";
    m_options->autoPlay = autoplayCheckBox->isChecked();
	m_options->autoJoin = autoJoinCheckBox->isChecked();
	m_options->menuDepth = menusDepthSpinBox->value();
	m_options->saveChanges = saveChangesCheckBox->isChecked();
    m_options->askIfTemporary = askIfTemporaryCheckBox->isChecked();
	m_options->widgetsIndependent = widgetsIndependentCheckBox->isChecked();
	m_options->rememberFile = rememberFileCheckBox->isChecked();
	m_options->saveWidgets = saveWidgetsCheckBox->isChecked();
	m_options->iconText = iconTextCheckBox->isChecked();
	m_options->showToolbar = toolbarCheckBox->isChecked();
    m_options->lockToolbar = lockToolbarCheckBox->isChecked();
    m_options->toolbarIconSize = toolbarIconSizeSpinBox->value();
	m_options->wrapLines = wrapLinesCheckBox->isChecked();
	m_options->autoComplete = autoCompleteCheckBox->isChecked();
    m_options->autoCompleteDelay = autoCompleteDelaySpinBox->value();
	m_options->autoParameterMode = autoParameterModeCheckBox->isChecked();
	m_options->enableWidgets = widgetsCheckBox->isChecked();
	m_options->midiCcToCurrentPageOnly = midiCcToCurrentPageOnlyCheckBox->isChecked();
	m_options->showWidgetsOnRun = showWidgetsOnRunCheckBox->isChecked();
	m_options->showTooltips = showTooltipsCheckBox->isChecked();
	m_options->enableFLTK = enableFLTKCheckBox->isChecked();
	m_options->terminalFLTK = terminalFLTKCheckBox->isChecked();
	m_options->oldFormat = oldFormatCheckBox->isChecked();
	m_options->openProperties = openPropertiesCheckBox->isChecked();
	m_options->fontScaling = fontScalingSpinBox->value();
	m_options->fontOffset = fontOffsetSpinBox->value();
    m_options->graphUpdateRate = graphUpdateRateSpinBox->value();
	m_options->debugPort = debugPortSpinBox->value();
    m_options->tabShortcutActive = tabShortcutActiveCheckBox->isChecked();
	m_options->useAPI = ApiRadioButton->isChecked();
	//  m_options->thread = threadCheckBox->isChecked();
	m_options->keyRepeat = keyRepeatCheckBox->isChecked();
	m_options->debugLiveEvents = debugLiveEventsCheckBox->isChecked();
	m_options->consoleBufferSize = consoleBufferComboBox->itemText(consoleBufferComboBox->currentIndex()).toInt();
	m_options->midiInterface = midiInterfaceComboBox->itemData(midiInterfaceComboBox->currentIndex()).toInt(); // actually not necessary to store thta any more but for any case...
	m_options->midiInterfaceName = midiInterfaceComboBox->currentText();
	m_options->midiOutInterface = midiOutInterfaceComboBox->itemData(midiOutInterfaceComboBox->currentIndex()).toInt();
	m_options->midiOutInterfaceName = midiOutInterfaceComboBox->currentText();
	m_options->noMessages = noMessagesCheckBox->isChecked();
	m_options->noBuffer = noBufferCheckBox->isChecked();
	m_options->noPython = noPythonCheckBox->isChecked();
	m_options->noEvents = noEventsCheckBox->isChecked();
	if (m_options->consoleBufferSize < 0)
		m_options->consoleBufferSize = 0;
	m_options->bufferSize = BufferSizeLineEdit->text().toInt();
	m_options->bufferSizeActive = BufferSizeCheckBox->isChecked();
	m_options->HwBufferSize = HwBufferSizeLineEdit->text().toInt();
	m_options->HwBufferSizeActive = HwBufferSizeCheckBox->isChecked();
    // m_options->dither = DitherCheckBox->isChecked();
    m_options->realtimeFlag = realtimeCheckBox->isChecked(); // this was empty after = ?
    // m_options->newParser = newParserCheckBox->isChecked() ? 1 : 0;
    m_options->useLimiter = limiterCheckBox->isEnabled() && limiterCheckBox->isChecked();
    m_options->limitValue = limiterSpinBox->value();
	m_options->multicore = multicoreCheckBox->isChecked();
	m_options->numThreads = numThreadsSpinBox->value();
	m_options->additionalFlags = AdditionalFlagsLineEdit->text();
	m_options->additionalFlagsActive = AdditionalFlagsCheckBox->isChecked();
	m_options->fileUseOptions = FileUseOptionsCheckBox->isChecked();
	m_options->fileOverrideOptions = FileOverrideCheckBox->isChecked();
	m_options->fileAskFilename = AskFilenameCheckBox->isChecked();
	m_options->filePlayFinished = FilePlayFinishedCheckBox->isChecked();
	m_options->fileFileType = FileTypeComboBox->currentIndex();
	m_options->fileSampleFormat = FileFormatComboBox->currentIndex();
	m_options->fileInputFilenameActive = InputFilenameCheckBox->isChecked();
	m_options->fileInputFilename = InputFilenameLineEdit->text();
	m_options->fileOutputFilenameActive = OutputFilenameCheckBox->isChecked();
	m_options->fileOutputFilename = OutputFilenameLineEdit->text();
	m_options->rtUseOptions = RtUseOptionsCheckBox->isChecked();
	m_options->rtOverrideOptions = RtOverrideCheckBox->isChecked();
    m_options->rtAudioModule = RtModuleComboBox->itemData(
                RtModuleComboBox->currentIndex()).toString();
	m_options->rtInputDevice = RtInputLineEdit->text();
	m_options->rtOutputDevice = RtOutputLineEdit->text();
	m_options->rtJackName = JackNameLineEdit->text();
    // m_options->useSystemSamplerate = useSystemSamplerateCheckBox->isChecked();
    auto srmenu = samplerateComboBox->currentText();
    if(srmenu == "System") {
        m_options->useSystemSamplerate = true;
        m_options->samplerate = 0;
    } else if (srmenu == "Orchestra") {
        m_options->useSystemSamplerate = false;
        m_options->samplerate = 0;
    } else {
        m_options->useSystemSamplerate = false;
        m_options->samplerate = srmenu.toInt();
    }
    m_options->overrideNumChannels = numChannelsCheckBox->isChecked();
    m_options->numChannels = numChannelsSpinBox->value();

    m_options->numInputChannels = numInputChannelsSpinBox->value();
    m_options->realtimeFlag = realtimeCheckBox->isChecked();
    m_options->sampleAccurateFlag = sampleAccurateCheckBox->isChecked();


	m_options->rtMidiModule = RtMidiModuleComboBox->currentText();
	m_options->rtMidiInputDevice = RtMidiInputLineEdit->text();
	m_options->rtMidiOutputDevice = RtMidiOutputLineEdit->text();
	m_options->useCsoundMidi = csoundMidiCheckBox->isChecked();
	m_options->simultaneousRun = simultaneousCheckBox->isChecked();

	m_options->sampleFormat = sampleFormatComboBox->currentIndex();

	m_options->csdocdir = CsdocdirLineEdit->text();
    m_options->opcode7dir64Active = Opcode7dir64CheckBox->isChecked();
    m_options->opcode7dir64 = Opcode7dir64LineEdit->text();
	m_options->sadirActive = SadirCheckBox->isChecked();
	m_options->sadir = SadirLineEdit->text();
	m_options->ssdirActive = SsdirCheckBox->isChecked();
	m_options->ssdir = SsdirLineEdit->text();
	m_options->sfdirActive = SfdirCheckBox->isChecked();
	m_options->sfdir = SfdirLineEdit->text();
	m_options->incdirActive = IncdirCheckBox->isChecked();
	m_options->incdir = IncdirLineEdit->text();
	m_options->rawWaveActive = rawWaveCheckBox->isChecked();
	m_options->rawWave = rawWaveLineEdit->text();
	//  m_options->defaultCsdActive = defaultCsdCheckBox->isChecked();
	//  m_options->defaultCsd = defaultCsdLineEdit->text();
	m_options->favoriteDir = favoriteLineEdit->text();
    m_options->examplePath = examplePathlineEdit->text();
	m_options->pythonDir = pythonDirLineEdit->text();
	m_options->pythonExecutable = pythonExecutableLineEdit->text();
	m_options->csoundExecutable = csoundExecutableLineEdit->text();
	m_options->logFile = logFileLineEdit->text();
	m_options->sdkDir = sdkLineEdit->text();
	m_options->templateDir = templateLineEdit->text();

	m_options->terminal = TerminalLineEdit->text();
	m_options->browser = browserLineEdit->text();
	m_options->dot = dotLineEdit->text();
	m_options->waveeditor = WaveEditorLineEdit->text();
	m_options->waveplayer = WavePlayerLineEdit->text();
	m_options->pdfviewer = pdfViewerLineEdit->text();
	m_options->language = languageComboBox->currentIndex();

	m_options->csdTemplate = templateTextEdit->toPlainText();
    m_options->checkSyntaxBeforeRun = checkSyntaxBeforeRunCheckBox->isChecked();
    m_options->highlightScore = highlightScoreCheckBox->isChecked();

	//  emit(changeFont());
	QDialog::accept();
}

void ConfigDialog::browseInputFilename()
{
	browseSaveFile(m_options->fileInputFilename);
	InputFilenameLineEdit->setText(m_options->fileInputFilename);
}

void ConfigDialog::browseOutputFilename()
{
	browseSaveFile(m_options->fileOutputFilename);
	OutputFilenameLineEdit->setText(m_options->fileOutputFilename);
}

void ConfigDialog::browseCsdocdir()
{
	browseDir(m_options->csdocdir);
	CsdocdirLineEdit->setText(m_options->csdocdir);
}


void ConfigDialog::browseOpcode7dir64()
{
	browseDir(m_options->opcode7dir64);
    Opcode7dir64LineEdit->setText(m_options->opcode7dir64);
}

void ConfigDialog::browseSadir()
{
	browseDir(m_options->sadir);
	SadirLineEdit->setText(m_options->sadir);
}

void ConfigDialog::browseSsdir()
{
	browseDir(m_options->ssdir);
	SsdirLineEdit->setText(m_options->ssdir);
}

void ConfigDialog::browseSfdir()
{
	browseDir(m_options->sfdir);
	SfdirLineEdit->setText(m_options->sfdir);
}

void ConfigDialog::browseIncdir()
{
	browseDir(m_options->incdir);
	IncdirLineEdit->setText(m_options->incdir);
}

void ConfigDialog::browseRawWaveDir()
{
	browseDir(m_options->rawWave);
	rawWaveLineEdit->setText(m_options->rawWave);
}

void ConfigDialog::browseCsoundExecutable()
{
	browseFile(m_options->csoundExecutable);
	csoundExecutableLineEdit->setText(m_options->csoundExecutable);
}

void ConfigDialog::browsePythonExecutable()
{
    browseFile(m_options->pythonExecutable);
    pythonExecutableLineEdit->setText(m_options->pythonExecutable);
}


//void ConfigDialog::browseDefaultCsd()
//{
//  browseFile(m_options->defaultCsd);
//  if (!m_options->defaultCsd.endsWith(".csd")) {
//    QMessageBox::warning(this, tr("CsoundQt"),
//                         tr("Only files with extension .csd are accepted"));
//    return;
//  }
//  defaultCsdLineEdit->setText(m_options->defaultCsd);
//}

void ConfigDialog::browseFavorite()
{
	browseDir(m_options->favoriteDir);
	favoriteLineEdit->setText(m_options->favoriteDir);
}

void ConfigDialog::browseExamplePath()
{
    browseDir(m_options->examplePath);
    examplePathlineEdit->setText(m_options->examplePath);
}

void ConfigDialog::browseTerminal()
{
	browseFile(m_options->terminal);
	TerminalLineEdit->setText(m_options->terminal);
}

void ConfigDialog::browseBrowser()
{
	browseFile(m_options->browser);
	browserLineEdit->setText(m_options->browser);
}

void ConfigDialog::browseDot()
{
	browseFile(m_options->dot);
	dotLineEdit->setText(m_options->dot);
}

void ConfigDialog::browseWaveEditor()
{
	browseFile(m_options->waveeditor);
	WaveEditorLineEdit->setText(m_options->waveeditor);
}

void ConfigDialog::browseWavePlayer()
{
	browseFile(m_options->waveplayer);
	WavePlayerLineEdit->setText(m_options->waveplayer);
}

void ConfigDialog::browsePdfViewer()
{
	browseFile(m_options->pdfviewer);
	pdfViewerLineEdit->setText(m_options->pdfviewer);
}

void ConfigDialog::browsePythonDir()
{
	browseDir(m_options->pythonDir);
	pythonDirLineEdit->setText(m_options->pythonDir);
}

void ConfigDialog::browseLogFile()
{
	browseSaveFile(m_options->logFile);
	logFileLineEdit->setText(m_options->logFile);
}

void ConfigDialog::browseSdkDir()
{
	browseDir(m_options->sdkDir);
	sdkLineEdit->setText(m_options->sdkDir);
}

void ConfigDialog::browseTemplateDir()
{
	browseDir(m_options->templateDir);
	templateLineEdit->setText(m_options->templateDir);
}

void ConfigDialog::selectAudioInput()
{
    QList<QStringPair> deviceList = m_configlists->getAudioInputDevices(
				RtModuleComboBox->itemData(RtModuleComboBox->currentIndex()).toString());
    // QMenu menu(this);
    QMenu menu(audioInputToolButton);
	QVector<QAction*> actions;

    QString module = RtModuleComboBox->currentText();
    if(module == "jack") {
        deviceList.prepend(QStringPair("adc", "adc"));
        deviceList.prepend(QStringPair("Do Not Autoconnect", "adc:null"));
        deviceList.prepend(QStringPair("System Inputs", "adc:system:capture_"));
    } else {
        deviceList.prepend(QStringPair("Default", "adc"));
    }

    deviceList.prepend(QStringPair("No Input", ""));

    auto currentSelection = RtInputLineEdit->text();
	for (int i = 0; i < deviceList.size(); i++) {
        auto option = !deviceList[i].second.isEmpty() ?
                    deviceList[i].first + "  (" + deviceList[i].second + ")":
                    deviceList[i].first;
        if(currentSelection == deviceList[i].second)
            option.prepend("> ");
        auto action = menu.addAction(option);
        actions.append(action);
	}

    auto pos = audioInputToolButton->mapToGlobal(QPoint(0, audioInputToolButton->height()));
    int index = actions.indexOf(menu.exec(pos));
    if (index >= 0)
		RtInputLineEdit->setText(deviceList[index].second);
}

void ConfigDialog::selectAudioOutput()
{
    QList<QStringPair> deviceList = m_configlists->getAudioOutputDevices(
				RtModuleComboBox->itemData(RtModuleComboBox->currentIndex()).toString());
	QMenu menu(this);
	QVector<QAction*> actions;


    QString module = RtModuleComboBox->currentText();
    if(module == "jack") {
        deviceList.prepend(QStringPair("dac", "dac"));
        deviceList.prepend(QStringPair("Do Not Autoconnect", "dac:null"));
        deviceList.prepend(QStringPair("System Outputs", "dac:system:playback_"));
    } else {
        deviceList.prepend(QStringPair("Default", "dac"));
    }

    deviceList.append(QStringPair("No Output", ""));
    auto currentSelection = RtOutputLineEdit->text();

	for (int i = 0; i < deviceList.size(); i++) {
        auto option = !deviceList[i].second.isEmpty() ?
                    deviceList[i].first + "  (" + deviceList[i].second + ")" :
                    deviceList[i].first;
        if(currentSelection == deviceList[i].second)
            option.prepend("> ");
        auto action = menu.addAction(option);
		actions.append(action);
	}

    auto pos = audioOutputToolButton->mapToGlobal(QPoint(0, audioOutputToolButton->height()));
    int index = actions.indexOf(menu.exec(pos));
    if (index >= 0) {
		RtOutputLineEdit->setText(deviceList[index].second);
    }
}

void ConfigDialog::selectMidiInput()
{
	QString module = RtMidiModuleComboBox->currentText();
	QHash<QString, QString> deviceList = m_configlists->getMidiInputDevices(module);

	QMenu menu(this);

	if (module == "jack") {
        deviceList.insert("jack", "jack"); // just to create client in jack
    }

    deviceList.insert("Disabled", "");

	if (module == "portmidi") {
		deviceList.insert("all", "a");
	}

	QHashIterator<QString, QString> i(deviceList);
	while (i.hasNext()) {
		i.next();
		QAction* action =  menu.addAction(i.key() + " (" + i.value() +")");
		action->setData(i.value());
	}

    QPoint pos = midiInputToolButton->mapToGlobal(QPoint(0, midiInputToolButton->height()));
    QAction *selected = menu.exec(pos);

    if (selected) {
		RtMidiInputLineEdit->setText(selected->data().toString());
	}
}

void ConfigDialog::selectMidiOutput()
{
	QString module = RtMidiModuleComboBox->currentText();
    QList<QStringPair> deviceList = m_configlists->getMidiOutputDevices(module);
    QMenu menu(this);
	QVector<QAction*> actions;

    QStringPair device;

	if (module == "jack") {
        // since getMidiInputDevices does not return jack clients yet and empty
        // parametery may crash csound
        deviceList.prepend(QStringPair("jack", "jack"));
    }

    deviceList.append(QStringPair("Disabled", ""));

	for (int i = 0; i < deviceList.size(); i++) {
		QAction* action =  menu.addAction(deviceList[i].first + " (" + deviceList[i].second +")");
		actions.append(action);
	}

    QPoint pos = midiOutputToolButton->mapToGlobal(QPoint(0, midiOutputToolButton->height()));
    int index = actions.indexOf(menu.exec(pos));
    // int index = actions.indexOf(menu.exec(this->mapToGlobal(pos)));
    if (index >= 0)
		RtMidiOutputLineEdit->setText(deviceList[index].second);
}

void ConfigDialog::browseFile(QString &destination)
{
    QString file =  QFileDialog::getOpenFileName(this,tr("Select File"),destination);
	if (file!="")
		destination = file;
}

void ConfigDialog::browseSaveFile(QString &destination)
{
    QString file =  QFileDialog::getSaveFileName(this,tr("Select File"),destination);
	if (file!="")
		destination = file;
}

void ConfigDialog::browseDir(QString &destination)
{
    QString dir =  QFileDialog::getExistingDirectory(this,tr("Select Directory"),destination);
	if (dir!="")
		destination = dir;
}


void ConfigDialog::selectTextColor()
{
	QColor color = QColorDialog::getColor(m_options->consoleFontColor, this);
	if (color.isValid()) {
		m_options->consoleFontColor = color;
		QPixmap pixmap (64,64);
		pixmap.fill(m_options->consoleFontColor);
		consoleFontColorPushButton->setIcon(pixmap);
		QPalette palette(m_options->consoleFontColor);
		consoleFontColorPushButton->setPalette(palette);
	}
}

void ConfigDialog::selectBgColor()
{
	QColor color = QColorDialog::getColor(m_options->consoleBgColor, this);
	if (color.isValid()) {
		m_options->consoleBgColor = color;
		QPixmap pixmap (64,64);
		pixmap.fill(m_options->consoleBgColor);
		consoleBgColorPushButton->setIcon(pixmap);
		QPalette palette(m_options->consoleBgColor);
		consoleBgColorPushButton->setPalette(palette);
	}
}


void ConfigDialog::selectEditorBgColor()
{
	QColor color = QColorDialog::getColor(m_options->editorBgColor, this);
	if (color.isValid()) {
		m_options->editorBgColor = color;
		QPixmap pixmap (64,64);
		pixmap.fill(m_options->editorBgColor);
        // editorBgColorButton->setIcon(pixmap);
		QPalette palette(m_options->editorBgColor);
        // editorBgColorButton->setPalette(palette);
	}
}


void ConfigDialog::clearTemplate()
{
	templateTextEdit->clear();
}

void ConfigDialog::defaultTemplate()
{
	QString defaultText = QCS_DEFAULT_TEMPLATE;
	templateTextEdit->setPlainText(defaultText );
}

void ConfigDialog::on_csoundMidiCheckBox_toggled(bool checked)
{
	if (checked) { // close internal rtmidi
		midiInterfaceComboBox->setEnabled(false);
		midiOutInterfaceComboBox->setEnabled(false);
		qDebug()<<Q_FUNC_INFO<<" closing internal rtmidi now.";
		emit disableInternalRtMidi();
	} else {
		midiInterfaceComboBox->setEnabled(true);
		midiOutInterfaceComboBox->setEnabled(true);
	}
}

void ConfigDialog::checkRtMidiModule(QString module)
{
	if (module=="jack") { // && there is no lists; but now not connected. later: remove this function!
		qDebug()<<Q_FUNC_INFO<<"Setting dummy input and output for jack midi";
		RtMidiInputLineEdit->setText("dummy");
		RtMidiOutputLineEdit->setText("dummy");
	}
}

bool backendSupportsSystemSamplerate(QString module) {
    if(module == "jack"  ||
       module == "auhal")
        return true;
    return false;
}

void ConfigDialog::testAudioSetup() {
    // here we only accept to close the dialog. The actual functionality
    // is called from CsoundQt::configure, where the testAudioSetupButton
    // is connected to CsoundQt::testAudioSetup
    // This is done like this because CsoundQt creates this dialog
    // each time and holds no reference to it.
    this->accept();
}

void ConfigDialog::recommendEnvironmentSettings() {
#ifdef Q_OS_UNIX
    QString default_opcodes64 = "/usr/local/lib/csound/plugins64-6.0:";
#endif
#ifdef Q_OS_WIN
    // What's the default plugin folder in win?
    QString default_opcodes64 = "";
#endif
    auto home = qgetenv("HOME");
    auto dataLocationApp = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0];
    auto dataLocation = QDir(dataLocationApp);
    dataLocation.cdUp();
    if(this->Opcode7dir64LineEdit->text().isEmpty()) {
        auto opcdirenv = qgetenv("OPCODE7DIR64");
        if(!opcdirenv.isEmpty()) {
            // if the user already set OPCODE7DIR64 then leave it as is. This is probably
            // an expert user
            QDEBUG << "$OPCODE7DIR64 is already set to " << opcdirenv;
            this->Opcode7dir64LineEdit->setText(opcdirenv);
            this->Opcode7dir64CheckBox->setChecked(false);
        } else {
            // We set OPCODE7DIR64 to the default + a user writable location
            auto opcdirstr = dataLocation.path() + "/csound6/plugins64";
            auto opcdir = QDir(opcdirstr);
            if(!opcdir.exists()) {
                opcdir.mkpath(".");
            }
            if(!default_opcodes64.isEmpty())
                opcdirstr = default_opcodes64 + opcdirstr;
            this->Opcode7dir64LineEdit->setText(opcdirstr);
            this->Opcode7dir64CheckBox->setChecked(true);
        }
    }
    if(this->IncdirLineEdit->text().isEmpty()) {
        auto incdirenv = qgetenv("INCDIR");
        if(!incdirenv.isEmpty()) {
            // if the user already set it, then leave it as is. This is probably
            // an expert user
            QDEBUG << "$INCDIR is already set to " << incdirenv;
            this->IncdirLineEdit->setText(incdirenv);
            this->IncdirCheckBox->setChecked(false);
        } else {
            // propose an include folder parallel to the plugins folder
            auto incdirstr = dataLocation.path() + "/csound6/udos";
            auto incdir = QDir(incdirstr);
            if(!incdir.exists()) {
                incdir.mkpath(".");
            }
            this->IncdirLineEdit->setText(incdirstr);
            this->IncdirCheckBox->setChecked(true);
        }
    }
}
