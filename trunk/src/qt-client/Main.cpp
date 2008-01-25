#include <QApplication>

#include "MainWindow.h"
#include "../network-handler.h"
#include <boost/bind.hpp>
#include "QtBooster.h"

using namespace std;

int main( int argc, char **argv )
{
	QApplication app(argc, argv);

	MainWindow mainwindow;
	mainwindow.show();

	qRegisterMetaType<std::vector<server_info> >("std::vector<server_info>");
	qRegisterMetaType<server_info>("server_info");

	network_handler nh(12345);
	nh.server_list_update_signal.connect(
		QTBOOSTER(&mainwindow, MainWindow::UpdateServerList)
	);

	nh.server_list_removed_signal.connect(
		QTBOOSTER(&mainwindow, MainWindow::UpdateServerList_remove)
	);

	return app.exec();
}
