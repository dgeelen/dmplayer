#ifndef UPXFIX_H
#define UPXFIX_H

	// Allow UPX Compression (prevents use of TLS callbacks in boost::thread)
	extern "C" void tss_cleanup_implemented(void){}

#endif //UPXFIX_H
