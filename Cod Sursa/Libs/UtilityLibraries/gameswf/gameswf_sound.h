// gameswf_sound.h   -- Thatcher Ulrich, Vitaly Alexeev

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef GAMESWF_SOUND_H
#define GAMESWF_SOUND_H


#include "gameswf_impl.h"


namespace gameswf
{
	struct sound_sample_impl : public sound_sample
	{
		int	m_sound_handler_id;

		sound_sample_impl(int id)
			:
			m_sound_handler_id(id)
		{
		}

		virtual ~sound_sample_impl();
	};
}


#endif // GAMESWF_SOUND_H
