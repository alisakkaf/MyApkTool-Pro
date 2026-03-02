#include "keystoredialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include "customdialog.h"
#include "qdebug.h"

#include "settingsmanager.h"

KeystoreDialog::KeystoreDialog(const QString &keysDir, const QString &javaPath, QWidget *parent)
    : QDialog(parent), m_keysDir(keysDir), m_javaPath(javaPath)
{
    setupUi();
}

void KeystoreDialog::setupUi()
{
    setWindowTitle("Create New Keystore 🔐");
    setMinimumWidth(450);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header
    auto *lblTitle = new QLabel("<b>Create Keystore 🔑</b>", this);
    lblTitle->setStyleSheet("font-size: 18px;");
    auto *lblSub = new QLabel("Generate a professional signing certificate for your Android app.", this);
    lblSub->setStyleSheet("color: gray;");
    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(lblSub);
    mainLayout->addSpacing(10);

    // Sec 1
    auto *lblSec1 = new QLabel("<b>1. Keystore Details</b>", this);
    mainLayout->addWidget(lblSec1);

    auto addRow = [&](const QString &label, QLineEdit *&ext, bool isPass = false) {
        auto *row = new QHBoxLayout();
        auto *l = new QLabel(label, this);
        l->setMinimumWidth(100);
        ext = new QLineEdit(this);
        if (isPass) ext->setEchoMode(QLineEdit::Password);
        row->addWidget(l);
        row->addWidget(ext);
        mainLayout->addLayout(row);
    };

    addRow("Filename (.jks):", leFilename);
    leFilename->setText("my-release-key.jks");
    addRow("Key Alias:", leAlias);
    leAlias->setText("my_alias");
    
    auto *passLayout = new QHBoxLayout();
    lePassword = new QLineEdit(this); lePassword->setEchoMode(QLineEdit::Password); lePassword->setPlaceholderText("Password");
    leConfirm = new QLineEdit(this); leConfirm->setEchoMode(QLineEdit::Password); leConfirm->setPlaceholderText("Confirm");
    passLayout->addWidget(new QLabel("Passwords:", this));
    passLayout->addWidget(lePassword);
    passLayout->addWidget(leConfirm);
    mainLayout->addLayout(passLayout);

    addRow("Validity (Years):", leValidity);
    leValidity->setText("25");
    mainLayout->addSpacing(10);

    // Sec 2
    auto *lblSec2 = new QLabel("<b>2. Certificate Information</b>", this);
    mainLayout->addWidget(lblSec2);
    
    addRow("CN (Name):", leCN);
    leCN->setPlaceholderText("e.g. John Doe");
    addRow("OU (Org Unit):", leOU);
    leOU->setPlaceholderText("e.g. Mobile Development");
    addRow("O (Organization):", leO);
    leO->setPlaceholderText("e.g. My Company Inc.");
    
    auto *locLayout = new QHBoxLayout();
    leL = new QLineEdit(this); leL->setPlaceholderText("City (L)");
    leST = new QLineEdit(this); leST->setPlaceholderText("State (ST)");
    locLayout->addWidget(new QLabel("Location:", this));
    locLayout->addWidget(leL);
    locLayout->addWidget(leST);
    mainLayout->addLayout(locLayout);

    auto *cLayout = new QHBoxLayout();
    cmbCountry = new QComboBox(this);
    cmbCountry->setEditable(true); // هذا السطر يجعله قابلاً للكتابة
    cmbCountry->addItems({"US", "GB", "CA", "AU", "DE", "FR", "JP", "CN", "IN", "BR", "RU", "SA", "AE", "EG", "TR", "ID"});
    auto *btnHelp = new QPushButton("❓", this);
    btnHelp->setFixedWidth(30);
    connect(btnHelp, &QPushButton::clicked, this, &KeystoreDialog::onShowCountryCodes);
    cLayout->addWidget(new QLabel("Country Code:", this));
    cLayout->addWidget(cmbCountry);
    cLayout->addWidget(btnHelp);
    mainLayout->addLayout(cLayout);
    mainLayout->addSpacing(15);

    // Buttons
    auto *btnLayout = new QHBoxLayout();
    auto *btnCancel = new QPushButton("Cancel", this);
    auto *btnGenerate = new QPushButton("✨ Generate Keystore", this);
    btnGenerate->setStyleSheet("font-weight: bold;");
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnGenerate);
    mainLayout->addLayout(btnLayout);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnGenerate, &QPushButton::clicked, this, &KeystoreDialog::onGenerate);
}

void KeystoreDialog::onShowCountryCodes()
{
    CustomDialog::showInfo(this, "Country Codes",
        "Common Country Codes:\n\n"
        "US - United States\nGB - United Kingdom\nCA - Canada\n"
        "DE - Germany\nFR - France\nSA - Saudi Arabia\nAE - UAE\n"
        "EG - Egypt\nIN - India\nCN - China\n\n"
        "Use 2-letter ISO code."
    );
}

void KeystoreDialog::onGenerate()
{
    QString file = leFilename->text().trimmed();
    QString alias = leAlias->text().trimmed();
    QString pass = lePassword->text();
    QString conf = leConfirm->text();

    if (file.isEmpty() || alias.isEmpty()) {
        CustomDialog::showWarning(this, "Error", "Filename and Alias are required!");
        return;
    }
    if (pass.length() < 6) {
        CustomDialog::showWarning(this, "Error", "Password must be at least 6 characters!");
        return;
    }
    if (pass != conf) {
        CustomDialog::showWarning(this, "Error", "Passwords do not match!");
        return;
    }

    if (!file.endsWith(".jks")) file += ".jks";
    
    QDir().mkpath(m_keysDir);
    QString outPath = m_keysDir + "/" + file;
    if (QFile::exists(outPath)) {
        if (!CustomDialog::showQuestion(this, "Warning", QString("File '%1' already exists.\nOverwrite?").arg(file)))
            return;
        QFile::remove(outPath);
    }

    QStringList dname;
    if (!leCN->text().isEmpty()) dname << "CN=" + leCN->text().trimmed();
    if (!leOU->text().isEmpty()) dname << "OU=" + leOU->text().trimmed();
    if (!leO->text().isEmpty()) dname << "O=" + leO->text().trimmed();
    if (!leL->text().isEmpty()) dname << "L=" + leL->text().trimmed();
    if (!leST->text().isEmpty()) dname << "ST=" + leST->text().trimmed();
    if (!cmbCountry->currentText().isEmpty()) dname << "C=" + cmbCountry->currentText();

    QString dnameStr = dname.isEmpty() ? "CN=Unknown" : dname.join(", ");
    int days = leValidity->text().toInt();
    if (days <= 0) days = 25;
    days *= 365;

    QString keytool = SettingsManager::autoDetectKeytool(m_javaPath);
    // Safety fallback for QProcess on Windows if PATH is missing extension
    if (keytool == "keytool") {
        keytool = "keytool.exe";
    }

    QStringList args;
    args << "-genkeypair" << "-v"
         << "-keystore" << outPath
         << "-alias" << alias
         << "-keyalg" << "RSA"
         << "-keysize" << "2048"
         << "-validity" << QString::number(days)
         << "-storepass" << pass
         << "-keypass" << pass
         << "-dname" << dnameStr;

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(keytool, args);

    if (!proc.waitForStarted(5000)) {
        CustomDialog::showError(this, "Process Error", "Keytool failed to start.\nError: " + proc.errorString() + "\nPath used: " + keytool);
        return;
    }

    proc.waitForFinished(30000);
    QString output = QString::fromUtf8(proc.readAll());

    if (QFile::exists(outPath) && proc.exitCode() == 0) {
        CustomDialog::showInfo(this, "Success", QString("Keystore created successfully!\nSaved to: %1").arg(outPath));
        m_createdPath = outPath;
        m_createdPassword = pass;
        m_createdAlias = alias;
        accept();
    } else {
        QString errMsg = QString("Keytool failed with code %1.\n").arg(proc.exitCode());
        errMsg += "Missing File: " + QString(QFile::exists(outPath) ? "No" : "Yes") + "\n";
        errMsg += "Used Path: " + keytool + "\n";
        errMsg += "Output:\n" + output;
        
        CustomDialog::showError(this, "Error", errMsg);
        qDebug() << "Keytool Error:" << errMsg;
    }
}
