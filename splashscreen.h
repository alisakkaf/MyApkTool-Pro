/**
 * =====================================================================
 *  MyApkTool Pro — Animated Splash Screen
 * =====================================================================
 *  Developer  : Ali Sakkaf
 *  Since      : 2016
 *  Website    : https://mysterious-dev.com/
 *  Facebook   : https://www.facebook.com/AliSakkaf.Dev/
 *  GitHub     : https://github.com/alisakkaf
 *  Copyright  : © 2016 - 2026 Ali Sakkaf. All rights reserved.
 * =====================================================================
 *  All text values are read from version.h — change them there.
 * =====================================================================
 **/

#pragma once

#include "version.h"
#include <qmath.h>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <QPropertyAnimation>

class SplashScreen : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal arcAngle READ arcAngle WRITE setArcAngle)
    Q_PROPERTY(qreal pulse    READ pulse    WRITE setPulse)

public:
    explicit SplashScreen(QWidget * = nullptr)
        : QWidget(nullptr, Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        setFixedSize(780, 480);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);

        QRect scr = QGuiApplication::primaryScreen()->geometry();
        move((scr.width() - width()) / 2, (scr.height() - height()) / 2);

        buildUi();
        startAnimations();
        QTimer::singleShot(5000, this, &SplashScreen::readyToClose);
    }

    qreal arcAngle() const { return m_arc; }
    void  setArcAngle(qreal v) { m_arc = v; update(); }

    qreal pulse() const { return m_pulse; }
    void  setPulse(qreal v) { m_pulse = v; update(); }

signals:
    void readyToClose();

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        // ── Background gradient ────────────────────────────────
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.00, QColor(8,  12, 32));
        bg.setColorAt(0.50, QColor(12, 18, 44));
        bg.setColorAt(1.00, QColor(16,  8, 36));

        QPainterPath card;
        card.addRoundedRect(rect(), 20, 20);
        p.fillPath(card, bg);

        // ── Dot grid overlay ───────────────────────────────────
        p.setPen(QPen(QColor(255, 255, 255, 12), 1));
        for (int x = 24; x < width(); x += 32)
            for (int y = 24; y < height(); y += 32)
                p.drawEllipse(QPointF(x, y), 0.9, 0.9);

        // ── Pulsing glow halo ──────────────────────────────────
        const int cx = 190, cy = 130;
        QRadialGradient halo(cx, cy, 140);
        halo.setColorAt(0.0, QColor(0, 220, 180, int(m_pulse * 80)));
        halo.setColorAt(0.6, QColor(80,  0, 220, int(m_pulse * 30)));
        halo.setColorAt(1.0, Qt::transparent);
        p.fillRect(rect(), halo);

        // ── Spinning gradient arc ──────────────────────────────
        const int r = 50;
        QRect arcRect(cx - r, cy - r, r * 2, r * 2);

        // dim track
        p.setPen(QPen(QColor(255, 255, 255, 20), 4.0, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(arcRect);

        // active coloured sweep
        QConicalGradient arcG(cx, cy, 90.0 - m_arc * 0.1);
        arcG.setColorAt(0.0, QColor(0,   240, 190));
        arcG.setColorAt(0.5, QColor(130,  80, 255));
        arcG.setColorAt(1.0, QColor(0,   240, 190));
        p.setPen(QPen(QBrush(arcG), 4.5, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arcRect, 90 * 16, int(-m_arc) * 16);

        // glowing tip dot
        double rad = qDegreesToRadians(90.0 - m_arc);
        QPointF tip(cx + r * qCos(rad), cy - r * qSin(rad));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 240, 190));
        p.drawEllipse(tip, 5, 5);

        // percentage text inside circle
        int pct = qBound(0, qRound(m_arc / 3.6), 100);
        p.setPen(QColor(255, 255, 255, 220));
        QFont f = p.font();
        f.setFamily("Segoe UI");
        f.setPixelSize(18);
        f.setBold(true);
        p.setFont(f);
        p.drawText(arcRect, Qt::AlignCenter, QString("%1%").arg(pct));

        // card border
        p.setPen(QPen(QColor(255, 255, 255, 25), 1));
        p.setBrush(Qt::NoBrush);
        p.drawPath(card);
    }

private slots:
    void cycleStatus() {
        static const QStringList msgs = {
            "Initializing engine...",
            "Loading resources...",
            "Extracting tools (ADB, AAPT, zipalign)...",
            "Preparing workspace...",
            "Configuring ADB & device manager...",
            "Loading keystore profiles...",
            "Almost ready...",
            "\u2713  All systems go!"
        };
        static int idx = 0;
        if (idx < msgs.size()) m_statusLbl->setText(msgs[idx++]);
        else m_statusTimer->stop();
    }

private:
    // ──────────────────────────────────────────────────────────
    void buildUi()
    {
        auto *root = new QHBoxLayout(this);
        root->setContentsMargins(35, 30, 35, 30);
        root->setSpacing(40);

        // ════════════════════════════════════════════════════
        // LEFT PANEL
        // ════════════════════════════════════════════════════
        auto *left = new QVBoxLayout();
        left->setSpacing(0);
        left->addSpacing(185);  // clear space for the painted arc

        // App name (from version.h → APP_NAME)
        auto *nameLbl = new QLabel(APP_NAME, this);
        nameLbl->setStyleSheet(
            "background:transparent;"
            "color:#ffffff;"
            "font-family:'Segoe UI',Roboto,sans-serif;"
            "font-size:32px;"
            "font-weight:800;"
        );
        left->addWidget(nameLbl);

        // Tagline (from version.h → APP_DESCRIPTION)
        auto *tagLbl = new QLabel(APP_DESCRIPTION, this);
        tagLbl->setStyleSheet(
            "background:transparent;"
            "color:rgba(180,220,255,0.75);"
            "font-family:'Segoe UI',Roboto,sans-serif;"
            "font-size:12px;"
            "font-weight:600;"
        );
        left->addWidget(tagLbl);

        left->addSpacing(22);

        // Dev link (from version.h → APP_WEBSITE)
        auto *webLbl = new QLabel(APP_WEBSITE, this);
        webLbl->setStyleSheet(
            "background:transparent;"
            "color:rgba(0,200,160,0.70);"
            "font-family:'Segoe UI',Roboto,sans-serif;"
            "font-size:10px;"
            "font-weight:500;"
        );
        left->addWidget(webLbl);

        left->addSpacing(12);

        // Version badge (from version.h → APP_VERSION_STR)
        auto *ver = new QLabel(QString("  v%1  ").arg(APP_VERSION_STR), this);
        ver->setStyleSheet(
            "background:rgba(0,220,180,0.14);"
            "color:#00ddb4;"
            "font-family:'Segoe UI',Roboto,sans-serif;"
            "font-size:11px;"
            "font-weight:700;"
            "border:1px solid rgba(0,220,180,0.38);"
            "border-radius:12px;"
            "padding:3px 10px;"
        );
        ver->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        left->addWidget(ver);

        left->addSpacing(20);

        // Thin separator
        auto *sep = new QFrame(this);
        sep->setFixedHeight(1);
        sep->setStyleSheet("background:rgba(255,255,255,0.10); border:none;");
        left->addWidget(sep);

        left->addSpacing(16);

        // Status label
        m_statusLbl = new QLabel("Initializing engine...", this);
        m_statusLbl->setStyleSheet(
            "background:transparent;"
            "color:rgba(0,220,180,0.90);"
            "font-family:'Segoe UI',Roboto,sans-serif;"
            "font-size:12px;"
            "font-weight:600;"
        );
        left->addWidget(m_statusLbl);

        left->addSpacing(10);

        // Progress bar
        m_bar = new QProgressBar(this);
        m_bar->setRange(0, 100);
        m_bar->setValue(0);
        m_bar->setTextVisible(false);
        m_bar->setFixedHeight(5);
        m_bar->setStyleSheet(R"(
            QProgressBar {
                background: rgba(255,255,255,0.08);
                border: none;
                border-radius: 2px;
            }
            QProgressBar::chunk {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                    stop:0 #00ddb4, stop:1 #7c5eff);
                border-radius: 2px;
            }
        )");
        left->addWidget(m_bar);

        left->addStretch();

        // Copyright footer (from version.h → APP_AUTHOR / APP_COPYRIGHT)
        auto *foot = new QLabel(
            QString("© 2016 - 2026 %1  |  mysterious-dev.com").arg(APP_AUTHOR), this);
        foot->setStyleSheet(
            "background:transparent;"
            "color:rgba(255,255,255,0.28);"
            "font-family:'Segoe UI',sans-serif;"
            "font-size:10px;"
        );
        left->addWidget(foot);

        root->addLayout(left, 4);

        // ════════════════════════════════════════════════════
        // VERTICAL DIVIDER
        // ════════════════════════════════════════════════════
        auto *vSep = new QFrame(this);
        vSep->setFrameShape(QFrame::VLine);
        vSep->setStyleSheet(
            "background:rgba(255,255,255,0.08);"
            "border:none; max-width:1px;"
            "margin-top:20px; margin-bottom:20px;"
        );
        root->addWidget(vSep);

        // ════════════════════════════════════════════════════
        // RIGHT PANEL — Feature List
        // ════════════════════════════════════════════════════
        auto *right = new QVBoxLayout();
        right->setSpacing(6);
        right->setContentsMargins(14, 8, 0, 8);

        auto *featTitle = new QLabel("F E A T U R E S", this);
        featTitle->setStyleSheet(
            "background:transparent; color:#ffffff;"
            "font-family:'Segoe UI',Roboto,sans-serif;"
            "font-size:13px; font-weight:800;"
        );
        right->addWidget(featTitle);
        right->addSpacing(10);

        struct Feat { const char *icon; const char *text; const char *col; };
        static const Feat feats[] = {
            { "🔓", "Decompile APK  (Apktool / APKEditor)",    "#00ddb4" },
            { "🔨", "Compile & rebuild APK",                    "#00ddb4" },
            { "✍",  "Sign  (V1 / V2 / V3)  + Zipalign",       "#7c5eff" },
            { "🔑", "Keystore generator & profile manager",     "#7c5eff" },
            { "🔀", "Merge APK patches",                        "#f5a623" },
            { "⚙",  "Baksmali / Smali  DEX tools",             "#f5a623" },
            { "ℹ",  "APK Info  (AAPT dump badging)",           "#4db6ff" },
            { "📱", "ADB Device Manager",                       "#4db6ff" },
            { "📡", "WiFi Pair & network scan  (Android 11+)", "#4db6ff" },
            { "📦", "Backup / Uninstall / Install APK",         "#c0c0ff" },
            { "🎨", "Dark / Light theme  (custom QSS)",         "#c0c0ff" },
            { "💾", "Multi-keystore profiles  (auto-fill)",     "#c0c0ff" },
        };

        for (auto &f : feats) {
            auto *row = new QHBoxLayout();
            row->setSpacing(12);

            auto *ic = new QLabel(f.icon, this);
            ic->setFixedWidth(24);
            ic->setAlignment(Qt::AlignCenter);
            ic->setStyleSheet("background:transparent; font-size:14px;");

            auto *tx = new QLabel(f.text, this);
            tx->setStyleSheet(QString(
                "background:transparent;"
                "color:%1;"
                "font-family:'Segoe UI',Roboto,sans-serif;"
                "font-size:11.5px;"
                "font-weight:600;"
            ).arg(f.col));

            row->addWidget(ic);
            row->addWidget(tx);
            row->addStretch();
            right->addLayout(row);
        }

        right->addStretch();
        root->addLayout(right, 5);
    }

    // ──────────────────────────────────────────────────────────
    void startAnimations()
    {
        // Arc 0 → 360° in 4.5 s
        auto *arcAnim = new QPropertyAnimation(this, "arcAngle", this);
        arcAnim->setDuration(4500);
        arcAnim->setStartValue(0.0);
        arcAnim->setEndValue(360.0);
        arcAnim->setEasingCurve(QEasingCurve::OutCubic);
        arcAnim->start(QAbstractAnimation::DeleteWhenStopped);

        // Progress bar 0 → 100 in 4.5 s
        auto *barAnim = new QPropertyAnimation(m_bar, "value", this);
        barAnim->setDuration(4500);
        barAnim->setStartValue(0);
        barAnim->setEndValue(100);
        barAnim->setEasingCurve(QEasingCurve::OutCubic);
        barAnim->start(QAbstractAnimation::DeleteWhenStopped);

        // Glow pulse ∞
        auto *glow = new QPropertyAnimation(this, "pulse", this);
        glow->setDuration(1400);
        glow->setStartValue(0.4);
        glow->setEndValue(1.0);
        glow->setLoopCount(-1);
        glow->setEasingCurve(QEasingCurve::SineCurve);
        glow->start();

        // Cycling status messages
        m_statusTimer = new QTimer(this);
        connect(m_statusTimer, &QTimer::timeout, this, &SplashScreen::cycleStatus);
        m_statusTimer->start(700);
    }

    // ── Members ───────────────────────────────────────────────
    qreal         m_arc        = 0;
    qreal         m_pulse      = 0.6;
    QProgressBar *m_bar        = nullptr;
    QLabel       *m_statusLbl  = nullptr;
    QTimer       *m_statusTimer = nullptr;
};
