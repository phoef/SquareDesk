#-------------------------------------------------
#
# Project created by QtCreator 2016-07-07T21:58:24
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SquareDeskPlayer
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    bass_audio.cpp \
    preferencesdialog.cpp \
    mytablewidget.cpp \
    tablenumberitem.cpp \
    myslider.cpp \
    levelmeter.cpp \
    analogclock.cpp \
    prefsmanager.cpp \
    clickablelabel.cpp \
    songsettings.cpp \
    typetracker.cpp \
    console.cpp \
    renderarea.cpp \
    sdhighlighter.cpp \
    startupwizard.cpp

HEADERS  += mainwindow.h \
    bass.h \
    bass_fx.h \
    bass_audio.h \
    myslider.h \
    bassmix.h \
    preferencesdialog.h \
    utility.h \
    mytablewidget.h \
    tablenumberitem.h \
    levelmeter.h \
    analogclock.h \
    common_enums.h \
    prefs_options.h \
    prefsmanager.h \
    default_colors.h \
    macUtils.h \
    clickablelabel.h \
    typetracker.h \
    console.h \
    renderarea.h \
    common.h \
    sdhighlighter.h \
    startupwizard.h

FORMS    += mainwindow.ui \
    preferencesdialog.ui

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

# NOTE: there is no debug version of libbass
#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -luser32 -lsqlite3
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -luser32 -lsqlite3
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -luser32
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -luser32
else:unix:!macx: LIBS += -L$$PWD/ -lbass -lbass_fx -lbassmix -ltag -lsqlite3

win32 {
    RC_FILE = desk1d.rc
    LIBS += -L$$OUT_PWD/../taglib -ltaglib
    INCLUDEPATH += $$PWD/../taglib/binaries/include
}

# copy the 3 libbass DLLs and the allcalls.csv file to the executable directory (DEBUG ONLY)
win32:CONFIG(debug, debug|release): {
    copydata.commands = xcopy /q /y $$shell_path($$PWD/windll/*.dll) $$shell_path($$OUT_PWD\debug)
    copydata3.commands = xcopy /q /y $$shell_path($$PWD/allcalls.csv) $$shell_path($$OUT_PWD\debug)
    first.depends = $(first) copydata copydata3
    export(first.depends)
    export(copydata.commands)
    export(copydata3.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata3
}

# copy the 3 libbass DLLs and the allcalls.csv file to the executable directory (DEBUG ONLY)
win32:CONFIG(release, debug|release): {
    copydata.commands = xcopy /q /y $$shell_path($$PWD/windll/*.dll) $$shell_path($$OUT_PWD\release)
    copydata3.commands = xcopy /q /y $$shell_path($$PWD/allcalls.csv) $$shell_path($$OUT_PWD\release)
    first.depends = $(first) copydata copydata3
    export(first.depends)
    export(copydata.commands)
    export(copydata3.commands)
    QMAKE_EXTRA_TARGETS += first copydata copydata3
}

macx {
    # LIBBASS, LIBBASS_FX, LIBBASSMIX ---------------
    # http://stackoverflow.com/questions/1361229/using-a-static-library-in-qt-creator
    LIBS += $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    LIBS += -framework CoreFoundation
    LIBS += -framework AppKit
    mylib.path = Contents/MacOS
    mylib.files = $$PWD/libbass.dylib $$PWD/libbass_fx.dylib $$PWD/libbassmix.dylib
    QMAKE_BUNDLE_DATA += mylib

    # TAGLIB ----------------------------------------
    LIBS += -L$$OUT_PWD/../taglib -ltaglib
    INCLUDEPATH += $$PWD/../taglib/binaries/include

    # ZLIB ------------------------------------------
    LIBS += /usr/lib/libz.dylib

    # ICONS, ALLCALLS.CSV ---------------------------
    ICON = $$PWD/desk1d.icns
    DISTFILES += desk1d.icns
    DISTFILES += $$PWD/allcalls.csv  # RESOURCE: list of calls, and which level they are

    # https://forum.qt.io/topic/58926/solved-xcode-7-and-qt-error/2
    # Every time you get this error, do "ls /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/"
    #   in Terminal and change the QMAKE_MAC_SDK variable accordingly.
    QMAKE_MAC_SDK = macosx10.12

    # SD --------------------------------------------
    # Copy the sd executable and the sd_calls.dat data file to the same place as the sd executable
    #  (inside the SquareDeskPlayer.app bundle)
    # This way, it's easy for SDP to find the executable for sd, and it's easy for SDP to start up sd.
    copydata1.commands = $(COPY_DIR) $$PWD/../sd/sd_calls.dat $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata2.commands = $(COPY_DIR) $$OUT_PWD/../sd/sd $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata3.commands = $(COPY_DIR) $$PWD/allcalls.csv $$OUT_PWD/SquareDeskPlayer.app/Contents/Resources

    # PS --------------------------------------------
    # SEE the postBuildStepMacOS for a description of how pocketsphinx is modified for embedding.
    #   https://github.com/auriamg/macdylibbundler  <-- BEST, and the one I used
    #   https://doc.qt.io/archives/qq/qq09-mac-deployment.html
    #   http://stackoverflow.com/questions/1596945/building-osx-app-bundle
    #   http://www.chilkatforum.com/questions/4235/how-to-distribute-a-dylib-with-a-mac-os-x-application
    #   http://stackoverflow.com/questions/2092378/macosx-how-to-collect-dependencies-into-a-local-bundle

    # Copy the ps executable and the libraries it depends on (into the SquareDeskPlayer.app bundle)
    # ***** WARNING: the path to pocketsphinx source files is specific to my particular laptop! *****
    copydata4.commands = $(COPY_DIR) /Users/mpogue/Documents/QtProjects/SquareDeskPlayer/SquareDesk-DEV/pocketsphinx/binaries/macosx_yosemite/exe/pocketsphinx_continuous $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata5.commands = $(COPY_DIR) /Users/mpogue/Documents/QtProjects/SquareDeskPlayer/SquareDesk-DEV/pocketsphinx/binaries/macosx_yosemite/libs $$OUT_PWD/SquareDeskPlayer.app/Contents

    copydata6a.commands = $(MKDIR) $$OUT_PWD/SquareDeskPlayer.app/Contents/models/en-us
    copydata6b.commands = $(COPY_DIR) /Users/mpogue/Documents/QtProjects/SquareDeskPlayer/SquareDesk-DEV/pocketsphinx/binaries/macosx_yosemite/models/en-us $$OUT_PWD/SquareDeskPlayer.app/Contents/models

    # SQUAREDESK-SPECIFIC DICTIONARY, LANGUAGE MODEL --------------------------------------------
    copydata7.commands = $(COPY_DIR) $$PWD/5365a.dic $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS
    copydata8.commands = $(COPY_DIR) $$PWD/plus.jsgf $$OUT_PWD/SquareDeskPlayer.app/Contents/MacOS

    first.depends = $(first) copydata1 copydata2 copydata3 copydata4 copydata5 copydata6a copydata6b copydata7 copydata8

    export(first.depends)
    export(copydata1.commands)
    export(copydata2.commands)
    export(copydata3.commands)
    export(copydata4.commands)
    export(copydata5.commands)
    export(copydata6a.commands)
    export(copydata6b.commands)
    export(copydata7.commands)
    export(copydata8.commands)

    QMAKE_EXTRA_TARGETS += first copydata1 copydata2 copydata3 copydata4 copydata5 copydata6a copydata6b copydata7 copydata8
}

win32 {
    # PS --------------------------------------------
    # Copy the ps executable and the libraries it depends on (into the SquareDeskPlayer.app bundle)
    # ***** WARNING: the path to pocketsphinx source files is specific to my particular laptop! *****
    copydata4.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/pocketsphinx_continuous.exe) $$shell_path($$OUT_PWD/release)
    copydata5.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/pocketsphinx.dll) $$shell_path($$OUT_PWD/release)
    copydata6.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/exe/sphinxbase.dll) $$shell_path($$OUT_PWD/release)

    copydata7.commands = if not exist $$shell_path($$OUT_PWD/release/models/en-us) mkdir $$shell_path($$OUT_PWD/release/models/en-us)
    # NOTE: The models used for Win32 PocketSphinx are NOT the same as the Mac OS X models.
    copydata8.commands = xcopy /q /y $$shell_path($$PWD/../pocketsphinx/binaries/win32/models/en-us/*) $$shell_path($$OUT_PWD/release/models/en-us)

    # SQUAREDESK-SPECIFIC DICTIONARY, LANGUAGE MODEL --------------------------------------------
    copydata9.commands = xcopy /q /y $$shell_path($$PWD/5365a.dic) $$shell_path($$OUT_PWD/release)
    copydata10.commands = xcopy /q /y $$shell_path($$PWD/plus.jsgf) $$shell_path($$OUT_PWD/release)

    first.depends = $(first) copydata4 copydata5 copydata6 copydata7 copydata8 copydata9 copydata10

    export(first.depends)
    export(copydata4.commands)
    export(copydata5.commands)
    export(copydata6.commands)
    export(copydata7.commands)
    export(copydata8.commands)
    export(copydata9.commands)
    export(copydata10.commands)

    QMAKE_EXTRA_TARGETS += first copydata4 copydata5 copydata6 copydata7 copydata8 copydata9 copydata10
}

RESOURCES += resources.qrc
RESOURCES += startupwizard.qrc


#DISTFILES += \
#    README.txt

OBJECTIVE_SOURCES += \
    macUtils.mm

DISTFILES += \
    LICENSE.GPL3 \
    LICENSE.GPL2

CONFIG += c++11

