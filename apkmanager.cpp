#include "apkmanager.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <QProcessEnvironment>

// ─────────────────────────────────────────────────────────────────────────────
ApkManager::ApkManager(ResourceManager *res, SettingsManager *settings, QObject *parent)
    : QObject(parent), m_res(res), m_settings(settings)
{}

// ─── Internal: build java command ────────────────────────────────────────────
QStringList ApkManager::buildJavaCmd(const QStringList &jarArgs) const
{
    const AppSettings &s = m_settings->settings();
    QStringList args;

    // Java executable
    QString java = s.javaPath.isEmpty() ? "java" : s.javaPath;
    args << java;

    // Heap size
    if (s.heapSize > 0)
        args << QString("-Xmx%1m").arg(s.heapSize);

    // Module opens (Java 9+)
    args << "--add-opens" << "java.base/sun.security.rsa=ALL-UNNAMED"
         << "--add-opens" << "java.base/java.util=ALL-UNNAMED"
         << "--add-opens" << "java.base/java.lang.invoke=ALL-UNNAMED";

    args << "-jar";
    args += jarArgs;
    return args;
}

// ─── Resolvers ───────────────────────────────────────────────────────────────
QString ApkManager::resolveApktoolPath()
{
    const AppSettings &s = m_settings->settings();
    if (s.useCustomApktool && !s.customApktoolPath.isEmpty()) {
        QString candidate;
        if (s.customApktoolPath.contains('/') || s.customApktoolPath.contains('\\'))
            candidate = s.customApktoolPath;
        else
            candidate = m_res->customToolsDir() + "/" + s.customApktoolPath;
        if (QFile::exists(candidate))
            return candidate;
        emit log("Custom Apktool not found, using default.", "warning");
    }
    return m_res->toolPath("apktool.jar");
}

QString ApkManager::resolveAaptPath() const
{
    const AppSettings &s = m_settings->settings();
    if (s.useCustomAapt && !s.customAaptPath.isEmpty()) {
        QString c = s.customAaptPath.contains('/') ? s.customAaptPath
                  : m_res->customToolsDir() + "/" + s.customAaptPath;
        if (QFile::exists(c)) return c;
    }
    return m_res->toolPath("aapt.exe");
}

QString ApkManager::resolveAapt2Path() const
{
    const AppSettings &s = m_settings->settings();
    if (s.useCustomAapt2 && !s.customAapt2Path.isEmpty()) {
        QString c = s.customAapt2Path.contains('/') ? s.customAapt2Path
                  : m_res->customToolsDir() + "/" + s.customAapt2Path;
        if (QFile::exists(c)) return c;
    }
    return m_res->toolPath("aapt2.exe");
}

QString ApkManager::resolveApksignerPath() const
{
    const AppSettings &s = m_settings->settings();
    if (!s.apksignerPath.isEmpty() && QFile::exists(s.apksignerPath))
        return s.apksignerPath;
    return m_res->toolPath("apksigner.jar");
}

// ─── Internal: run process ───────────────────────────────────────────────────
void ApkManager::runProcess(const QStringList &args, const QString &expectedOutput, const QString &cwd)
{
    if (!m_process) {
        m_process = new QProcess(this);
        m_process->setProcessChannelMode(QProcess::MergedChannels);
    }

    m_cancelled = false;
    QString program = args.first();
    QStringList pargs = args.mid(1);

    if (!cwd.isEmpty())
        m_process->setWorkingDirectory(cwd);

    // Build environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const AppSettings &s = m_settings->settings();
    if (s.useAapt2) {
        QString aapt2Path = resolveAapt2Path();
        if (!aapt2Path.isEmpty())
            env.insert("PATH", QFileInfo(aapt2Path).absolutePath() + ";" + env.value("PATH"));
    }
    m_process->setProcessEnvironment(env);

    emit log(QString("Running: %1 %2").arg(program, pargs.join(' ')), "info");

    m_process->start(program, pargs);

    // Stream output line by line without freezing UI
    while (m_process->state() != QProcess::NotRunning) {
        if (!m_process->waitForReadyRead(50))
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        while (m_process->canReadLine()) {
            QString line = QString::fromUtf8(m_process->readLine()).trimmed();
            if (line.isEmpty()) continue;
            // Classify log level
            QString lower = line.toLower();
            QString level = "info";
            if (lower.contains("error") || lower.contains("fail") || lower.contains("exception"))
                level = "error";
            else if (lower.contains("warning") || lower.contains("skip"))
                level = "warning";
            else if (lower.contains("success") || lower.contains("done") || lower.contains("complete"))
                level = "success";

            emit log(line, level);
            emit progressLine(line);
        }
        if (m_cancelled) {
            m_process->kill();
            emit log("🛑 Operation cancelled by user.", "warning");
            emit finished(false, {});
            return;
        }
    }

    // Drain remaining output
    while (m_process->canReadLine()) {
        QString line = QString::fromUtf8(m_process->readLine()).trimmed();
        if (!line.isEmpty()) emit log(line, "info");
    }

    m_process->waitForFinished(1000);
    bool success = (m_process->exitCode() == 0);

    emit finished(success, expectedOutput);
}

// ─── Cancel ─────────────────────────────────────────────────
void ApkManager::cancel()
{
    m_cancelled = true;
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->terminate();
}

// ─── Decompile (Apktool) ─────────────────────────────────────────────────────
void ApkManager::decompileApktool(const QString &apkPath, const QString &outputFolder)
{
    QString apktool = resolveApktoolPath();
    if (!QFile::exists(apktool)) {
        emit log("apktool.jar not found!", "error");
        emit finished(false, {});
        return;
    }

    QString apkName = QFileInfo(apkPath).completeBaseName();
    QString outputDir = outputFolder.isEmpty()
                      ? m_res->workspaceDir() + "/" + apkName
                      : outputFolder;

    // Remove existing output
    if (QDir(outputDir).exists()) {
        emit log("Removing existing folder: " + outputDir, "warning");
        QDir(outputDir).removeRecursively();
    }

    const AppSettings &s = m_settings->settings();
    QStringList jarArgs = {apktool, "d", apkPath, "-o", outputDir, "-f"};
    if (!s.decodeResources) jarArgs << "-r";
    if (!s.decodeSources)   jarArgs << "-s";
    if (s.useAapt2)         jarArgs << "--use-aapt2";

    // Custom AAPT
    QString aaptPath = resolveAaptPath();
    if (s.useCustomAapt && !aaptPath.isEmpty())
        jarArgs << "--aapt" << aaptPath;

    // Framework
    jarArgs << "-p" << m_res->frameworkDir();

    emit log(QString("Starting Apktool (%1)...").arg(QFileInfo(apktool).fileName()), "info");
    QStringList fullCmd = buildJavaCmd(jarArgs);
    // fullCmd[0] is java, rest are args
    runProcess(fullCmd, outputDir);

    // Create marker
    QFile(outputDir + "/.apktool_project").open(QIODevice::WriteOnly);
}

// ─── Decompile (APKEditor) ───────────────────────────────────────────────────
void ApkManager::decompileAPKEditor(const QString &apkPath, const QString &outputFolder)
{
    QString apkeditor = m_res->toolPath("APKEditor.jar");
    if (!QFile::exists(apkeditor)) {
        emit log("APKEditor.jar not found!", "error");
        emit finished(false, {});
        return;
    }

    QString apkName  = QFileInfo(apkPath).completeBaseName();
    QString outputDir = outputFolder.isEmpty()
                      ? m_res->workspaceDir() + "/" + apkName
                      : outputFolder;

    if (QDir(outputDir).exists())
        QDir(outputDir).removeRecursively();

    QStringList jarArgs = {apkeditor, "d", "-i", apkPath, "-o", outputDir};
    emit log("Starting APKEditor decompilation...", "info");
    runProcess(buildJavaCmd(jarArgs), outputDir);

    QFile(outputDir + "/.apkeditor_project").open(QIODevice::WriteOnly);
}

// ─── Compile (Apktool) ─────────────────────────────────────────────────────
void ApkManager::compileApktool(const QString &folderPath)
{
    QString apktool = resolveApktoolPath();
    if (!QFile::exists(apktool)) {
        emit log("apktool.jar not found!", "error");
        emit finished(false, {});
        return;
    }

    // Extract correct package name to build correct APK (fix for user reported "com.apk" issue)
    QString cleanName = folderPath.split("/").last();
    if (cleanName.isEmpty()) cleanName = QDir(folderPath).dirName();
    
    QString outputApk = folderPath + "/dist/" + cleanName + ".apk";

    const AppSettings &s = m_settings->settings();
    QStringList jarArgs = {apktool, "b", folderPath, "-o", outputApk, "-f"};
    if (s.useAapt2) jarArgs << "--use-aapt2";

    QString aaptPath = resolveAaptPath();
    if (s.useCustomAapt && !aaptPath.isEmpty())
        jarArgs << "--aapt" << aaptPath;

    jarArgs << "-p" << m_res->frameworkDir();

    emit log(QString("Starting Apktool build (%1)...").arg(QFileInfo(apktool).fileName()), "info");
    runProcess(buildJavaCmd(jarArgs), outputApk);
}

// ─── Compile (APKEditor) ─────────────────────────────────────────────────────
void ApkManager::compileAPKEditor(const QString &folderPath)
{
    QString apkeditor = m_res->toolPath("APKEditor.jar");
    if (!QFile::exists(apkeditor)) {
        emit log("APKEditor.jar not found!", "error");
        emit finished(false, {});
        return;
    }

    QString name = QFileInfo(folderPath).fileName();
    QString outputApk = m_res->outputDir() + "/" + name + "_compiled.apk";

    QStringList jarArgs = {apkeditor, "b", "-i", folderPath, "-o", outputApk};
    emit log("Starting APKEditor build...", "info");
    runProcess(buildJavaCmd(jarArgs), outputApk);
}

// ─── Merge Bundle ─────────────────────────────────────────────────────────────
void ApkManager::mergeBundle(const QString &bundlePath, const QString &outputApk)
{
    QString apkeditor = m_res->toolPath("APKEditor.jar");
    if (!QFile::exists(apkeditor)) {
        emit log("APKEditor.jar not found!", "error");
        emit finished(false, {});
        return;
    }

    QStringList jarArgs = {apkeditor, "m", "-i", bundlePath, "-o", outputApk};
    emit log("Merging bundle: " + QFileInfo(bundlePath).fileName(), "info");
    runProcess(buildJavaCmd(jarArgs), outputApk);
}

// ─── Sign APK ─────────────────────────────────────────────────────────────────
void ApkManager::signApk(const QString &inputApk, const QString &outputApk)
{
    QString signer = resolveApksignerPath();
    if (!QFile::exists(signer)) {
        emit log("apksigner.jar not found!", "error");
        emit finished(false, {});
        return;
    }

    QString output = outputApk.isEmpty()
                   ? QFileInfo(inputApk).path() + "/" + QFileInfo(inputApk).baseName() + "_signed.apk"
                   : outputApk;

    const AppSettings &s = m_settings->settings();
    QStringList jarArgs = {signer, "sign"};

    // Keystore
    if (s.useCustomKeystore && !s.keystorePath.isEmpty()) {
        QString ksPath = s.keystorePath;
        if (!QFile::exists(ksPath))
            ksPath = m_res->keysDir() + "/" + s.keystorePath;
        if (!QFile::exists(ksPath)) {
            emit log("Custom keystore not found: " + s.keystorePath, "error");
            emit finished(false, {});
            return;
        }
        jarArgs << "--ks" << ksPath;
        if (!s.keystorePass.isEmpty()) jarArgs << "--ks-pass" << ("pass:" + s.keystorePass);
        if (!s.keyAlias.isEmpty())     jarArgs << "--ks-key-alias" << s.keyAlias;
        if (!s.keyPass.isEmpty())      jarArgs << "--key-pass" << ("pass:" + s.keyPass);
    } else {
        // Test keys
        QString pk8 = m_res->keyPath("testkey.pk8");
        QString pem = m_res->keyPath("testkey.x509.pem");
        if (QFile::exists(pk8) && QFile::exists(pem)) {
            jarArgs << "--key" << pk8 << "--cert" << pem;
        } else {
            // Fallback: auto-generate debug.keystore via keytool
            QString debugKs = m_res->keysDir() + "/debug.keystore";
            if (!QFile::exists(debugKs)) {
                emit log("⚠ Test keys not found. Generating debug.keystore...", "warning");
                const AppSettings &cs = m_settings->settings();
                QString keytool = cs.javaPath.isEmpty() ? "keytool"
                                  : QFileInfo(cs.javaPath).absolutePath() + "/keytool.exe";
                if (!QFile::exists(keytool)) keytool = "keytool";

                QProcess proc;
                proc.start(keytool, QStringList()
                    << "-genkey" << "-v"
                    << "-keystore" << debugKs
                    << "-storepass" << "android"
                    << "-alias" << "androiddebugkey"
                    << "-keypass" << "android"
                    << "-keyalg" << "RSA"
                    << "-keysize" << "2048"
                    << "-validity" << "10000"
                    << "-dname" << "CN=Android Debug,O=Android,C=US");
                proc.waitForFinished(30000);
                if (proc.exitCode() == 0)
                    emit log("✓ debug.keystore generated.", "success");
                else {
                    emit log("Failed to generate debug.keystore.", "error");
                    emit finished(false, {});
                    return;
                }
            }
            if (QFile::exists(debugKs)) {
                jarArgs << "--ks" << debugKs
                        << "--ks-pass" << "pass:android"
                        << "--ks-key-alias" << "androiddebugkey"
                        << "--key-pass" << "pass:android";
                emit log("Using debug.keystore", "info");
            } else {
                emit log("No signing keys found.", "error");
                emit finished(false, {});
                return;
            }
        }
    }

    // Signing schemes
    if (s.v1Signing) jarArgs << "--v1-signing-enabled" << "true";
    if (s.v2Signing) jarArgs << "--v2-signing-enabled" << "true";
    if (s.v3Signing) jarArgs << "--v3-signing-enabled" << "true";

    jarArgs << "--out" << output << inputApk;

    emit log("Signing APK...", "info");
    runProcess(buildJavaCmd(jarArgs), output);
}

// ─── Zipalign ─────────────────────────────────────────────────────────────────
void ApkManager::zipalignApk(const QString &inputApk, const QString &outputApk, int alignment)
{
    QString zipalign = m_res->toolPath("zipalign.exe");
    if (!QFile::exists(zipalign)) {
        emit log("zipalign.exe not found!", "error");
        emit finished(false, {});
        return;
    }

    int alignVal = m_settings->settings().zipalignAlignment > 0
                 ? m_settings->settings().zipalignAlignment
                 : alignment;

    QString output = outputApk.isEmpty()
                   ? QFileInfo(inputApk).path() + "/" + QFileInfo(inputApk).baseName() + "_aligned.apk"
                   : outputApk;

    QStringList args = {zipalign, "-f", QString::number(alignVal), inputApk, output};
    emit log(QString("Zipaligning APK (alignment: %1)...").arg(alignVal), "info");
    runProcess(args, output);
}

// ─── Verify Signature ─────────────────────────────────────────────────────────
QString ApkManager::verifySignature(const QString &apkPath)
{
    QString signer = resolveApksignerPath();
    if (!QFile::exists(signer))
        return "apksigner.jar not found.";

    const AppSettings &s = m_settings->settings();
    QString java = s.javaPath.isEmpty() ? "java" : s.javaPath;

    QProcess proc;
    QStringList args;
    args << java;
    if (s.heapSize > 0) args << QString("-Xmx%1m").arg(s.heapSize);
    args << "-jar" << signer << "verify" << "--print-certs" << "-v" << apkPath;

    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(args.first(), args.mid(1));
    proc.waitForFinished(30000);
    return QString::fromUtf8(proc.readAll());
}

// ─── Baksmali ─────────────────────────────────────────────────────────────────
void ApkManager::baksmali(const QString &dexFile, const QString &outputDir)
{
    QString jar = m_res->toolPath("baksmali.jar");
    if (!QFile::exists(jar)) {
        emit log("baksmali.jar not found!", "error");
        emit finished(false, {});
        return;
    }
    QStringList jarArgs = {jar, "d", dexFile, "-o", outputDir};
    emit log("Disassembling DEX file...", "info");
    runProcess(buildJavaCmd(jarArgs), outputDir);
}

// ─── Smali ────────────────────────────────────────────────────────────────────
void ApkManager::smali(const QString &smaliDir, const QString &outputDex)
{
    QString jar = m_res->toolPath("smali.jar");
    if (!QFile::exists(jar)) {
        emit log("smali.jar not found!", "error");
        emit finished(false, {});
        return;
    }
    QStringList jarArgs = {jar, "a", smaliDir, "-o", outputDex};
    emit log("Assembling Smali files...", "info");
    runProcess(buildJavaCmd(jarArgs), outputDex);
}

// ─── Framework ────────────────────────────────────────────────────────────────
void ApkManager::installFramework(const QString &frameworkApk)
{
    QString apktool = resolveApktoolPath();
    if (!QFile::exists(apktool)) {
        emit log("apktool.jar not found!", "error");
        emit finished(false, {});
        return;
    }
    QStringList jarArgs = {apktool, "if", frameworkApk, "-p", m_res->frameworkDir()};
    emit log("Installing framework: " + QFileInfo(frameworkApk).fileName(), "info");
    runProcess(buildJavaCmd(jarArgs), {});
}

bool ApkManager::clearFramework()
{
    QDir fw(m_res->frameworkDir());
    bool ok = true;
    for (const QFileInfo &fi : fw.entryInfoList(QDir::Files))
        ok &= fi.dir().remove(fi.fileName());
    if (ok) emit log("✓ Framework cache cleared", "success");
    return ok;
}

// ─── APK Info (aapt) ──────────────────────────────────────────────────────────
QMap<QString,QString> ApkManager::getApkInfo(const QString &apkPath)
{
    QMap<QString,QString> info;
    QString aapt = resolveAaptPath();
    if (!QFile::exists(aapt)) return info;

    QProcess proc;
    proc.start(aapt, {"dump", "badging", apkPath});
    if (!proc.waitForFinished(30000)) return info;

    QString output = QString::fromUtf8(proc.readAllStandardOutput());

    // Package / version
    QRegularExpression pkgRe(R"(package: name='([^']+)' versionCode='([^']+)' versionName='([^']+)')");
    auto m = pkgRe.match(output);
    if (m.hasMatch()) {
        info["package_name"]  = m.captured(1);
        info["version_code"]  = m.captured(2);
        info["version_name"]  = m.captured(3);
    }

    // SDK
    QRegularExpression minRe(R"(sdkVersion:'(\d+)')");
    m = minRe.match(output);
    if (m.hasMatch()) info["min_sdk"] = m.captured(1);

    QRegularExpression tgtRe(R"(targetSdkVersion:'(\d+)')");
    m = tgtRe.match(output);
    if (m.hasMatch()) info["target_sdk"] = m.captured(1);

    // Permissions
    QRegularExpression permRe(R"(uses-permission: name='([^']+)')");
    QStringList perms;
    auto it = permRe.globalMatch(output);
    while (it.hasNext()) perms << it.next().captured(1);
    info["permissions"] = perms.join(", ");

    return info;
}

// ─── Decompiler detection ─────────────────────────────────────────────────────
QString ApkManager::detectDecompiler(const QString &projectPath)
{
    if (QFile::exists(projectPath + "/.apktool_project") ||
        QFile::exists(projectPath + "/apktool.yml"))
        return "apktool";
    if (QFile::exists(projectPath + "/.apkeditor_project"))
        return "apkeditor";
    return "apktool"; // default
}
