// gameswf_3ds_def.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#include "base/tu_config.h"

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include "base/tu_opengl_includes.h"

#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>

#include "gameswf_3ds_def.h"
#include "gameswf_3ds_inst.h"

namespace gameswf
{

	x3ds_definition* create_3ds_definition(player* player, const char* url)
	{
		gc_ptr<character_def>	m;
		player->get_chardef_library()->get(url, &m);
		if (m != NULL)
		{
			// Return cached movie.
			return cast_to<x3ds_definition>(m.get_ptr());
		}

		x3ds_definition* def = new x3ds_definition(player, url);
		player->get_chardef_library()->add(url, def);
		return def;
	}

	x3ds_definition::x3ds_definition(player* player, const char* url) : 
		character_def(player),
		m_file(NULL)
	{
		m_file = lib3ds_file_load(url);
		if (m_file == NULL)
		{
			static int n = 0;
			if (n < 1)
			{
				n++;
				log_error("can't load '%s'\n", url);
			}
			return;
		}

		// load textures
	  for (Lib3dsMaterial* p = m_file->materials; p != 0; p = p->next)
		{
			load_texture(p->texture1_map.name);
			load_texture(p->bump_map.name);
		}

		ensure_camera();
		ensure_lights();
		ensure_nodes();

		lib3ds_file_eval(m_file, 0.);
	}

	x3ds_definition::~x3ds_definition()
	{
		if (m_file)
		{
			lib3ds_file_free(m_file);
		}
	}

	void x3ds_definition::load_texture(const char* infile)
	{
		if (infile == NULL)
		{
			return;
		}			

		if (strlen(infile) == 0)
		{
			return;
		}			

		if (m_material.get(infile, NULL))
		{
			// loaded already
			return;
		}

		// try to load & create texture from file

		// is path relative ?
		tu_string url = get_player()->get_workdir();
		if (strstr(infile, ":") || infile[0] == '/')
		{
			url = "";
		}
		url += infile;

		image::rgb* im = image::read_jpeg(url.c_str());
		if (im)
		{
			bitmap_info* bi = get_render_handler()->create_bitmap_info_rgb(im);
			delete im;
			bi->layout();
			m_material[infile] = bi;
		}
		else
		{
			log_error("can't load '%s'\n", infile);
		}
	}

	character* x3ds_definition::create_character_instance(character* parent, int id)
	{
		character* ch = new x3ds_instance(get_player(), this, parent, id);
		// instanciate_registered_class(ch);	//TODO: test it
		return ch;
	}

	void x3ds_definition::get_bounding_center(Lib3dsVector bmin, Lib3dsVector bmax, Lib3dsVector center, float* size)
	{
		lib3ds_file_bounding_box_of_objects(m_file, LIB3DS_TRUE, LIB3DS_FALSE, LIB3DS_FALSE, bmin, bmax);

		// bounding box dimensions
		float	sx, sy, sz;
		sx = bmax[0] - bmin[0];
		sy = bmax[1] - bmin[1];
		sz = bmax[2] - bmin[2];

		*size = fmax(sx, sy); 
		*size = fmax(*size, sz);

		center[0] = (bmin[0] + bmax[0]) / 2;
		center[1] = (bmin[1] + bmax[1]) / 2;
		center[2] = (bmin[2] + bmax[2]) / 2;
	}

	void x3ds_definition::ensure_camera()
	{
		if (m_file->cameras == NULL)
		{
			// Fabricate camera

			Lib3dsCamera* camera = lib3ds_camera_new("Perscpective");

			float size;
			Lib3dsVector bmin, bmax;
			get_bounding_center(bmin, bmax, camera->target, &size);

			memcpy(camera->position, camera->target, sizeof(camera->position));
			camera->position[0] = bmax[0] + 0.75f * size;
			camera->position[1] = bmin[1] - 0.75f * size;
			camera->position[2] = bmax[2] + 0.75f * size;

			// hack
			Lib3dsMatrix m;
			lib3ds_matrix_identity(m);
			lib3ds_matrix_rotate_z(m, 1.6f);	// PI/2
			lib3ds_matrix_rotate_y(m, 0.3f);
			
			lib3ds_vector_transform(camera->position, m, camera->position);

			camera->near_range = (camera->position[0] - bmax[0] ) * 0.5f;
			camera->far_range = (camera->position[0] - bmin[0] ) * 3.0f;
			lib3ds_file_insert_camera(m_file, camera);
		}
	}


	void x3ds_definition::ensure_lights()
	{
		// No lights in the file?  Add one. 
		if (m_file->lights == NULL)
		{
			Lib3dsLight* light = lib3ds_light_new("light0");

			float size;
			Lib3dsVector bmin, bmax;
			get_bounding_center(bmin, bmax, light->position, &size);

			light->spot_light = 0;
			light->see_cone = 0;
			light->color[0] = light->color[1] = light->color[2] = .6f;
			light->position[0] += size * 0.75f;
			light->position[1] -= size * 1.f;
			light->position[2] += size * 1.5f;
			light->position[3] = 0.f;
			light->outer_range = 100;
			light->inner_range = 10;
			light->multiplier = 1;
			lib3ds_file_insert_light(m_file, light);
		}
	}

	void x3ds_definition::ensure_nodes()
	{
		// No nodes?  Fabricate nodes to display all the meshes.
		if (m_file->nodes == NULL)
		{
			Lib3dsMesh *mesh;
			Lib3dsNode *node;
			for (mesh = m_file->meshes; mesh != NULL; mesh = mesh->next)
			{
				node = lib3ds_node_new_object();
				strcpy(node->name, mesh->name);
				node->parent_id = LIB3DS_NO_PARENT;
				node->data.object.scl_track.keyL = lib3ds_lin3_key_new();
				node->data.object.scl_track.keyL->value[0] = 1.;
				node->data.object.scl_track.keyL->value[1] = 1.;
				node->data.object.scl_track.keyL->value[2] = 1.;
				lib3ds_file_insert_node(m_file, node);
			}
		}
	}

	void x3ds_definition::remove_camera(Lib3dsCamera* camera)
	{
		lib3ds_file_remove_camera(m_file, camera);
	}

} // end of namespace gameswf

#endif
