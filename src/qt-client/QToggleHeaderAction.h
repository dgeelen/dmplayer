#ifndef Q_TOGGLE_HEADER_ACTION_H
#define Q_TOGGLE_HEADER_ACTION_H

#include <QAction>
#include <QHeaderView>

class QToggleHeaderAction: public QAction {
	Q_OBJECT

	private:
		QHeaderView* header;
		int pos;
		QString name;

	private Q_SLOTS:
		void execute(bool);
		
	public:
		QToggleHeaderAction(QHeaderView* header_, const QString& name, int pos_);
};

#endif//Q_TOGGLE_HEADER_ACTION_H
