#pragma once
#include <QDialog>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPoint>

class CustomDialog : public QDialog
{
    Q_OBJECT
public:
    enum Type { Info, Warning, Error, Question };

    static void showInfo(QWidget *parent, const QString &title, const QString &text);
    static void showWarning(QWidget *parent, const QString &title, const QString &text);
    static void showError(QWidget *parent, const QString &title, const QString &text);
    static bool showQuestion(QWidget *parent, const QString &title, const QString &text);

    explicit CustomDialog(Type type, const QString &title, const QString &text, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint m_dragPos;
    Type m_type;
};
