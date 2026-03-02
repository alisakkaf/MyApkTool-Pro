#pragma once
#include <QString>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>

/**
 * ResourceManager – extracts Qt-embedded resources (tools, keys, etc.)
 * from the .qrc into a temporary working directory so the OS can execute them.
 *
 * Usage:
 *   ResourceManager rm;
 *   rm.extractAll();
 *   QString adbPath = rm.toolPath("adb.exe");
 */
class ResourceManager
{
public:
    /**
     * Construct the manager with a base directory.
     * Default: <AppData>/MyApkTool/  (persisted between runs for performance)
     */
    explicit ResourceManager(const QString &baseDir = QString());

    /** Extract all embedded resources if not already extracted. */
    bool extractAll();

    /** Path to a specific tool (e.g. "adb.exe", "apktool.jar") */
    QString toolPath(const QString &toolName) const;

    /** Path to a key file (e.g. "testkey.pk8") */
    QString keyPath(const QString &keyName) const;

    /** Path to a resource image (e.g. "splash.png") */
    QString resourcePath(const QString &name) const;

    /** Base runtime directory (where all tools are extracted) */
    QString baseDir() const { return m_baseDir; }

    /** Runtime directories */
    QString outputDir()    const { return m_baseDir + "/output"; }
    QString backupDir()    const { return m_baseDir + "/output/backups"; }
    QString workspaceDir() const { return m_baseDir + "/workspace"; }
    QString tempDir()      const { return m_baseDir + "/temp"; }
    QString frameworkDir() const { return m_baseDir + "/framework"; }
    QString logsDir()      const { return m_baseDir + "/logs"; }
    QString keysDir()      const { return m_baseDir + "/keys"; }
    QString customToolsDir() const { return m_baseDir + "/Resources/custom"; }

    /** Ensure runtime directories exist */
    void ensureDirectories();

private:
    QString m_baseDir;

    /** Extract a single resource file, skip if already present and same size. */
    bool extractFile(const QString &qrcPath, const QString &destPath);

    /** List of tool files embedded in resources.qrc */
    static const QStringList &toolFiles();
};
