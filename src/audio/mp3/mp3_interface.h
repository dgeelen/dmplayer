#ifndef MP3_INTERFACE_H
#define MP3_INTERFACE_H
/* Date: 20 december 2007
 * Author: Daan Wissing
 * This component is meant to play MP3 files, given by the interface
 * This class is the interface to the MP3 component, it passes most functionality
 * to other clases. Those classes can only be revoked by this class!

*/

#include <string>
#include "fmod.h"
#include "fmod_errors.h"
#include "../gplayback_interface.h"

class mp3_handler: public gaudiohandler
{
	private:
		FSOUND_STREAM* g_mp3_stream;
		bool paused;

	public:
		mp3_handler();
		~mp3_handler();
		void Play();
		void Pause();
		void Stop();
		int Position();
		void setPosition(int ms);
		void Load(std::string mp3file);
		bool isValid();

};
#endif
