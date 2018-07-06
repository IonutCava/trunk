// sqlite_db.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// sqlite plugin implementation for the gameswf SWF player library.

#ifndef GAMESWF_SQLITE_DB_H
#define GAMESWF_SQLITE_DB_H

#include <sqlite3.h>
#include "../sql_db.h"
#include "sqlite_table.h"

using namespace gameswf;

namespace sqlite_plugin
{

	struct sqlite_db : public sql_db
	{

		// Unique id of a gameswf resource
		enum { m_class_id = AS_PLUGIN_SQLITE_DB };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return sql_db::is(class_id);
		}

		exported_module sqlite_db(player* player);
		exported_module ~sqlite_db();

		exported_module bool connect(const char* dbfile, bool read_only, const char* vfs);
		exported_module void disconnect();
		exported_module sql_table* open(const char* sql);
		exported_module int run(const char *sql);
		exported_module void set_autocommit(bool autocommit);
		exported_module void commit();
		exported_module void create_function(const char* name, as_function* func);

		bool m_trace;
		gc_ptr<sqlite_table> m_result;

	private:

		bool runsql(const char* sql);
		sqlite3* m_db;
		bool m_autocommit;
		string_hash< gc_ptr<as_object_interface> > m_callback_context;
	};

	struct func_context : public as_object_interface
	{
		func_context(sqlite_db* this_ptr, as_function* func) :
			m_this_ptr(this_ptr),
			m_func(func)
		{
		}
		
		virtual bool is(int class_id) const
		{
			return false;
		}

		weak_ptr<sqlite_db> m_this_ptr;
		weak_ptr<as_function> m_func;
	};


}

#endif // GAMESWF_SQLITE_DB_H
