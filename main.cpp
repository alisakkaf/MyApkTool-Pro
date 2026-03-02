/**
 * =====================================================================
 *  MyApkTool Pro — Main Entry Point
 * =====================================================================
 *  Developer  : Ali Sakkaf  (since 2016)
 *  Website    : https://mysterious-dev.com/
 *  Facebook   : https://www.facebook.com/AliSakkaf.Dev/
 *  GitHub     : https://github.com/alisakkaf
 *  Copyright  : © 2016 - 2026 Ali Sakkaf. All rights reserved.
 * =====================================================================
 **/

#include "mainwindow.h"
#include "splashscreen.h"
#include "version.h"
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QEventLoop>
#include <QScreen>

int main(int argc, char *argv[])
{
    // High DPI support for modern screens
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION_STR);
    a.setOrganizationName(APP_COMPANY);
    a.setWindowIcon(QIcon(":/Resources/icon.ico"));

    // ── Animated Splash Screen ──────────────────────────────────
    SplashScreen *splash = new SplashScreen();
    splash->show();
    a.processEvents();

    // Wait for splash signal or 5 seconds (whichever comes first)
    {
        QEventLoop loop;
        QObject::connect(splash, &SplashScreen::readyToClose, &loop, &QEventLoop::quit);
        loop.exec();
    }

    splash->close();
    splash->deleteLater();

    // ── Main Window ─────────────────────────────────────────────
    MainWindow w;
    w.show();

    return a.exec();
}
