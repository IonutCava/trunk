// gameswf_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#ifndef GAMESWF_3DS_DEF_H
#define GAMESWF_3DS_DEF_H

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "gameswf/gameswf_impl.h"
#include <lib3ds/types.h>

namespace gameswf
{
	struct x3ds_definition : public character_def
	{
		Lib3dsFile* m_file;
		int m_lightList;
		rect	m_rect;

		// textures required for 3D model
		// <texture name, bitmap> m_material
		stringi_hash<gc_ptr<bitmap_info> > m_material;


		x3ds_definition(player* player, const char* url);
		~x3ds_definition();

		virtual character* create_character_instance(character* parent, int id);
		virtual void get_bound(rect* bound)
		{
			*bound = m_rect;
		}

		void set_bound(const rect& bound)
		{
			m_rect = bound;
		}

		void remove_camera(Lib3dsCamera* camera);
		void load_texture(const char* infile);

		void get_bounding_center(Lib3dsVector bmin, Lib3dsVector bmax, Lib3dsVector center, float* size);
		void ensure_camera();
		void ensure_lights();
		void ensure_nodes();

		bool is_loaded() const { return m_file != NULL; }

	};

	x3ds_definition* create_3ds_definition(player* player, const char* url);

}	// end namespace gameswf

#else

#endif


#endif
