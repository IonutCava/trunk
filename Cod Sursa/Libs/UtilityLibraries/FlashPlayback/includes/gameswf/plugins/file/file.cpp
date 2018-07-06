// file.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient file I/O plugin for the gameswf SWF player library.

#include "file.h"
#include "gameswf/gameswf_log.h"


namespace file_plugin
{

	// file.read(), text only
	void file_read(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi)
		{
			if (fi->m_file->get_error() == TU_FILE_NO_ERROR)
			{
				char buf[256];
				fi->m_file->read_string(buf, 256);
				fn.result->set_string(buf);
			}
		}
	}

	// file.write()
	void file_write(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi && fn.nargs > 0)
		{
			if (fi->m_file->get_error() == TU_FILE_NO_ERROR)
			{
				fi->m_file->write_string(fn.arg(0).to_string());
			}
		}
	}

	// file.eof readonly property
	void file_get_eof(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi)
		{
			fn.result->set_bool(fi->m_file->get_eof());
		}
	}

	// file.error readonly property
	void file_get_error(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi)
		{
			fn.result->set_int(fi->m_file->get_error());
		}
	}

	// DLL interface
	extern "C"
	{
		exported_module as_object* gameswf_module_init(player* player, const array<as_value>& params)
		{
			if (params.size() >= 2)
			{
				return new file(player, params[0].to_tu_string(), params[1].to_tu_string());
			}
			log_error("new File(): not enough args\n");
			return NULL;
		}
	}

	file::file(player* player, const tu_string& path, const tu_string& mode) :
		as_object(player),
		m_file(NULL)
	{
		// is path relative ?
		tu_string file_name = get_player()->get_workdir();
		if (strstr(path.c_str(), ":") || *path.c_str() == '/')
		{
			file_name = "";
		}
		file_name += path;

		m_file = new tu_file(file_name.c_str(), mode.c_str());

		// methods
		builtin_member("read", file_read);
		builtin_member("write", file_write);
		builtin_member("eof", as_value(file_get_eof, as_value()));	// readonly property
		builtin_member("error", as_value(file_get_error, as_value()));	// readonly property

	}

	file::~file()
	{
		delete m_file;
	}

}

