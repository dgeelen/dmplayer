#include "backend_libao.h"

#define BUF_SIZE 4096
LibAOBackend::LibAOBackend(IDecoder* dec)	: IBackend(dec) {
	ao_initialize();
	int default_driver = ao_default_driver_id();
	format.bits = 16;
	format.channels = 2;
	format.rate = 44100;
	format.byte_format = AO_FMT_LITTLE;
	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		throw "LibAOBackend: Error opening device!";
	}

}

LibAOBackend::~LibAOBackend() {
	ao_close(device);
	ao_shutdown();
}
