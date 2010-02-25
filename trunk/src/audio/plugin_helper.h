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

#include "plugin_util.h"

typedef ppvoid_t (*find_decoder_callback_type)(ppvoid_t);

extern "C" DLL_PUBLIC const char* getplugintype();
extern "C" DLL_PUBLIC ppvoid_t create_decoder(ppvoid_t);
extern "C" DLL_PUBLIC void set_find_decoder_callback(find_decoder_callback_type);

#endif//PLUGIN_DLL_H
