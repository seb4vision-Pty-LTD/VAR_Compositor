#include "VAR_Compositor_Control_Application.h"

#include <QAction>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTcpSocket>
#include <QToolTip>
#include <QVBoxLayout>

VAR_Compositor_Control_Application::VAR_Compositor_Control_Application(QWidget *parent)
    : QMainWindow(parent),
    customTextList_(nullptr),
    serverHost_("127.0.0.1"),
    serverPort_(5000)
{
    ui.setupUi(this);
    buildMenu();
    buildLayout();
    loadCustomTexts();
    refreshCustomTextList();
    statusBar()->showMessage("Ready", 1500);
}

VAR_Compositor_Control_Application::~VAR_Compositor_Control_Application()
{
    saveCustomTexts();
}

void VAR_Compositor_Control_Application::buildLayout() {
    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setSpacing(14);

    auto* leftLayout = new QVBoxLayout();
    auto* listLabel = new QLabel("Custom Texts", central);
    leftLayout->addWidget(listLabel);

    customTextList_ = new QListWidget(central);
    customTextList_->setMinimumWidth(280);
    leftLayout->addWidget(customTextList_, 1);

    auto* sendSelectedButton = new QPushButton("Send Selected Text", central);
    sendSelectedButton->setMinimumHeight(48);
    leftLayout->addWidget(sendSelectedButton);

    rootLayout->addLayout(leftLayout, 1);

    auto* gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(14);
    gridLayout->setVerticalSpacing(14);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    for (int index = 0; index < 3; ++index) {
        gridLayout->setColumnStretch(index, 1);
        gridLayout->setRowStretch(index, 1);
    }

    addGridButton(gridLayout, 0, 0, "RED CARD CHECK", "TEXT RED CARD CHECK");
    addGridButton(gridLayout, 0, 1, "GOAL CHECK", "TEXT GOAL CHECK");
    addGridButton(gridLayout, 0, 2, "MISTAKEN IDENTITY", "TEXT MISTAKEN IDENTITY");

    addGridButton(gridLayout, 1, 0, "PENALTY CHECK", "TEXT PENALTY CHECK");
    addGridButton(gridLayout, 1, 1, "MODE PROGRAM", "MODE PROGRAM");
    addGridButton(gridLayout, 1, 2, "MODE VAR", "MODE VAR");

    addGridButton(gridLayout, 2, 0, "CLEAR OVERLAY", "TEXT (Clear Overlay)");
    addGridButton(gridLayout, 2, 1, "TEXT VAR REVIEW FILES", "TEXT VAR REVIEW FILES");
    addGridButton(gridLayout, 2, 2, "TEXT OFFSIDE", "TEXT OFFSIDE");

    auto* gridContainer = new QWidget(central);
    gridContainer->setLayout(gridLayout);
    rootLayout->addWidget(gridContainer, 2);

    setCentralWidget(central);

    connect(sendSelectedButton, &QPushButton::clicked, this, [this]() {
        if (!customTextList_->currentItem()) {
            showToast("No custom text selected");
            return;
        }

        sendCustomText(customTextList_->currentItem()->text());
    });

    connect(customTextList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (!item) {
            return;
        }

        sendCustomText(item->text());
    });
}

void VAR_Compositor_Control_Application::buildMenu() {
    auto* commandsMenu = menuBar()->addMenu("Commands");
    auto* customMenu = menuBar()->addMenu("Custom");

    auto* killAction = commandsMenu->addAction("Kill Compositor");
    auto* exitAction = commandsMenu->addAction("Exit Compositor");
    auto* addCustomTextAction = customMenu->addAction("Add Custom Text");

    connect(killAction, &QAction::triggered, this, [this]() {
        QString reply;
        if (sendTcpCommand("KILL", reply)) {
            showToast("Kill command sent");
        }
        else {
            showToast(reply);
        }
    });

    connect(exitAction, &QAction::triggered, this, [this]() {
        QString reply;
        if (sendTcpCommand("EXIT", reply)) {
            showToast("Exit command sent");
        }
        else {
            showToast(reply);
        }
    });

    connect(addCustomTextAction, &QAction::triggered, this, [this]() {
        addCustomText();
    });
}

void VAR_Compositor_Control_Application::addGridButton(QGridLayout* layout, int row, int col, const QString& label, const QString& command) {
    auto* button = new QPushButton(label, this);
    button->setMinimumSize(160, 90);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    button->setStyleSheet("font-size: 20px; font-weight: 700;");
    layout->addWidget(button, row, col);

    connect(button, &QPushButton::clicked, this, [this, command]() {
        QString reply;
        if (sendTcpCommand(command, reply)) {
            showToast(reply.isEmpty() ? "Command sent" : reply);
        }
        else {
            showToast(reply);
        }
    });
}

void VAR_Compositor_Control_Application::addCustomText() {
    bool ok = false;
    const QString text = QInputDialog::getText(this, "Add Custom Text", "Text:", QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok) {
        return;
    }

    if (text.isEmpty()) {
        showToast("Empty text not stored");
        return;
    }

    if (!customTexts_.contains(text, Qt::CaseSensitive)) {
        customTexts_.append(text);
        saveCustomTexts();
        refreshCustomTextList();
    }

    sendCustomText(text);
}

void VAR_Compositor_Control_Application::sendCustomText(const QString& text) {
    QString reply;
    QString command = "TEXT " + text;
    if (text == "(Clear Overlay)") {
        command = "TEXT (Clear Overlay)";
    }

    if (sendTcpCommand(command, reply)) {
        showToast(reply.isEmpty() ? "Custom text sent" : reply);
    }
    else {
        showToast(reply);
    }
}

bool VAR_Compositor_Control_Application::sendTcpCommand(const QString& command, QString& reply) {
    QTcpSocket socket;
    socket.connectToHost(serverHost_, serverPort_);
    if (!socket.waitForConnected(1200)) {
        reply = "Failed to connect to server";
        return false;
    }

    const QByteArray payload = command.toUtf8() + '\n';
    if (socket.write(payload) == -1 || !socket.waitForBytesWritten(1200)) {
        reply = "Failed to send command";
        return false;
    }

    if (socket.waitForReadyRead(1200)) {
        reply = QString::fromUtf8(socket.readAll()).trimmed();
    }
    else {
        reply = "Command sent";
    }

    return true;
}

void VAR_Compositor_Control_Application::showToast(const QString& message) {
    statusBar()->showMessage(message, 2500);
    QToolTip::showText(mapToGlobal(rect().center()), message, this, rect(), 2000);
}

void VAR_Compositor_Control_Application::refreshCustomTextList() {
    customTextList_->clear();
    customTextList_->addItems(customTexts_);
}

void VAR_Compositor_Control_Application::loadCustomTexts() {
    QSettings settings("VARCompositor", "ControlApplication");
    customTexts_ = settings.value("customTexts").toStringList();
}

void VAR_Compositor_Control_Application::saveCustomTexts() const {
    QSettings settings("VARCompositor", "ControlApplication");
    settings.setValue("customTexts", customTexts_);
}

