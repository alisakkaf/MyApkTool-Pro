#include "settingsmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QFileInfo>

// ─── AppSettings JSON serialisation ──────────────────────────────────────────
QJsonObject AppSettings::toJson() const
{
    QJsonObject o;
    o["language"]              = language;
    o["theme"]                 = theme;
    o["defaultDecompiler"]     = defaultDecompiler;
    o["decodeResources"]       = decodeResources;
    o["decodeSources"]         = decodeSources;
    o["clearFrameworkBefore"]  = clearFrameworkBefore;
    o["useAapt2"]              = useAapt2;
    o["autoSignAfterCompile"]  = autoSignAfterCompile;
    o["autoZipalign"]          = autoZipalign;
    o["zipalignBeforeSign"]    = zipalignBeforeSign;
    o["v1Signing"]             = v1Signing;
    o["v2Signing"]             = v2Signing;
    o["v3Signing"]             = v3Signing;
    o["zipalignAlignment"]     = zipalignAlignment;
    o["javaPath"]              = javaPath;
    o["heapSize"]              = heapSize;
    o["suppressJavaWarnings"]  = suppressJavaWarnings;
    o["useCustomApktool"]      = useCustomApktool;
    o["customApktoolPath"]     = customApktoolPath;
    o["useCustomAapt"]         = useCustomAapt;
    o["customAaptPath"]        = customAaptPath;
    o["useCustomAapt2"]        = useCustomAapt2;
    o["customAapt2Path"]       = customAapt2Path;
    o["apksignerPath"]         = apksignerPath;
    o["useCustomKeystore"]     = useCustomKeystore;
    o["keystorePath"]          = keystorePath;
    o["keystorePass"]          = keystorePass;
    o["keyAlias"]              = keyAlias;
    o["keyPass"]               = keyPass;
    o["savedKeystores"]        = savedKeystores;
    o["defaultOutputDir"]      = defaultOutputDir;
    o["defaultWorkspaceDir"]   = defaultWorkspaceDir;
    o["backupOutputDir"]       = backupOutputDir;
    o["compressionLevel"]      = compressionLevel;
    o["forceOverwrite"]        = forceOverwrite;
    return o;
}

void AppSettings::fromJson(const QJsonObject &o)
{
    auto s  = [&](const char *k, QString &v)  { if (o.contains(k)) v  = o[k].toString(); };
    auto b  = [&](const char *k, bool   &v)  { if (o.contains(k)) v  = o[k].toBool(); };
    auto i  = [&](const char *k, int    &v)  { if (o.contains(k)) v  = o[k].toInt(); };

    s("language",            language);
    s("theme",               theme);
    s("defaultDecompiler",   defaultDecompiler);
    b("decodeResources",     decodeResources);
    b("decodeSources",       decodeSources);
    b("clearFrameworkBefore",clearFrameworkBefore);
    b("useAapt2",            useAapt2);
    b("autoSignAfterCompile",autoSignAfterCompile);
    b("autoZipalign",        autoZipalign);
    b("zipalignBeforeSign",  zipalignBeforeSign);
    b("v1Signing",           v1Signing);
    b("v2Signing",           v2Signing);
    b("v3Signing",           v3Signing);
    i("zipalignAlignment",   zipalignAlignment);
    s("javaPath",            javaPath);
    i("heapSize",            heapSize);
    b("suppressJavaWarnings",suppressJavaWarnings);
    b("useCustomApktool",    useCustomApktool);
    s("customApktoolPath",   customApktoolPath);
    b("useCustomAapt",       useCustomAapt);
    s("customAaptPath",      customAaptPath);
    b("useCustomAapt2",      useCustomAapt2);
    s("customAapt2Path",     customAapt2Path);
    s("apksignerPath",       apksignerPath);
    b("useCustomKeystore",   useCustomKeystore);
    s("keystorePath",        keystorePath);
    s("keystorePass",        keystorePass);
    s("keyAlias",            keyAlias);
    s("keyPass",             keyPass);
    if (o.contains("savedKeystores")) savedKeystores = o["savedKeystores"].toObject();
    s("defaultOutputDir",    defaultOutputDir);
    s("defaultWorkspaceDir", defaultWorkspaceDir);
    s("backupOutputDir",     backupOutputDir);
    i("compressionLevel",    compressionLevel);
    b("forceOverwrite",      forceOverwrite);
}

// ─── SettingsManager ─────────────────────────────────────────────────────────
SettingsManager::SettingsManager(const QString &configPath, QObject *parent)
    : QObject(parent), m_configPath(configPath)
{
    load();
    // Auto-detect java if not configured
    if (m_settings.javaPath.isEmpty())
        m_settings.javaPath = autoDetectJava();
}

void SettingsManager::load()
{
    QFile f(m_configPath);
    if (!f.exists()) return;
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();

    if (err.error != QJsonParseError::NoError) return;
    if (!doc.isObject()) return;

    m_settings.fromJson(doc.object());
}

void SettingsManager::save()
{
    QFile f(m_configPath);
    QDir().mkpath(QFileInfo(m_configPath).absolutePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;

    QJsonDocument doc(m_settings.toJson());
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
}

// ─── Java Auto-detection ──────────────────────────────────────────────────────
QString SettingsManager::autoDetectJava()
{
    // 1. Check JAVA_HOME
    QString javaHome = qgetenv("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaExe = javaHome + "/bin/java.exe";
        if (QFile::exists(javaExe))
            return javaExe;
    }

    // 2. Try "java" in PATH (QProcess::execute returns 0 on success)
    {
        QProcess p;
        p.start("java", {"-version"});
        if (p.waitForFinished(3000) && p.exitCode() == 0)
            return "java";
    }

    // 3. Search common Windows install paths
    const QStringList roots = {
        "C:/Program Files/Java",
        "C:/Program Files/Eclipse Adoptium",
        "C:/Program Files/Microsoft",
        "C:/Program Files/Amazon Corretto",
        "C:/Program Files/Azul Systems",
        QString(qgetenv("LOCALAPPDATA")) + "/Android/Sdk/jre/bin",
    };

    for (const QString &root : roots) {
        QDir d(root);
        if (!d.exists()) continue;
        for (const QString &sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString javaExe = root + "/" + sub + "/bin/java.exe";
            if (QFile::exists(javaExe))
                return javaExe;
        }
    }

    return "java"; // Fallback: hope it's on PATH
}

QString SettingsManager::autoDetectKeytool(const QString &javaPathHint)
{
    // 1. Check direct java path hint
    if (!javaPathHint.isEmpty() && javaPathHint != "java" && javaPathHint != "java.exe") {
        QFileInfo fi(javaPathHint);
        if (fi.exists() && fi.fileName() == "java.exe") {
            QString hintKeytool = fi.absolutePath() + "/keytool.exe";
            if (QFile::exists(hintKeytool)) return hintKeytool;
        }
    }
    
    // 2. Check JAVA_HOME
    QString javaHome = qgetenv("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaExe = javaHome + "/bin/keytool.exe";
        if (QFile::exists(javaExe)) return javaExe;
        
        // Sometimes JAVA_HOME points strictly to root without bin ending
        QString alternative = javaHome + "/keytool.exe";
        if (QFile::exists(alternative)) return alternative;
    }

    // 3. Search common Windows install paths (matches Python's logic)
    const QStringList roots = {
        "C:/Program Files/Java",
        "C:/Program Files (x86)/Java",
        "C:/Program Files/Eclipse Adoptium",
        "C:/Program Files/Microsoft",
        "C:/Program Files/Amazon Corretto",
        "C:/Program Files/Azul Systems",
        "C:/Program Files/OpenLogic",
        "C:/Program Files/Android/Android Studio/jbr/bin",
        "C:/Program Files/Android/Android Studio/jre/bin",
        QString(qgetenv("LOCALAPPDATA")) + "/Android/Sdk/jre/bin",
        QString(qgetenv("USERPROFILE")) + "/.jdks"
    };

    for (const QString &root : roots) {
        QDir d(root);
        if (!d.exists()) continue;
        
        // If the path itself is a bin directory (like jbr/bin)
        if (d.dirName() == "bin") {
            QString kt = root + "/keytool.exe";
            if (QFile::exists(kt)) return kt;
        }

        // Otherwise iterate kids
        for (const QString &sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString ktPath1 = root + "/" + sub + "/bin/keytool.exe"; // typical jdk layout
            if (QFile::exists(ktPath1)) return ktPath1;
            
            QString ktPath2 = root + "/" + sub + "/jre/bin/keytool.exe";
            if (QFile::exists(ktPath2)) return ktPath2;
        }
    }

    return "keytool"; // System PATH fallback
}
