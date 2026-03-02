#ifndef NETWORKSCANDIALOG_H
#define NETWORKSCANDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QFutureWatcher>
#include <atomic>

class NetworkScanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NetworkScanDialog(QWidget *parent = nullptr);
    ~NetworkScanDialog();

    QStringList foundIps() const;

    QString pingHost(const QString &ip);

private slots:
    void onCancel();
    void updateProgress();
    void onScanFinished();

private:
    void startScan();
    QStringList detectSubnets();

    QLabel *m_statusLabel;
    QLabel *m_statsLabel;
    QProgressBar *m_progress;
    QPushButton *m_btnCancel;

    QStringList m_foundIps;
    QStringList m_targets;
    
    std::atomic<int> m_scannedCount{0};
    std::atomic<int> m_foundCount{0};
    int m_totalTargets = 0;
    bool m_cancelled = false;

    QFutureWatcher<QString> m_watcher;
};

#endif // NETWORKSCANDIALOG_H
