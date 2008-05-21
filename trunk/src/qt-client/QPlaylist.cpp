#include "QPlaylist.h"

QPlaylist::QPlaylist(QWidget* parent)
	: QListWidget(parent)
{
}

void QPlaylist::vote(TrackID id)
{
}

void QPlaylist::add(Track track)
{
	QListWidget::addItem(QString::fromStdString(track.metadata["filename"]));
	//Playlist::add(track);
	//QListWidget::addTopLevelItem
}

void QPlaylist::clear()
{
	QListWidget::clear();
	//Playlist::clear();
}

QPlaylist::~QPlaylist()
{
}