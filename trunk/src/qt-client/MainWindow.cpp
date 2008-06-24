#include "../cross-platform.h"
#include "MainWindow.moc"
#include <QFileDialog>

#include "../util/StrFormat.h"
#include "../error-handling.h"
#include <QString>

#include <boost/foreach.hpp>

#include "QtBooster.h"

using namespace std;

MainWindow::MainWindow()
{
	setupUi(this);

#ifdef DEBUG
	lognamespace::logsignal.connect(
		QTBOOSTER(this, MainWindow::DebugLogger)
	);
#endif

	/* remove central widget & statusbar */
	this->centralWidget()->setParent(&QWidget());

	{
		//QStringList headers;
		//headers << "id";
		//dcerr("before : " << PlayListWidget->header());
		//PlayListWidget->setHeaderLabels(headers);
		//dcerr("after : " << PlayListWidget->header());
	}
	//this->setStatusBar(NULL);

	/* fill view menu & force docks to top*/
	BOOST_FOREACH(QObject* obj, children()) {
		if (QString(obj->metaObject()->className()) == "QDockWidget") {
			QDockWidget* dock = (QDockWidget*)obj;
			menu_View->addAction(dock->toggleViewAction());
			dock->setAllowedAreas(Qt::TopDockWidgetArea);
			this->addDockWidget(Qt::TopDockWidgetArea, dock);
		}
	}

	/* load/set dock layout */
	{
		QFile layoutfile("layout.dat");
		if (layoutfile.open(QIODevice::ReadOnly)) {
			QRect rect;
			layoutfile.read((char*)&rect, sizeof(QRect));
			this->setGeometry(rect);
			QByteArray data = layoutfile.readAll();
			if (data.size() > 0)
				restoreState(data);
			layoutfile.close();
		} else {
			// default layout
			this->splitDockWidget (dockFileHistory, dockServerList , Qt::Vertical  );
			this->splitDockWidget (dockFileHistory, dockDebugLog   , Qt::Horizontal);
			this->splitDockWidget (dockServerList , dockServerInfo , Qt::Horizontal);
			this->tabifyDockWidget(dockServerInfo , dockAudioPlayer);
		}
	}
	
	progressTimer.stop();

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

	// TODO: make gui input for this
	tdb.add_directory( "d:\\stuff\\music");
	tdb.add_directory( "d:\\mp3");
	tdb.add_directory( "d:\\audio");
	tdb.add_directory( "d:\\My Documents\\My Music");

	Track mdmt;
	mdmt.id.second = LocalTrackID(0xffffffff);
	mdmt.metadata["FILENAME"] = "mp3";
	std::vector<LocalTrack> yay = tdb.search(mdmt);
	BOOST_FOREACH(LocalTrack& tr, yay) {
		DataBaseWidget->add(tr.getTrack());
		//dcerr( tr.filename );
	}


}

void MainWindow::closeEvent(QCloseEvent * evnt)
{
	// save layout to file
	QFile layoutfile("layout.dat");
	if (layoutfile.open(QIODevice::WriteOnly)) {
		QRect rect = this->geometry();
		layoutfile.write((char*)&rect, sizeof(QRect));
		QByteArray data = saveState();
		if (data.size() > 0)
			layoutfile.write(data);
		layoutfile.close();
	}
	QWidget::closeEvent(evnt);
}

MainWindow::~MainWindow()
{
}

void MainWindow::UpdateServerList(std::vector<server_info> sl)
{
	for (uint i = 0; i < sl.size(); ++i) {
		QList<QTreeWidgetItem*> flist = serverlist->findItems(QString(sl[i].name.c_str()), Qt::MatchFlags());
		if (flist.size() == 0) {
			QTreeWidgetItem* rwi = new QTreeWidgetItem(serverlist, 0);
			rwi->setText(0, sl[i].name.c_str());
			rwi->setText(1, STRFORMAT("%i ms", sl[i].ping_micro_secs).c_str());
			currentServers.push_back(sl[i]);
		}
		else { //Update ping
			Q_FOREACH(QTreeWidgetItem* item, flist)
				item->setText(1, STRFORMAT("%i ms", sl[i].ping_micro_secs).c_str());
			for (uint j = 0; j < currentServers.size(); ++j)
			{
				if (currentServers[j].sock_addr == sl[i].sock_addr)
					currentServers[j] = sl[i];
			}
		}
	}
}

void MainWindow::UpdateServerList_remove(server_info si)
{
	QList<QTreeWidgetItem*> flist = serverlist->findItems(si.name.c_str(), Qt::MatchFlags());
	Q_FOREACH(QTreeWidgetItem* item, flist)
		delete item;
	for (std::vector<server_info>::iterator i = currentServers.begin(); i != currentServers.end();)
	{
		if (i->sock_addr == si.sock_addr)
		{
			currentServers.erase(i);
			break;
		}
		++i;
	}
	std::cerr << "Disconnect" << std::endl;
}

void MainWindow::updateProgressBar()
{
//	trackProgress->setValue(handler->Position());
}

void MainWindow::on_actionOpen_triggered()
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

void MainWindow::openFile(const QString& str)
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
	progressTimer.start(250);
	audiocontroller.StartPlayback();
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
	audiocontroller.StopPlayback();
}


void MainWindow::on_NextButton_clicked()
{

}

void MainWindow::on_ConnectButton_clicked()
{
	int index = serverlist->indexOfTopLevelItem(serverlist->currentItem());

	if (index > -1)
	{
		lblServerName->setText(currentServers[index].name.c_str());
		lblPing->setText(STRFORMAT("%i ms", currentServers[index].ping_micro_secs).c_str());
		lblServerAddress->setText(currentServers[index].sock_addr.std_str().c_str());

		//gmpmpc_network_handler->message_receive_signal.connect(handle_received_message);
		sigconnect(currentServers[index].sock_addr);
		//gmpmpc_network_handler->client_connect_to_server(currentServers[index].sock_addr);
	}
	else
	{
		lblServerName->setText("");
		lblPing->setText("");
	}
}

void MainWindow::on_DisconnectButton_clicked()
{

}

void MainWindow::on_RefreshButton_clicked()
{

}

void MainWindow::DebugLogger(std::string msg, std::string file, std::string func, int line)
{
	textEdit->append(QString(msg.c_str()));
}

void MainWindow::handleReceivedMessage(const messageref m)
{
	switch (m->get_type()) {
		case message::MSG_ACCEPT: {
			message_accept_ref msg = boost::static_pointer_cast<message_accept>(m);
			localid = msg->cid;
		}; break;
		case message::MSG_DISCONNECT: {
			labelConnected->setText("No");
		}; break;
		case message::MSG_PLAYLIST_UPDATE: {
			message_playlist_update_ref msg = boost::static_pointer_cast<message_playlist_update>(m);
			msg->apply(PlayListWidget);
		}; break;
		case message::MSG_REQUEST_FILE: {
			message_request_file_ref msg = boost::static_pointer_cast<message_request_file>(m);

			Track track;
			track.id = msg->id;
			vector<LocalTrack> local = tdb.search(track);
			fspath filepath = local[0].filename;
			QFile::FileError err;
			QFile file (QString::fromStdString(filepath.native_file_string()));
			file.open(QIODevice::ReadOnly);
			err = file.error();
			QByteArray data = file.readAll();
			uint64 done = 0;
			err = file.error();
			while (done < data.size()) {
				uint64 todo = data.size()-done;
				if (todo > 32*1024)
					todo = 32*1024;
				std::vector<uint8> vdata;
				vdata.resize(todo);
				memcpy(&vdata[0], (uint8*)data.data() + done, todo);
				message_request_file_result_ref result(new message_request_file_result(vdata));
				nh->send_server_message(result);
				done += todo;
			}
			{
				std::vector<uint8> vdata;
				message_request_file_result_ref result(new message_request_file_result(vdata));
				nh->send_server_message(result);
			}
		}; break;
		case message::MSG_QUERY_TRACKDB: {
			message_query_trackdb_ref msg = boost::static_pointer_cast<message_query_trackdb>(m);

			vector<LocalTrack> local = tdb.search(msg->search);
			vector<Track> result;
			BOOST_FOREACH(LocalTrack tr, local) {
				result.push_back(Track(TrackID(localid, tr.id), tr.metadata));
			}

			message_query_trackdb_result_ref sendmsg(new message_query_trackdb_result(msg->qid, result));
			nh->send_server_message(sendmsg);
		}; break;
		default: {
			dcerr("unknown message type:" << m->get_type() << '\n');
		};
	}
	m.get();
}

void MainWindow::on_DataBaseWidget_doubleClicked(QModelIndex mi)
{
	Track tr = DataBaseWidget->get(mi.row());

	tr.id.first = localid;

	syncedplaylist.add(tr);

	messageref msg;

	while (msg = syncedplaylist.pop_msg()) {
		nh->send_server_message(msg);
	}
}

void MainWindow::on_editTrackSearch_textEdited(QString text)
{
	MetaDataMap map;
	map["FILENAME"] = text.toStdString();
	Track query(TrackID(ClientID(0xffffffff), LocalTrackID(0xffffffff)), map);
	vector<LocalTrack> s = tdb.search(query);
	DataBaseWidget->clear();

	BOOST_FOREACH(LocalTrack& tr, s) {
		DataBaseWidget->add(tr.getTrack());
	}
}

void MainWindow::on_buttonMusicAdd_clicked()
{
	QString directory;
	directory = QFileDialog::getExistingDirectory(this,
		tr("Choose directory to share"),
		"");
	tdb.add_directory(directory.toStdString());

	on_editTrackSearch_textEdited(editTrackSearch->text());
}

void MainWindow::on_buttonPlaylistAdd_clicked()
{
	QList<QModelIndex> selected = DataBaseWidget->selectedIndexes();

	BOOST_FOREACH(QModelIndex& mi, selected) {
		on_DataBaseWidget_doubleClicked(mi);
	}
}
