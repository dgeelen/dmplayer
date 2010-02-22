#include "DynLoadLibrary.h"

#ifdef WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

struct DynamicLibrary::impl_t {
	#ifdef WIN32
		HMODULE libhandle;
	#else
		void* libhandle;
	#endif
};

DynamicLibrary::DynamicLibrary(boost::filesystem::path& path)
: impl(new impl_t)
{
	#ifdef WIN32
		impl->libhandle = LoadLibrary(path.string().c_str());
	#else
		impl->libhandle = dlopen(path.string().c_str(), RTLD_LAZY);
	#endif
}

DynamicLibrary::~DynamicLibrary()
{
	#ifdef WIN32
		if (impl->libhandle != NULL) FreeLibrary(impl->libhandle);
	#else
		if (impl->libhandle != NULL) dlclose(impl->libhandle);
	#endif
}

dll_proc_ptr DynamicLibrary::getFunction(std::string funcname)
{
	#ifdef WIN32
		if (impl->libhandle == NULL) return NULL;
		FARPROC func = GetProcAddress(impl->libhandle, funcname.c_str());
		return (dll_proc_ptr)func;
	#else
		if (impl->libhandle == NULL) return NULL;
		void* func = dlsym(impl->libhandle, funcname.c_str());
		return (dll_proc_ptr)func;
	#endif
}
