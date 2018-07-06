// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef __TIMERS_H__
#define __TIMERS_H_


#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"

#include "base/tu_timer.h"

namespace gameswf 
{
  
  struct variable {
    tu_string name;
    as_value value;
  };

  class Timer
    {
    public:
      Timer();
      Timer(as_value *obj, int ms);
      Timer(as_value method, int ms);
      ~Timer();
      int setInterval(as_value obj, int ms);
      int setInterval(as_value obj, int ms, as_object *this_ptr, as_environment *env);
      int setInterval(as_value obj, int ms, as_environment *env);
      int setInterval(as_value obj, int ms, SWF::array<variable *> *locals);
      void setInterval(int ms) 
      {
        _interval = ms * 0.000001;
      }

      void clearInterval();
      void start();
      bool expired();
      void setObject(as_object *ao) { _object = ao; }
      as_object *getObject() { return _object; }
      
      // Accessors
      const as_value& getASFunction() { return _function;  }
      as_environment *getASEnvironment() { return _env;  }
      as_object *getASObject() { return _object;  }
      SWF::array<struct variable *> *getLocals() { return _locals;  }
      int getIntervalID()  { return _which;  }
      void add_local(tu_string name, as_value value) {
        struct variable *var = new struct variable;
        var->name = name;
        var->value = value;
        _locals->push_back(var);
      }
      

    private:
      tu_string       _method;
      int             _which;                // Which timer
      double          _interval;
      double          _start;
      as_value        _function;
      as_object      *_object;
      as_environment *_env;
      SWF::array<struct variable *> *_locals;
      
    };
  
  struct timer_as_object : public gameswf::as_object
  {
    Timer obj;
  };
  
  void timer_setinterval(const fn_call& fn);
  void timer_clearinterval(const fn_call& fn);
  void timer_expire(const fn_call& fn);
  
} // end of namespace gameswf

  // __TIMERS_H_
#endif
