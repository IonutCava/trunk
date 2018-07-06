// mytable.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient MYSQL plugin
// for the gameswf SWF player library.

#include "gameswf/gameswf_mutex.h"
#include "gameswf/gameswf_action.h"
#include "table.h"

namespace mysql_plugin
{

	tu_mutex s_mysql_plugin_mutex;

	void	to_string_method(const fn_call& fn)
	// Returns amount of the rows in the table
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_string("[mytable]");
		}
	}

	void	size_method(const fn_call& fn)
	// Returns amount of the rows in the table
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_int(tbl->size());
		}
	}

	void	next_method(const fn_call& fn)
	// Moves row pointer to next row
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_bool(tbl->next());
		}
	}

	void	prev_method(const fn_call& fn)
	// Moves row pointer to prev row
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_bool(tbl->prev());
		}
	}

	void	first_method(const fn_call& fn)
	// Moves row pointer to the first row
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			tbl->first();
		}
	}

	void	field_count_method(const fn_call& fn)
	// Returns amount of the fields in the table
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			fn.result->set_int(tbl->fld_count());
		}
	}

	void	goto_record_method(const fn_call& fn)
	// Moves row pointer to the fn.arg(0).to_number()
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			assert(fn.nargs == 1);
			fn.result->set_bool(tbl->goto_record(fn.arg(0).to_int()));
		}
	}

	void	get_title_method(const fn_call& fn)
	// Returns the name of fn.arg(0).to_number() field
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			assert(fn.nargs == 1);
			fn.result->set_string(tbl->get_field_title(fn.arg(0).to_int()));
		}
	}

	void	to_get_recno_method(const fn_call& fn)
	// Returns the current record number
	{
		mytable* tbl = cast_to<mytable>(fn.this_ptr);
		if (tbl)
		{
			if (tbl->size() > 0)
			{
				fn.result->set_int(tbl->get_recno() + 1);
			}
		}
	}

	mytable::mytable(player* player) :
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

	mytable::~mytable()
	{
	}

	bool	mytable::get_member(const tu_stringi& name, as_value* val)
	{
		tu_autolock locker(s_mysql_plugin_mutex);

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

	int mytable::size() const
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		return m_data.size();
	}

	int mytable::get_recno() const
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		return m_index;
	}

	bool mytable::prev()
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		if (m_index > 0)
		{
			m_index--;
			return true;
		}
		return false;
	}

	bool mytable::next()
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		if (m_index < m_data.size() - 1)
		{
			m_index++;
			return true;
		}
		return false;
	}

	void mytable::first()
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		m_index = 0;
	}

	int mytable::fld_count()
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		return m_title.size();
	}

	bool mytable::goto_record(int index)
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		if (index < (int) m_data.size() && index >= 0)
		{
			m_index = index;
			return true;
		}
		return false;
	}

	const char* mytable::get_field_title(int n)
	{
		tu_autolock locker(s_mysql_plugin_mutex);

		if (n >= 0 && n < (int) m_title.size())
		{
			return m_title[n].c_str();
		}
		return NULL;
	}

	void mytable::retrieve_data(MYSQL_RES* result)
	{

		tu_autolock locker(s_mysql_plugin_mutex);

		MYSQL_FIELD* fld = mysql_fetch_fields(result);
		int num_fields = mysql_num_fields(result);
		int num_rows =  (int) mysql_num_rows(result);

		m_title.resize(num_fields);
		for (int i = 0; i < num_fields; i++)
		{
			m_title[i] = fld[i].name;
		}

		m_data.resize(num_rows);
		for (int i = 0; i < num_rows; i++)
		{
			MYSQL_ROW row = mysql_fetch_row(result);

			m_data[i] = new as_object(get_player());

			for (int j = 0; j < num_fields; j++)
			{
				as_value val;
				if (row[j] == NULL)
				{
					//					val.set_undefined();
					val.set_string("");
				}
				else
				{
					switch (fld[j].type)
					{
						default:
							break;

						case MYSQL_TYPE_TINY:
						case MYSQL_TYPE_SHORT:
						case MYSQL_TYPE_INT24:
							val.set_int(atoi(row[j]));
							break;

						case MYSQL_TYPE_DECIMAL:
						case MYSQL_TYPE_LONG:
						case MYSQL_TYPE_FLOAT:
						case MYSQL_TYPE_DOUBLE:
						case MYSQL_TYPE_LONGLONG:
							val.set_double(atof(row[j]));
							break;

						case MYSQL_TYPE_NULL:
						case MYSQL_TYPE_TIMESTAMP:
						case MYSQL_TYPE_DATE:
						case MYSQL_TYPE_TIME:
						case MYSQL_TYPE_DATETIME:
						case MYSQL_TYPE_YEAR:
						case MYSQL_TYPE_NEWDATE:
						case MYSQL_TYPE_VARCHAR:
						case MYSQL_TYPE_BIT:
						case MYSQL_TYPE_NEWDECIMAL:
						case MYSQL_TYPE_ENUM:
						case MYSQL_TYPE_SET:
						case MYSQL_TYPE_TINY_BLOB:
						case MYSQL_TYPE_MEDIUM_BLOB:
						case MYSQL_TYPE_LONG_BLOB:
						case MYSQL_TYPE_BLOB:
						case MYSQL_TYPE_VAR_STRING:
						case MYSQL_TYPE_STRING:
						case MYSQL_TYPE_GEOMETRY:
							val.set_string(row[j]);
							break;
						}
				}
				m_data[i]->set_member(fld[j].name, val);
			}
		}
	}
}
