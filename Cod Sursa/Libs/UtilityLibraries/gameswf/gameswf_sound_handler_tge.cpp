// gameswf_sound_handler_tge.cpp	-- Thomas "Man of Ice" Lund, 2005

// A gameswf::sound_handler that uses Torque Game Engine for output
// based on the audio handler for SDL

/*
#include "platform/platformAudio.h"
#include "platform/platform.h"
#include "console/console.h"

#include "gameswf.h"
#include "base/container.h"
#include "gameswf_log.h"
#include "gameswf_types.h"	// for IF_VERBOSE_* macros

// Use TGE to handle gameswf sounds.
struct TGE_sound_handler : gameswf::sound_handler
{
	bool	m_opened;
	bool	m_stereo;
	int		m_sample_rate;
	Uint16	m_format;
	array<ALuint>	m_buffers;
	array<ALuint>	m_sources;

	TGE_sound_handler()
		:
		m_opened(false),
		m_stereo(false),
		m_sample_rate(0),
		m_format(0)
	{
		// Initialize OpenAL audio and variables
	}

	~TGE_sound_handler()
	{
	}


	virtual int	create_sound(
		void* data,
		int data_bytes,
		int sample_count,
		format_type format,
		int sample_rate,
		bool stereo)
	{
		Con::errorf("Trying to create sound");
		if (m_opened == false)
		{
			return 0;
		}
		return 0;
	}


	virtual void	play_sound(int sound_handle, int loop_count) // other params 
	{
		Con::errorf("Trying to play sound");
	}

	
	virtual void	stop_sound(int sound_handle)
	{
		Con::errorf("Trying to stop sound");
	}


	virtual void	delete_sound(int sound_handle)
	{
		Con::errorf("Trying to delete sound");
	}

	// From the SDL sound renderer
	virtual void convert_raw_data(
		Sint16** adjusted_data,
		int* adjusted_size,
		void* data,
		int sample_count,
		int sample_size,
		int sample_rate,
		bool stereo)
	// VERY crude sample-rate & sample-size conversion.  Converts
	// input data to the SDL_mixer output format (SAMPLE_RATE,
	// stereo, 16-bit native endianness)
	{
// 		// xxxxx debug pass-thru
// 		{
// 			int	output_sample_count = sample_count * (stereo ? 2 : 1);
// 			Sint16*	out_data = new Sint16[output_sample_count];
// 			*adjusted_data = out_data;
// 			*adjusted_size = output_sample_count * 2;	// 2 bytes per sample
// 			memcpy(out_data, data, *adjusted_size);
// 			return;
// 		}
// 		// xxxxx

		// simple hack to handle dup'ing mono to stereo
		if ( !stereo && m_stereo)
		{
			sample_rate >>= 1;
		}

		 // simple hack to lose half the samples to get mono from stereo
		if ( stereo && !m_stereo)
		{
			sample_rate <<= 1; 
		}

		// Brain-dead sample-rate conversion: duplicate or
		// skip input samples an integral number of times.
		int	inc = 1;	// increment
		int	dup = 1;	// duplicate
		if (sample_rate > m_sample_rate)
		{
			inc = sample_rate / m_sample_rate;
		}
		else if (sample_rate < m_sample_rate)
		{
			dup = m_sample_rate / sample_rate;
		}

		int	output_sample_count = (sample_count * dup) / inc;
		Sint16*	out_data = new Sint16[output_sample_count];
		*adjusted_data = out_data;
		*adjusted_size = output_sample_count * 2;	// 2 bytes per sample

		if (sample_size == 1)
		{
			// Expand from 8 bit to 16 bit.
			Uint8*	in = (Uint8*) data;
			for (int i = 0; i < output_sample_count; i++)
			{
				Uint8	val = *in;
				for (int j = 0; j < dup; j++)
				{
					*out_data++ = (int(val) - 128);
				}
				in += inc;
			}
		}
		else
		{
			// 16-bit to 16-bit conversion.
			Sint16*	in = (Sint16*) data;
			for (int i = 0; i < output_sample_count; i += dup)
			{
				Sint16	val = *in;
				for (int j = 0; j < dup; j++)
				{
					*out_data++ = val;
				}
				in += inc;
			}
		}
	}

};


gameswf::sound_handler*	gameswf::create_sound_handler_tge()
// Factory.
{
	return new TGE_sound_handler;
}

*/