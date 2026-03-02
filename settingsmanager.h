#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 * AppSettings – mirrors Python's AppSettings dataclass.
 * All tool paths are relative to ResourceManager::baseDir().
 */
struct AppSettings {
    // General
    QString language            = "en";
    QString theme               = "dark";

    // Decompile
    QString defaultDecompiler   = "apktool";  // "apktool" | "apkeditor"
    bool decodeResources        = true;
    bool decodeSources          = true;
    bool clearFrameworkBefore   = false;
    bool useAapt2               = false;

    // Compile / Sign
    bool autoSignAfterCompile   = true;
    bool autoZipalign           = true;
    bool zipalignBeforeSign     = true;
    bool v1Signing              = true;
    bool v2Signing              = true;
    bool v3Signing              = false;
    int  zipalignAlignment      = 4;

    // Java
    QString javaPath            = "";       // auto-detected at startup
    int     heapSize            = 2048;     // MB for -Xmx
    bool    suppressJavaWarnings = true;

    // Custom tool paths
    bool    useCustomApktool    = false;
    QString customApktoolPath   = "";       // filename in custom/ dir

    bool    useCustomAapt       = false;
    QString customAaptPath      = "";

    bool    useCustomAapt2      = false;
    QString customAapt2Path     = "";

    QString apksignerPath       = "";       // override custom path

    // Keystore
    bool    useCustomKeystore   = false;
    QString keystorePath        = "";
    QString keystorePass        = "";
    QString keyAlias            = "";
    QString keyPass             = "";
    QJsonObject savedKeystores; // Cached keystore profiles (path -> {pass, alias, keypass})

    // Output / paths
    QString defaultOutputDir    = "output";
    QString defaultWorkspaceDir = "workspace";
    QString backupOutputDir     = "output/backups";
    int     compressionLevel    = 5;
    bool    forceOverwrite      = false;

    // Serialisation helpers
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);
};

// ─────────────────────────────────────────────────────────────
/**
 * SettingsManager – load/save AppSettings from a JSON file.
 * Mirrors Python's SettingsManager class.
 */
class SettingsManager : public QObject
{
    Q_OBJECT
public:
    explicit SettingsManager(const QString &configPath, QObject *parent = nullptr);

    /** Load settings from file. Missing keys keep defaults. */
    void load();

    /** Save current settings to file. */
    void save();

    AppSettings &settings()             { return m_settings; }
    const AppSettings &settings() const { return m_settings; }

    /** Auto-detect java.exe on Windows and update settings. */
    static QString autoDetectJava();
    
    /** Auto-detect keytool.exe using java path hint or broad system search. */
    static QString autoDetectKeytool(const QString &javaPathHint = QString());

private:
    QString      m_configPath;
    AppSettings  m_settings;
};
