#include <QApplication>
#include <QTimer>

#include <cstdint>
#include <iostream>
#include <string>

#include "MainWindow.h"
#include "PipelineController.h"
#include "TcpCommandServer.h"

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	const std::string templateUri = "C:/Temp/VAR.mp4";
	const std::string initialOverlayText = " ";

	PipelineController pipelineController;
	if (!pipelineController.initialize(templateUri, initialOverlayText)) {
		return -1;
	}

	MainWindow window(pipelineController);
	window.resize(1600, 900);
	window.show();

	TcpCommandServer commandServer(pipelineController, window, app);
	if (!commandServer.start(5000)) {
		std::cerr << "Failed to start TCP command server on port 5000." << std::endl;
		return -1;
	}

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

	std::cout << "Running compositor with Qt UI and TCP command server on port 5000." << std::endl;
	const int result = app.exec();

	pipelineController.stop();
	return result;
}
