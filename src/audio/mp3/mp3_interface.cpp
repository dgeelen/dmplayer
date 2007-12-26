#include "mp3_interface.h"
#include <iostream>

mp3_handler::mp3_handler()
{
	g_mp3_stream = 0;
	if (!FSOUND_Init(44000,64,0))
	{
		std::cerr << " Could not initialise FMOD" << std::endl;
		return;
	}
}

mp3_handler::~mp3_handler()
{
	FSOUND_Stream_Stop( g_mp3_stream );
	FSOUND_Stream_Close( g_mp3_stream );
	FSOUND_Close();
}

void mp3_handler::Play()
{
	FSOUND_Stream_Play(0,g_mp3_stream);
}

void mp3_handler::Load(std::string mp3file)
{
	FSOUND_Stream_Close( g_mp3_stream );
	g_mp3_stream = FSOUND_Stream_Open( mp3file.c_str() , FSOUND_2D , 0 , 0 );
	if (!g_mp3_stream)
	{
		std::cerr << "Could not open mp3 file" << std:: endl;
		return;
	}
}

void mp3_handler::Pause()
{
	FSOUND_SetPaused(0, !paused);
	paused = !paused;
}

void mp3_handler::Stop()
{
	FSOUND_Stream_Stop( g_mp3_stream );
}

int mp3_handler::Position()
{
	return FSOUND_Stream_GetTime( g_mp3_stream );
}

void mp3_handler::setPosition(int ms)
{
	FSOUND_Stream_SetTime( g_mp3_stream, ms);
}
