#include "customdialog.h"
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>

// ─────────────────────────────────────────────────────────────────────────────
// Colour tokens per type
// ─────────────────────────────────────────────────────────────────────────────
static const char *accent(CustomDialog::Type t) {
    switch (t) {
        case CustomDialog::Info:     return "#00d4aa";   // teal
        case CustomDialog::Warning:  return "#f5a623";   // amber
        case CustomDialog::Error:    return "#e05560";   // red
        case CustomDialog::Question: return "#7b78f5";   // indigo
        default:                     return "#00d4aa";
    }
}

static const char *accentDim(CustomDialog::Type t) {
    switch (t) {
        case CustomDialog::Info:     return "rgba(0,212,170,0.12)";
        case CustomDialog::Warning:  return "rgba(245,166,35,0.12)";
        case CustomDialog::Error:    return "rgba(224,85,96,0.12)";
        case CustomDialog::Question: return "rgba(123,120,245,0.12)";
        default:                     return "rgba(0,212,170,0.12)";
    }
}

static const char *icon(CustomDialog::Type t) {
    switch (t) {
        case CustomDialog::Info:     return "ℹ";
        case CustomDialog::Warning:  return "⚠";
        case CustomDialog::Error:    return "✕";
        case CustomDialog::Question: return "?";
        default:                     return "ℹ";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
CustomDialog::CustomDialog(Type type, const QString &title, const QString &text, QWidget *parent)
    : QDialog(parent), m_type(type)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(380);
    setMaximumWidth(500);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);  // margin for outer shadow
    root->setSpacing(0);

    // ── Card container ────────────────────────────────────────
    QWidget *card = new QWidget(this);
    card->setObjectName("card");
    card->setStyleSheet(QString(
        "#card {"
        "  background: #16162a;"
        "  border: 1px solid rgba(255,255,255,0.08);"
        "  border-radius: 14px;"
        "}"
    ));

    // Drop shadow on card
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 140));
    card->setGraphicsEffect(shadow);

    QVBoxLayout *cl = new QVBoxLayout(card);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);

    // ── Accent top stripe ─────────────────────────────────────
    QWidget *stripe = new QWidget(card);
    stripe->setFixedHeight(3);
    stripe->setStyleSheet(QString(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "  stop:0 %1, stop:1 transparent);"
        "border-top-left-radius: 14px;"
        "border-top-right-radius: 14px;"
    ).arg(accent(type)));
    cl->addWidget(stripe);

    // ── Body ──────────────────────────────────────────────────
    QWidget *body = new QWidget(card);
    QVBoxLayout *bl = new QVBoxLayout(body);
    bl->setContentsMargins(22, 18, 22, 20);
    bl->setSpacing(14);

    // Icon + Title row
    QHBoxLayout *hd = new QHBoxLayout();
    hd->setSpacing(12);

    // Icon badge
    QLabel *iconBadge = new QLabel(icon(type), body);
    iconBadge->setFixedSize(36, 36);
    iconBadge->setAlignment(Qt::AlignCenter);
    iconBadge->setStyleSheet(QString(
        "background: %1;"
        "color: %2;"
        "font-size: 15px;"
        "font-weight: bold;"
        "border-radius: 18px;"
    ).arg(accentDim(type)).arg(accent(type)));

    // Title
    QLabel *titleLbl = new QLabel(title, body);
    titleLbl->setStyleSheet(
        "color: #f0f0ff;"
        "font-size: 14px;"
        "font-weight: 700;"
        "background: transparent;"
    );

    hd->addWidget(iconBadge);
    hd->addWidget(titleLbl);
    hd->addStretch();
    bl->addLayout(hd);

    // Divider
    QFrame *div = new QFrame(body);
    div->setFrameShape(QFrame::HLine);
    div->setStyleSheet("background: rgba(255,255,255,0.06); border: none; max-height: 1px;");
    bl->addWidget(div);

    // Message text
    QLabel *textLbl = new QLabel(text, body);
    textLbl->setWordWrap(true);
    textLbl->setStyleSheet(
        "color: rgba(210,210,240,0.80);"
        "font-size: 12px;"
        "line-height: 1.5;"
        "background: transparent;"
    );
    bl->addWidget(textLbl);

    // ── Buttons ───────────────────────────────────────────────
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch();

    QString btnBase =
        "QPushButton {"
        "  min-width: 80px;"
        "  padding: 7px 20px;"
        "  border-radius: 7px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  border: none;"
        "}";

    if (type == Question) {
        QPushButton *btnNo  = new QPushButton("No",  body);
        QPushButton *btnYes = new QPushButton("Yes", body);

        btnNo->setStyleSheet(btnBase +
            "QPushButton { background: rgba(255,255,255,0.07); color: #c0c0d8; }"
            "QPushButton:hover { background: rgba(255,255,255,0.12); }");

        btnYes->setStyleSheet(btnBase + QString(
            "QPushButton { background: %1; color: #fff; }"
            "QPushButton:hover { background: %1; opacity: 0.85; }"
        ).arg(accent(type)));

        connect(btnNo,  &QPushButton::clicked, this, &QDialog::reject);
        connect(btnYes, &QPushButton::clicked, this, &QDialog::accept);

        btnRow->addWidget(btnNo);
        btnRow->addWidget(btnYes);
        btnNo->setFocus();
    } else {
        QPushButton *btnOk = new QPushButton("OK", body);
        btnOk->setStyleSheet(btnBase + QString(
            "QPushButton { background: %1; color: #fff; }"
            "QPushButton:hover { background: %1; opacity: 0.85; }"
        ).arg(accent(type)));
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        btnRow->addWidget(btnOk);
        btnOk->setFocus();
    }

    bl->addLayout(btnRow);
    cl->addWidget(body);
    root->addWidget(card);
}

// ─────────────────────────────────────────────────────────────────────────────
void CustomDialog::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton)
        m_dragPos = e->globalPos() - frameGeometry().topLeft();
    QDialog::mousePressEvent(e);
}

void CustomDialog::mouseMoveEvent(QMouseEvent *e) {
    if (e->buttons() & Qt::LeftButton) {
        move(e->globalPos() - m_dragPos);
        e->accept();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void CustomDialog::showInfo(QWidget *p, const QString &t, const QString &msg)
    { CustomDialog d(Info, t, msg, p); d.exec(); }

void CustomDialog::showWarning(QWidget *p, const QString &t, const QString &msg)
    { CustomDialog d(Warning, t, msg, p); d.exec(); }

void CustomDialog::showError(QWidget *p, const QString &t, const QString &msg)
    { CustomDialog d(Error, t, msg, p); d.exec(); }

bool CustomDialog::showQuestion(QWidget *p, const QString &t, const QString &msg)
    { CustomDialog d(Question, t, msg, p); return d.exec() == QDialog::Accepted; }
