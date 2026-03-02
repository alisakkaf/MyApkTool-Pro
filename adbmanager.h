#pragma once
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

// ─── Device ──────────────────────────────────────────────────────────────────
struct AdbDevice {
    QString id;
    QString status;      // "device" | "offline" | "unauthorized"
    QString model;
    QString product;
    QString transport;   // "usb" | "wifi"
    QString toString() const;
};

// ─── Package ─────────────────────────────────────────────────────────────────
struct AdbPackage {
    QString name;
    QString versionName;
    QString versionCode;
    QString apkPath;
    qint64  size        = 0;
    bool    isSystem    = false;
    QString minSdk;
    QString targetSdk;
    QString installTime;
    QString updateTime;
    QString uid;
    QString dataDir;
    QString installer;
    QStringList permissions;
    QStringList flags;
    bool    isDebuggable = false;
    bool    isTestOnly   = false;
    QString toString() const;
};

// ─── AdbManager ──────────────────────────────────────────────────────────────
/**
 * Full C++ port of Python's ADBManager.
 * Uses QProcess instead of subprocess.
 * All methods are synchronous (blocking) and emit log() signals.
 */
class AdbManager : public QObject
{
    Q_OBJECT
public:
    explicit AdbManager(const QString &adbPath = QString(),
                        QObject *parent = nullptr);

    bool isAvailable() const;
    void setAdbPath(const QString &path);
    QString adbPath() const { return m_adbPath; }

    // ── Device discovery
    QList<AdbDevice> checkDevices();
    void setCurrentDevice(const QString &deviceId) { m_currentDevice = deviceId; }
    QString currentDevice() const { return m_currentDevice; }

    // ── Connection
    QPair<bool,QString> connectWifi(const QString &ip, int port = 5555);
    QPair<bool,QString> pairWifi(const QString &ip, int port, const QString &code);
    bool disconnect(const QString &deviceId = QString());
    QPair<bool,QString> enableWifiDebugging(const QString &deviceId = QString());
    QString getDeviceIp(const QString &deviceId = QString());

    // ── APK install
    QPair<bool,QString> installApk(const QString &apkPath,
                                   const QString &deviceId = QString(),
                                   bool allowDowngrade = false,
                                   bool allowTestOnly = false);
    // ── Package management
    QList<AdbPackage> listPackages(const QString &deviceId = QString(),
                                   const QString &filterType = "all");  // all/user/system
    AdbPackage getPackageInfo(const QString &packageName,
                               const QString &deviceId = QString());

    // ── Backup / pull
    QPair<bool,QString> backupApk(const QString &packageName,
                                   const QString &outputDir,
                                   const QString &deviceId = QString());

    // ── Uninstall
    QPair<bool,QString> uninstallPackage(const QString &packageName,
                                          const QString &deviceId = QString(),
                                          bool keepData = false);

    // ── Raw ADB shell
    QPair<bool,QString> shell(const QString &cmd,
                               const QString &deviceId = QString(),
                               int timeoutMs = 30000);

    // ─── Static helper
    static QString detectAdbPath(const QString &baseDir = QString());

signals:
    void log(const QString &message, const QString &level); // level: info/success/warning/error

private:
    QPair<bool,QString> runAdb(const QStringList &args,
                               const QString &deviceId = QString(),
                               int timeoutMs = 30000);

    QString m_adbPath;
    QString m_currentDevice;
};
