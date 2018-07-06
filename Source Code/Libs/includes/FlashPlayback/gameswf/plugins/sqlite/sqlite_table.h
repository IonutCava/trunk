// sqlite_table.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// sqlite table implementation for the gameswf SWF player library.

#ifndef GAMESWF_SQLITE_TABLE_H
#define GAMESWF_SQLITE_TABLE_H

#include <sqlite3.h>
#include "gameswf/gameswf_action.h"	// for as_object
#include "../sql_table.h"

using namespace gameswf;

namespace sqlite_plugin
{

	struct sqlite_table: public sql_table
	{

		// Unique id of a gameswf resource
		enum { m_class_id = AS_PLUGIN_SQLITE_TABLE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		exported_module sqlite_table(player* player);
		exported_module ~sqlite_table();

		exported_module virtual bool	get_member(const tu_stringi& name, as_value* val);

		exported_module int size() const;
		exported_module bool prev();
		exported_module bool next();
		exported_module void first();
		exported_module int fld_count();
		exported_module bool goto_record(int index);
		exported_module const char* get_field_title(int n);
		void retrieve_data(sqlite3_stmt* stmt);
		exported_module int get_recno() const;


	private:
		int m_index;
		array<gc_ptr<as_object> > m_data;	// [columns][rows]
		array<tu_stringi> m_title;
	};

}

#endif // GAMESWF_SQLITE_TABLE_H
