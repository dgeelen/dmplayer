#ifndef Q_TOGGLE_HEADER_ACTION_H
#define Q_TOGGLE_HEADER_ACTION_H

#include <QAction>
#include <QTreeView>

class QToggleHeaderAction: public QAction {
	Q_OBJECT

	private:
		QTreeView* view;
		int pos;
		QString name;

	private Q_SLOTS:
		void execute(bool);
		
	public:
		QToggleHeaderAction(QTreeView* widget, const QString& name, int pos_);
};

#endif//Q_TOGGLE_HEADER_ACTION_H
