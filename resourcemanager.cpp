#include "resourcemanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

// ──────────────────────────────────────────────────────────────
ResourceManager::ResourceManager(const QString &baseDir)
{
    if (baseDir.isEmpty()) {
        // Use application directory instead of %APPDATA%
        m_baseDir = QCoreApplication::applicationDirPath();
    } else {
        m_baseDir = baseDir;
    }
    QDir().mkpath(m_baseDir);
}

// ──────────────────────────────────────────────────────────────
const QStringList &ResourceManager::toolFiles()
{
    static QStringList list = {
        // JARs
        "Resources/apktool.jar",
        "Resources/APKEditor.jar",
        "Resources/apksigner.jar",
        "Resources/baksmali.jar",
        "Resources/smali.jar",
        "Resources/uber-apk-signer.jar",
        // EXEs
        "Resources/adb.exe",
        "Resources/aapt.exe",
        "Resources/aapt2.exe",
        "Resources/zipalign.exe",
        // DLLs
        "Resources/AdbWinApi.dll",
        "Resources/AdbWinUsbApi.dll",
        "Resources/libwinpthread-1.dll",
        // Keys
        "Resources/testkey.pk8",
        "Resources/testkey.x509.pem",
    };
    return list;
}

// ──────────────────────────────────────────────────────────────
bool ResourceManager::extractFile(const QString &qrcPath, const QString &destPath)
{
    QFileInfo destInfo(destPath);

    // Check if we need to extract
    if (destInfo.exists()) {
        QFile qrcFile(":" + qrcPath);
        // Skip if sizes match (already extracted)
        if (qrcFile.size() == destInfo.size())
            return true;
    }

    // Create parent dir
    QDir().mkpath(destInfo.absolutePath());

    QFile src(":" + qrcPath);
    if (!src.open(QIODevice::ReadOnly)) {
        qWarning() << "[ResourceManager] Cannot open qrc:" << qrcPath;
        return false;
    }

    QFile dst(destPath);
    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[ResourceManager] Cannot write to:" << destPath;
        return false;
    }

    dst.write(src.readAll());
    dst.close();
    src.close();

    // Make EXEs executable (relevant on Linux/macOS)
    if (destPath.endsWith(".exe"))
        QFile::setPermissions(destPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
            QFileDevice::ReadOther | QFileDevice::ExeOther);

    return true;
}

// ──────────────────────────────────────────────────────────────
bool ResourceManager::extractAll()
{
    ensureDirectories();
    bool allOk = true;
    for (const QString &qrcPath : toolFiles()) {
        QString fileName = QFileInfo(qrcPath).fileName();
        QString dest = m_baseDir + "/Resources/" + fileName;
        if (!extractFile(qrcPath, dest))
            allOk = false;
    }
    return allOk;
}

// ──────────────────────────────────────────────────────────────
void ResourceManager::ensureDirectories()
{
    const QStringList dirs = {
        "/Resources",
        "/Resources/custom",
        "/output",
        "/output/backups",
        "/workspace",
        "/temp",
        "/framework",
        "/logs",
        "/keys",
    };
    for (const QString &d : dirs)
        QDir().mkpath(m_baseDir + d);
}

// ──────────────────────────────────────────────────────────────
QString ResourceManager::toolPath(const QString &toolName) const
{
    return m_baseDir + "/Resources/" + toolName;
}

QString ResourceManager::keyPath(const QString &keyName) const
{
    // First check user's keys/ directory
    QString userKey = m_baseDir + "/keys/" + keyName;
    if (QFile::exists(userKey))
        return userKey;
    // Fall back to embedded key in Resources
    return m_baseDir + "/Resources/" + keyName;
}

QString ResourceManager::resourcePath(const QString &name) const
{
    return m_baseDir + "/Resources/" + name;
}
