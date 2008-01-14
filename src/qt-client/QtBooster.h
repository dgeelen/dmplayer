#ifndef QT_BOOSTER_H
#define QT_BOOSTER_H

#include <iostream>
#include <vector>

class QtBooster {
	private:
		QObject* target;
		std::vector<std::string> sig;
	public:
		typedef void result_type;

		QtBooster() {};
		QtBooster(QObject* obj, char* slot) {
			target = obj;
			std::string* ref = (sig.push_back(""), &sig.back());

			assert(slot[0] == '1');
			for (unsigned int i = 1; slot[i]; ++i) {
				if (slot[i] == '(')
					ref = (sig.push_back(""), &sig.back());
				else if (slot[i] == ',')
					ref = (sig.push_back(""), &sig.back());
				else if (slot[i] == ' ' && (*ref)[ref->size()-1] == ' ')
					;
				else if (slot[i] == ')')
					break;
				else
					ref->push_back(slot[i]);
			}
		}
		QtBooster(const QtBooster & o) {
			target = o.target;
			sig    = o.sig;
		}
		void operator()() {
			QMetaObject::invokeMethod(target, sig[0].c_str());
		}

		template<typename T1>
		void operator()(const T1 & t1) {
			QMetaObject::invokeMethod(target, sig[0].c_str(),
				QArgument<T1>(sig[1].c_str(), t1)
			);
		}

		template<typename T1, typename T2>
		void operator()(const T1 & t1, const T2 & t2) {
			QMetaObject::invokeMethod(target, sig[0].c_str(),
				QArgument<T1>(sig[1].c_str(), t1),
				QArgument<T2>(sig[2].c_str(), t2)
			);
		}

		template<typename T1, typename T2, typename T3>
		void operator()(const T1 & t1, const T2 & t2, const T3 & t3) {
			QMetaObject::invokeMethod(target, sig[0].c_str(),
				QArgument<T1>(sig[1].c_str(), t1),
				QArgument<T2>(sig[2].c_str(), t2),
				QArgument<T3>(sig[3].c_str(), t3)
			);
		}

		// TODO - typesafety
		// TODO - on construct char* creation (maybe)
		// TODO - add all operator() upto 10 args
/*
		template<typename T0, typename T1, typename T2, typename TT>
		void operator()(const T0 & t0, const TT & tt, const T1 & t1, const T2 & t2) {
			void* x = NULL;
			boost::bind<void>(t0, tt, _1, _2)(t1, t2);
			QMetaObject::invokeMethod(target, func.c_str(),
				QArgument<T1>(args[0].c_str(), t1),
				QArgument<T2>(args[1].c_str(), t2)
			);
			std::cout << "invoke:" << func << std::endl;
			for (unsigned int i = 0; i < args.size(); ++i)
				cout << args[i] << '-';
		}
*/
};

#endif//QT_BOOSTER_H
