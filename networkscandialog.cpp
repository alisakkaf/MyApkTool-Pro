#include "networkscandialog.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QThreadPool>
#include <QtConcurrent>
#include <QSet>
#include <QDebug>
#include <QApplication>

NetworkScanDialog::NetworkScanDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Network Scan");
    setFixedSize(450, 220);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    QLabel *title = new QLabel("🔍 Scanning Network...", this);
    title->setStyleSheet("font-size:14px; font-weight:bold;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    m_statusLabel = new QLabel("Identifying network interfaces...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: gray;");
    mainLayout->addWidget(m_statusLabel);

    m_statsLabel = new QLabel("Found: 0 | Scanned: 0/0", this);
    m_statsLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statsLabel);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    mainLayout->addWidget(m_progress);

    m_btnCancel = new QPushButton("Cancel", this);
    m_btnCancel->setStyleSheet("background-color: #c62828; color: white; border-radius: 4px; padding: 6px;");
    mainLayout->addWidget(m_btnCancel, 0, Qt::AlignHCenter);

    connect(m_btnCancel, &QPushButton::clicked, this, &NetworkScanDialog::onCancel);
    connect(&m_watcher, &QFutureWatcherBase::progressValueChanged, this, &NetworkScanDialog::updateProgress);
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &NetworkScanDialog::onScanFinished);

    // Give UI a moment to show up
    QTimer::singleShot(500, this, &NetworkScanDialog::startScan);
}

NetworkScanDialog::~NetworkScanDialog()
{
    m_cancelled = true;
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

QStringList NetworkScanDialog::foundIps() const
{
    return m_foundIps;
}

void NetworkScanDialog::onCancel()
{
    m_cancelled = true;
    m_statusLabel->setText("Cancelling...");
    m_watcher.cancel();
}

void NetworkScanDialog::updateProgress()
{
    int i = m_watcher.progressValue();
    m_progress->setValue(i);
    m_statsLabel->setText(QString("Found: %1 | Scanned: %2/%3")
                          .arg(m_foundCount.load())
                          .arg(i)
                          .arg(m_totalTargets));
}

void NetworkScanDialog::onScanFinished()
{
    // Collect valid results
    m_foundIps.clear();
    for (int i = 0; i < m_watcher.future().resultCount(); ++i) {
        QString ip = m_watcher.future().resultAt(i);
        if (!ip.isEmpty()) m_foundIps.append(ip);
    }
    accept();
}

QStringList NetworkScanDialog::detectSubnets()
{
    QSet<QString> subnets;
    QSet<QString> excludedIps;

    QProcess p;
    p.start("ipconfig");
    p.waitForFinished();
    QString output = QString::fromLocal8Bit(p.readAllStandardOutput());

    // Basic heuristic to grab IPv4 Address. On Arabic Windows 'IPv4 Address' might be localized.
    QRegularExpression r("IPv4 Address[ .]*: (\\d+\\.\\d+\\.\\d+\\.\\d+)");
    QRegularExpressionMatchIterator i = r.globalMatch(output);
    bool foundAny = false;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString ip = match.captured(1);
        foundAny = true;
        if (ip.startsWith("127.") || ip.startsWith("169.254.") || ip.endsWith(".255") || ip == "0.0.0.0")
            continue;
        
        excludedIps.insert(ip);
        QStringList parts = ip.split(".");
        if (parts.size() == 4) {
            subnets.insert(QString("%1.%2.%3").arg(parts[0], parts[1], parts[2]));
        }
    }

    if (!foundAny) {
        // Fallback catch-all for any IP address
        QRegularExpression r2("(\\d+\\.\\d+\\.\\d+\\.\\d+)");
        QRegularExpressionMatchIterator i2 = r2.globalMatch(output);
        while (i2.hasNext()) {
            QRegularExpressionMatch match = i2.next();
            QString ip = match.captured(1);
            if (ip.startsWith("127.") || ip.startsWith("169.254.") || ip.endsWith(".255") || ip == "0.0.0.0")
                continue;
            
            excludedIps.insert(ip);
            QStringList parts = ip.split(".");
            if (parts.size() == 4) {
                subnets.insert(QString("%1.%2.%3").arg(parts[0], parts[1], parts[2]));
            }
        }
    }

    // Default fallbacks
    subnets.insert("192.168.0");
    subnets.insert("192.168.1");
    subnets.insert("192.168.8");
    subnets.insert("192.168.10");
    subnets.insert("192.168.31");
    subnets.insert("192.168.43");
    subnets.insert("172.20.10");
    subnets.insert("10.0.0");

    QStringList finalTargets;
    for (const QString &sub : subnets) {
        for (int k = 2; k < 255; ++k) {
            QString target = sub + "." + QString::number(k);
            if (!excludedIps.contains(target) && !target.endsWith(".1")) {
                finalTargets.append(target);
            }
        }
    }
    finalTargets.removeDuplicates();
    return finalTargets;
}

QString NetworkScanDialog::pingHost(const QString &ip)
{
    if (m_cancelled) return QString();

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

#ifdef Q_OS_WIN
    // Mirror Python's CREATE_NO_WINDOW + STARTF_USESHOWWINDOW to avoid
    // hundreds of flashing CMD windows that block Windows process spawning
    p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW;
        args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow = SW_HIDE;
    });
    p.start("ping", QStringList() << "-n" << "1" << "-w" << "300" << ip);
#else
    p.start("ping", QStringList() << "-c" << "1" << "-W" << "0.3" << ip);
#endif

    if (!p.waitForFinished(1500)) {
        p.kill();
        return QString();
    }

    QString output = QString::fromLocal8Bit(p.readAll()).toLower();

    // On Windows, ping returns 0 even for "Destination host unreachable". Check for "ttl="
    if (p.exitCode() == 0 && output.contains("ttl=")) {
        m_foundCount++;
        return ip;
    }
    return QString();
}

struct PingFunctor {
    typedef QString result_type;
    NetworkScanDialog *dlg;
    QString operator()(const QString &ip) {
        return dlg->pingHost(ip);
    }
};

void NetworkScanDialog::startScan()
{
    m_targets = detectSubnets();
    m_totalTargets = m_targets.size();
    
    m_statusLabel->setText(QString("Scanning %1 IP addresses...").arg(m_totalTargets));
    m_progress->setRange(0, m_totalTargets);

    QThreadPool::globalInstance()->setMaxThreadCount(200);

    PingFunctor pinger;
    pinger.dlg = this;

    auto future = QtConcurrent::mapped(m_targets, pinger);

    m_watcher.setFuture(future);
}
