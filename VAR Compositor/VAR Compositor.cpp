#include <QApplication>
#include <QTimer>

#include <cstdint>
#include <iostream>
#include <string>

#include "MainWindow.h"
#include "PipelineController.h"

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	const std::string templateUri = "C:/Temp/VAR.mp4";
	const std::string initialOverlayText = "RED CARD";

	PipelineController pipelineController;
	if (!pipelineController.initialize(templateUri, initialOverlayText)) {
		return -1;
	}

	MainWindow window(pipelineController);
	window.resize(1600, 900);
	window.show();

	pipelineController.setPreviewWindowHandle(static_cast<std::uintptr_t>(window.previewWindowId()));

	if (!pipelineController.start()) {
		return -1;
	}

	QTimer busTimer;
	QObject::connect(&busTimer, &QTimer::timeout, [&pipelineController, &app]() {
		if (!pipelineController.processBusMessages()) {
			app.quit();
		}
		});
	busTimer.start(15);

	std::cout << "Running compositor with Qt UI." << std::endl;
	const int result = app.exec();

	pipelineController.stop();
	return result;
}
