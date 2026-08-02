#include "MainWindow.h"

#include "PipelineController.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
#include <QVBoxLayout>

MainWindow::MainWindow(PipelineController& controller, QWidget* parent)
	: QWidget(parent), controller_(controller), previewWidget_(new QWidget(this)), textListWidget_(new QListWidget(this)) {
	setWindowTitle("VAR Compositor");

	auto* rootLayout = new QHBoxLayout(this);
	auto* leftPanelLayout = new QVBoxLayout();
	auto* rightPanelLayout = new QVBoxLayout();

	auto* quickTextLabel = new QLabel("Default Text", this);
	leftPanelLayout->addWidget(quickTextLabel);

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

	connect(textListWidget_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
		if (item) {
			controller_.setOverlayText(item->text().toStdString());
		}
		});

	if (textListWidget_->count() > 0) {
		textListWidget_->setCurrentRow(0);
	}
}

WId MainWindow::previewWindowId() const {
	return previewWidget_->winId();
}
