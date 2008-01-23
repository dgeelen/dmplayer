#include "../cross-platform.h"
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


// TODO:put in seperate header

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

MainWindow::MainWindow()
{
	setupUi(this);
	progressTimer.stop();
	trackProgress->setMinimum(0);
	trackProgress->setValue(0);
	QObject::connect(&progressTimer, SIGNAL(timeout()), this, SLOT(updateProgressBar()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::UpdateServerList(std::vector<server_info> sl)
{
	serverlist->clear();

	for (uint i = 0; i < sl.size(); ++i) {
		QTreeWidgetItem* rwi = new QTreeWidgetItem(serverlist, 0);
		rwi->setText(0, sl[i].name.c_str());
		rwi->setText(1, STRFORMAT("%i ms", sl[i].ping_micro_secs).c_str());
	}
	//serverlist->addTopLevelItem(
}

void MainWindow::updateProgressBar()
{
//	trackProgress->setValue(handler->Position());
}

void MainWindow::on_OpenButton_clicked()
{
	QString fileName;
	trackProgress->setValue(0);
	fileName = QFileDialog::getOpenFileName(this,
		tr("Open Image"), "", tr("Audio Files (*.mp3 *.wav *.aac *.ogg)"));
	if (fileName == "")
		return;
	file = fileName.toStdString();
	audiocontroller.test_functie(file);
//    handler->Load(file);
//	trackProgress->setMaximum(handler->Length());
}

void MainWindow::on_OpenEditButton_clicked()
{
	QString fileName = lineEdit->text();
	std::string str = fileName.toStdString();
	audiocontroller.test_functie(str);
}

void MainWindow::on_PreviousButton_clicked()
{
	//skip
}

void MainWindow::on_PlayButton_clicked()
{
//	handler->Play();
//	new PortAudioBackend(NULL);
	progressTimer.start(250);
}

void MainWindow::on_PauseButton_clicked()
{
//	handler->Pause();
}

void MainWindow::on_StopButton_clicked()
{
//	handler->Stop();
	progressTimer.stop();
	updateProgressBar();
}


void MainWindow::on_NextButton_clicked()
{

}

