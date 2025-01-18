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

#include "options.h"
// #include "configlists.h"
#include "types.h"
#include "stdlib.h"

Options::Options(ConfigLists *configlists) :
    CsoundOptions(configlists)
{
#ifdef Q_OS_MACOS
    font = "Menlo";
#elif defined(Q_OS_WIN)
    font = "Consolas";
#else
    font = "Monospace";
#endif
    theme = "breeze";
    fontPointSize = 10;
    showLineNumberArea = true;
    lineEnding = 0; // 0=Unix (LF) 1=Windows(CR+LF)
    consoleFont = "Courier";
    consoleFontPointSize = 9;
    consoleFontColor = Qt::black;
    consoleBgColor = Qt::white;
    editorBgColor = Qt::white;

    tabWidth = 4;
    colorVariables = true;
    highlightingTheme = "light";
    autoPlay = true;
    autoJoin = false;
	midiCcToCurrentPageOnly = false;
    saveChanges = true;
    askIfTemporary = false;
    rememberFile = true;
    saveWidgets = true;
    // iconText = true;
    iconText = false;
    showToolbar = true;
    lockToolbar = false;
    toolbarIconSize = 20;
    wrapLines = true;
    autoComplete = true;

    showWidgetsOnRun = true;
    showTooltips = false;
    terminalFLTK = false;
    oldFormat = false;  // Store old MacCsound widget format
    openProperties = true;  // Open properties automatically when creating a widget
    fontScaling = 1.0;
    fontOffset = 0.0;
    graphUpdateRate = 30;

    useAPI = true;
    enableWidgets = true;
    widgetsIndependent = false;  // Widget layouts in Separate Window (instead of Dock Widget)
    keyRepeat = true;
    debugLiveEvents = false;
    consoleBufferSize = 1024;
    midiInterface = 0; // For internal CsoundQt MIDI control
    midiOutInterface = 0; // For internal CsoundQt MIDI control

    rtMidiApi = 0;

    csdPath = "./"; //path of active csd needed for setting -o -i paths

    menuDepth = 3;
    defaultCsd = "";
    defaultCsdActive = false;
    opcodexmldir = "";
    opcodexmldirActive = false;
    favoriteDir = "";
    examplePath = "";
    pythonDir = "";
    pythonExecutable = "python3";
#ifdef Q_OS_MACOS
    csoundExecutable = "/usr/local/bin/csound ";
#else
    csoundExecutable = "csound ";
#endif
    logFile = ""; // "log.txt"; - don't log by default...
    sdkDir = "";
    templateDir = "";
    evalLanguage = CS_ORC;

#ifdef USE_WEBENGINE
    debugPort = 34711;
#endif

    // External applications
    terminal = "";
    browser = "";
    dot = "";
    waveeditor = "";
    waveplayer = "";
    pdfviewer = "";
    language = 0;  // Interface language

    csdTemplate = CSQT_DEFAULT_TEMPLATE ; // was: ""

    // Csound Utilities options
    cvInputName = "input.wav";
    cvOutputName = "input.cv";
    cvSampleRate = "44100";
    cvBeginTime = "0.0";
    cvDuration = "0.0";
    cvChannels = "";

    hetInputName = "input.wav";
    hetOutputName = "output.het";
    hetSampleRate = "44100";
    hetChannel = "1";
    hetBeginTime = "0.0";
    hetDuration = "0.0";
    hetStartFrequency = "100.0";
    hetNumPartials = "10";
    hetMaxAmplitude = "32767";
    hetMinAplitude = "64";
    hetNumBreakPoints = "256";
    hetFilterCutoff = "0";

    lpInputName = "input.wav";
    lpOutputName = "output.lp";
    lpSampleRate ="44100" ;
    lpChannel = "1";
    lpBeginTime = "0.0";
    lpDuration = "0.0";
    lpNumPoles = "34";
    lpHopSize = "200";
    lpLowestFreq = "";
    lpVerbosity = 0;
    lpAlternateStorage = true;

    pvInputName = "input.wav";
    pvOutputName = "output.pvx";
    pvSampleRate = "";
    pvChannel = "1";
    pvBeginTime = "0.0";
    pvDuration = "0.0";
    pvFrameSize = "1024";
    pvOverlap = "4";
    pvWindow = 0;
    pvBeta = "6.4";

    atsInputName = "input.wav";
    atsOutputName = "output.ats";
    atsBeginTime = "0.0";
    atsEndTime = "0.0";
    atsLowestFreq = "20";
    atsHighestFreq = "20000";
    atsFreqDeviat = "0.1";
    atsWinCycle = "4";
    atsHopSize = "0.25";
    atsLowestMag = "-60";
    atsTrackLen = "3";
    atsMinSegLen = "3";
    atsMinGapLen = "3";
    atsSmrThresh = "30";
    atsLastPkCon = "0.0";
    atsSmrContr = "0.5";
    atsFileType = 3;
    atsWindow = 1;
    }


Options::~Options()
{
}
