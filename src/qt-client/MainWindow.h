#ifndef MAIN_H
#define MAIN_H

#include "ui_MainWindow.h"
#include <string>
#include "../network-handler.h"
#include "../audio/audio_controller.h"
#include "../synced_playlist.h"
#include <QTimer>

class MainWindow: public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

	private:
		network_handler* nh;
		ClientID localid;
		SyncedPlaylist syncedplaylist;

	protected:
		void closeEvent(QCloseEvent * evnt);

	public:
		MainWindow();
		~MainWindow();

		void setNetworkHandler(network_handler* nh_) 
		{
			nh = nh_;
		}

	public Q_SLOTS:
		void UpdateServerList(std::vector<server_info>);
		void UpdateServerList_remove(server_info);
		void on_actionOpen_triggered();
		void on_OpenEditButton_clicked();
		void on_listRecentFiles_itemDoubleClicked(QListWidgetItem*);
		void on_PreviousButton_clicked();
		void on_PlayButton_clicked();
		void on_PauseButton_clicked();
		void on_StopButton_clicked();
		void on_NextButton_clicked();
		void on_ConnectButton_clicked();
		void on_RefreshButton_clicked();
		void on_DisconnectButton_clicked();
		void on_DataBaseWidget_doubleClicked(QModelIndex);
		void updateProgressBar();
		void handleReceivedMessage(const messageref m);

		void DebugLogger(std::string msg, std::string file, std::string func, int);

	private:
		std::string file;
		AudioController audiocontroller;
		QTimer progressTimer;
		std::vector<server_info> currentServers;

		void openFile(const QString& str);
		TrackDataBase tdb;

	public:
		boost::signal<void (ipv4_socket_addr)> sigconnect;
};
#endif
