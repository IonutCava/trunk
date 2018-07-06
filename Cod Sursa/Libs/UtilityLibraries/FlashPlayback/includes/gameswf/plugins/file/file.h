// file.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient file I/O plugin for the gameswf SWF player library.

#ifndef GAMESWF_FILE_PLUGIN_H
#define GAMESWF_FILE_PLUGIN_H

#include "gameswf/gameswf_action.h"	// for as_object
#include "base/tu_file.h"

using namespace gameswf;

namespace file_plugin
{

	struct file : public as_object
	{

		// Unique id of a gameswf resource
		enum { m_class_id = AS_PLUGIN_FILE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		exported_module file(player* player, const tu_string& path, const tu_string& mode);
		exported_module ~file();

	//	exported_module void open(const char* path, const char* mode);

//	private:

		tu_file* m_file;

	};

}

#endif // GAMESWF_FILE_PLUGIN_H
