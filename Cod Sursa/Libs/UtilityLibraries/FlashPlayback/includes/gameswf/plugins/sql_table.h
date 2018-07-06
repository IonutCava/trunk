// sql_table.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// interface module to mysql & sqlite plugins

#ifndef GAMESWF_SQL_TABLE_H
#define GAMESWF_SQL_TABLE_H

#include "gameswf/gameswf_action.h"	// for as_object

using namespace gameswf;

struct sql_table: public as_object
{
	// Unique id of a gameswf resource
	enum { m_class_id = AS_PLUGIN_SQL_TABLE };
	virtual bool is(int class_id) const
	{
		if (m_class_id == class_id) return true;
		else return as_object::is(class_id);
	}

	sql_table(player* player) :	as_object(player)	{}

	virtual int size() const = 0;
	virtual bool prev() = 0;
	virtual bool next() = 0;
	virtual void first() = 0;
	virtual int fld_count() = 0;
	virtual bool goto_record(int index) = 0;
	virtual const char* get_field_title(int n) = 0;
	virtual int get_recno() const = 0;
};

#endif // GAMESWF_SQL_TABLE_H
