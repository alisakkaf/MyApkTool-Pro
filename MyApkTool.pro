#/**
# * =====================================================================
# *  MyApkTool Pro — Central Version & Identity Configuration
# * =====================================================================
# *  Developer  : Ali Sakkaf
# *  Website    : https://mysterious-dev.com/
# *  Facebook   : https://www.facebook.com/AliSakkaf.Dev/
# *  GitHub     : https://github.com/alisakkaf
# *  Copyright  : © 2016 - 2026 Ali Sakkaf. All rights reserved.
# * =====================================================================
# *
# *  SINGLE SOURCE OF TRUTH — Everything reads from this file.
# *  Change any value here and it propagates automatically to:
# *    • SplashScreen        (splashscreen.h)
# *    • Main Window title   (mainwindow.cpp)
# *    • QApplication meta   (main.cpp)
# *    • Windows EXE details (app.rc)  → File Properties / Details tab
# * =====================================================================
# **/


QT       += core gui widgets network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = MyApkTool
TEMPLATE = app

# C++14 standard and large resources support
CONFIG += c++14 resources_big

# Static link for Qt 5.14.1 (when building with static Qt kit)
# CONFIG += static   <-- uncomment this only when using a static Qt build kit

DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += resources_big

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    resourcemanager.cpp \
    settingsmanager.cpp \
    adbmanager.cpp \
    apkmanager.cpp \
    customdialog.cpp \
    keystoredialog.cpp \
    networkscandialog.cpp

HEADERS += \
    mainwindow.h \
    resourcemanager.h \
    settingsmanager.h \
    adbmanager.h \
    apkmanager.h \
    customdialog.h \
    keystoredialog.h \
    splashscreen.h \
    networkscandialog.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Windows icon
RC_ICONS = Resources/icon.ico

# Output & build directories
DESTDIR = $$PWD/build
OBJECTS_DIR = $$PWD/build/.obj
MOC_DIR     = $$PWD/build/.moc
RCC_DIR     = $$PWD/build/.rcc
UI_DIR      = $$PWD/build/.ui
