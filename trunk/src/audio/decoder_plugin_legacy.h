#ifndef DECODER_PLUGIN_LEGACY_H
#define DECODER_PLUGIN_LEGACY_H

#include "plugin_util.h"
#include "../util/DynLoadLibrary.h"
#include "datasource_interface.h"

// returns a 'Decoder-2' compatible decoder_obj from a 'Decoder-1' type plugin
ppvoid_t decoder_plugin_legacy_1(DynamicLibrary& dll, IDataSourceRef source);

#endif//DECODER_PLUGIN_LEGACY_H
