#pragma once

#include "MainWindow.h"
#include "PipelineController.h"

#include <QApplication>
#include <QByteArray>
#include <QObject>
#include <QHash>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

class TcpCommandServer : public QObject {
public:
	explicit TcpCommandServer(PipelineController& controller, MainWindow& window, QApplication& app, QObject* parent = nullptr);
	~TcpCommandServer() override;

	bool start(quint16 port);

private:
	void handleNewConnection();
	void handleSocketData(QTcpSocket* socket);
	void removeSocket(QTcpSocket* socket);
	void handleCommand(const QString& command, QTcpSocket* socket);
	void sendReply(QTcpSocket* socket, const QString& reply);

	PipelineController& controller_;
	MainWindow& window_;
	QApplication& app_;
	QTcpServer* server_;
	QHash<QTcpSocket*, QByteArray> receiveBuffers_;
};

inline TcpCommandServer::TcpCommandServer(PipelineController& controller, MainWindow& window, QApplication& app, QObject* parent)
	: QObject(parent),
	controller_(controller),
	window_(window),
	app_(app),
	server_(new QTcpServer(this)) {
}

inline TcpCommandServer::~TcpCommandServer() {
	if (server_->isListening()) {
		server_->close();
	}
}

inline bool TcpCommandServer::start(quint16 port) {
	connect(server_, &QTcpServer::newConnection, this, [this]() {
		handleNewConnection();
	});

	return server_->listen(QHostAddress::Any, port);
}

inline void TcpCommandServer::handleNewConnection() {
	while (server_->hasPendingConnections()) {
		QTcpSocket* socket = server_->nextPendingConnection();
		receiveBuffers_.insert(socket, QByteArray());

		connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
			handleSocketData(socket);
		});

		connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
			removeSocket(socket);
			socket->deleteLater();
		});

		sendReply(socket, "OK Connected\n");
	}
}

inline void TcpCommandServer::handleSocketData(QTcpSocket* socket) {
	if (!socket) {
		return;
	}

	QByteArray& buffer = receiveBuffers_[socket];
	buffer.append(socket->readAll());

	while (true) {
		const int newLineIndex = buffer.indexOf('\n');
		if (newLineIndex < 0) {
			break;
		}

		QByteArray rawLine = buffer.left(newLineIndex);
		buffer.remove(0, newLineIndex + 1);

		if (!rawLine.isEmpty() && rawLine.endsWith('\r')) {
			rawLine.chop(1);
		}

		handleCommand(QString::fromUtf8(rawLine), socket);
	}
}

inline void TcpCommandServer::removeSocket(QTcpSocket* socket) {
	receiveBuffers_.remove(socket);
}

inline void TcpCommandServer::handleCommand(const QString& command, QTcpSocket* socket) {
	const QString trimmed = command.trimmed();
	if (trimmed.isEmpty()) {
		sendReply(socket, "ERR Empty command\n");
		return;
	}

	const QString upper = trimmed.toUpper();

	if (upper == "KILL" || upper == "EXIT" || upper == "QUIT") {
		sendReply(socket, "OK Shutting down\n");
		app_.quit();
		return;
	}

	if (upper == "VAR" || upper == "MODE VAR" || upper == "SWITCH VAR") {
		if (controller_.setMode(PipelineMode::Var)) {
			window_.refreshModeUi();
			sendReply(socket, "OK MODE VAR\n");
		}
		else {
			sendReply(socket, "ERR Failed to switch to VAR\n");
		}
		return;
	}

	if (upper == "PROGRAM" || upper == "MODE PROGRAM" || upper == "SWITCH PROGRAM") {
		if (controller_.setMode(PipelineMode::Program)) {
			window_.refreshModeUi();
			sendReply(socket, "OK MODE PROGRAM\n");
		}
		else {
			sendReply(socket, "ERR Failed to switch to PROGRAM\n");
		}
		return;
	}

	if (upper == "TEXT") {
		controller_.setOverlayText("");
		sendReply(socket, "OK TEXT CLEARED\n");
		return;
	}

	if (trimmed.startsWith("TEXT ", Qt::CaseInsensitive)) {
		QString textPayload = command.mid(5);
		if (textPayload == "(Clear Overlay)") {
			textPayload.clear();
		}

		controller_.setOverlayText(textPayload.toStdString());
		sendReply(socket, "OK TEXT SET\n");
		return;
	}

	sendReply(socket, "ERR Unknown command\n");
}

inline void TcpCommandServer::sendReply(QTcpSocket* socket, const QString& reply) {
	if (!socket) {
		return;
	}

	socket->write(reply.toUtf8());
	socket->flush();
}
