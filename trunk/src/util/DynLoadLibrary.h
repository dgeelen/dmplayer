#ifndef DYN_LOAD_LIBRARY_H
#define DYN_LOAD_LIBRARY_H

#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>

typedef void (*dll_proc_ptr)(void);

class DynamicLibrary {
	private:
		struct impl_t;
		boost::scoped_ptr<impl_t> impl;

	public:
		DynamicLibrary(boost::filesystem::path& path);
		~DynamicLibrary();

		dll_proc_ptr getFunction(std::string funcname);

		template<typename T>
		bool getFunction(std::string funcname, boost::function<T>& func)
		{
			func = boost::function<T>();
			dll_proc_ptr dllfunc = getFunction(funcname);
			if (dllfunc == NULL) return false;
			func = boost::function<T>((T*)dllfunc);
			return true;
		}
};

#endif//DYN_LOAD_LIBRARY_H
