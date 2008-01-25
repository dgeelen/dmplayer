#include "../cross-platform.h"
#include "MainWindow.moc"
#include <QFileDialog>

#include <boost/format.hpp>
#include <QString>

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

	QFile rcfile("recent_files_cache.txt");
	rcfile.open(QFile::ReadOnly);
	QStringList strlist;
	while (!rcfile.atEnd()) {
		QString line = rcfile.readLine();
		while (line.size() > 0 && (line.at(line.size()-1) == '\n' || line.at(line.size()-1) == '\r'))
			line.remove(line.size()-1, 1);
		if (line != "") {
			strlist.removeAll(line);
			strlist.append(line);
		}
	}
	std::reverse(strlist.begin(), strlist.end());
	listRecentFiles->addItems(strlist);
	std::reverse(strlist.begin(), strlist.end());
	rcfile.close();
	if (rcfile.open(QFile::WriteOnly)) {
		Q_FOREACH(QString str, strlist) {
			rcfile.write(str.toStdString().c_str(), str.size());
			rcfile.write("\n",1);
		}
	}
}

MainWindow::~MainWindow()
{
}

void MainWindow::UpdateServerList(std::vector<server_info> sl)
{
	for (uint i = 0; i < sl.size(); ++i) {
		if (serverlist->findItems(QString(sl[i].name.c_str()), Qt::MatchFlags()).size() == 0) {
			QTreeWidgetItem* rwi = new QTreeWidgetItem(serverlist, 0);
			rwi->setText(0, sl[i].name.c_str());
			rwi->setText(1, STRFORMAT("%i ms", sl[i].ping_micro_secs).c_str());
		}
	}
}

void MainWindow::UpdateServerList_remove(server_info si)
{
	QList<QTreeWidgetItem*> flist = serverlist->findItems(si.name.c_str(), Qt::MatchFlags());
	Q_FOREACH(QTreeWidgetItem* item, flist)
		delete item;
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
	openFile(fileName);
//    handler->Load(file);
//	trackProgress->setMaximum(handler->Length());
}

void MainWindow::on_OpenEditButton_clicked()
{
	openFile(lineEdit->text());
}

void MainWindow::openFile(QString str)
{
	lineEdit->setText(str);

	QList<QListWidgetItem*> flist = listRecentFiles->findItems(str, Qt::MatchFlags());
	QFile rcfile("recent_files_cache.txt");
	if (rcfile.open(QFile::Append)) {
		rcfile.write(str.toStdString().c_str(), str.size());
		rcfile.write("\n",1);
	}
	if (flist.size() != 0) {
		Q_FOREACH(QListWidgetItem* item, flist) {
			delete item;
		}
	}
	listRecentFiles->insertItem(0, new QListWidgetItem(str));
	audiocontroller.test_functie(str.toStdString());
}

void MainWindow::on_listRecentFiles_itemDoubleClicked(QListWidgetItem* item )
{
	openFile(item->text());
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

void MainWindow::on_ConnectButton_clicked()
{
	lblServerName->setText(serverlist->itemAt(0,0)->text(0));
	lblPing->setText(serverlist->itemAt(0,0)->text(1));
}

void MainWindow::on_DisconnectButton_clicked()
{

}

void MainWindow::on_RefreshButton_clicked()
{

}
