#include "QtSlotBoost.moc"

QtSlotBoost::QtSlotBoost(const boost::function<void()>& func_)
: func(func_) 
{
}

void QtSlotBoost::trigger()
{
	func();
}