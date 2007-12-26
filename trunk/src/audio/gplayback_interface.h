#ifndef GPLAYBACK_INTERFACE_H
#define GPLAYBACK_INTERFACE_H
/* Date: 23 december 2007
 * Author: Daan Wissing
 *
 * This class is a generic interface for all audio files to be supported.
 * There will be no implementation for this, instead all formats must have
 * their own implementation of this

 * The meaning is to create one instance for each format
*/

#include<string>

class gaudiohandler
{
	public:
		gaudiohandler();
		~gaudiohandler();
		void Play();
		void Pause();
		void Stop();
		int Position();
		void Load(std::string mp3file);
		bool isValid();
};
#endif

