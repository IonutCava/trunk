// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation of ActionScript String class.


#include "base/tu_config.h"
#include "gameswf_string.h"
#include "gameswf_log.h"
#include "base/smart_ptr.h"


namespace gameswf
{  
	void string_method(const fn_call& fn, const tu_stringi& method_name, const tu_string& this_string)
	// Executes the string method named by method_name.
	{
		if (method_name == "charCodeAt")
	{
			int	index = (int) fn.arg(0).to_number();
			if (index >= 0 && index < this_string.utf8_length())
	{
				fn.result->set_double(this_string.utf8_char_at(index));
				return;
	}

			fn.result->set_double(0);	// FIXME: according to docs, we're supposed to return "NaN"
			return;
	}
		else if (method_name == "charAt")
	{
			int	index = (int) fn.arg(0).to_number();
			if (index >= 0 && index < this_string.utf8_length())
	{
				tu_string result;
				result += this_string.utf8_char_at(index);
				fn.result->set_tu_string(result);
	}

			fn.result->set_string("");
			return;
	}
		else if (method_name == "fromCharCode")
	{
			// Takes a variable number of args.  Each arg
			// is a numeric character code.  Construct the
			// string from the character codes.

			tu_string result;

			for (int i = 0; i < fn.nargs; i++)
	{
				uint32 c = (uint32) fn.arg(i).to_number();
				result.append_wide_char(c);
	}

			fn.result->set_tu_string(result);
			return;
		}
		else if (method_name == "toUpperCase")
	{
			fn.result->set_tu_string(this_string.utf8_to_upper());
			return;
	}
		else if (method_name == "toLowerCase")
	{
			fn.result->set_tu_string(this_string.utf8_to_lower());
			return;
	}
		else if (method_name == "indexOf")
		{
			if (fn.nargs < 1)
	{
				fn.result->set_double(-1);
				return;
	}
			else
			{
				int	start_index = 0;
				if (fn.nargs > 1)
	{
					start_index = (int) fn.arg(1).to_number();
	}
				const char*	str = this_string.c_str();
				const char*	p = strstr(
					str + start_index,	// FIXME: not UTF-8 correct!
					fn.arg(0).to_string());
				if (p == NULL)
	{
					fn.result->set_double(-1);
					return;
	}

				fn.result->set_double(tu_string::utf8_char_count(str, p - str));
				return;
			}
		}
		else if (method_name == "substring")
		{
			// Pull a slice out of this_string.
			int	start = 0;
			int	utf8_len = this_string.utf8_length();
			int	end = utf8_len;
			if (fn.nargs >= 1)
	{
				start = (int) fn.arg(0).to_number();
				start = iclamp(start, 0, utf8_len);
	}
			if (fn.nargs >= 2)
	{
				end = (int) fn.arg(1).to_number();
				end = iclamp(end, 0, utf8_len);
	}

			if (end < start) swap(&start, &end);	// dumb, but that's what the docs say
			assert(end >= start);

			fn.result->set_tu_string(this_string.utf8_substring(start, end));
			return;
	}
		// concat()
		// lastIndexOf()
		// length property
		// slice()
		// split()
		// substr()

		fn.result->set_undefined();
	}



	struct tu_string_as_object : public gameswf::as_object
	{
		tu_string m_string;
	};
  

	void string_last_index_of(const fn_call& fn)
	{
		tu_string_as_object* this_string_ptr = (tu_string_as_object*) fn.this_ptr;
		assert(this_string_ptr);

		// tulrich: Ugh!  The caller has done hash.get() on
		// the method name to find the method, and now we're
		// going to construct a new method name and run it
		// through a big if-else.
		//
		// TODO do this more efficiently.
		string_method(fn, tu_stringi("lastIndexOf"), this_string_ptr->m_string);
	}

	void string_from_char_code(const fn_call& fn)
	{
		tu_string_as_object* this_string_ptr = (tu_string_as_object*) fn.this_ptr;
		assert(this_string_ptr);

		// tulrich: Ugh!  The caller has done hash.get() on
		// the method name to find the method, and now we're
		// going to construct a new method name and run it
		// through a big if-else.
		//
		// TODO do this more efficiently.
		string_method(fn, tu_stringi("fromCharCode"), this_string_ptr->m_string);
	}


	void string_char_code_at(const fn_call& fn)
	{
		tu_string_as_object* this_string_ptr = (tu_string_as_object*) fn.this_ptr;
		assert(this_string_ptr);

		// tulrich: Ugh!  The caller has done hash.get() on
		// the method name to find the method, and now we're
		// going to construct a new method name and run it
		// through a big if-else.
		//
		// TODO do this more efficiently.
		string_method(fn, tu_stringi("charCodeAt"), this_string_ptr->m_string);
	}


	void string_to_string(const fn_call& fn)
	{
		tu_string_as_object* this_string_ptr = (tu_string_as_object*) fn.this_ptr;
		assert(this_string_ptr);

		fn.result->set_tu_string(this_string_ptr->m_string);
	}


	void string_ctor(const fn_call& fn)
	{
		smart_ptr<tu_string_as_object> str = new tu_string_as_object;

		if (fn.nargs > 0)
	{
			str->m_string = fn.arg(0).to_tu_string();
	}

		// TODO fill in the rest
		str->set_member("toString", &string_to_string);
		str->set_member("fromCharCode", &string_from_char_code);
		str->set_member("charCodeAt", &string_char_code_at);
		str->set_member("lastIndexOf", &string_last_index_of);
    
		fn.result->set_as_object_interface(str.get_ptr());
	}

} // namespace gameswf
