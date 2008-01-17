#ifndef MAIN_H
#define MAIN_H

#include "ui_MainWindow.h"
#include <string>
#include "../network-handler.h"
#include "../audio/audio_controller.h"
#include <QTimer>

class MainWindow: public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

	public:
		MainWindow();
		~MainWindow();


	public Q_SLOTS:
		void UpdateServerList(std::vector<server_info>);
		void on_OpenButton_clicked();
		void on_PreviousButton_clicked();
		void on_PlayButton_clicked();
		void on_PauseButton_clicked();
		void on_StopButton_clicked();
		void on_NextButton_clicked();
		void updateProgressBar();
	private:
		std::string file;
		AudioController audiocontroller;
		//mp3_handler* handler;
		QTimer progressTimer;
};
#endif
