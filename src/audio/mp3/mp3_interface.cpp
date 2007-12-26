#include "mp3_interface.h"
#include <iostream>

mp3_handler::mp3_handler()
{
	if (!FSOUND_Init(44000,64,0))
	{
		std::cerr << " Could not initialise FMOD" << std::endl;
		return;
	}

	g_mp3_stream = FSOUND_Stream_Open( "24 BF2 theme.wav" , FSOUND_2D , 0 , 0 );

	FSOUND_Stream_Play(0,g_mp3_stream);


}
