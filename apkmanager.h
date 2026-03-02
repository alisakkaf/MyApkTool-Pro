#pragma once
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QMap>
#include <functional>
#include "settingsmanager.h"
#include "resourcemanager.h"

/**
 * ApkManager – port of Python managers.py.
 * Wraps all Java-based tool operations (apktool, APKEditor, apksigner,
 * baksmali/smali, zipalign, aapt) using QProcess.
 *
 * Operations run asynchronously; the caller is notified via:
 *   - log(msg, level) signal during execution
 *   - finished(success, resultPath) signal when done
 *
 * Cancellation: call cancel() while an operation is running.
 */
class ApkManager : public QObject
{
    Q_OBJECT
public:
    explicit ApkManager(ResourceManager *res,
                        SettingsManager *settings,
                        QObject *parent = nullptr);

    // ── Decompile
    void decompileApktool(const QString &apkPath,
                          const QString &outputFolder = QString());
    void decompileAPKEditor(const QString &apkPath,
                             const QString &outputFolder = QString());

    // ── Compile
    void compileApktool(const QString &folderPath);
    void compileAPKEditor(const QString &folderPath);

    // ── Merge bundles (.xapk / .apks / .apkm)
    void mergeBundle(const QString &bundlePath, const QString &outputApk);

    // ── Sign
    void signApk(const QString &inputApk, const QString &outputApk = QString());

    // ── Zipalign
    void zipalignApk(const QString &inputApk,
                     const QString &outputApk = QString(),
                     int alignment = 4);

    // ── Baksmali / Smali
    void baksmali(const QString &dexFile, const QString &outputDir);
    void smali(const QString &smaliDir, const QString &outputDex);

    // ── Framework
    void installFramework(const QString &frameworkApk);
    bool clearFramework();

    // ── APK Info (via aapt)
    QMap<QString,QString> getApkInfo(const QString &apkPath);

    // ── Signature verification
    QString verifySignature(const QString &apkPath);

    // ── Decompiler detection helper
    static QString detectDecompiler(const QString &projectPath);

    // ── Cancel current operation
    void cancel();

    bool isBusy() const { return (m_process && m_process->state() != QProcess::NotRunning); }

signals:
    void log(const QString &message, const QString &level);
    void finished(bool success, const QString &resultPath);
    void progressLine(const QString &line);  // raw stdout line

private:
    // Build a java -jar command with heap flags and module opens
    QStringList buildJavaCmd(const QStringList &jarArgs) const;

    // Resolve custom tool paths
    QString resolveApktoolPath();
    QString resolveAaptPath() const;
    QString resolveAapt2Path() const;
    QString resolveApksignerPath() const;

    void runProcess(const QStringList &args,
                    const QString &expectedOutput,
                    const QString &cwd = QString());

    ResourceManager *m_res;
    SettingsManager *m_settings;
    QProcess        *m_process = nullptr;
    bool             m_cancelled = false;
};
