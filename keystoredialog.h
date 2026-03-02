#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class KeystoreDialog : public QDialog
{
    Q_OBJECT
public:
    explicit KeystoreDialog(const QString &keysDir, const QString &javaPath, QWidget *parent = nullptr);

    QString createdKeystorePath() const { return m_createdPath; }
    QString createdPassword() const { return m_createdPassword; }
    QString createdAlias() const { return m_createdAlias; }

private slots:
    void onGenerate();
    void onShowCountryCodes();

private:
    void setupUi();

    QLineEdit *leFilename;
    QLineEdit *leAlias;
    QLineEdit *lePassword;
    QLineEdit *leConfirm;
    QLineEdit *leValidity;

    QLineEdit *leCN;
    QLineEdit *leOU;
    QLineEdit *leO;
    QLineEdit *leL;
    QLineEdit *leST;
    QComboBox *cmbCountry;

    QString m_keysDir;
    QString m_javaPath;
    
    QString m_createdPath;
    QString m_createdPassword;
    QString m_createdAlias;
};
