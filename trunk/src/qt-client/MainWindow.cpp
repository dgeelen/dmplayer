#include "MainWindow.moc"
#include <QFileDialog>

MainWindow::MainWindow()
{
	setupUi(this);
	handler = new mp3_handler();
}

MainWindow::~MainWindow()
{
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

