#include <QApplication>

#include "MainWindow.h"
#include "../network-handler.h"
#include "../error-handling.h"
#include <boost/bind.hpp>
#include "QtBooster.h"
#include "../util/UPXFix.h"

#include <QProcess>
#include <QDir>

using namespace std;

class MyQApplication : public QApplication {
	private:
#ifdef DEBUG
		int stack;
#endif
	public:
		MyQApplication(int& argc, char** argv)
			:	QApplication(argc,argv)
#ifdef DEBUG
			,	stack(0)
#endif
		{};

		bool notify(QObject *receiver, QEvent *e)
		{
#ifdef DEBUG
			if (stack == 0) {
				bool ret;
				++stack;
				try {
					// boost::bind will bind QApplication::notify, but because it is virtual
					// the function actually called will be MyQApplication::notify
					// the 'stack' variable is nonzero now though, so an infinite loop is prevented
					ret = makeErrorHandler(boost::bind(&QApplication::notify, this, receiver, e))();
				} catch (...) {
					dcerr("Exception caught in qt event loop, see cerr for details");
					ret = false;
				};
				--stack;
				return ret;
			} else
				return QApplication::notify(receiver, e);
#else
			try {
				return QApplication::notify(receiver, e);
			} catch (...) {
				/// @todo gracefull shutdown here or something
				return false;
			}
#endif
		}
};

int main_impl(int argc, char **argv )
{
	MyQApplication app(argc, argv);

	MainWindow mainwindow;
	mainwindow.show();

	qRegisterMetaType<std::vector<server_info> >("std::vector<server_info>");
	qRegisterMetaType<server_info>("server_info");
	qRegisterMetaType<std::string>("std::string");
	qRegisterMetaType<messageref>("messageref");

	network_handler nh(12345);
	mainwindow.setNetworkHandler(&nh);
	nh.server_list_update_signal.connect(
		QTBOOSTER(&mainwindow, MainWindow::UpdateServerList)
	);

/* FIXME: This is handled through the middle_end now
	nh.server_list_removed_signal.connect(
		QTBOOSTER(&mainwindow, MainWindow::UpdateServerList_remove)
	);
*/

	nh.client_message_receive_signal.connect(
		QTBOOSTER(&mainwindow, MainWindow::handleReceivedMessage)
	);

	mainwindow.sigconnect.connect(
		boost::bind(
			&network_handler::client_connect_to_server,
			&nh, _1
		)
	);

	return app.exec();
}

int main(int argc, char* argv[]) {
	return makeErrorHandler(boost::bind(&main_impl, argc, argv))();
}

