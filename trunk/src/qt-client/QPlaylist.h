#ifndef QPLAYLIST_H
#define QPLAYLIST_H

#include <QTreeWidget>
#include <QMenu>
#include "../playlist_management.h"


class QPlaylist: public QTreeWidget, public PlaylistVector {
	Q_OBJECT

	private:
		QMenu contextMenu;

	public:
		QPlaylist(QWidget* parent);
		~QPlaylist();

		virtual void add(const Track& track);
		virtual void remove(uint32 pos);
		virtual void insert(uint32 pos, const Track& track);
		virtual void move(uint32 from, uint32 to);
		virtual void clear();

		virtual QModelIndexList selectedIndexes() {
			return QTreeView::selectedIndexes();
		}
};

#endif//QPLAYLIST_H
