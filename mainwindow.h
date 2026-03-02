#pragma once
#include <QMainWindow>
#include <QTableWidgetItem>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QMouseEvent>
#include <QPoint>

#include "resourcemanager.h"
#include "settingsmanager.h"
#include "adbmanager.h"
#include "apkmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // ── Decompile tab
    void onBrowseApkDecompile();
    void onDecompile();

    // ── Compile tab
    void onBrowseProjectCompile();
    void onCompile();

    // ── Sign/Zipalign tab
    void onBrowseApkSign();
    void onSign();
    void onZipalign();
    void onSignAndAlign();

    // ── Merge tab
    void onBrowseMergeInput();
    void onMerge();

    // ── Baksmali tab
    void onBrowseDex();
    void onBrowseSmaliDir();
    void onBaksmali();
    void onSmali();

    // ── APK Info tab
    void onBrowseApkInfo();
    void onGetApkInfo();          // aapt info + signature verify

    // ── ADB tab
    void onRefreshDevices();
    void onConnectWifi();
    void onWifiPairDialog();       // NEW: WiFi Pairing
    void onInstallApk();           // NEW: ADB Install APK
    void onDisconnect();
    void onRefreshPackages();
    void onBackupSelected();
    void onUninstallSelected();
    void onPackageInfo();          // NEW: show package info
    void onOpenBackupDir();        // NEW: open backup directory
    void onPackageFilterChanged(const QString &text);
    void onDeviceSelectionChanged();

    // ── Settings tab
    void onBrowseJavaPath();
    void onBrowseApktoolPath();
    void onBrowseAaptPath();
    void onBrowseAapt2Path();
    void onBrowseApksignerPath();
    void onBrowseKeystorePath();
    void onCreateKeystore();       // NEW: Create Keystore
    void onSaveSettings();
    void onResetSettings();
    void onClearFramework();
    void onInstallFramework();

    // ── Log area
    void onClearLog();
    void onCopyLog();
    void onToggleLog();

    // ── Theme
    void onToggleTheme();

    // ── Manager callbacks
    void onLog(const QString &message, const QString &level);
    void onOperationFinished(bool success, const QString &resultPath);

    // ── Auto action after compile
    void onCompileFinished(bool success, const QString &apkPath);

    // ── Tab change
    void onTabChanged(int index);

    // ── Window Controls
    void onMinimize();
    void onMaximize();
    void onClose();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void setupUI();
    void applyStylesheet();
    void loadSettingsToUI();
    void saveUIToSettings();
    void setOperationRunning(bool running, const QString &msg = {});
    void appendLog(const QString &msg, const QString &level = "info");
    void updateStatusBar(const QString &msg, const QString &level = "info");
    void openFolder(const QString &path);
    void showSplash();
    void refreshPackageList(const QString &filter = {});
    QString selectedDeviceId() const;
    void reconnectApkFinished();

    Ui::MainWindow      *ui;
    ResourceManager     *m_rm;
    SettingsManager     *m_settings;
    AdbManager          *m_adb;
    ApkManager          *m_apk;

    bool                 m_logVisible       = true;
    bool                 m_logWasVisible     = true;
    bool                 m_darkMode          = true;
    QString              m_currentApkPath;
    QString              m_currentProjectPath;
    QString              m_lastCompiledApk;
    QString              m_pendingSignPath;   // APK waiting for sign after compile
    QList<AdbPackage>    m_packageCache;      // cached package list

    QLabel              *m_statusLabel  = nullptr;
    QProgressBar        *m_progressBar  = nullptr;
    QPoint               m_dragPos;
};
