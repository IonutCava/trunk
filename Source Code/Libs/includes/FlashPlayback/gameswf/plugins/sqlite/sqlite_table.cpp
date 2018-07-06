// sqlite_table.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient sqlite plugin
// for the gameswf SWF player library.

#include "gameswf/gameswf_mutex.h"
#include "gameswf/gameswf_action.h"
#include "sqlite_table.h"

namespace sqlite_plugin
{

	tu_mutex s_sqlite_plugin_mutex;

	void	to_string_method(const fn_call& fn)
	// Returns amount of the rows in the table
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_string("[sqlite_table]");
		}
	}

	void	size_method(const fn_call& fn)
	// Returns amount of the rows in the table
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_int(tbl->size());
		}
	}

	void	next_method(const fn_call& fn)
	// Moves row pointer to next row
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_bool(tbl->next());
		}
	}

	void	prev_method(const fn_call& fn)
	// Moves row pointer to prev row
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_bool(tbl->prev());
		}
	}

	void	first_method(const fn_call& fn)
	// Moves row pointer to the first row
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			tbl->first();
		}
	}

	void	field_count_method(const fn_call& fn)
	// Returns amount of the fields in the table
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_int(tbl->fld_count());
		}
	}

	void	goto_record_method(const fn_call& fn)
	// Moves row pointer to the fn.arg(0).to_number()
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			assert(fn.nargs == 1);
			fn.result->set_bool(tbl->goto_record(fn.arg(0).to_int()));
		}
	}

	void	get_title_method(const fn_call& fn)
	// Returns the name of fn.arg(0).to_number() field
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			assert(fn.nargs == 1);
			fn.result->set_string(tbl->get_field_title(fn.arg(0).to_int()));
		}
	}

	void	to_get_recno_method(const fn_call& fn)
	// Returns the current record number
	{
		sqlite_table* tbl = cast_to<sqlite_table>(fn.this_ptr);
		if (tbl)
		{
			if (tbl->size() > 0)
			{
				fn.result->set_int(tbl->get_recno() + 1);
			}
		}
	}

	sqlite_table::sqlite_table(player* player) :
		sql_table(player),
		m_index(0)
	{
		builtin_member("size", size_method);
		builtin_member("next", next_method);
		builtin_member("prev", prev_method);
		builtin_member("first", first_method);
		builtin_member("fields", field_count_method);
		builtin_member("goto", goto_record_method);
		builtin_member("title", get_title_method);
		builtin_member("toString", to_string_method);
		builtin_member("getRecNo", to_get_recno_method);
	}

	sqlite_table::~sqlite_table()
	{
	}

	bool	sqlite_table::get_member(const tu_stringi& name, as_value* val)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		val->set_undefined();

		// check table methods
		if (as_object::get_member(name, val) == false)
		{

			if (m_index >= m_data.size())
			{
				return false;
			}

			if (m_data[m_index]->get_member(name, val))
			{
				return true;
			}

			// try as field number
			int n;
			if (string_to_number(&n, name.c_str()))
			{
				if (n >= 0 && n < (int) m_title.size())
				{
					if (m_data[m_index]->get_member(m_title[n], val))
					{
						return true;
					}
				}
			}
			return false;
		}
		return true;
	}

	int sqlite_table::size() const
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		return m_data.size();
	}

	int sqlite_table::get_recno() const
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		return m_index;
	}

	bool sqlite_table::prev()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		if (m_index > 0)
		{
			m_index--;
			return true;
		}
		return false;
	}

	bool sqlite_table::next()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		if (m_index < m_data.size() - 1)
		{
			m_index++;
			return true;
		}
		return false;
	}

	void sqlite_table::first()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		m_index = 0;
	}

	int sqlite_table::fld_count()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		return m_title.size();
	}

	bool sqlite_table::goto_record(int index)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		if (index < (int) m_data.size() && index >= 0)
		{
			m_index = index;
			return true;
		}
		return false;
	}

	const char* sqlite_table::get_field_title(int n)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		if (n >= 0 && n < (int) m_title.size())
		{
			return m_title[n].c_str();
		}
		return NULL;
	}

	void sqlite_table::retrieve_data(sqlite3_stmt* stmt)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		int col_count = sqlite3_column_count(stmt);
		m_title.resize(col_count);
		for (int i = 0; i < col_count; i++)
		{
			m_title[i] = sqlite3_column_name(stmt, i);
		}
 
		int rc;
		do
		{
			as_object* row = new as_object(get_player());
			m_data.push_back(row);

			for (int i = 0; i < col_count; i++)
			{
				as_value val;
				int col_type = sqlite3_column_type(stmt, i);
				switch (col_type)
				{
					case SQLITE_INTEGER :
						val.set_int(sqlite3_column_int(stmt, i));
						break;
					case SQLITE_FLOAT :
						val.set_double(sqlite3_column_double(stmt, i));
						break;
					case SQLITE_BLOB :
// FIXME:						val.set_string((const char*) sqlite3_column_blob(stmt, i));
						break;
					case SQLITE_NULL :
//						val.set_null();
						val.set_string("");
						break;
					case SQLITE3_TEXT :
						val.set_string((const char*) sqlite3_column_text(stmt, i));
						break;
					default:
						assert(0);
						break;
				}
				row->set_member(m_title[i], val);
			}
			rc = sqlite3_step(stmt);
		}
		while (rc == SQLITE_ROW);
	}
}

