// sqlite_db.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient sqlite plugin
// for the gameswf SWF player library.

#include "sqlite_db.h"

#include "gameswf/gameswf_mutex.h"
#include "gameswf/gameswf_character.h"
#include "gameswf/gameswf_log.h"

namespace sqlite_plugin
{
	extern tu_mutex s_sqlite_plugin_mutex;

	void	sqlite_connect(const fn_call& fn)
	//  Closes a previously opened connection & create new connection to db
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		fn.result->set_bool(false);
		if (db && fn.nargs > 0)
		{
			// connect(db_name, read_only, vfs_name)
			bool read_only = false;
			const char* vfs = NULL;		// by default is used OS file system
			if (fn.nargs > 1)
			{
				read_only = fn.arg(1).to_bool();
			}
			if (fn.nargs > 2 && fn.arg(2).to_tu_string().size() > 0)
			{
				vfs = fn.arg(2).to_string();
			}

			fn.result->set_bool(db->connect(fn.arg(0).to_string(), read_only, vfs));
		}
	}

	void	sqlite_disconnect(const fn_call& fn)
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db)
		{
			db->disconnect();
		}
	}

	void	sqlite_open(const fn_call& fn)
	// Creates new table from sql statement & returns pointer to it
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db)
		{
			if (fn.nargs < 1)
			{
				log_error("Db.open() needs 1 arg\n");
				return;
			}

			sql_table* tbl = (db == NULL) ? NULL : db->open(fn.arg(0).to_string());
			if (tbl)
			{
				fn.result->set_as_object(tbl);
				return;
			}
		}
	}

	void	sqlite_run(const fn_call& fn)
	// Executes sql statement & returns affected rows
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db)
		{
			if (fn.nargs < 1)
			{
				log_error("Db.run() needs 1 arg\n");
				return;
			}

			fn.result->set_int(db->run(fn.arg(0).to_string()));
		}
	}

	void	sqlite_commit(const fn_call& fn)
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db)
		{
			db->commit();
		}
	}

	void sqlite_autocommit_setter(const fn_call& fn)
	{
		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db && fn.nargs == 1)
		{
			db->set_autocommit(fn.arg(0).to_bool());
		}
	}

	void sqlite_trace_setter(const fn_call& fn)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db && fn.nargs == 1)
		{
			db->m_trace = fn.arg(0).to_bool();
		}
	}

	void sqlite_create_function(const fn_call& fn)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		sqlite_db* db = cast_to<sqlite_db>(fn.this_ptr);
		if (db && fn.nargs >= 2)
		{
			db->create_function(fn.arg(0).to_string(), fn.arg(1).to_function());
		}
	}

	// DLL interface
	extern "C"
	{
		exported_module as_object* gameswf_module_init(player* player, const array<as_value>& params)
		{
			return new sqlite_db(player);
		}
	}

	sqlite_db::sqlite_db(player* player) :
		sql_db(player),
		m_trace(false),
		m_db(NULL),
		m_autocommit(true)
	{
		// methods
		builtin_member("connect", sqlite_connect);
		builtin_member("disconnect", sqlite_disconnect);
		builtin_member("open", sqlite_open);
		builtin_member("run", sqlite_run);
		builtin_member("commit", sqlite_commit);
		builtin_member("auto_commit", as_value(as_value(), sqlite_autocommit_setter));
		builtin_member("trace", as_value(as_value(), sqlite_trace_setter));
		builtin_member("createFunction", sqlite_create_function);
	}

	sqlite_db::~sqlite_db()
	{
		disconnect();
	}

	void sqlite_db::disconnect()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		if (m_db != NULL)
		{
			sqlite3_close(m_db);
			m_db = NULL;
		}
	}

	bool sqlite_db::connect(const char* dbfile, bool read_only, const char* vfs)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		// Closes a previously opened connection &
		// also deallocates the connection handle
		disconnect();

		tu_string file_name = get_full_url(get_player()->get_workdir(), dbfile);
		int rc = sqlite3_open_v2(file_name.c_str(), &m_db, read_only ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE, vfs);
	  if (rc != SQLITE_OK)
		{
			log_error("Can't open sqlite database %s(VFS=%s): %s\n",
				file_name.c_str(), vfs, sqlite3_errmsg(m_db));
			sqlite3_close(m_db);
			return false;
		}

		set_autocommit(true);
		return true;
	}

	bool sqlite_db::runsql(const char* sql)
	{
		if (m_db == NULL)
		{
			log_error("%s\n", sql);
			log_error("missing connection\n");
			return false;
		}

		sqlite3_stmt* stmt;		// Statement handle
		int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, NULL);
	  if (rc != SQLITE_OK)
		{
			if (m_trace)
			{
				log_error("%s\n%s\n", sql, sqlite3_errmsg(m_db));
			}
			return false;
		}

		rc = sqlite3_step(stmt);
		switch (rc)
		{
			case SQLITE_DONE:	// has finished executing
				sqlite3_finalize(stmt);
				return true;

			case SQLITE_ROW:	// has another row ready
				assert(m_result);
				m_result->retrieve_data(stmt);
				sqlite3_finalize(stmt);
				return true;

			default:
				log_error("%s\nerror #%d:%s\n", sql, rc, sqlite3_errmsg(m_db));
				sqlite3_finalize(stmt);
				return false;
		}
		return false;
	}

	int sqlite_db::run(const char *sql)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);
		if (m_trace) log_msg("run: %s\n", sql);

		runsql(sql);
		return sqlite3_changes(m_db);
	}

	sql_table* sqlite_db::open(const char* sql)
	{
		tu_autolock locker(s_sqlite_plugin_mutex);
		if (m_trace) log_msg("open: %s\n", sql);

		m_result = new sqlite_table(get_player());
		runsql(sql);
		return m_result;
	}

	void sqlite_db::set_autocommit(bool autocommit)
	{
		m_autocommit = autocommit;
		commit();
	}

	void sqlite_db::commit()
	{
		tu_autolock locker(s_sqlite_plugin_mutex);

		// The sqlite3_get_autocommit() interface returns non-zero or zero
		// if the given database connection is or is not in autocommit mode, respectively
		// Autocommit mode is disabled by a BEGIN statement
		int rc = sqlite3_get_autocommit(m_db);

		// was a BEGIN statement ?
		if (rc == 0)
		{
			run("commit");
		}

		if (m_autocommit == false)
		{
			run("begin");
		}
	}

	// sqlite callback function
	static void sqlite_func(sqlite3_context *context, int argc, sqlite3_value **argv)
	{
		func_context* ctx = (func_context*) sqlite3_user_data(context);

		// keep alive
		gc_ptr<sqlite_db> this_ptr = ctx->m_this_ptr.get_ptr();
		gc_ptr<as_function> func = ctx->m_func.get_ptr();
		if (this_ptr == NULL || func == NULL)
		{
			return;
		}

		// use _root environment
		character* mroot = this_ptr->get_player()->get_root_movie();
		as_environment* env = mroot->get_environment();
					
		for (int i = 0; i < argc; i++)
		{
			as_value val;
			switch (sqlite3_value_type(argv[i]))
			{
				case SQLITE_INTEGER:
					val.set_double(sqlite3_value_int64(argv[i]));
					break;
				case SQLITE_FLOAT:
					val.set_double(sqlite3_value_double(argv[i]));
					break;
				case SQLITE_NULL:
					val.set_null();
					break;
				case SQLITE3_TEXT:
					val.set_string((const char*) sqlite3_value_text(argv[i]));
					break;
				case SQLITE_BLOB:
// FIXME					val.set_string(sqlite3_value_text(argv[i]));
					break;
				default:
					assert(0);
			}
			env->push(val);
		}

		as_value ret = call_method(func, env, mroot, argc, env->get_top_index());
		env->drop(argc);

		if (ret.is_number())
		{
			sqlite3_result_double(context, ret.to_number());
		}
		else
		{
			// SQLITE_TRANSIENT - makes a copy of the result
			sqlite3_result_text(context, ret.to_string(), -1, SQLITE_TRANSIENT);
		}
	}

	void sqlite_db::create_function(const char* name, as_function* func)
	{
		if (m_db && func && name)
		{
			func_context* ctx = new func_context(this, func);
			int rc = sqlite3_create_function(m_db, name, -1, SQLITE_UTF8, ctx, sqlite_func, NULL, NULL);
			if (rc == SQLITE_OK)
			{
				m_callback_context[name] = ctx;
			}
		}
	}
}

