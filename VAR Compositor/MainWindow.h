#pragma once

#include <QWidget>

class QListWidget;
class PipelineController;

class MainWindow : public QWidget {
public:
	explicit MainWindow(PipelineController& controller, QWidget* parent = nullptr);

	WId previewWindowId() const;

private:
	PipelineController& controller_;
	QWidget* previewWidget_;
	QListWidget* textListWidget_;
};
