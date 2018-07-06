// gameswf_3ds.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for gameswf library

#include "base/tu_config.h"

#if TU_CONFIG_LINK_TO_LIB3DS == 1

#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>

#include "gameswf_3ds_inst.h"

namespace gameswf
{

	void	as_3d_play(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		assert(x3ds);

		x3ds->m_play_state = character::PLAY;
	}

	void	as_3d_stop(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		assert(x3ds);

		x3ds->m_play_state = character::STOP;
	}

	void	as_map_material(const fn_call& fn)
	{
		x3ds_instance* x3ds = cast_to<x3ds_instance>(fn.this_ptr);
		if (x3ds && fn.nargs == 2)
		{
			x3ds->m_map[fn.arg(0).to_tu_string()] = fn.arg(1);
		}
	}

	//
	// x3ds_instance
	//

	x3ds_instance::x3ds_instance(player* player, x3ds_definition* def, character* parent, int id)	:
		character(player, parent, id),
		m_def(def),
		m_current_frame(0.0f),
		m_play_state(PLAY)
	{
		assert(m_def != NULL);

		// take the first camera
		m_camera = m_def->m_file->cameras;

		set_member("mapMaterial", as_map_material);
		set_member("play", as_3d_play);
		set_member("stop", as_3d_stop);
//		set_member("gotoAndStop", &as_3d_goto_and_stop);
//		set_member("gotoAndPlay", &as_3d_goto_and_play);
//		set_member("nextFrame", &as_3d_next_frame);
//		set_member("prevFrame", &as_3d_prev_frame);
	}

	void x3ds_instance::clear_dlist(Lib3dsNode* node)
	{
		for (Lib3dsNode* p = node; p != 0; p = p->next)
		{
			if (p->user.i)
			{
				glDeleteLists(p->user.i, 1);
			}

			clear_dlist(node->childs);
		}
	}

	x3ds_instance::~x3ds_instance()
	{
		clear_dlist(m_def->m_file->nodes);
		m_def->remove_camera(m_camera);
	}

	void	x3ds_instance::advance(float delta_time)
	{
		on_event(event_id::ENTER_FRAME);
		if (m_play_state == PLAY)
		{
			m_current_frame = fmod(m_current_frame + 1.0f, (float) m_def->m_file->frames);
			lib3ds_file_eval(m_def->m_file, m_current_frame);
		}
	}

	bool	x3ds_instance::get_member(const tu_stringi& name, as_value* val)
	{
		if (character::get_member(name, val) == false)
		{
			return m_variables.get(name, val);
		}
		return true;
	}

	bool	x3ds_instance::set_member(const tu_stringi& name, const as_value& val)
	{
		if (character::set_member(name, val) == false)
		{
			m_variables.set(name, val);
		};
		return true;
	}


//	void	x3ds_instance::apply_matrix(float* target, float* camera_pos)
//	{
//		Lib3dsVector v;
//		lib3ds_vector_sub(v, target, camera_pos);
//		float dist = lib3ds_vector_length(v);

//		glTranslatef(0., dist, 0.);
//		glMultMatrixf(&m_matrix[0][0]);
//		glTranslatef(0., -dist, 0.);
//	}

	// apply light
	// Set them from light nodes if possible.
	// If not, use the light objects directly.
	void	x3ds_instance::set_light()
	{
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_def->m_file->ambient);
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
		glShadeModel(GL_SMOOTH);

		Lib3dsLight* l = m_def->m_file->lights;
		for (int gl_light = GL_LIGHT0; gl_light <= GL_LIGHT7; gl_light++)
		{
			if (l)
			{
				glEnable(gl_light);

				// default we use light objects directly
				GLfloat color[4];
				color[0] = l->color[0];
				color[1] = l->color[1];
				color[2] = l->color[2];
				color[3] = 1;

				// default position
				GLfloat position[4];
				position[0] = l->position[0];
				position[1] = l->position[1];
				position[2] = l->position[2];
				position[3] = 1;

				// try light nodes if possible
				Lib3dsNode* ln = lib3ds_file_node_by_name(m_def->m_file, l->name, LIB3DS_LIGHT_NODE);
				if (ln)
				{
					color[0] = ln->data.light.col[0];
					color[1] = ln->data.light.col[1];
					color[2] = ln->data.light.col[2];
					position[0] = ln->data.light.pos[0];
					position[1] = ln->data.light.pos[1];
					position[2] = ln->data.light.pos[2];
				}

				// try spot nodes if possible
				Lib3dsNode* sn = lib3ds_file_node_by_name(m_def->m_file, l->name, LIB3DS_SPOT_NODE);
				if (sn)
				{
					l->spot[0] = sn->data.spot.pos[0];
					l->spot[1] = sn->data.spot.pos[1];
					l->spot[2] = sn->data.spot.pos[2];
				}

				static const GLfloat a[] = {0.0f, 0.0f, 0.0f, 1.0f};
				glLightfv(gl_light, GL_AMBIENT, a);
				glLightfv(gl_light, GL_DIFFUSE, color);
				glLightfv(gl_light, GL_SPECULAR, color);
				glLightfv(gl_light, GL_POSITION, position);
				glLightf(gl_light, GL_LINEAR_ATTENUATION, l->attenuation);
//				glLightf(gl_light, GL_CONSTANT_ATTENUATION, 0);
//				glLightf(gl_light, GL_QUADRATIC_ATTENUATION, 0);

				if (l->spot_light)
				{
					position[0] = l->spot[0];
					position[1] = l->spot[1];
					position[2] = l->spot[2];
					glLightfv(gl_light, GL_SPOT_DIRECTION, position);
					glLightf(gl_light, GL_SPOT_CUTOFF, l->fall_off);
					glLightf(gl_light, GL_SPOT_EXPONENT, 0);	// hack
				}

				l = l->next;
			}
			else
			{
				glDisable(gl_light);
			}
		}
		glEnable(GL_LIGHTING);
	}

	void	x3ds_instance::display()
	{
		assert(m_def != NULL);

		if (m_def->m_file == NULL)
		{
			return;
		}

		//update_material();

		// save GL state
		glPushAttrib (GL_ALL_ATTRIB_BITS);	
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		// apply gameSWF matrix

		rect bound;
		m_def->get_bound(&bound);

		matrix m = get_parent()->get_world_matrix();
		m.transform(&bound);

		// get viewport size
		GLint vp[4]; 
		glGetIntegerv(GL_VIEWPORT, vp); 
//		int vp_width = vp[2];
		int vp_height = vp[3];

		bound.twips_to_pixels();
		int w = (int) (bound.m_x_max - bound.m_x_min);
		int h = (int) (bound.m_y_max - bound.m_y_min);
		int x = (int) bound.m_x_min;
		int y = (int) bound.m_y_min;
		glViewport(x, vp_height - y - h, w, h);

		// set 3D params

		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);

		glEnable(GL_POLYGON_SMOOTH);
//		glEnable(GL_BLEND);
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);

		// set GL matrix to identity
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		assert(m_camera);

		Lib3dsNode* cnode = lib3ds_file_node_by_name(m_def->m_file, m_camera->name, LIB3DS_CAMERA_NODE);
		Lib3dsNode* tnode = lib3ds_file_node_by_name(m_def->m_file, m_camera->name, LIB3DS_TARGET_NODE);

		float* target = tnode ? tnode->data.target.pos : m_camera->target;

		float	view_angle;
		float roll;
		float* camera_pos;
		if (cnode)
		{
			view_angle = cnode->data.camera.fov;
			roll = cnode->data.camera.roll;
			camera_pos = cnode->data.camera.pos;
		}
		else
		{
			view_angle = m_camera->fov;
			roll = m_camera->roll;
			camera_pos = m_camera->position;
		}

		float ffar = m_camera->far_range;
		float nnear = m_camera->near_range <= 0 ? ffar * .001f : m_camera->near_range;

		float top = tan(view_angle * 0.5f) * nnear;
		float bottom = -top;
		float aspect = 1.3333f;	// 4/3 == width /height
		float left = aspect* bottom;
		float right = aspect * top;

		//	gluPerspective(fov, aspect, nnear, ffar) ==> glFrustum(left, right, bottom, top, nnear, ffar);
		//	fov * 0.5 = arctan ((top-bottom)*0.5 / near)
		//	Since bottom == -top for the symmetrical projection that gluPerspective() produces, then:
		//	top = tan(fov * 0.5) * near
		//	bottom = -top
		//	Note: fov must be in radians for the above formulae to work with the C math library. 
		//	If you have comnputer your fov in degrees (as in the call to gluPerspective()), 
		//	then calculate top as follows:
		//	top = tan(fov*3.14159/360.0) * near
		//	The left and right parameters are simply functions of the top, bottom, and aspect:
		//	left = aspect * bottom
		//	right = aspect * top
		glFrustum(left, right, bottom, top, nnear, ffar);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90., 1.0, 0., 0.);

		// apply camera matrix
		Lib3dsMatrix cmatrix;
		lib3ds_matrix_camera(cmatrix, camera_pos, target, roll);
		glMultMatrixf(&cmatrix[0][0]);

		// apply light
		set_light();

		// draw 3D model

		for (Lib3dsNode* p = m_def->m_file->nodes; p != 0; p = p->next)
		{
			render_node(p);
		}

		// restore openGL state
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();
	}

	void x3ds_instance::bind_texture(const char* infile)
	{
		glDisable(GL_TEXTURE_2D);
		if (infile)
		{
			gc_ptr<bitmap_info> bi;
			if (m_def->m_material.get(infile, &bi))
			{
				bi->activate();

				glDisable(GL_TEXTURE_GEN_S);
				glDisable(GL_TEXTURE_GEN_T);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}
		}
	}

	void x3ds_instance::set_material(Lib3dsMaterial* mat)
	{
		if (mat)
		{
//			if (mat->two_sided)
//			{
//				glDisable(GL_CULL_FACE);
//			}
//			else
//			{
//				glEnable(GL_CULL_FACE);
//			}
			glDisable(GL_CULL_FACE);

			glMaterialfv(GL_FRONT, GL_AMBIENT, mat->ambient);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, mat->diffuse);
			glMaterialfv(GL_FRONT, GL_SPECULAR, mat->specular);
			glMaterialf(GL_FRONT, GL_SHININESS, pow(2.0f, 10.0f * mat->shininess));

			bind_texture(mat->texture1_map.name);
		}
		else
		{
			// no material, assume the default
			static const Lib3dsRgba a = {0.2f, 0.2f, 0.2f, 1.0f};
			static const Lib3dsRgba d = {0.8f, 0.8f, 0.8f, 1.0f};
			static const Lib3dsRgba s = {0.0f, 0.0f, 0.0f, 1.0f};
			glMaterialfv(GL_FRONT, GL_AMBIENT, a);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
			glMaterialfv(GL_FRONT, GL_SPECULAR, s);
      glMaterialf(GL_FRONT, GL_SHININESS, pow(2.0f, 10.0f * 0.5f));
		}
	}

	void x3ds_instance::bind_material(Lib3dsMaterial* mat, float U, float V)
	{
		if (mat)
		{
			gc_ptr<bitmap_info> bi = NULL;
			if (m_def->m_material.get(mat->texture1_map.name, &bi))
			{
				// get texture size
				int	tw = 1; while (tw < bi->get_width()) { tw <<= 1; }
				int	th = 1; while (th < bi->get_height()) { th <<= 1; }

				float scale_x = (float) bi->get_width() / tw;
				float scale_y = (float) bi->get_height() / th;

				float s = scale_y * (1 - V);
				float t = U * scale_x;
				glTexCoord2f(s, t);
			}
		}
	}

	void x3ds_instance::create_mesh_list(Lib3dsMesh* mesh)
	{
		Lib3dsVector* normalL = (Lib3dsVector*) malloc(3 * sizeof(Lib3dsVector) * mesh->faces);
		Lib3dsMatrix m;
		lib3ds_matrix_copy(m, mesh->matrix);
		lib3ds_matrix_inv(m);
		glMultMatrixf(&m[0][0]);

		lib3ds_mesh_calculate_normals(mesh, normalL);

		Lib3dsMaterial* prev_mat = NULL;
		for (Uint32 p = 0; p < mesh->faces; ++p)
		{
			Lib3dsFace* f = &mesh->faceL[p];
			Lib3dsMaterial* mat = f->material[0] ? 
				lib3ds_file_material_by_name(m_def->m_file, f->material) : NULL;

			if (prev_mat != mat)
			{
				set_material(mat);
			}

			glBegin(GL_TRIANGLES);
			glNormal3fv(f->normal);
			for (int i = 0; i < 3; ++i)
			{
				glNormal3fv(normalL[3 * p + i]);
				if (mesh->texelL)
				{
					bind_material(mat, mesh->texelL[f->points[i]][1], mesh->texelL[f->points[i]][0]);
				}
				glVertex3fv(mesh->pointL[f->points[i]].pos);
			}
			glEnd();
			glDisable(GL_TEXTURE_2D);
		}

		free(normalL);
	}

	void x3ds_instance::render_node(Lib3dsNode* node)
	{
		for (Lib3dsNode* p = node->childs; p != 0; p = p->next)
		{
			render_node(p);
		}

		if (node->type == LIB3DS_OBJECT_NODE)
		{
			if (strcmp(node->name, "$$$DUMMY") == 0)
			{
				return;
			}

			Lib3dsMesh* mesh = lib3ds_file_mesh_by_name(m_def->m_file, node->data.object.morph);
			if (mesh == NULL)
			{
				mesh = lib3ds_file_mesh_by_name(m_def->m_file, node->name);
			}
			assert(mesh);
			
			// build mesh list
			if (node->user.i == 0)
			{
				node->user.i = glGenLists(1);
				glNewList(node->user.i, GL_COMPILE);
				create_mesh_list(mesh);
				glEndList();
			}

			// exec mesh list
			glPushMatrix();
			Lib3dsObjectData* d = &node->data.object;
			glMultMatrixf(&node->matrix[0][0]);
			glTranslatef( - d->pivot[0], - d->pivot[1], - d->pivot[2]);
			glCallList(node->user.i);
			glPopMatrix();
		}
	}

	bool	x3ds_instance::on_event(const event_id& id)
	// Dispatch event handler(s), if any.
	{
		// Keep m_as_environment alive during any method calls!
		gc_ptr<as_object>	this_ptr(this);

		bool called = false;

		// Check for member function.
		const tu_stringi&	method_name = id.get_function_name().to_tu_stringi();
		if (method_name.length() > 0)
		{
			as_value	method;
			if (get_member(method_name, &method))
			{
				as_environment env(get_player());
				gameswf::call_method(method, &env, this, 0, env.get_top_index());
				called = true;
			}
		}
		return called;
	}

#if 0
	// useless stuff ?
	void	x3ds_instance::update_material()
	{
		GLint aux_buffers;
		glGetIntegerv(GL_AUX_BUFFERS, &aux_buffers);
		if (aux_buffers < 2)
		{
			static int n = 0;
			if (n < 1)
			{
				n++;
				log_error("Your video card does not have AUX buffers, can't snapshot movieclips\n");
			}
			return;
		}

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		// update textures
		for (stringi_hash<gc_ptr <bitmap_info> >::iterator it = 
			m_material.begin(); it != m_material.end(); ++it)
		{
			as_value target = m_map[it->first];
			character* ch = find_target(target);
			if (ch)
			{
				if (ch->get_parent() == NULL)
				{
					log_error("Can't snapshot _root movieclip, material '%s'\n", it->first.c_str());
					continue;
				}

				GLint draw_buffer;
				glGetIntegerv(GL_DRAW_BUFFER, &draw_buffer);

				glDrawBuffer(GL_AUX1);
				glReadBuffer(GL_AUX1);

				// save "ch" matrix
				matrix ch_matrix = ch->get_matrix();

				float ch_width = TWIPS_TO_PIXELS(ch->get_width());
				float ch_height = TWIPS_TO_PIXELS(ch->get_height());

				// get viewport size
        GLint vp[4]; 
        glGetIntegerv(GL_VIEWPORT, vp); 
				int vp_width = vp[2];
				int vp_height = vp[3];

				// get texture size
				int	tw = 1; while (tw < ch_width) { tw <<= 1; }
				int	th = 1; while (th < ch_height) { th <<= 1; }

				// texture size must be <= viewport size
				if (tw > vp_width)
				{
					tw = vp_width;
				}
				if (th > vp_height)
				{
					th = vp_height;
				}

				ch->set_member("_width", tw);
				ch->set_member("_height", th);

				rect bound;
				ch->get_bound(&bound);

				// parent world matrix moves point(0,0) to "pzero"
				matrix mparent = ch->get_parent()->get_world_matrix();
				point pzero;
				mparent.transform_by_inverse(&pzero, point(0, 0));

				// after transformation of "ch" matrix left-top corner is in point(bound.m_x_min, bound.m_y_min),
				// therefore we need to move point(bound.m_x_min, bound.m_y_min) to point(pzero.m_x, pzero.m_y)
				// that "ch" movieclip's left-top corner will be in point(0,0) of screen
				matrix m = ch->get_matrix();
				float xt = (pzero.m_x - bound.m_x_min) / m.get_x_scale();
				float yt = (pzero.m_y - bound.m_y_min) / m.get_y_scale();

				// move "ch" to left-bottom corner (as point of origin of OpenGL is in left-bottom)
				// later glCopyTexImage2D will copy snapshot of "ch" into texture
				yt += PIXELS_TO_TWIPS(vp_height - th) / m.get_y_scale();
				m.concatenate_translation(xt, yt);

				ch->set_matrix(m);

				glClearColor(1, 1, 1, 1);
				glClear(GL_COLOR_BUFFER_BIT);

				ch->display();

				// restore "ch" matrix
				ch->set_matrix(ch_matrix);

				gc_ptr<bitmap_info> bi = it->second;
				if (bi->m_texture_id == 0)
				{
					glGenTextures(1, (GLuint*) &bi->m_texture_id);
					bi->m_original_height = (int) ch_height;
					bi->m_original_width = (int) ch_width;
				}
				glBindTexture(GL_TEXTURE_2D, bi->m_texture_id);
				glEnable(GL_TEXTURE_2D);
				glDisable(GL_TEXTURE_GEN_S);
				glDisable(GL_TEXTURE_GEN_T);				

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0,0, tw, th, 0);

				glDisable(GL_TEXTURE_2D);

				glDrawBuffer(draw_buffer);
				glReadBuffer(draw_buffer);
			}
		}
		glPopAttrib();
	}

#endif

} // end of namespace gameswf

#endif
