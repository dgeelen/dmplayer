#include "QPlaylist.moc"

#include "QToggleHeaderAction.h"
#include <QHeaderView>

QPlaylist::QPlaylist(QWidget* parent)
	: QTreeWidget(parent)
{
	QTreeWidget::setIndentation(0);

	this->header()->setContextMenuPolicy(Qt::ActionsContextMenu);

	{
		QStringList headers;
		headers << "id";
		headers << "filename";
		//dcerr("before : " << PlayListWidget->header());
		this->setHeaderLabels(headers);
		//this->hideColumn(1);

		//dcerr("after : " << PlayListWidget->header());
	}

	//this->headeri

	QAction* a = new QToggleHeaderAction(this, "id", 0);
	this->header()->addAction(a);
	QAction* b = new QToggleHeaderAction(this, "filename", 1);
	this->header()->addAction(b);
	//QObject::connect(a, SIGNAL(triggered()), this->header(),

	//QAction* a = new QToggleHeaderAction(this->header(), 0);
	//this->header()->addAction(a);
	//QObject::connect(a, SIGNAL(triggered()), this->header(),

	//contextMenu.addAction(
}

void QPlaylist::add(const Track& track)
{
	PlaylistVector::add(track);
	//Track& tr = PlaylistVector::get(PlaylistVector::size()-1);


	QTreeWidgetItem* item = new QTreeWidgetItem(this, QStringList());
	item->setText(0, QString::fromStdString(track.idstr()));
	item->setText(1, QString::fromStdString(track.metadata.find("FILENAME")->second));
	//QTreeWidget::addItem(track.id().str());
}

void QPlaylist::remove(uint32 pos)
{
	PlaylistVector::remove(pos);
	delete QTreeWidget::topLevelItem(pos);
}

void QPlaylist::insert(uint32 pos, const Track& track)
{
	PlaylistVector::insert(pos, track);

	QTreeWidgetItem* item = new QTreeWidgetItem(QStringList());
	item->setText(0, QString::fromStdString(track.idstr()));
	QTreeWidget::insertTopLevelItem(pos, item);
}

void QPlaylist::move(uint32 from, uint32 to)
{
	// will call virtual functions insert/remove
	PlaylistVector::move(from, to);
}

void QPlaylist::clear()
{
	QTreeWidget::clear();
	// will call virtual functions add
	PlaylistVector::clear();
}

QPlaylist::~QPlaylist()
{
}
