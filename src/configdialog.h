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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "ui_configdialog.h"

class CsoundQt;
class Options;
class ConfigLists;

class ConfigDialog : public QDialog, public Ui::ConfigDialog
{
	Q_OBJECT
public:
	ConfigDialog(CsoundQt *parent, Options *options, ConfigLists *configlists);

	~ConfigDialog();
	int currentTab();
	void setCurrentTab(int index);


public slots:
	void warnOpcodeDir(bool on);

private:
	CsoundQt* m_parent;
	Options *m_options;
	ConfigLists *m_configlists;
	void browseFile(QString &destination);
	void browseSaveFile(QString &destination);
	void browseDir(QString &destination);
    int m_selectedOutputDeviceIndex;

private slots:
	virtual void accept();
	void browseInputFilename();
	void browseOutputFilename();
	void browseCsdocdir();
    void browseOpcode7dir64();
	void browseSadir();
	void browseSsdir();
	void browseSfdir();
	void browseIncdir();
	void browseRawWaveDir();
	//    void browseDefaultCsd();
	void browseFavorite();
    void browseExamplePath();
	void browsePythonDir();
	void browseCsoundExecutable();
    void browsePythonExecutable();
	void browseLogFile();
	void browseSdkDir();
	void browseTemplateDir();
	void browseTerminal();
	void browseBrowser();
	void browseDot();
	void browseWaveEditor();
	void browseWavePlayer();
	void browsePdfViewer();
	void selectAudioInput();
	void selectAudioOutput();
	void selectMidiInput();
	void selectMidiOutput();
	void selectTextColor();
	void selectBgColor();
	void selectEditorBgColor();
	void clearTemplate();
	void defaultTemplate();

	void on_csoundMidiCheckBox_toggled(bool checked);
	void checkRtMidiModule(QString module);
    void testAudioSetup();
    void recommendEnvironmentSettings();

    void onRtModuleComboBoxChanged(int index);



signals:
	void disableInternalRtMidi();
	//    void changeFont();

};

#endif
