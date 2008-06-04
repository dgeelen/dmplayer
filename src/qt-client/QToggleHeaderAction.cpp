#include "QToggleHeaderAction.moc"

#include <QTreeWidget>

QToggleHeaderAction::QToggleHeaderAction(QHeaderView* header_, const QString& name_, int pos_)
: header(header_)
, pos(pos)
, QAction(name_, NULL)
{
	QObject::connect(this, SIGNAL(triggered(bool)), this, SLOT(execute(bool)));
}

void QToggleHeaderAction::execute(bool checked)
{
	if (checked)
		header->showSection(pos);
	else
		header->hideSection(pos);
}
