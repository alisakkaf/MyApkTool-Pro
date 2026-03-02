#include "adbmanager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QElapsedTimer>
#include <QCoreApplication>

// ─── AdbDevice::toString ─────────────────────────────────────────────────────
QString AdbDevice::toString() const
{
    return QString("%1 - %2 (%3) [%4]")
        .arg(id)
        .arg(model.isEmpty() ? product : model)
        .arg(status)
        .arg(transport);
}

// ─── AdbPackage::toString ────────────────────────────────────────────────────
QString AdbPackage::toString() const
{
    QString mb = size > 0 ? QString("%1 MB").arg(size / 1048576.0, 0, 'f', 1) : "N/A";
    return QString("%1 v%2 (%3)").arg(name).arg(versionName).arg(mb);
}

// ─── AdbManager ──────────────────────────────────────────────────────────────
AdbManager::AdbManager(const QString &adbPath, QObject *parent)
    : QObject(parent)
{
    m_adbPath = adbPath.isEmpty() ? detectAdbPath() : adbPath;
    if (m_adbPath.isEmpty())
        emit log("⚠ ADB not found. Please check Settings or install Android SDK Platform Tools.", "warning");
}

bool AdbManager::isAvailable() const
{
    return !m_adbPath.isEmpty() && QFile::exists(m_adbPath);
}

void AdbManager::setAdbPath(const QString &path)
{
    m_adbPath = path;
}

// ── Static: detect ADB path ─────────────────────────────────────────────────
QString AdbManager::detectAdbPath(const QString &baseDir)
{
    QStringList candidates;
    if (!baseDir.isEmpty()) {
        candidates << baseDir + "/Resources/adb.exe"
                   << baseDir + "/Resources/platform-tools/adb.exe";
    }
    candidates << "C:/Android/platform-tools/adb.exe";

    QString appData = qgetenv("LOCALAPPDATA");
    if (!appData.isEmpty())
        candidates << appData + "/Android/Sdk/platform-tools/adb.exe";

    for (const QString &c : candidates)
        if (QFile::exists(c))
            return c;

    // Try PATH
    QProcess p;
    p.start("adb", {"version"});
    if (p.waitForFinished(2000) && p.exitCode() == 0)
        return "adb";

    return {};
}

// ── Low-level runner ─────────────────────────────────────────────────────────
QPair<bool,QString> AdbManager::runAdb(const QStringList &args,
                                        const QString &deviceId,
                                        int timeoutMs)
{
    if (m_adbPath.isEmpty())
        return qMakePair(false, QString("ADB not found"));

    QStringList fullArgs;
    if (!deviceId.isEmpty())
        fullArgs << "-s" << deviceId;
    fullArgs << args;

    QProcess proc;
    proc.start(m_adbPath, fullArgs);

    // Keep UI responsive while waiting
    QElapsedTimer timer;
    timer.start();

    while (proc.state() != QProcess::NotRunning) {
        if (!proc.waitForReadyRead(50))
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        if (timeoutMs > 0 && timer.elapsed() > timeoutMs) {
            proc.kill();
            return qMakePair(false, QString("Command timed out after %1 ms").arg(timeoutMs));
        }
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput())
                   + QString::fromUtf8(proc.readAllStandardError());
    return qMakePair(proc.exitCode() == 0, output.trimmed());
}

// ── Shell helper ─────────────────────────────────────────────────────────────
QPair<bool,QString> AdbManager::shell(const QString &cmd, const QString &deviceId, int timeoutMs)
{
    return runAdb({"shell", cmd}, deviceId.isEmpty() ? m_currentDevice : deviceId, timeoutMs);
}

// ── Devices ──────────────────────────────────────────────────────────────────
QList<AdbDevice> AdbManager::checkDevices()
{
    QPair<bool,QString> result = runAdb({"devices", "-l"});
    bool ok = result.first;
    QString output = result.second;

    QList<AdbDevice> devices;

    if (!ok) {
        emit log("Failed to check devices: " + output, "error");
        return devices;
    }

    QStringList lines = output.split('\n');
    if (lines.isEmpty()) return devices;
    lines.removeFirst(); // skip header

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 2) continue;

        AdbDevice dev;
        dev.id     = parts[0];
        dev.status = parts[1];
        dev.transport = dev.id.contains(':') ? "wifi" : "usb";

        for (int i = 2; i < parts.size(); ++i) {
            if (parts[i].startsWith("model:"))   dev.model   = parts[i].mid(6);
            if (parts[i].startsWith("product:")) dev.product = parts[i].mid(8);
        }
        devices.append(dev);
    }
    return devices;
}

// ── WiFi connect ─────────────────────────────────────────────────────────────
QPair<bool,QString> AdbManager::connectWifi(const QString &ip, int port)
{
    emit log(QString("Connecting to %1:%2...").arg(ip).arg(port), "info");
    QPair<bool,QString> res = runAdb({"connect", QString("%1:%2").arg(ip).arg(port)});
    bool ok = res.first;
    QString output = res.second;

    if (ok && (output.contains("connected", Qt::CaseInsensitive) ||
               output.contains("already connected", Qt::CaseInsensitive)))
    {
        m_currentDevice = QString("%1:%2").arg(ip).arg(port);
        emit log(QString("✓ Connected to %1:%2").arg(ip).arg(port), "success");
        return qMakePair(true, output);
    }
    emit log("✗ Failed to connect: " + output, "error");
    return qMakePair(false, output);
}

bool AdbManager::disconnect(const QString &deviceId)
{
    QString target = deviceId.isEmpty() ? (m_currentDevice.isEmpty() ? "all" : m_currentDevice) : deviceId;
    QStringList args = {"disconnect"};
    if (target != "all") args << target;

    QPair<bool,QString> res = runAdb(args);
    bool ok = res.first;
    if (ok) {
        if (target == m_currentDevice)
            m_currentDevice.clear();
        emit log("✓ Disconnected from " + target, "success");
    }
    return ok;
}

QPair<bool,QString> AdbManager::enableWifiDebugging(const QString &deviceId)
{
    QString target = deviceId.isEmpty() ? m_currentDevice : deviceId;
    QPair<bool,QString> res = runAdb({"tcpip", "5555"}, target);
    if (!res.first) return qMakePair(false, QString());

    QString ip = getDeviceIp(target);
    if (!ip.isEmpty()) {
        emit log(QString("✓ WiFi debugging enabled: %1:5555").arg(ip), "success");
        return qMakePair(true, ip);
    }
    return qMakePair(false, QString());
}

QString AdbManager::getDeviceIp(const QString &deviceId)
{
    QString target = deviceId.isEmpty() ? m_currentDevice : deviceId;
    QPair<bool,QString> res = runAdb({"shell", "ip", "addr", "show", "wlan0"}, target);
    if (res.first) {
        QRegularExpression re(R"(inet (\d+\.\d+\.\d+\.\d+))");
        auto m = re.match(res.second);
        if (m.hasMatch()) return m.captured(1);
    }
    return {};
}

// ── WiFi Pair ─────────────────────────────────────────────────────────────────
QPair<bool,QString> AdbManager::pairWifi(const QString &ip, int port, const QString &code)
{
    emit log(QString("Pairing with %1:%2...").arg(ip).arg(port), "info");
    QPair<bool,QString> res = runAdb({"pair", QString("%1:%2").arg(ip).arg(port), code}, {}, 30000);
    QString output = res.second;
    if (output.contains("success", Qt::CaseInsensitive)) {
        emit log("✓ Pairing successful!", "success");
        return qMakePair(true, output);
    }
    emit log("✗ Pairing failed: " + output, "error");
    return qMakePair(false, output);
}

// ── APK Install ───────────────────────────────────────────────────────────────
QPair<bool,QString> AdbManager::installApk(const QString &apkPath,
                                            const QString &deviceId,
                                            bool allowDowngrade,
                                            bool allowTestOnly)
{
    QString target = deviceId.isEmpty() ? m_currentDevice : deviceId;
    emit log(QString("Installing %1...").arg(QFileInfo(apkPath).fileName()), "info");

    QStringList args = {"install", "-r"};
    if (allowDowngrade) args << "-d";
    if (allowTestOnly)  args << "-t";
    args << apkPath;

    QPair<bool,QString> res = runAdb(args, target, 300000); // 5 min timeout
    QString output = res.second;

    if (output.contains("Success", Qt::CaseSensitive)) {
        emit log("✓ APK installed successfully", "success");
        return qMakePair(true, output);
    }

    emit log("✗ Install failed: " + output, "error");
    return qMakePair(false, output);
}


QList<AdbPackage> AdbManager::listPackages(const QString &deviceId, const QString &filterType)
{
    QString target = deviceId.isEmpty() ? m_currentDevice : deviceId;
    QStringList args = {"shell", "pm", "list", "packages", "-f", "-u"};
    if (filterType == "user")   args << "-3";
    if (filterType == "system") args << "-s";

    QPair<bool,QString> res = runAdb(args, target, 60000);
    bool ok = res.first;
    QString output = res.second;

    QList<AdbPackage> packages;

    if (!ok) {
        emit log("Failed to list packages: " + output, "error");
        return packages;
    }

    for (const QString &rawLine : output.split('\n')) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || !line.startsWith("package:")) continue;

        AdbPackage pkg;
        if (line.contains('=')) {
            int eqIdx = line.lastIndexOf('=');
            pkg.apkPath = line.mid(8, eqIdx - 8).trimmed();
            pkg.name    = line.mid(eqIdx + 1).trimmed();
        } else {
            pkg.name = line.mid(8).trimmed();
        }
        if (pkg.name.isEmpty()) continue;

        pkg.isSystem = pkg.apkPath.startsWith("/system") ||
                       pkg.apkPath.startsWith("/product") ||
                       pkg.apkPath.startsWith("/apex") ||
                       pkg.apkPath.startsWith("/vendor");
        packages.append(pkg);
    }

    emit log(QString("Found %1 packages").arg(packages.size()), "info");
    return packages;
}

// ── Package info (dumpsys) ───────────────────────────────────────────────────
AdbPackage AdbManager::getPackageInfo(const QString &packageName, const QString &deviceId)
{
    QString target = deviceId.isEmpty() ? m_currentDevice : deviceId;
    AdbPackage pkg;
    pkg.name = packageName;

    // ── 1. APK path ────────────────────────────────────────────────────────
    {
        QPair<bool,QString> res = runAdb({"shell","pm","path",packageName}, target);
        for (const QString &l : res.second.split('\n')) {
            QString t = l.trimmed();
            if (t.startsWith("package:")) { pkg.apkPath = t.mid(8).trimmed(); break; }
        }
        pkg.isSystem = pkg.apkPath.startsWith("/system")
                    || pkg.apkPath.startsWith("/product")
                    || pkg.apkPath.startsWith("/apex")
                    || pkg.apkPath.startsWith("/system_ext");
    }

    // ── 2. dumpsys package <pkg> ────────────────────────────────────────────
    //
    //  Real output key locations (Samsung Android 14+):
    //
    //  Packages:
    //    Package [com.xxx] (xxxxxxx):        ← marks start of package block
    //      appId=10049                        ← UID here
    //      versionCode=36 minSdk=36 targetSdk=36   ← all on ONE line
    //      versionName=16
    //      flags=[ SYSTEM HAS_CODE … ]
    //      installerPackageName=null
    //    User 0: … installed=true …
    //      dataDir=/data/user_de/0/com.xxx
    //      firstInstallTime=2022-01-01 …
    //      lastUpdateTime=…  (only in lastUpdateTime= line in User 0 block? no)
    //  ← lastUpdateTime appears at TOP of file in User X block too
    //
    {
        QPair<bool,QString> dumpRes = runAdb(
            {"shell","dumpsys","package",packageName}, target, 60000);

        if (dumpRes.first) {
            bool  inPkgBlock   = false;  // inside "Packages:" → "Package [xxx]" block
            bool  inUser0      = false;  // inside "User 0:" sub-block

            QString pkgBlockHeader = QString("Package [%1]").arg(packageName);

            for (const QString &rawL : dumpRes.second.split('\n')) {
                QString l = rawL.trimmed();

                // ── Detect entry into our Package block ──────────────────
                if (l.contains(pkgBlockHeader) && l.contains("(")) {
                    inPkgBlock = true;
                    inUser0    = false;
                    continue;
                }
                // Detect entry into "User 0:" sub-block (only inside our pkg block)
                if (inPkgBlock && l.startsWith("User 0:")) {
                    inUser0 = true;
                    continue;
                }
                // Detect end of User 0 block (next User X or new Package block)
                if (inUser0 && (l.startsWith("User ") || l.startsWith("Package ["))) {
                    inUser0 = false;
                    if (!l.startsWith("User ")) inPkgBlock = false;
                }

                // ── Helpers ──────────────────────────────────────────────
                auto grabAfter = [&](const QString &key) -> QString {
                    int idx = l.indexOf(key);
                    if (idx < 0) return {};
                    QString rest = l.mid(idx + key.length());
                    int end = rest.indexOf(QRegularExpression("\\s"));
                    return (end < 0 ? rest : rest.left(end)).trimmed();
                };
                auto grabLine = [&](const QString &key) -> QString {
                    int idx = l.indexOf(key);
                    if (idx < 0) return {};
                    return l.mid(idx + key.length()).trimmed();
                };

                // ── Fields inside Package block (before User 0) ──────────
                if (inPkgBlock && !inUser0) {
                    // appId=10049  ← the real numerical UID
                    if (pkg.uid.isEmpty() && l.startsWith("appId="))
                        pkg.uid = grabAfter("appId=");

                    // versionCode=36 minSdk=36 targetSdk=36  (same line)
                    if (l.contains("versionCode=")) {
                        // grab only the integer (stop at space/letter)
                        QString vc = grabAfter("versionCode=");
                        // strip trailing non-digit chars
                        int end2 = 0;
                        while (end2 < vc.size() && vc[end2].isDigit()) end2++;
                        if (end2 > 0) pkg.versionCode = vc.left(end2);
                    }
                    if (l.contains("minSdk=") && pkg.minSdk.isEmpty())
                        pkg.minSdk    = grabAfter("minSdk=");
                    if (l.contains("targetSdk=") && pkg.targetSdk.isEmpty())
                        pkg.targetSdk = grabAfter("targetSdk=");

                    if (l.startsWith("versionName=") && pkg.versionName.isEmpty())
                        pkg.versionName = grabLine("versionName=");

                    // installerPackageName=null / =com.android.vending
                    if (pkg.installer.isEmpty() && l.startsWith("installerPackageName=")) {
                        QString val = grabLine("installerPackageName=");
                        if (val != "null" && !val.isEmpty())
                            pkg.installer = val;
                    }

                    // flags=[ SYSTEM HAS_CODE … ]
                    if (l.startsWith("flags=[")) {
                        QRegularExpression re(R"(\[([^\]]+)\])");
                        auto m = re.match(l);
                        if (m.hasMatch()) {
                            pkg.flags = m.captured(1).split(' ', QString::SkipEmptyParts);
                            pkg.isDebuggable = pkg.flags.contains("DEBUGGABLE");
                            if (pkg.flags.contains("SYSTEM")) pkg.isSystem = true;
                        }
                    }
                }

                // ── Fields inside User 0 sub-block ────────────────────────
                if (inUser0) {
                    if (pkg.dataDir.isEmpty() && l.startsWith("dataDir="))
                        pkg.dataDir = grabLine("dataDir=");
                    if (pkg.installTime.isEmpty() && l.startsWith("firstInstallTime="))
                        pkg.installTime = grabLine("firstInstallTime=");
                }

                // ── lastUpdateTime appears at top of dump (before Packages) ──
                if (pkg.updateTime.isEmpty() && l.startsWith("lastUpdateTime="))
                    pkg.updateTime = grabLine("lastUpdateTime=");
            }
        }
    }

    // ── 3. Installer fallback: pm list packages -i ──────────────────────────
    if (pkg.installer.isEmpty()) {
        QPair<bool,QString> res = runAdb(
            {"shell","pm","list","packages","-i",packageName}, target, 20000);
        for (const QString &l : res.second.split('\n')) {
            if (l.contains(packageName) && l.contains("installer=")) {
                QRegularExpression re(R"(installer=(\S+))");
                auto m = re.match(l);
                if (m.hasMatch()) {
                    QString v = m.captured(1).trimmed();
                    if (v != "null") pkg.installer = v;
                }
                break;
            }
        }
    }

    // ── 4. APK size ─────────────────────────────────────────────────────────
    if (!pkg.apkPath.isEmpty()) {
        QPair<bool,QString> szRes = runAdb(
            {"shell","du","-b",pkg.apkPath}, target, 15000);
        if (szRes.first && !szRes.second.trimmed().isEmpty())
            pkg.size = szRes.second.split(QRegularExpression("\\s")).first().toLongLong();
    }

    return pkg;
}



// ── Backup / Pull APK ────────────────────────────────────────────────────────
QPair<bool,QString> AdbManager::backupApk(const QString &packageName,
                                            const QString &outputDir,
                                            const QString &deviceId)
{
    QString target = deviceId.isEmpty() ? m_currentDevice : deviceId;
    emit log(QString("🔍 Deep scanning: %1").arg(packageName), "info");

    QPair<bool,QString> res = runAdb({"shell", "pm", "path", packageName}, target);
    if (!res.first || res.second.isEmpty()) {
        emit log("✗ Package not found.", "error");
        return qMakePair(false, QString());
    }

    // Find all .apk paths
    QRegularExpression re(R"(package:(.*?\.apk))");
    QStringList paths;
    auto it = re.globalMatch(res.second);
    while (it.hasNext()) {
        QString p = it.next().captured(1).trimmed();
        if (!paths.contains(p)) paths << p;
    }

    if (paths.isEmpty()) {
        emit log("✗ No valid APK paths could be parsed.", "error");
        return qMakePair(false, QString());
    }

    QDir().mkpath(outputDir);
    bool split = paths.size() > 1;

    if (!split) {
        QString local = outputDir + "/" + packageName + ".apk";
        emit log("ℹ Single APK detected.", "info");
        QPair<bool,QString> pullRes = runAdb({"pull", paths[0], local}, target, 300000);
        if (pullRes.first && QFile::exists(local)) {
            emit log("✓ Backup saved: " + QFileInfo(local).fileName(), "success");
            return qMakePair(true, local);
        }
        emit log("✗ Backup failed: " + pullRes.second, "error");
        return qMakePair(false, QString());
    }

    // Split APKs
    emit log(QString("🚀 Split APKs detected! Found %1 parts.").arg(paths.size()), "info");
    QString pkgFolder = outputDir + "/" + packageName;
    QDir().mkpath(pkgFolder);

    int successCount = 0;
    for (int i = 0; i < paths.size(); ++i) {
        QString fileName = QFileInfo(paths[i]).fileName();
        QString local    = pkgFolder + "/" + fileName;
        emit log(QString("  ⬇ [%1/%2] Pulling: %3").arg(i+1).arg(paths.size()).arg(fileName), "info");
        QPair<bool,QString> pullRes = runAdb({"pull", paths[i], local}, target, 60000);
        if (pullRes.first && QFile::exists(local))
            ++successCount;
        else
            emit log("  ⚠ Failed to pull " + fileName + ": " + pullRes.second, "warning");
    }

    if (successCount > 0) {
        emit log(QString("✓ Backup complete: %1 parts saved").arg(successCount), "success");
        return qMakePair(true, pkgFolder);
    }
    emit log("✗ Critical failure: No parts were downloaded.", "error");
    return qMakePair(false, QString());
}

// ── Uninstall ────────────────────────────────────────────────────────────────
QPair<bool,QString> AdbManager::uninstallPackage(const QString &packageName,
                                                   const QString &deviceId,
                                                   bool keepData)
{
    QString target = deviceId.isEmpty() ? m_currentDevice : deviceId;
    emit log(QString("Uninstalling %1...").arg(packageName), "info");

    QStringList args = {"uninstall"};
    if (keepData) args << "-k";
    args << packageName;

    QPair<bool,QString> res = runAdb(args, target);
    if (res.first && res.second.contains("Success")) {
        emit log("✓ Uninstall complete", "success");
        return qMakePair(true, res.second);
    }
    emit log("✗ Uninstall failed: " + res.second, "error");
    return qMakePair(false, res.second);
}
