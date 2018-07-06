// gameswf_3ds.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#ifndef GAMESWF_3DS_INST_H
#define GAMESWF_3DS_INST_H

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "gameswf/gameswf_impl.h"
#include "base/tu_opengl_includes.h"
#include "gameswf_3ds_def.h"

#include <lib3ds/types.h>

namespace gameswf
{

	struct x3ds_instance : public character
	{
		// Unique id of a gameswf resource
		enum { m_class_id = AS_PLUGIN_3DS };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return character::is(class_id);
		}

		Lib3dsCamera* m_camera;

		// <material, movieclip> map
		stringi_hash<as_value> m_map;
		
		gc_ptr<x3ds_definition>	m_def;
		Lib3dsFloat m_current_frame;

		play_state	m_play_state;
		stringi_hash<as_value>	m_variables;

		x3ds_instance(player* player, x3ds_definition* def,	character* parent, int id);
		~x3ds_instance();

		virtual character_def* get_character_def() { return m_def.get_ptr();	}
		virtual void	display();
		void	set_light();
		virtual void	advance(float delta_time);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual bool	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	on_event(const event_id& id);

		// binds texture to triangle (from mesh)
		void bind_material(Lib3dsMaterial* mat, float U, float V);

		// enables texture if there is material & loaded image
		void set_material(Lib3dsMaterial* mat);

		// load image & create texture or get bitmap_info pointer to loaded texture
		void bind_texture(const char* finame);

		void create_mesh_list(Lib3dsMesh* mesh);
		void render_node(Lib3dsNode* node);
		void clear_dlist(Lib3dsNode* nodes);

//		void	update_material();
	};

}	// end namespace gameswf

#else

#endif


#endif
