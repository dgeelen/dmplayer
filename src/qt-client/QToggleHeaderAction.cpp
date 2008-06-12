#include "QToggleHeaderAction.moc"

#include <QTreeWidget>

QToggleHeaderAction::QToggleHeaderAction(QTreeView* widget, const QString& name_, int pos_)
: view(widget)
, pos(pos_)
, QAction(name_, widget)
{
	QObject::connect(this, SIGNAL(triggered(bool)), this, SLOT(execute(bool)));
	this->setCheckable(true);
}

void QToggleHeaderAction::execute(bool checked)
{
	if (checked)
		view->showColumn(pos);
	else
		view->hideColumn(pos);
	//this->toggle();
}
