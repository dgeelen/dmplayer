#include <QApplication>

#include "MainWindow.h"
#include "../network-handler.h"
#include <boost/bind.hpp>

using namespace std;

int main( int argc, char **argv )
{
	QApplication app(argc, argv);

	MainWindow mainwindow;
	mainwindow.show();

	network_handler nh(12345);
	nh.add_server_signal.connect(
		boost::bind(&MainWindow::UpdateServerList, &mainwindow, _1
		)
	);

	return app.exec();
}
