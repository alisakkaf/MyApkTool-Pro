/**
 * =====================================================================
 *  MyApkTool Pro — Central Version & Identity Configuration
 * =====================================================================
 *  Developer  : Ali Sakkaf
 *  Since      : 2016
 *  Website    : https://mysterious-dev.com/
 *  Facebook   : https://www.facebook.com/AliSakkaf.Dev/
 *  GitHub     : https://github.com/alisakkaf
 *  Copyright  : © 2016 - 2026 Ali Sakkaf. All rights reserved.
 * =====================================================================
 *
 *  SINGLE SOURCE OF TRUTH — Everything reads from this file.
 *  Change any value here and it propagates automatically to:
 *    • SplashScreen        (splashscreen.h)
 *    • Main Window title   (mainwindow.cpp)
 *    • QApplication meta   (main.cpp)
 *    • Windows EXE details (app.rc)  → File Properties / Details tab
 * =====================================================================
 **/

#pragma once

// ─── Application Identity ────────────────────────────────────────────────────
#define APP_NAME          "MyApkTool Pro"
#define APP_DESCRIPTION   "Professional APK Reverse Engineering Toolkit"
#define APP_COMPANY       "Ali Sakkaf"
#define APP_AUTHOR        "Ali Sakkaf"
#define APP_AUTHOR_GITHUB "https://github.com/alisakkaf"
#define APP_WEBSITE       "https://mysterious-dev.com/"
#define APP_FACEBOOK      "https://www.facebook.com/AliSakkaf.Dev/"
#define APP_COPYRIGHT     "Copyright (c) 2016 - 2026 Ali Sakkaf. All rights reserved."

// ─── Version Numbers ─────────────────────────────────────────────────────────
#define APP_VERSION_MAJOR  1
#define APP_VERSION_MINOR  0
#define APP_VERSION_PATCH  0
#define APP_VERSION_BUILD  0

// Convenience string forms
#define APP_VERSION_STR    "1.0.0"
#define APP_VERSION_FULL   "MyApkTool Pro v1.0.0"

// Windows resource file form  (must be four comma-separated integers)
#define APP_VERSION_COMMA  1,0,0,0
