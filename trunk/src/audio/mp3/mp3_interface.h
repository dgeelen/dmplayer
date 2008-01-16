#ifndef MP3_INTERFACE_H
#define MP3_INTERFACE_H
/* Date: 20 december 2007
 * Author: Daan Wissing
 * This component is meant to play MP3 files, given by the interface
 * This class is the interface to the MP3 component, it passes most functionality
 * to other clases. Those classes can only be revoked by this class!

*/

#include <string>
#include "../gplayback_interface.h"
#include "../../types.h"

class mp3_handler: public gaudiohandler {
	private:
		bool paused;

	public:
		mp3_handler();
		~mp3_handler();
		void Play();
		void Pause();
		void Stop();
		uint Position();
		uint Length();
		void setPosition(const uint ms);
		void Load(const std::string& mp3file);
		bool isValid();

};
#endif
