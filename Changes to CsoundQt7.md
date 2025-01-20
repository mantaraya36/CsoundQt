# Changes to CsoundQt7

CsoundQt 1.x stays as bugfix series of the current CsoundQt,  to work with Csound 6. It is based on Qt 5 but could be built also with Qt6 from the Qt6 branch. The main branch is `develop`. `master` branch is at the moment of releases of CsoundQt 1.x

CsoundQt 7 is based and requires Qt6 and Csound 7. The main branch is `csoundqt7`. Let's do development  in separate branches  or via pull requests and merge when it feels right and ready enough.

## Main changes from the user's view
- support for Csound 7 (no highlighting changes yet though)
- dropped PythonQt
- dropped Live Event Sheets (as there is now  live evaluation of code and Scratch Pad)
- new icon and splash screen
-  more to come

## Building
- pull from branch csoundqt7 https://github.com/CsoundQt/CsoundQt/tree/csoundqt7
-  cmake support is in progress but not ready yet
-  if Csound 7 is not installed in usual path, use config.user.pri (in case of qmake build ) to set it. Like:  
```
CSOUND_LIBRARY_DIR=~/.local/lib
CSOUND_INCLUDE_DIR =~/.local/include/csound/
```
- Use Qt 6 (tested with Qt 6.5)
-  new option to  build with html support is `COFIG+=html_support` (was: html_webengine or html_webkit before). Support for webkit is dropped now.

## Changes in the code

- About porting to Qt6 see branch https://github.com/CsoundQt/CsoundQt/tree/Qt6
-  About Csound 7 related changes (a lot) see branch https://github.com/CsoundQt/CsoundQt/tree/csound7
-  Initial work on cmake cupport: https://github.com/CsoundQt/CsoundQt/tree/cmake_support
-  Tried to get rid of *'qutecsound'* everywhere: 
    - The main source file `qutecsound.cpp/h`is renamed to `csoundqt.cpp/h`
    - macros are renamed from  `QCS_something` to `CSQT_something`
    - settings are handled as `QSettings("csoundqt", "csoundqt")`  (was: "csound", "qutecsound". On Linux it means config file in `~/.config/csoundqt/csoundqt.conf` )
    - Icons are named `csoundqt.png` and `csoundqt.svg`  (was: qtcs.png)
    
- use OPCODE7DIR64 enivormnment variable for the plugins. Dropped support for OPCODEDIR64 and OPCODEDIR
-  to keep CsoundHtmlOnlyWrapper alive, I copied `csound_threaded.hpp`  from Csound 6 sources and made necessary changes to make it work with Csound 7. The local file is `csound_threaded_csqt.hpp` - it is not maintained any more in the Csound 7 sources. In future it is better to get rid of that file and use CsoundEngine instead.
-  fixed most of warnings/problems reported by code analyses 


Due so many changes, CsoundQt might have turned less stable but seems to work.
