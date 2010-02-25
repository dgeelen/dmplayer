#ifndef PLUGIN_DLL_H
#define PLUGIN_DLL_H

#define BUILDING_DLL

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__((dllexport))
    #else
      #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__((dllimport))
    #else
      #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility("default")))
    #define DLL_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif

#include "../types.h"

typedef uint32 (*source_callback_getData_cbt)(void*, uint8*, uint32);
typedef void (*source_callback_reset_cbt)(void*);
typedef bool (*source_callback_exhausted_cbt)(void*);
typedef uint32 (*source_callback_getpos_cbt)(void*);

extern "C" DLL_PUBLIC const char* getplugintype();
extern "C" DLL_PUBLIC void* create(void*,source_callback_getData_cbt,source_callback_reset_cbt,source_callback_exhausted_cbt,source_callback_getpos_cbt);
extern "C" DLL_PUBLIC void destroy(void*);
extern "C" DLL_PUBLIC uint32 getdata(void*,uint8*,uint32);
extern "C" DLL_PUBLIC bool exhausted(void*);
extern "C" DLL_PUBLIC void getaudioformat(void*, uint32*,uint32*,uint32*,bool*,bool*,bool*);

#endif//PLUGIN_DLL_H
