/**
 * =====================================================================
 *  MyApkTool Pro — Main Window
 * =====================================================================
 *  Developer  : Ali Sakkaf
 *  Facebook   : https://www.facebook.com/AliSakkaf.Dev/
 *  GitHub     : https://github.com/alisakkaf
 *  Website    : https://mysterious-dev.com/
 *  Copyright  : © 2016 - 2026 Ali Sakkaf. All rights reserved.
 * =====================================================================
 **/

#include "mainwindow.h"
#include <QFileDialog>
#include "customdialog.h"
#include "keystoredialog.h"
#include "networkscandialog.h"
#include "ui_mainwindow.h"
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QStandardPaths>
#include <QThread>
#include <QScrollBar>
#include <QDateTime>
#include <QSplashScreen>
#include <QPixmap>
#include <QApplication>
#include <QDir>
#include <QMap>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QFileInfo>
#include "keystoredialog.h"
#include "version.h"

// ──────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 1. Managers
    m_rm = new ResourceManager();          // extracts to AppData
    m_rm->ensureDirectories();
    m_rm->extractAll();

    QString configPath = m_rm->baseDir() + "/config.json";
    m_settings = new SettingsManager(configPath, this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    // 2. Setup UI
    ui->setupUi(this);
    setWindowTitle(QString("%1 – v%2").arg(APP_NAME).arg(APP_VERSION_STR));
    ui->labelTitle->setText(QString("%1 – v%2").arg(APP_NAME).arg(APP_VERSION_STR));
    setWindowIcon(QIcon(":/Resources/icon.png"));
    setMinimumSize(1000, 700);
    resize(1200, 820);

    applyStylesheet();

    // Window controls
    connect(ui->btnMinimize, &QPushButton::clicked, this, &MainWindow::onMinimize);
    connect(ui->btnMaximize, &QPushButton::clicked, this, &MainWindow::onMaximize);
    connect(ui->btnClose, &QPushButton::clicked, this, &MainWindow::onClose);

    // 3. ADB & APK managers
    m_adb = new AdbManager(AdbManager::detectAdbPath(m_rm->baseDir()), this);
    m_apk = new ApkManager(m_rm, m_settings, this);

    // 4. Status bar widgets
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setObjectName("labelStatus");
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setFixedSize(160, 14);
    m_progressBar->setVisible(false);
    ui->statusBar->addWidget(m_statusLabel, 1);
    ui->statusBar->addPermanentWidget(m_progressBar);

    // 5. Wire default finished handler
    reconnectApkFinished();

    // 6. Wire signals
    connect(m_adb, &AdbManager::log,   this, &MainWindow::onLog);
    connect(m_apk, &ApkManager::log,   this, &MainWindow::onLog);

    // 7. Button connections ──────────────────────────────────────
    // Decompile
    connect(ui->btnBrowseApkDecompile, &QPushButton::clicked, this, &MainWindow::onBrowseApkDecompile);
    connect(ui->btnDecompile,          &QPushButton::clicked, this, &MainWindow::onDecompile);
    connect(ui->btnOpenProjectFolder,  &QPushButton::clicked, this, [this]{
        if (!m_currentProjectPath.isEmpty()) openFolder(m_currentProjectPath);
    });
    connect(ui->btnOpenManifest,       &QPushButton::clicked, this, [this]{
        if (!m_currentProjectPath.isEmpty())
            openFolder(m_currentProjectPath + "/AndroidManifest.xml");
    });
    connect(ui->btnCancelDecompile,    &QPushButton::clicked, m_apk, &ApkManager::cancel);

    // Compile
    connect(ui->btnBrowseProjectCompile, &QPushButton::clicked, this, &MainWindow::onBrowseProjectCompile);
    connect(ui->btnCompile,              &QPushButton::clicked, this, &MainWindow::onCompile);
    connect(ui->btnOpenOutput,           &QPushButton::clicked, this, [this]{
        if (!m_lastCompiledApk.isEmpty() && QFile::exists(m_lastCompiledApk)) {
            openFolder(m_lastCompiledApk);
        } else {
            openFolder(m_rm->outputDir());
        }
    });
    connect(ui->btnCancelCompile,        &QPushButton::clicked, m_apk, &ApkManager::cancel);

    // Sign / Zipalign
    connect(ui->btnBrowseApkSign, &QPushButton::clicked, this, &MainWindow::onBrowseApkSign);
    connect(ui->btnSign,          &QPushButton::clicked, this, &MainWindow::onSign);
    connect(ui->btnZipalign,      &QPushButton::clicked, this, &MainWindow::onZipalign);
    connect(ui->btnSignAndAlign,  &QPushButton::clicked, this, &MainWindow::onSignAndAlign);

    // Merge
    connect(ui->btnBrowseMergeInput, &QPushButton::clicked, this, &MainWindow::onBrowseMergeInput);
    connect(ui->btnMerge,            &QPushButton::clicked, this, &MainWindow::onMerge);

    // Baksmali
    connect(ui->btnBrowseDex,       &QPushButton::clicked, this, &MainWindow::onBrowseDex);
    connect(ui->btnBrowseSmaliDir,   &QPushButton::clicked, this, &MainWindow::onBrowseSmaliDir);
    connect(ui->btnBaksmali,         &QPushButton::clicked, this, &MainWindow::onBaksmali);
    connect(ui->btnSmali,            &QPushButton::clicked, this, &MainWindow::onSmali);

    // APK Info
    connect(ui->btnBrowseApkInfo, &QPushButton::clicked, this, &MainWindow::onBrowseApkInfo);
    connect(ui->btnGetApkInfo,    &QPushButton::clicked, this, &MainWindow::onGetApkInfo);

    // ADB
    connect(ui->btnRefreshDevices,  &QPushButton::clicked, this, &MainWindow::onRefreshDevices);
    connect(ui->btnConnectWifi,     &QPushButton::clicked, this, &MainWindow::onConnectWifi);
    connect(ui->btnWifiPair,        &QPushButton::clicked, this, &MainWindow::onWifiPairDialog);
    connect(ui->btnConnectWifi,      &QPushButton::clicked, this, &MainWindow::onConnectWifi);
    connect(ui->btnDisconnect,       &QPushButton::clicked, this, &MainWindow::onDisconnect);
    connect(ui->btnRefreshPackages,  &QPushButton::clicked, this, &MainWindow::onRefreshPackages);
    connect(ui->btnBackupApk,        &QPushButton::clicked, this, &MainWindow::onBackupSelected);
    connect(ui->btnUninstall,        &QPushButton::clicked, this, &MainWindow::onUninstallSelected);
    connect(ui->btnInstallApk,       &QPushButton::clicked, this, &MainWindow::onInstallApk);
    connect(ui->btnPackageInfo,      &QPushButton::clicked, this, &MainWindow::onPackageInfo);
    connect(ui->btnOpenBackupDir,    &QPushButton::clicked, this, &MainWindow::onOpenBackupDir);
    connect(ui->lePackageSearch,     &QLineEdit::textChanged, this, &MainWindow::onPackageFilterChanged);
    connect(ui->cmbDevices, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDeviceSelectionChanged);

    connect(ui->tblPackages, &QTableWidget::itemSelectionChanged, this, [this](){
        int row = ui->tblPackages->currentRow();
        if (row >= 0) {
            QString pkg = ui->tblPackages->item(row, 1)->text();
            ui->lblPackageSelected->setText("Selected: " + pkg);
            ui->lblPackageSelected->setStyleSheet("color: #4ecdc4;");
        } else {
            ui->lblPackageSelected->setText("Selected: None");
            ui->lblPackageSelected->setStyleSheet("");
        }
    });

    connect(ui->cmbPackageFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ onRefreshPackages(); });

    // Settings
    connect(ui->btnBrowseJavaPath,   &QPushButton::clicked, this, &MainWindow::onBrowseJavaPath);
    connect(ui->btnBrowseApktool,    &QPushButton::clicked, this, &MainWindow::onBrowseApktoolPath);
    connect(ui->btnBrowseAapt,       &QPushButton::clicked, this, &MainWindow::onBrowseAaptPath);
    connect(ui->btnBrowseAapt2,      &QPushButton::clicked, this, &MainWindow::onBrowseAapt2Path);
    connect(ui->btnBrowseApksigner,  &QPushButton::clicked, this, &MainWindow::onBrowseApksignerPath);
    connect(ui->btnBrowseKeystore,   &QPushButton::clicked, this, &MainWindow::onBrowseKeystorePath);
    connect(ui->btnCreateKeystore,   &QPushButton::clicked, this, &MainWindow::onCreateKeystore);
    
    // Auto-fill keystore fields when changing selection
    connect(ui->cmbKeystorePath, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        if (text.isEmpty()) return;
        QJsonObject saved = m_settings->settings().savedKeystores;
        if (saved.contains(text)) {
            QJsonObject obj = saved.value(text).toObject();
            ui->leKeystorePass->setText(obj.value("pass").toString());
            ui->leKeyAlias->setText(obj.value("alias").toString());
            ui->leKeyPass->setText(obj.value("keypass").toString());
        }
    });

    connect(ui->btnSaveSettings,     &QPushButton::clicked, this, &MainWindow::onSaveSettings);
    connect(ui->btnResetSettings,    &QPushButton::clicked, this, &MainWindow::onResetSettings);
    connect(ui->btnClearFramework,   &QPushButton::clicked, this, &MainWindow::onClearFramework);
    connect(ui->btnInstallFramework, &QPushButton::clicked, this, &MainWindow::onInstallFramework);

    // Log
    connect(ui->btnClearLog,   &QPushButton::clicked, this, &MainWindow::onClearLog);
    connect(ui->btnCopyLog,    &QPushButton::clicked, this, &MainWindow::onCopyLog);
    connect(ui->btnToggleLog,  &QPushButton::clicked, this, &MainWindow::onToggleLog);

    // Theme
    connect(ui->btnToggleTheme, &QPushButton::clicked, this, &MainWindow::onToggleTheme);

    // Tab change
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // 8. Load settings to UI
    loadSettingsToUI();

    // 9. Welcome log
    appendLog("✓ MyApkTool Pro v1.0 started", "success");
    appendLog(QString("Base dir: %1").arg(m_rm->baseDir()), "info");
    appendLog(QString("Java: %1").arg(m_settings->settings().javaPath), "info");
}

// ──────────────────────────────────────────────────────────────────────────────
MainWindow::~MainWindow()
{
    m_settings->save();
    delete ui;
}

// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::onMinimize() { showMinimized(); }

void MainWindow::onMaximize() {
    if (isMaximized()) showNormal();
    else showMaximized();
}

void MainWindow::onClose() { close(); }

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 52) { // 52 is headerBar height
        m_dragPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton && event->pos().y() < 52) {
        move(event->globalPos() - m_dragPos);
        event->accept();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::reconnectApkFinished()
{
    disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
    connect(m_apk, &ApkManager::finished, this, &MainWindow::onOperationFinished);
}

// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::applyStylesheet()
{
    QString qssFile = m_darkMode ? ":/style.qss" : ":/stylelight.qss";
    QFile f(qssFile);
    if (f.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::loadSettingsToUI()
{
    const AppSettings &s = m_settings->settings();

    // Decompile tab
    ui->chkDecodeResources->setChecked(s.decodeResources);
    ui->chkDecodeSources->setChecked(s.decodeSources);
    ui->chkUseAapt2Decompile->setChecked(s.useAapt2);
    ui->chkClearFramework->setChecked(s.clearFrameworkBefore);
    int decompilerIdx = (s.defaultDecompiler == "apkeditor") ? 1 : 0;
    ui->cmbDecompiler->setCurrentIndex(decompilerIdx);

    // Compile tab
    ui->chkUseAapt2Compile->setChecked(s.useAapt2);
    ui->chkAutoSign->setChecked(s.autoSignAfterCompile);
    ui->chkAutoZipalign->setChecked(s.autoZipalign);
    ui->chkZipalignBefore->setChecked(s.zipalignBeforeSign);

    // Sign tab
    ui->chkV1Sign->setChecked(s.v1Signing);
    ui->chkV2Sign->setChecked(s.v2Signing);
    ui->chkV3Sign->setChecked(s.v3Signing);
    ui->spnAlignment->setValue(s.zipalignAlignment);

    // Settings tab
    ui->leJavaPath->setText(s.javaPath);
    ui->spnHeapSize->setValue(s.heapSize);
    ui->chkSuppressJavaWarn->setChecked(s.suppressJavaWarnings);
    ui->chkUseCustomApktool->setChecked(s.useCustomApktool);
    ui->chkUseCustomAapt->setChecked(s.useCustomAapt);
    ui->leCustomAaptPath->setText(s.customAaptPath);
    ui->chkUseCustomAapt2->setChecked(s.useCustomAapt2);
    ui->leCustomAapt2Path->setText(s.customAapt2Path);
    ui->leApksignerPath->setText(s.apksignerPath);
    ui->chkUseCustomKeystore->setChecked(s.useCustomKeystore);
    
    // Populate keystore Combobox
    QDir keysDir(m_rm->keysDir());
    ui->cmbKeystorePath->clear();
    for (const QString &f : keysDir.entryList({"*.jks", "*.keystore"}, QDir::Files))
        ui->cmbKeystorePath->addItem(f);
    
    if (!s.keystorePath.isEmpty())
        ui->cmbKeystorePath->setCurrentText(s.keystorePath);
    ui->leKeystorePass->setText(s.keystorePass);
    ui->leKeyAlias->setText(s.keyAlias);
    ui->leKeyPass->setText(s.keyPass);

    // Populate custom Apktool combobox
    QDir customDir(m_rm->customToolsDir());
    ui->cmbCustomApktool->clear();
    for (const QString &f : customDir.entryList({"*.jar"}, QDir::Files))
        ui->cmbCustomApktool->addItem(f);
    if (!s.customApktoolPath.isEmpty())
        ui->cmbCustomApktool->setCurrentText(s.customApktoolPath);
}

void MainWindow::saveUIToSettings()
{
    AppSettings &s = m_settings->settings();

    // Decompile
    s.decodeResources    = ui->chkDecodeResources->isChecked();
    s.decodeSources      = ui->chkDecodeSources->isChecked();
    s.useAapt2           = ui->chkUseAapt2Compile->isChecked();
    s.clearFrameworkBefore = ui->chkClearFramework->isChecked();
    s.defaultDecompiler  = (ui->cmbDecompiler->currentIndex() == 1) ? "apkeditor" : "apktool";

    // Compile / sign
    s.autoSignAfterCompile = ui->chkAutoSign->isChecked();
    s.autoZipalign         = ui->chkAutoZipalign->isChecked();
    s.zipalignBeforeSign   = ui->chkZipalignBefore->isChecked();
    s.v1Signing            = ui->chkV1Sign->isChecked();
    s.v2Signing            = ui->chkV2Sign->isChecked();
    s.v3Signing            = ui->chkV3Sign->isChecked();
    s.zipalignAlignment    = ui->spnAlignment->value();

    // Java / settings
    s.javaPath             = ui->leJavaPath->text();
    s.heapSize             = ui->spnHeapSize->value();
    s.suppressJavaWarnings = ui->chkSuppressJavaWarn->isChecked();
    s.useCustomApktool     = ui->chkUseCustomApktool->isChecked();
    s.customApktoolPath    = ui->cmbCustomApktool->currentText();
    s.useCustomAapt        = ui->chkUseCustomAapt->isChecked();
    s.customAaptPath       = ui->leCustomAaptPath->text();
    s.useCustomAapt2       = ui->chkUseCustomAapt2->isChecked();
    s.customAapt2Path      = ui->leCustomAapt2Path->text();
    s.apksignerPath        = ui->leApksignerPath->text();
    s.useCustomKeystore    = ui->chkUseCustomKeystore->isChecked();
    s.keystorePath         = ui->cmbKeystorePath->currentText();
    s.keystorePass         = ui->leKeystorePass->text();
    s.keyAlias             = ui->leKeyAlias->text();
    s.keyPass              = ui->leKeyPass->text();
    
    // Cache keystore credentials for easy toggling
    if (!s.keystorePath.isEmpty() && !s.keyAlias.isEmpty()) {
        QJsonObject ksInfo;
        ksInfo["pass"] = s.keystorePass;
        ksInfo["alias"] = s.keyAlias;
        ksInfo["keypass"] = s.keyPass;
        s.savedKeystores[s.keystorePath] = ksInfo;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Log helpers
// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::appendLog(const QString &msg, const QString &level)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString html;

    QString textColor;
    QString timeColor;

    if (m_darkMode) {
        timeColor = "#8a8a8a"; // رمادي فاتح مريح للوقت
        textColor = "#b0b0d0"; // الافتراضي (Info)
        if (level == "success") textColor = "#69f0ae";      // أخضر فاتح
        else if (level == "error") textColor = "#ff5252";   // أحمر فاتح
        else if (level == "warning") textColor = "#ffca28"; // أصفر
        else if (level == "cmd") textColor = "#80d8ff";     // أزرق سماوي
    } else {
        timeColor = "#6c757d"; // رمادي داكن للوقت
        textColor = "#333333"; // الافتراضي (Info - أسود/رمادي داكن)
        if (level == "success") textColor = "#198754";      // أخضر داكن
        else if (level == "error") textColor = "#dc3545";   // أحمر داكن
        else if (level == "warning") textColor = "#d97706"; // برتقالي داكن (لأن الأصفر العادي لا يُقرأ على الأبيض)
        else if (level == "cmd") textColor = "#0d6efd";     // أزرق داكن
    }

    html = QString("<span style='color:%1;'>[%2]</span> "
                   "<span style='color:%3;'>%4</span>")
               .arg(timeColor, timestamp, textColor, msg.toHtmlEscaped());

    ui->logTextEdit->append(html);

    // التمرير التلقائي للأسفل
    ui->logTextEdit->verticalScrollBar()->setValue(
        ui->logTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::onLog(const QString &message, const QString &level)
{
    appendLog(message, level);
}

void MainWindow::onClearLog()
{
    ui->logTextEdit->clear();
}

void MainWindow::onCopyLog()
{
    QApplication::clipboard()->setText(ui->logTextEdit->toPlainText());
    updateStatusBar("Log copied to clipboard", "info");
}

void MainWindow::onToggleLog()
{
    m_logVisible = !m_logVisible;
    ui->logTextEdit->setVisible(m_logVisible);
    ui->btnToggleLog->setText(m_logVisible ? "⬇ Hide" : "⬆ Show");
}

// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::updateStatusBar(const QString &msg, const QString &level)
{
    Q_UNUSED(level);
    m_statusLabel->setText(msg);
}

void MainWindow::setOperationRunning(bool running, const QString &msg)
{
    m_progressBar->setVisible(running);
    ui->btnCancelDecompile->setEnabled(running);
    ui->btnCancelCompile->setEnabled(running);
    updateStatusBar(running ? (msg.isEmpty() ? "Processing…" : msg) : "Ready");
    QApplication::processEvents();
}

void MainWindow::onOperationFinished(bool success, const QString &resultPath)
{
    setOperationRunning(false);
    if (success)
        appendLog("✓ Operation complete" + (resultPath.isEmpty() ? "" : ": " + resultPath), "success");
    else
        appendLog("✗ Operation failed.", "error");
}

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::openFolder(const QString &path)
{
    if (path.isEmpty()) return;
    QFileInfo fi(path);
    if (!fi.exists()) {
        CustomDialog::showError(this, "Error", "Path does not exist:\n" + path);
        return;
    }
    
    if (fi.isFile()) {
        // Highlight file in Windows Explorer
        QString param = QString("/select,\"%1\"").arg(QDir::toNativeSeparators(path));
        QProcess::startDetached("explorer.exe", {param});
    } else {
        // Open directory
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Tab change
// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::onTabChanged(int index)
{
    // Settings tab index = 7: hide log (matches Python behaviour)
    if (index == 7) {
        if (m_logVisible) { m_logWasVisible = true; onToggleLog(); }
    } else {
        if (!m_logVisible && m_logWasVisible) { onToggleLog(); m_logWasVisible = false; }
    }

    if (!m_currentProjectPath.isEmpty())
        ui->leProjectPathCompile->setText(m_currentProjectPath);
}

// ──────────────────────────────────────────────────────────────────────────────
// Theme
// ──────────────────────────────────────────────────────────────────────────────
void MainWindow::onToggleTheme()
{
    ui->logTextEdit->clear();
    m_darkMode = !m_darkMode;
    applyStylesheet();
    ui->btnToggleTheme->setText(m_darkMode ? "🌗 Theme" : "🌙 Dark");
    appendLog("✓ MyApkTool Professional v1.0 started", "success");

}

// ==============================================================================
// DECOMPILE TAB
// ==============================================================================
void MainWindow::onBrowseApkDecompile()
{
    QString path = QFileDialog::getOpenFileName(this, "Select APK", QString(),
                                                "APK Files (*.apk);;All Files (*)");
    if (!path.isEmpty()) {
        ui->leApkPathDecompile->setText(path);
        m_currentApkPath = path;
    }
}

void MainWindow::onDecompile()
{
    QString apkPath = ui->leApkPathDecompile->text().trimmed();
    if (apkPath.isEmpty()) {
        CustomDialog::showWarning(this, "No APK", "Please select an APK file first.");
        return;
    }
    if (m_apk->isBusy()) {
        CustomDialog::showWarning(this, "Busy", "Another operation is running.");
        return;
    }

    // Sync settings from UI
    AppSettings &s = m_settings->settings();
    s.decodeResources   = ui->chkDecodeResources->isChecked();
    s.decodeSources     = ui->chkDecodeSources->isChecked();
    s.useAapt2          = ui->chkUseAapt2Decompile->isChecked();
    s.clearFrameworkBefore = ui->chkClearFramework->isChecked();
    s.defaultDecompiler = ui->cmbDecompiler->currentIndex() == 1 ? "apkeditor" : "apktool";

    if (s.clearFrameworkBefore) m_apk->clearFramework();

    setOperationRunning(true, "Decompiling…");

    if (s.defaultDecompiler == "apkeditor")
        m_apk->decompileAPKEditor(apkPath);
    else
        m_apk->decompileApktool(apkPath);

    m_currentProjectPath = m_rm->workspaceDir() + "/" +
                           QFileInfo(apkPath).completeBaseName();

    // Reconnect finished to handle decompile-specific results
    disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
    connect(m_apk, &ApkManager::finished, this, [this](bool ok, const QString &path){
        disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
        setOperationRunning(false);
        if (ok && !path.isEmpty()) {
            m_currentProjectPath = path;
            ui->leProjectPathCompile->setText(path);
            appendLog("✓ Decompile done: " + path, "success");
        } else if (!ok) {
            appendLog("✗ Decompile failed.", "error");
        }
        reconnectApkFinished();
    });
}

// ==============================================================================
// COMPILE TAB
// ==============================================================================
void MainWindow::onBrowseProjectCompile()
{
    QString path = QFileDialog::getExistingDirectory(this, "Select Project Folder",
                                                     m_rm->workspaceDir());
    if (!path.isEmpty()) {
        ui->leProjectPathCompile->setText(path);
        m_currentProjectPath = path;
    }
}

void MainWindow::onCompile()
{
    QString folder = ui->leProjectPathCompile->text().trimmed();
    if (folder.isEmpty()) {
        CustomDialog::showWarning(this, "No Project", "Please select a project folder first.");
        return;
    }
    if (m_apk->isBusy()) {
        CustomDialog::showWarning(this, "Busy", "Another operation is running.");
        return;
    }

    AppSettings &s = m_settings->settings();
    s.useAapt2            = ui->chkUseAapt2Compile->isChecked();
    s.autoSignAfterCompile = ui->chkAutoSign->isChecked();
    s.autoZipalign         = ui->chkAutoZipalign->isChecked();
    s.zipalignBeforeSign   = ui->chkZipalignBefore->isChecked();

    setOperationRunning(true, "Compiling…");

    // Reconnect so we get our custom onCompileFinished
    disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
    connect(m_apk, &ApkManager::finished, this, &MainWindow::onCompileFinished);

    QString decompiler = ApkManager::detectDecompiler(folder);
    if (decompiler == "apkeditor")
        m_apk->compileAPKEditor(folder);
    else
        m_apk->compileApktool(folder);
}

void MainWindow::onCompileFinished(bool success, const QString &apkPath)
{
    // Disconnect so we don't get called again accidentally
    disconnect(m_apk, &ApkManager::finished, this, &MainWindow::onCompileFinished);

    if (!success) {
        setOperationRunning(false);
        appendLog("✗ Compile failed.", "error");
        reconnectApkFinished();
        return;
    }

    appendLog("✓ Compile done: " + apkPath, "success");
    const AppSettings s = m_settings->settings(); // copy, not reference

    auto finalizeCompile = [this](const QString &finalApk) {
        if (!finalApk.isEmpty() && QFile::exists(finalApk)) {
            QDir().mkpath(m_rm->outputDir());
            QString outPath = m_rm->outputDir() + "/" + QFileInfo(finalApk).fileName();
            if (QFile::exists(outPath)) QFile::remove(outPath);
            if (QFile::copy(finalApk, outPath)) {
                appendLog("✓ Saved to output: " + QFileInfo(outPath).fileName(), "success");
                m_lastCompiledApk = outPath;
            }
        }
        setOperationRunning(false);
        reconnectApkFinished();
    };

    // Auto zipalign + sign
    if (s.autoZipalign && s.zipalignBeforeSign) {
        disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
        connect(m_apk, &ApkManager::finished, this, [this, s, finalizeCompile](bool ok, const QString &aligned){
            disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
            if (ok && s.autoSignAfterCompile) {
                connect(m_apk, &ApkManager::finished, this, [this, finalizeCompile](bool ok2, const QString &signed_){
                    disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
                    if (ok2) appendLog("✓ Sign done: " + signed_, "success");
                    finalizeCompile(ok2 ? signed_ : "");
                });
                m_apk->signApk(aligned);
            } else {
                finalizeCompile(ok ? aligned : "");
            }
        });
        m_apk->zipalignApk(apkPath);
    } else if (s.autoSignAfterCompile) {
        disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
        connect(m_apk, &ApkManager::finished, this, [this, s, finalizeCompile](bool ok, const QString &signed_){
            disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
            if (ok && s.autoZipalign) {
                // Must capture signed_ by value because it's required inside the nested lambda
                connect(m_apk, &ApkManager::finished, this, [this, finalizeCompile, signed_](bool ok2, const QString &aligned){
                    disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
                    finalizeCompile(ok2 ? aligned : signed_);
                });
                m_apk->zipalignApk(signed_);
            } else {
                finalizeCompile(ok ? signed_ : "");
            }
        });
        m_apk->signApk(apkPath);
    } else {
        finalizeCompile(apkPath);
    }
}

// ==============================================================================
// SIGN / ZIPALIGN TAB
// ==============================================================================
void MainWindow::onBrowseApkSign()
{
    QString path = QFileDialog::getOpenFileName(this, "Select APK", QString(),
                                                "APK Files (*.apk)");
    if (!path.isEmpty()) ui->leApkPathSign->setText(path);
}

void MainWindow::onSign()
{
    QString apk = ui->leApkPathSign->text().trimmed();
    if (apk.isEmpty()) { CustomDialog::showWarning(this, "No APK", "Select an APK first."); return; }
    if (m_apk->isBusy()) { CustomDialog::showWarning(this, "Busy", "Another operation is running."); return; }

    AppSettings &s = m_settings->settings();
    s.v1Signing = ui->chkV1Sign->isChecked();
    s.v2Signing = ui->chkV2Sign->isChecked();
    s.v3Signing = ui->chkV3Sign->isChecked();

    setOperationRunning(true, "Signing…");
    m_apk->signApk(apk);
}

void MainWindow::onZipalign()
{
    QString apk = ui->leApkPathSign->text().trimmed();
    if (apk.isEmpty()) { CustomDialog::showWarning(this, "No APK", "Select an APK first."); return; }
    if (m_apk->isBusy()) { CustomDialog::showWarning(this, "Busy", "Another operation is running."); return; }

    setOperationRunning(true, "Zipaligning…");
    m_apk->zipalignApk(apk, {}, ui->spnAlignment->value());
}

void MainWindow::onSignAndAlign()
{
    QString apk = ui->leApkPathSign->text().trimmed();
    if (apk.isEmpty()) { CustomDialog::showWarning(this, "No APK", "Select an APK first."); return; }
    if (m_apk->isBusy()) { CustomDialog::showWarning(this, "Busy", "Another operation is running."); return; }

    AppSettings &s = m_settings->settings();
    s.v1Signing = ui->chkV1Sign->isChecked();
    s.v2Signing = ui->chkV2Sign->isChecked();
    s.v3Signing = ui->chkV3Sign->isChecked();
    s.zipalignAlignment = ui->spnAlignment->value();

    setOperationRunning(true, "Zipalign + Sign…");

    disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
    connect(m_apk, &ApkManager::finished, this, [this](bool ok, const QString &aligned){
        disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
        if (ok) {
            connect(m_apk, &ApkManager::finished, this, [this](bool, const QString &){
                disconnect(m_apk, &ApkManager::finished, nullptr, nullptr);
                setOperationRunning(false);
                reconnectApkFinished();
            });
            m_apk->signApk(aligned);
        } else {
            setOperationRunning(false);
            reconnectApkFinished();
        }
    });
    m_apk->zipalignApk(apk);
}

// ==============================================================================
// MERGE TAB
// ==============================================================================
void MainWindow::onBrowseMergeInput()
{
    QString path = QFileDialog::getOpenFileName(this, "Select Bundle", QString(),
                                                "Bundle Files (*.xapk *.apks *.apkm);;All Files (*)");
    if (!path.isEmpty()) ui->leMergeInput->setText(path);
}

void MainWindow::onMerge()
{
    QString bundle = ui->leMergeInput->text().trimmed();
    if (bundle.isEmpty()) { CustomDialog::showWarning(this, "No Bundle", "Select a bundle file."); return; }
    if (m_apk->isBusy()) { CustomDialog::showWarning(this, "Busy", "Another operation is running."); return; }

    QString outApk = m_rm->outputDir() + "/" + QFileInfo(bundle).baseName() + "_merged.apk";
    setOperationRunning(true, "Merging…");
    m_apk->mergeBundle(bundle, outApk);
}

// ==============================================================================
// BAKSMALI TAB
// ==============================================================================
void MainWindow::onBrowseDex()
{
    QString p = QFileDialog::getOpenFileName(this, "Select DEX", QString(), "DEX Files (*.dex)");
    if (!p.isEmpty()) ui->leDexFile->setText(p);
}

void MainWindow::onBrowseSmaliDir()
{
    QString p = QFileDialog::getExistingDirectory(this, "Select Smali Dir");
    if (!p.isEmpty()) ui->leSmaliDir->setText(p);
}

void MainWindow::onBaksmali()
{
    QString dex = ui->leDexFile->text().trimmed();
    if (dex.isEmpty()) { CustomDialog::showWarning(this, "No DEX", "Select a DEX file."); return; }
    QString outDir = m_rm->workspaceDir() + "/" + QFileInfo(dex).baseName() + "_smali";
    setOperationRunning(true, "Disassembling DEX…");
    m_apk->baksmali(dex, outDir);
}

void MainWindow::onSmali()
{
    QString dir = ui->leSmaliDir->text().trimmed();
    if (dir.isEmpty()) { CustomDialog::showWarning(this, "No Dir", "Select a Smali directory."); return; }
    QString outDex = m_rm->outputDir() + "/" + QFileInfo(dir).fileName() + ".dex";
    setOperationRunning(true, "Assembling Smali…");
    m_apk->smali(dir, outDex);
}

// ==============================================================================
// APK INFO TAB
// ==============================================================================
void MainWindow::onBrowseApkInfo()
{
    QString p = QFileDialog::getOpenFileName(this, "Select APK", QString(), "APK Files (*.apk)");
    if (!p.isEmpty()) ui->leApkPathInfo->setText(p);
}

void MainWindow::onGetApkInfo()
{
    QString apk = ui->leApkPathInfo->text().trimmed();
    if (apk.isEmpty()) { CustomDialog::showWarning(this, "No APK", "Select an APK first."); return; }

    appendLog("Reading APK info: " + QFileInfo(apk).fileName(), "info");
    QMap<QString,QString> info = m_apk->getApkInfo(apk);

    ui->leInfoPackageName->setText(info.value("package_name"));
    ui->leInfoVersionName->setText(info.value("version_name"));
    ui->leInfoVersionCode->setText(info.value("version_code"));
    ui->leInfoMinSdk->setText(info.value("min_sdk"));
    ui->leInfoTargetSdk->setText(info.value("target_sdk"));
    ui->tePermissions->setPlainText(info.value("permissions"));

    // Signature Verification
    appendLog("Verifying APK signature...", "info");
    QString certInfo = m_apk->verifySignature(apk);
    QString currentText = ui->tePermissions->toPlainText();
    ui->tePermissions->setPlainText(
        currentText +
        "\n\n=" + QString(60, '=') + "\n"
        "SIGNATURE VERIFICATION\n" +
        "=" + QString(60, '=') + "\n" +
        certInfo
    );

    appendLog("\u2713 APK info & signature loaded", "success");
}

// ==============================================================================
// ADB TAB
// ==============================================================================
QString MainWindow::selectedDeviceId() const
{
    int row = ui->cmbDevices->currentIndex();
    if (row < 0) return m_adb->currentDevice();
    return ui->cmbDevices->itemData(row).toString();
}

void MainWindow::onRefreshDevices()
{
    ui->cmbDevices->clear();
    QList<AdbDevice> devices = m_adb->checkDevices();
    for (const AdbDevice &d : devices) {
        QString display = QString("[%1] %2 – %3").arg(
            d.transport.toUpper(), d.id, d.model.isEmpty() ? d.product : d.model);
        ui->cmbDevices->addItem(display, d.id);
    }
    
    if (devices.isEmpty()) {
        ui->cmbDevices->addItem("No devices found", "");
    }
    
    appendLog(QString("Found %1 device(s)").arg(devices.size()), devices.isEmpty() ? "warning" : "success");
}

void MainWindow::onConnectWifi()
{
    QString ip = ui->leWifiIp->text().trimmed();
    if (ip.isEmpty()) { CustomDialog::showWarning(this, "No IP", "Enter device IP address."); return; }
    int port = ui->spnWifiPort->value();
    m_adb->connectWifi(ip, port);
    onRefreshDevices();
}

void MainWindow::onDisconnect()
{
    m_adb->disconnect(selectedDeviceId());
    onRefreshDevices();
}

void MainWindow::onDeviceSelectionChanged()
{
    QString devId = selectedDeviceId();
    m_adb->setCurrentDevice(devId);
}

void MainWindow::onRefreshPackages()
{
    QString devId = selectedDeviceId();
    if (devId.isEmpty()) {
        appendLog("No device selected.", "warning");
        return;
    }

    static const QStringList filterKeys = {"all", "user", "system"};
    int idx = ui->cmbPackageFilter->currentIndex();
    QString filter = (idx >= 0 && idx < filterKeys.size()) ? filterKeys[idx] : "all";

    setOperationRunning(true, "Loading packages…");
    m_packageCache = m_adb->listPackages(devId, filter);
    setOperationRunning(false);

    refreshPackageList(ui->lePackageSearch->text());
}

void MainWindow::refreshPackageList(const QString &filter)
{
    ui->tblPackages->setRowCount(0);
    int count = 0;
    
    ui->tblPackages->setColumnCount(2);
    ui->tblPackages->setHorizontalHeaderLabels({"TYPE", "PACKAGE NAME"});
    ui->tblPackages->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tblPackages->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    for (const AdbPackage &pkg : m_packageCache) {
        if (!filter.isEmpty() && !pkg.name.contains(filter, Qt::CaseInsensitive))
            continue;

        ui->tblPackages->insertRow(count);
        
        QTableWidgetItem *typeItem = new QTableWidgetItem(pkg.isSystem ? "SYSTEM" : "USER");
        if (pkg.isSystem) typeItem->setForeground(QBrush(QColor("#ff6b6b")));
        else typeItem->setForeground(QBrush(QColor("#4ecdc4")));
        
        QTableWidgetItem *nameItem = new QTableWidgetItem(pkg.name);
        
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        
        nameItem->setData(Qt::UserRole, pkg.name);
        
        ui->tblPackages->setItem(count, 0, typeItem);
        ui->tblPackages->setItem(count, 1, nameItem);
        
        ++count;
    }
    ui->lblPackageCount->setText(QString("%1 packages").arg(count));
    ui->lblPackageSelected->setText("Selected: None");
}

void MainWindow::onPackageFilterChanged(const QString &text)
{
    refreshPackageList(text);
}

void MainWindow::onBackupSelected()
{
    int row = ui->tblPackages->currentRow();
    if (row < 0) { CustomDialog::showWarning(this, "No Package", "Select a package to backup."); return; }
    QString pkg = ui->tblPackages->item(row, 1)->data(Qt::UserRole).toString();
    QString devId = selectedDeviceId();
    if (devId.isEmpty()) { CustomDialog::showWarning(this, "No Device", "Select a device first."); return; }

    QString outDir = m_rm->backupDir();
    setOperationRunning(true, "Backing up " + pkg + "…");
    QPair<bool,QString> result = m_adb->backupApk(pkg, outDir, devId);
    setOperationRunning(false);
    if (result.first) openFolder(QFileInfo(result.second).absolutePath());
}

void MainWindow::onOpenBackupDir()
{
    openFolder(m_rm->backupDir());
}

void MainWindow::onUninstallSelected()
{
    int row = ui->tblPackages->currentRow();
    if (row < 0) { CustomDialog::showWarning(this, "No Package", "Select a package to uninstall."); return; }
    QString pkg = ui->tblPackages->item(row, 1)->data(Qt::UserRole).toString();

    if (!CustomDialog::showQuestion(this, "Uninstall", "Uninstall \"" + pkg + "\"?"))
        return;

    m_adb->uninstallPackage(pkg, selectedDeviceId());
    QTimer::singleShot(500, this, &MainWindow::onRefreshPackages);
}

// ─── Install APK via ADB ──────────────────────────────────────────────────────
void MainWindow::onInstallApk()
{
    QString devId = selectedDeviceId();
    if (devId.isEmpty()) { CustomDialog::showWarning(this, "No Device", "Select a device first."); return; }

    QString apkPath = QFileDialog::getOpenFileName(this, "Select APK to Install", {},
                                                   "APK Files (*.apk);;All Files (*)");
    if (apkPath.isEmpty()) return;

    setOperationRunning(true, "Installing " + QFileInfo(apkPath).fileName() + "...");
    appendLog("Installing " + QFileInfo(apkPath).fileName() + " on " + devId, "info");

    QPair<bool,QString> result = m_adb->installApk(apkPath, devId);
    QString output = result.second;

    if (!result.first) {
        // Handle INSTALL_FAILED_TEST_ONLY
        if (output.contains("INSTALL_FAILED_TEST_ONLY")) {
            if (CustomDialog::showQuestion(this, "Test-Only APK",
                    "This APK is test-only.\nRetry with -t flag?")) {
                result = m_adb->installApk(apkPath, devId, false, true);
            }
        }
        // Handle INSTALL_FAILED_VERSION_DOWNGRADE
        else if (output.contains("INSTALL_FAILED_VERSION_DOWNGRADE")) {
            if (CustomDialog::showQuestion(this, "Version Downgrade",
                    "Version is older than installed. Force downgrade?\n(May lose app data)")) {
                result = m_adb->installApk(apkPath, devId, true, false);
            }
        }
    }

    setOperationRunning(false);
    if (result.first)
        appendLog("\u2713 APK installed successfully: " + QFileInfo(apkPath).fileName(), "success");
    else
        appendLog("\u2717 Install failed. Check log.", "error");
}

// ─── Package Info ─────────────────────────────────────────────────────────────
void MainWindow::onPackageInfo()
{
    int row = ui->tblPackages->currentRow();
    if (row < 0) { CustomDialog::showWarning(this, "No Package", "Select a package first."); return; }

    QString pkg  = ui->tblPackages->item(row, 1)->data(Qt::UserRole).toString();
    QString devId = selectedDeviceId();
    if (devId.isEmpty()) { CustomDialog::showWarning(this, "No Device", "No device connected."); return; }

    appendLog("Fetching info for " + pkg + "...", "info");
    AdbPackage info = m_adb->getPackageInfo(pkg, devId);

    QString sizeMb = info.size > 0
        ? QString::number(info.size / (1024.0 * 1024.0), 'f', 2) + " MB"
        : "N/A";

    QString text = QString(
        "📦  Package:    %1\n"
        "-------------------------------------------\n"
        "\u24d8   Version:    %2 (Code: %3)\n"
        "💾  Size:       %4\n"
        "🛠   SDK:        Min: %5  |  Target: %6\n"
        "👤  Type:       %7\n"
        "-------------------------------------------\n"
        "🕒  Installed:  %8\n"
        "🔄  Updated:    %9\n"
        "-------------------------------------------\n"
        "📁  APK Path:   %10\n"
        "📂  Data Dir:   %11\n"
        "🆔  UID:        %12\n"
        "💿  Installer:  %13"
    ).arg(info.name,
          info.versionName, info.versionCode,
          sizeMb,
          info.minSdk, info.targetSdk,
          info.isSystem ? "System App" : "User App",
          info.installTime, info.updateTime,
          info.apkPath, info.dataDir,
          info.uid,
          info.installer.isEmpty() ? "Unknown" : info.installer);

    CustomDialog::showInfo(this, "Package Info: " + pkg, text);
}

// ─── WiFi Pair Dialog ─────────────────────────────────────────────────────────
void MainWindow::onWifiPairDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("📡 WiFi Pairing Setup");
    dlg.setWindowFlags(dlg.windowFlags() | Qt::Dialog);
    dlg.setMinimumWidth(460);
    dlg.setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);

    auto *titleLbl = new QLabel("📡 WiFi Debugging Pairing", &dlg);
    titleLbl->setStyleSheet("font-size:15px; font-weight:bold;");
    layout->addWidget(titleLbl);

    auto *infoLbl = new QLabel(
        "1. On Android: Settings → Developer Options → Wireless Debugging\n"
        "2. Tap \"Pair device with pairing code\"\n"
        "3. Enter details below:", &dlg);
    infoLbl->setWordWrap(true);
    layout->addWidget(infoLbl);

    auto mkRow = [&](const QString &label, QLineEdit *&le, const QString &placeholder) {
        auto *row = new QHBoxLayout();
        auto *lbl = new QLabel(label, &dlg);
        lbl->setMinimumWidth(110);
        le = new QLineEdit(&dlg);
        le->setPlaceholderText(placeholder);
        row->addWidget(lbl);
        row->addWidget(le);
        layout->addLayout(row);
    };

    auto mkComboRow = [&](const QString &label, QComboBox *&cb, const QString &placeholder) {
        auto *row = new QHBoxLayout();
        auto *lbl = new QLabel(label, &dlg);
        lbl->setMinimumWidth(110);
        cb = new QComboBox(&dlg);
        cb->setEditable(true);
        cb->setCurrentText("");
        // cb->setPlaceholderText(placeholder);
        row->addWidget(lbl);
        row->addWidget(cb);
        layout->addLayout(row);
    };

    auto *btnScan = new QPushButton("🔍 Find Device IP (Network Scan)", &dlg);
    btnScan->setStyleSheet("font-weight:bold; height: 35px;");
    layout->addWidget(btnScan);

    QComboBox *cmbIp = nullptr;
    QLineEdit *lePort = nullptr, *leCode = nullptr;
    mkComboRow("IP Address:", cmbIp,   "192.168.x.x");
    mkRow("Pairing Port:",  lePort, "xxxxx");
    mkRow("Pairing Code:",  leCode, "123456");

    auto *statusLbl = new QLabel("Ready", &dlg);
    statusLbl->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLbl);

    connect(btnScan, &QPushButton::clicked, [&]() {
        statusLbl->setText("🔍 Scanning network...");
        QApplication::processEvents();
        NetworkScanDialog scanDlg(&dlg);
        if (scanDlg.exec() == QDialog::Accepted) {
            QStringList ips = scanDlg.foundIps();
            if (!ips.isEmpty()) {
                cmbIp->clear();
                cmbIp->addItems(ips);
                cmbIp->setCurrentIndex(0);
                statusLbl->setText(QString("✓ Found %1 device(s)").arg(ips.size()));
            } else {
                statusLbl->setText("⚠️ No active devices found.");
            }
        } else {
            statusLbl->setText("Search cancelled.");
        }
    });

    auto *btnRow = new QHBoxLayout();
    auto *btnPair  = new QPushButton("🔗 Pair & Connect", &dlg);
    auto *btnClose = new QPushButton("Cancel", &dlg);
    btnRow->addWidget(btnPair);
    btnRow->addWidget(btnClose);
    layout->addLayout(btnRow);

    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(btnPair,  &QPushButton::clicked, [&]() {
        QString ip   = cmbIp->currentText().trimmed();
        QString port = lePort->text().trimmed();
        QString code = leCode->text().trimmed();
        if (ip.isEmpty() || port.isEmpty() || code.isEmpty()) {
            statusLbl->setText("⚠️ Please fill all fields.");
            return;
        }
        statusLbl->setText("🔄 Pairing...");
        QApplication::processEvents();

        QPair<bool,QString> pairResult = m_adb->pairWifi(ip, port.toInt(), code);
        if (!pairResult.first) {
            statusLbl->setText("✗ Pairing failed: " + pairResult.second);
            return;
        }
        statusLbl->setText("✓ Paired! Enter connection port...");
        QApplication::processEvents();

        bool ok = false;
        QString connPort = QInputDialog::getText(&dlg, "Connection Port",
            "Pairing succeeded!\n"
            "Now enter the CONNECTION port shown on your device:",
            QLineEdit::Normal, "5555", &ok);
        if (!ok || connPort.isEmpty()) {
            statusLbl->setText("✓ Paired (connection skipped).");
            return;
        }
        statusLbl->setText("🔄 Connecting...");
        QApplication::processEvents();

        QPair<bool,QString> connResult = m_adb->connectWifi(ip, connPort.toInt());
        if (connResult.first) {
            statusLbl->setText("✓ Connected to " + ip + ":" + connPort);
            appendLog("✓ WiFi connected: " + ip + ":" + connPort, "success");
            QTimer::singleShot(1000, [&](){ dlg.accept(); });
            onRefreshDevices();
        } else {
            statusLbl->setText("✗ Connection failed: " + connResult.second);
        }
    });

    dlg.exec();
}

// ==============================================================================
// SETTINGS TAB
// ==============================================================================
void MainWindow::onBrowseJavaPath()
{
    QString p = QFileDialog::getOpenFileName(this, "Select java.exe", QString(),
                                             "java.exe (java.exe);;All (*)");
    if (!p.isEmpty()) ui->leJavaPath->setText(p);
}

void MainWindow::onBrowseApktoolPath()
{
    QString p = QFileDialog::getOpenFileName(this, "Select Apktool JAR", QString(),
                                             "JAR Files (*.jar)");
    if (p.isEmpty()) return;

    // Copy JAR to tools/custom/ so it's always available
    QDir().mkpath(m_rm->customToolsDir());
    QString destName = QFileInfo(p).fileName();
    QString destPath = m_rm->customToolsDir() + "/" + destName;

    if (p != destPath) {
        if (QFile::exists(destPath)) QFile::remove(destPath);
        if (!QFile::copy(p, destPath)) {
            appendLog("Failed to copy Apktool JAR to custom tools folder.", "error");
            return;
        }
    }

    // Refresh combo with all JARs now in custom dir
    ui->cmbCustomApktool->clear();
    QDir customDir(m_rm->customToolsDir());
    for (const QString &f : customDir.entryList({"*.jar"}, QDir::Files))
        ui->cmbCustomApktool->addItem(f);
    ui->cmbCustomApktool->setCurrentText(destName);

    appendLog("✓ Copied to custom tools: " + destName, "success");
}

void MainWindow::onBrowseAaptPath()
{
    QString p = QFileDialog::getOpenFileName(this, "Select aapt.exe", QString(), "aapt.exe (aapt.exe);;*");
    if (!p.isEmpty()) ui->leCustomAaptPath->setText(p);
}

void MainWindow::onBrowseAapt2Path()
{
    QString p = QFileDialog::getOpenFileName(this, "Select aapt2.exe", QString(), "aapt2.exe (aapt2.exe);;*");
    if (!p.isEmpty()) ui->leCustomAapt2Path->setText(p);
}

void MainWindow::onBrowseApksignerPath()
{
    QString p = QFileDialog::getOpenFileName(this, "Select apksigner.jar", QString(), "JAR (*.jar);;*");
    if (!p.isEmpty()) ui->leApksignerPath->setText(p);
}

void MainWindow::onBrowseKeystorePath()
{
    QString p = QFileDialog::getOpenFileName(this, "Select Keystore", m_rm->keysDir(),
                                             "Keystore (*.keystore *.jks);;All (*)");
    if (!p.isEmpty()) ui->cmbKeystorePath->setCurrentText(p);
}

void MainWindow::onCreateKeystore()
{
    QString java = m_settings->settings().javaPath;
    KeystoreDialog dlg(m_rm->keysDir(), java, this);
    if (dlg.exec() == QDialog::Accepted) {
        ui->chkUseCustomKeystore->setChecked(true);
        
        QString createdFile = QFileInfo(dlg.createdKeystorePath()).fileName();
        
        // Refresh combo box
        QDir keysDir(m_rm->keysDir());
        ui->cmbKeystorePath->clear();
        for (const QString &f : keysDir.entryList({"*.jks", "*.keystore"}, QDir::Files))
            ui->cmbKeystorePath->addItem(f);
            
        ui->cmbKeystorePath->setCurrentText(createdFile);
        
        ui->leKeystorePass->setText(dlg.createdPassword());
        ui->leKeyAlias->setText(dlg.createdAlias());
        ui->leKeyPass->setText(dlg.createdPassword());
        appendLog("✓ Keystore settings updated", "success");
        
        // Auto-save settings when generated
        onSaveSettings();
    }
}

void MainWindow::onSaveSettings()
{
    saveUIToSettings();
    m_settings->save();
    appendLog("✓ Settings saved", "success");
    updateStatusBar("Settings saved", "success");
}

void MainWindow::onResetSettings()
{
    if (!CustomDialog::showQuestion(this, "Reset Settings", "Reset all settings to defaults?"))
        return;
    m_settings->settings() = AppSettings();
    m_settings->settings().javaPath = SettingsManager::autoDetectJava();
    loadSettingsToUI();
    m_settings->save();
    appendLog("Settings reset to defaults", "info");
}

void MainWindow::onClearFramework()
{
    if (!CustomDialog::showQuestion(this, "Clear Framework", "Clear all installed framework files?"))
        return;
    m_apk->clearFramework();
}

void MainWindow::onInstallFramework()
{
    QString p = QFileDialog::getOpenFileName(this, "Select Framework APK", QString(),
                                             "APK Files (*.apk)");
    if (p.isEmpty()) return;
    setOperationRunning(true, "Installing framework…");
    m_apk->installFramework(p);
}

