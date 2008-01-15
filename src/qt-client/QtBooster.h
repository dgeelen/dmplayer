#ifndef QT_BOOSTER_H
#define QT_BOOSTER_H

#include <vector>
#include <cassert>
//#include <iostream>

#include <QMetaObject>
#include <QMetaMethod>

/*
 * Makes a boost compatible function object to call a QT slot.
 * handy for thread marshalling as QT will do this for you automagically.
 *
 * How to use example:
 *   QTBOOSTER(&mainwindow, MainWindow::UpdateServerList)
 *
 * First argument is the adress of the object to which the QT slot belongs.
 * Second argument is the slot function.
 *
 * Note all slot argument types need to be registered via qRegisterMetaType.
 * Overloaded slots are not supported
 */

/*
 * for type safety, makeQtBooster passes the corrent types to QtBooster
 * at the end of each operator(), there is this:
 * > return;
 * > ((*((X*)NULL)).*((Y)NULL))(t1, t2, ...);
 * the return is there to never have the sencod line executed
 * the second line is the check, it calls the member function 
 * with the passed arguments
 */

template <class X, typename Y>
class QtBooster {
	private:
		QObject* target;
		char* sigstore;
		int lenstore;
		char* csig[11]; // function name + 10 args
	public:
		typedef void result_type;

		QtBooster() {};
		QtBooster(X* obj, char* mdesc) {
			std::vector<std::string> sig;
			std::string objectname;
			std::string methodname;

			// split method description into objectname and methodname
			{
				int i = 0;
				while (mdesc[i] != ':' && mdesc[i] != 0)
					objectname += mdesc[i++];
				if (mdesc[i] == ':') ++i;
				if (mdesc[i] == ':') ++i;
				while (mdesc[i] != 0)
					methodname += mdesc[i++];
			}

			target = obj;
			assert(target->objectName() == objectname.c_str());

			const QMetaObject* mo = target->metaObject();
			assert(objectname == mo->className());

			// find index of the slot method
			int mi = -1;
			for (int i = 0; i < mo->methodCount(); ++i) {
				QMetaMethod mm = mo->method(i);
				if (mm.methodType() == QMetaMethod::Slot) {
					const char* msig = mm.signature();
					int j = 0;
					while (msig[j] != '(' && msig[j] != ' ' && msig[j] != 0) ++j;
					std::string sname(msig, j);
					if (sname == methodname) {
						assert(mi == -1); //overloaded slots not supported
						mi = i;
					}
				}
			}
			assert(mi != -1);

			QMetaMethod mm = mo->method(mi);

			sig.clear();
			sig.push_back(methodname);

			// exctract signiature information
			const QList<QByteArray> args = mm.parameterTypes();
			for (int i = 0; i < args.size(); ++i)
				sig.push_back(args[i].data());

			assert(sig.size() <= 11);

			// convert to char pointer store
			int tlen = 0;
			for (int i = 0; i < sig.size(); ++i)
				tlen += sig[i].size() +1;

			sigstore = new char[tlen];

			int tpos = 0;
			for (int i = 0; i < sig.size(); ++i) {
				csig[i] = sigstore + tpos;
				for (int j = 0; j < sig[i].size(); ++j)
					sigstore[tpos++] = sig[i][j];
				sigstore[tpos++] = 0;
			}

			lenstore = tlen;

			return;
			((*((X*)NULL)).*((Y)NULL)); // check if Y is a member of X (compile time)
		}

		QtBooster(const QtBooster & o) {
			target = o.target;
			lenstore = o.lenstore;
			sigstore = new char[lenstore];
			for (int i = 0; i < lenstore; ++i)
				sigstore[i] = o.sigstore[i];
			for (int i = 0; i < 11; ++i)
				csig[i] = sigstore + (o.csig[i] - o.sigstore);
		}

		~QtBooster() {
			delete sigstore;
		}

		void operator()() {
			QMetaObject::invokeMethod(target, csig[0]);
			return;
			((*((X*)NULL)).*((Y)NULL))(); // see top of file
		}

		template<typename T1>
		void operator()(const T1 & t1) {
			QMetaObject::invokeMethod(target, csig[0],
				QArgument<T1>(csig[1], t1)
			);
			return;
			((*((X*)NULL)).*((Y)NULL))(t1);
		}

		template<typename T1, typename T2>
		void operator()(const T1 & t1, const T2 & t2) {
			QMetaObject::invokeMethod(target, csig[0],
				QArgument<T1>(csig[1], t1),
				QArgument<T2>(csig[2], t2)
			);
			return;
			((*((X*)NULL)).*((Y)NULL))(t1, t2);
		}

		template<typename T1, typename T2, typename T3>
		void operator()(const T1 & t1, const T2 & t2, const T3 & t3) {
			QMetaObject::invokeMethod(target, csig[0],
				QArgument<T1>(csig[1], t1),
				QArgument<T2>(csig[2], t2),
				QArgument<T3>(csig[3], t3)
			);
			return;
			((*((X*)NULL)).*((Y)NULL))(t1, t2, t3);
		}

		template<typename T1, typename T2, typename T3, typename T4>
		void operator()(const T1 & t1, const T2 & t2, const T3 & t3, const T4 & t4) {
			QMetaObject::invokeMethod(target, csig[0],
				QArgument<T1>(csig[1], t1),
				QArgument<T2>(csig[2], t2),
				QArgument<T3>(csig[3], t3),
				QArgument<T4>(csig[4], t4)
			);
			return;
			((*((X*)NULL)).*((Y)NULL))(t1, t2, t3, t4);
		}
};

template <class X, typename Y>
QtBooster<X, Y> makeQtBooster(X* tgt, Y mtype, char* mdesc)
{
	return QtBooster<X, Y>(tgt, mdesc);
}

#define QTBOOSTER(x, y) makeQtBooster(x, &y, #y)

#endif//QT_BOOSTER_H
