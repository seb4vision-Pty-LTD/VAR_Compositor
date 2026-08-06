#pragma once

#include <QtWidgets/QMainWindow>
#include <QStringList>
#include "ui_VAR_Compositor_Control_Application.h"

class QListWidget;
class QGridLayout;

class VAR_Compositor_Control_Application : public QMainWindow
{
    Q_OBJECT

public:
    VAR_Compositor_Control_Application(QWidget *parent = nullptr);
    ~VAR_Compositor_Control_Application();

private:
    Ui::VAR_Compositor_Control_ApplicationClass ui;
    QListWidget* customTextList_;
    QStringList customTexts_;
    QString serverHost_;
    quint16 serverPort_;

    void buildLayout();
    void buildMenu();
    void addGridButton(QGridLayout* layout, int row, int col, const QString& label, const QString& command);
    void addCustomText();
    void sendCustomText(const QString& text);
    bool sendTcpCommand(const QString& command, QString& reply);
    void showToast(const QString& message);
    void refreshCustomTextList();
    void loadCustomTexts();
    void saveCustomTexts() const;
};

