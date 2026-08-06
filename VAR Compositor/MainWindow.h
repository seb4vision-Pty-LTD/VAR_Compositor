#pragma once

#include <QWidget>

class QListWidget;
class QPushButton;
class PipelineController;

class MainWindow : public QWidget {
public:
	explicit MainWindow(PipelineController& controller, QWidget* parent = nullptr);

	WId previewWindowId() const;
	void refreshModeUi();

private:
	PipelineController& controller_;
	QWidget* previewWidget_;
	QListWidget* textListWidget_;
	QPushButton* varModeButton_;
	QPushButton* programModeButton_;

	void syncModeUi();
};
