#include "MainWindow.moc"
#include <QFileDialog>

#include <boost/format.hpp>

/**
 * convenience wrapper for boost::format
 *
 * @param fmt format string
 * @param ... values to fill in
 * @return formatted std::string
 *
 * @see http://www.boost.org/libs/format/doc/format.html
 *      for formatting info etc (its mostly compatible with printf, though)
 */
/*
#define STRFORMAT(fmt, ...) \
	((TStringUtils::detail::macro_str_format(fmt), ## __VA_ARGS__).str())

namespace TStringUtils {
	namespace detail {
		// dont allow this class easily into normal namespace
		// it has dangerous overload for comma operator
		// so hide it in detail subnamespace
		class macro_str_format {
			private:
				boost::format bfmt;
				macro_str_format();
			public:
				macro_str_format(const std::string & fmt)
					: bfmt(fmt) {};
				template<typename T>
						macro_str_format& operator,(const T & v) {
					bfmt % v;
					return *this;
				}
				std::string str() {
					return bfmt.str();
				}
		};
	}
}
*/
MainWindow::MainWindow()
{
	setupUi(this);
	handler = new mp3_handler();
}

MainWindow::~MainWindow()
{
}

void MainWindow::UpdateServerList(std::vector<server_info> sl)
{
	serverlist->clear();

	for (int i = 0; i < sl.size(); ++i) {
		QTreeWidgetItem* rwi = new QTreeWidgetItem(serverlist, 0);
		rwi->setText(0, sl[i].name.c_str());
		//rwi->setText(1, STRFORMAT("%i ms", sl[i].ping_micro_secs).c_str());
	}
	//serverlist->addTopLevelItem(
}

void MainWindow::on_OpenButton_clicked()
{
	QString fileName;
	fileName = QFileDialog::getOpenFileName(this,
     tr("Open Image"), "", tr("Audio Files (*.mp3 *.wav)"));
     file = fileName.toStdString();
     handler->Load(file);
}

void MainWindow::on_PreviousButton_clicked()
{
	//skip
}

void MainWindow::on_PlayButton_clicked()
{
	handler->Play();
}

void MainWindow::on_PauseButton_clicked()
{
	handler->Pause();
}

void MainWindow::on_StopButton_clicked()
{
	handler->Stop();
}


void MainWindow::on_NextButton_clicked()
{

}

