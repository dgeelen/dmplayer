#ifndef QT_SLOT_BOOST_H
#define QT_SLOT_BOOST_H

#include <boost/function.hpp>

#include <QObject>

class QtSlotBoost: public QObject {
	Q_OBJECT

	private:
		boost::function<void()> func;
	public:
		QtSlotBoost(const boost::function<void()>& func_);

	public Q_SLOTS:
		void trigger();
};

#endif//QT_SLOT_BOOST_H