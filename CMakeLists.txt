cmake_minimum_required(VERSION 3.5)

project(CsoundQt VERSION 0.1 LANGUAGES CXX)

option(CSQT_BUILD_DEBUGGER_SUPPORT "Build Debugger Support" OFF)
option(CSQT_BUILD_RECORDING_SUPPORT "Build recording support" ON)
option(CSQT_BUILD_RTMIDI_SUPPORT "Build Internal MIDI (rtmidi) support" ON)
option(CSQT_BUILD_HTML_SUPPORT "Build Html support" ON)


# for user defined options: rename Custom.cmake.ex to Custom.cmake
# uncomment neccessary variable and make changes according to your system
find_file(CUSTOM_CMAKE "Custom.cmake" HINTS ${CMAKE_HOME_DIRECTORY})
if(CUSTOM_CMAKE)
    message(STATUS "Including Custom.cmake file: ${CUSTOM_CMAKE}")
    unset(CSOUND_LIBRARY CACHE) # seems necessary to avoid taking old values from cache, if Custom.cmake has changed
    unset(CSND6_LIBRARY CACHE)
    unset(CSQT_BUILD_DEBUGGER_SUPPORT CACHE)
    unset(CSQT_BUILD_RECORDING_SUPPORT CACHE)
    unset(CSQT_BUILD_RTMIDI_SUPPORT CACHE)
    unset(CSQT_BUILD_HTML_SUPPORT CACHE)
    include(${CUSTOM_CMAKE}) # tundub, et see ei mõjut Windowsis?
else()
    message(STATUS "Not using Custom.cmake file.")
endif()


include("sources.cmake")

# TODOs
# * Build for Windows
# * Build for MacOS
# * Package for MacOS
# * Package for Windows?
# * Linux installation
# * Transaltions
# * HTML version

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent Network PrintSupport QuickWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent Network Xml PrintSupport QuickWidgets LinguistTools)

# Validate Qt version
if(QT_VERSION_MAJOR EQUAL 5)
  if(QT_VERSION_MINOR LESS_EQUAL 8)
    message(FATAL_ERROR "Only QT version > 5.8 is supported")
  endif(QT_VERSION_MINOR LESS_EQUAL 8)
else()
  message(FATAL_ERROR "Only Qt5 is currently supported")
endif(QT_VERSION_MAJOR EQUAL 5)



set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/translations")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(CsoundQt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET CsoundQt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(CsoundQt SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(CsoundQt
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(CsoundQt PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  # Qt${QT_VERSION_MAJOR}::Concurrent
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Xml
  Qt${QT_VERSION_MAJOR}::PrintSupport
  Qt${QT_VERSION_MAJOR}::QuickWidgets
)

if(CSQT_BUILD_DEBUGGER_SUPPORT)
  target_compile_definitions(CsoundQt PRIVATE QCS_DEBUGGER)
  message("Building debugger support.")
endif(CSQT_BUILD_DEBUGGER_SUPPORT)

if(CSQT_BUILD_RECORDING_SUPPORT)
  target_compile_definitions(CsoundQt PRIVATE PERFTHREAD_RECORD)
  message("Building recording support.")
endif(CSQT_BUILD_RECORDING_SUPPORT)


# RTMIDI -  the sources should be in ./rtmidi, pulled with git submodules
# git submodule update --init --recursive (first time) or git submodule update --recursive (updating)
if(CSQT_BUILD_RTMIDI_SUPPORT)
    target_compile_definitions(CsoundQt PRIVATE QCS_RTMIDI) # TODO: perhaps CSQT_RTMIDI? let's change naming conventions from qutecsound to CsoundQt in v7

    set(RTMIDI_DIR "${CMAKE_HOME_DIRECTORY}/rtmidi")
    set(HEADERS "${RTMIDI_DIR}/RtMidi.h")
    set(SOURCES "${RTMIDI_DIR}/RtMidi.cpp")

    target_sources(CsoundQt PRIVATE ${HEADERS} ${SOURCES})
    message("Building internal MIDI (rtmidi) support.")
endif(CSQT_BUILD_RTMIDI_SUPPORT)


if(CSQT_BUILD_HTML_SUPPORT)
    message(STATUS "Building HTML support")
    target_compile_definitions(CsoundQt PRIVATE QCS_QTHTML USE_WEBENGINE) # TODO: change QCS_ to CSQT later in entire code

    target_sources(CsoundQt PRIVATE
        src/csoundhtmlview.cpp
        src/csoundhtmlwrapper.cpp
        src/CsoundHtmlOnlyWrapper.cpp
    )

    #target_include_directories(CsoundQt PRIVATE src)

    # # Headers are typically handled by the IDE, but you can also explicitly list them if needed
    # set(HEADERS
    #     src/csoundhtmlview.h
    #     src/csoundhtmlwrapper.h
    #     src/CsoundHtmlOnlyWrapper.h
    # )

    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network WebEngineWidgets WebChannel)
    target_link_libraries(CsoundQt PRIVATE Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::WebEngineWidgets Qt${QT_VERSION_MAJOR}::WebChannel)
endif()




# LINUX definition
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX YES)
    message(STATUS "LINUX operating system found")
else()
    set(LINUX NO)
endif()


#link to Csound
if (WIN32) # or rather MSVC?

    # in Csound7 PerormanceThread should be in the main Csound library
# maby needs also linking to pthreadVC2.lib and sndfile Mega-Nerd\libsndfile\bin


    if(DEFINED CSOUND_LIBRARY_PATH) # user define in custom.cmake
        set(CSOUND_LIBRARY_PATHS ${CSOUND_LIBRARY_PATH})
    else()
        set(CSOUND_LIBRARY_PATHS
            "c:\\Program Files\\Csound6_x64\\lib"
        )
    endif()


    find_library(CSOUND_LIBRARY csound64
      HINTS  "c:\\Program Files\\Csound6_x64\\lib"
      PATHS ${CSOUND_LIBRARY_PATHS}
      REQUIRED)

    #set(EXTERNAL_LIBS "c:\\Program Files\\Csound6_x64\\lib\\sndfile.lib" )

    if(DEFINED CSOUND_INCLUDE_PATH) # in custom.cmake
        set(CSOUND_INCLUDE_PATHS ${CSOUND_INCLUDE_PATH})
    else()
        set(CSOUND_INCLUDE_PATHS
            "C:\\Program Files\\Csound6_x64\\include\\csound"
        )
    endif()

    target_link_libraries(CsoundQt PRIVATE ${CSOUND_LIBRARY} )
    target_include_directories(CsoundQt PRIVATE ${CSOUND_INCLUDE_PATHS} ${RTMIDI_DIR})

elseif(APPLE)

  #LCSOUND = -F$${HOME_DIRECTORY}/Library/Frameworks -F/Library/Frameworks -F/usr/local/opt/csound/Frameworks -F/opt/homebrew/opt/csound/Frameworks -framework $${MAC_LIB}


elseif(LINUX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread") # seems necessary, see https://stackoverflow.com/questions/34143265/undefined-reference-to-symbol-pthread-createglibc-2-2-5



  if(DEFINED CSOUND_LIBRARY_PATH) # user define in custom.cmake
      set(CSOUND_LIBRARY_PATHS ${CSOUND_LIBRARY_PATH})
  else()
      set(CSOUND_LIBRARY_PATHS
          /usr/local/lib
          /usr/lib
          /usr/lib64
      )
  endif()

  #message(STATUS "Csound library paths: ${CSOUND_LIBRARY_PATHS}")

  find_library(CSOUND_LIBRARY csound64
    HINTS ${CSOUND_LIBRARY_PATH}
    PATHS ${CSOUND_LIBRARY_PATHS}
    REQUIRED)

  find_library(CSND6_LIBRARY csnd6
    PATHS ${CSOUND_LIBRARY_PATHS}
    REQUIRED)


    if(DEFINED CSOUND_INCLUDE_PATH) # in custom.cmake
        set(CSOUND_INCLUDE_PATHS ${CSOUND_INCLUDE_PATH})
    else()
        set(CSOUND_INCLUDE_PATHS
            /usr/local/include/csound
            /usr/include/csound
        )
    endif()


  if(CSQT_BUILD_RTMIDI_SUPPORT)
      target_compile_definitions(CsoundQt PRIVATE __LINUX_ALSASEQ__ __LINUX_ALSA__)

      # Link against the ALSA library
      target_link_libraries(CsoundQt PRIVATE asound)

      # Check if the JACK library exists in common paths
      find_library(JACK_LIB NAMES jack HINTS /usr/lib64 /usr/lib /usr/local/lib)

      if(JACK_LIB)
          message(STATUS "FOUND JACK")
          target_compile_definitions(CsoundQt PRIVATE __UNIX_JACK__)
          target_link_libraries(CsoundQt PRIVATE ${JACK_LIB})
      endif()
  endif()

  target_link_libraries(CsoundQt PRIVATE ${CSOUND_LIBRARY} ${CSND6_LIBRARY})
  target_include_directories(CsoundQt PRIVATE ${CSOUND_INCLUDE_PATHS} ${RTMIDI_DIR})

else()
  message(FATAL_ERROR "Unsupported platform")
endif()



if(CSOUND_LIBRARY)
  message(STATUS "Csound library found at: ${CSOUND_LIBRARY}")
else()
  message(FATAL_ERROR "Csound library not found.")
endif()


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.csound.CsoundQt)
endif()

set_target_properties(CsoundQt PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 5)
  target_compile_definitions(CsoundQt PRIVATE
    USE_QT_GT_53 USE_QT_GT_54 USE_QT_GT_55 USE_QT_GT_58)
  #FIXME translations don't appear to be working right now?
  qt5_add_translation(CsoundQt ${TS_FILES})

else()
  # TODO complete when adding support for Qt 6
endif(QT_VERSION_MAJOR EQUAL 5)

include(GNUInstallDirs)
install(TARGETS CsoundQt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CsoundQt)
endif()

# for testing only:

get_target_property(INCLUDE_DIRS CsoundQt INCLUDE_DIRECTORIES)

if(INCLUDE_DIRS)
    message(STATUS "Include paths for target 'CsoundQt':")
    foreach(INCLUDE_DIR ${INCLUDE_DIRS})
        message(STATUS "    ${INCLUDE_DIR}")
    endforeach()
else()
    message(STATUS "No include directories found for target 'CsoundQt'.")
endif()

get_target_property(LINK_LIBS CsoundQt LINK_LIBRARIES)
if(LINK_LIBS)
    message(STATUS "Linked libraries for target 'CsoundQt':")
    foreach(LIB ${LINK_LIBS})
        message(STATUS "    ${LIB}")
    endforeach()
else()
    message(STATUS "No libraries linked to target 'CsoundQt'.")
endif()

