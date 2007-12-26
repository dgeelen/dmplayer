#ifndef MP3_STRUCTURE_H_INCLUDED
#define MP3_STRUCTURE_H_INCLUDED

#include <string>

struct mp3_struct
{
	std::string file;
	int length;
	std::string artist;
	std::string track;
	std::string album;
	int tracknr;
	std::string genre;

};


#endif // MP3_STRUCTURE_H_INCLUDED
