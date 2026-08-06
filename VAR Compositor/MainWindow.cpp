#include "MainWindow.h"

#include "PipelineController.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

MainWindow::MainWindow(PipelineController& controller, QWidget* parent)
	: QWidget(parent),
	controller_(controller),
	previewWidget_(new QWidget(this)),
	textListWidget_(new QListWidget(this)),
	varModeButton_(new QPushButton("VAR", this)),
	programModeButton_(new QPushButton("Program", this)) {
	setWindowTitle("VAR Compositor");

	auto* rootLayout = new QHBoxLayout(this);
	auto* leftPanelLayout = new QVBoxLayout();
	auto* rightPanelLayout = new QVBoxLayout();

	auto* modeLabel = new QLabel("Output Mode", this);
	leftPanelLayout->addWidget(modeLabel);

	varModeButton_->setCheckable(true);
	programModeButton_->setCheckable(true);

	auto* modeButtonsLayout = new QHBoxLayout();
	modeButtonsLayout->addWidget(varModeButton_);
	modeButtonsLayout->addWidget(programModeButton_);
	leftPanelLayout->addLayout(modeButtonsLayout);

	auto* quickTextLabel = new QLabel("Default Text", this);
	leftPanelLayout->addWidget(quickTextLabel);

	auto* customTextInput = new QLineEdit(this);
	customTextInput->setPlaceholderText("Type custom text");
	leftPanelLayout->addWidget(customTextInput);

	auto* addCustomTextButton = new QPushButton("Add Text", this);
	leftPanelLayout->addWidget(addCustomTextButton);

	const QStringList defaultTexts{
		"RED CARD",
		"GOAL",
		"VAR REVIEW FILES",
		"PENALTY",
		"OFFSIDE",
		"POSSIBLE OFFSIDE"
	};

	textListWidget_->addItems(defaultTexts);
	textListWidget_->setMinimumWidth(220);
	leftPanelLayout->addWidget(textListWidget_, 1);

	auto* previewLabel = new QLabel("Output Preview", this);
	rightPanelLayout->addWidget(previewLabel);

	previewWidget_->setAttribute(Qt::WA_NativeWindow, true);
	previewWidget_->setStyleSheet("background-color: black;");
	rightPanelLayout->addWidget(previewWidget_, 1);

	rootLayout->addLayout(leftPanelLayout);
	rootLayout->addLayout(rightPanelLayout, 1);

	connect(varModeButton_, &QPushButton::clicked, this, [this]() {
		if (controller_.setMode(PipelineMode::Var)) {
			syncModeUi();
		}
	});

	connect(programModeButton_, &QPushButton::clicked, this, [this]() {
		if (controller_.setMode(PipelineMode::Program)) {
			syncModeUi();
		}
	});

	connect(textListWidget_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
		if (item) {
			controller_.setOverlayText(item->text().toStdString());
		}
		});

	auto addCustomText = [this, customTextInput]() {
		const QString customText = customTextInput->text().trimmed();
		if (customText.isEmpty()) {
			return;
		}

		textListWidget_->addItem(customText);
		textListWidget_->setCurrentRow(textListWidget_->count() - 1);
		controller_.setOverlayText(customText.toStdString());
		customTextInput->clear();
	};

	connect(addCustomTextButton, &QPushButton::clicked, this, addCustomText);
	connect(customTextInput, &QLineEdit::returnPressed, this, addCustomText);

	if (textListWidget_->count() > 0) {
		textListWidget_->setCurrentRow(0);
	}

	syncModeUi();
}

WId MainWindow::previewWindowId() const {
	return previewWidget_->winId();
}

void MainWindow::syncModeUi() {
	const bool isVarMode = controller_.mode() == PipelineMode::Var;
	varModeButton_->setChecked(isVarMode);
	programModeButton_->setChecked(!isVarMode);
	textListWidget_->setEnabled(isVarMode);
}
