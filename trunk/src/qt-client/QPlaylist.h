#ifndef QPLAYLIST_H
#define QPLAYLIST_H

#include <QListWidget>
#include "../playlist_management.h"

class QPlaylist: public PlaylistVector, public QListWidget {
	public:
		QPlaylist(QWidget* parent);
		virtual void vote(TrackID id);
		virtual void add(Track track);
		virtual void clear();
		virtual ~QPlaylist();
};

#endif//QPLAYLIST_H
