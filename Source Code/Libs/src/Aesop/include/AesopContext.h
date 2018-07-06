/// @file Aesop.h
/// Main file for Aesop open planning library.

#ifndef _AE_CONTEXT_H_
#define _AE_CONTEXT_H_

namespace Aesop {
   /// An interface used to log the planning process.
   /// Designed to be implemented by the end-user in a manner particular to
   /// their application and requirements.
   class Context {
   public:
      /// Record an event taking place. Uses printf-like syntax for now.
      /// @param[in] fmt event format string.
      /// @todo Make this function more useful! Also, provide more fine-grained
      ///       logging. Possibly split over multiple methods.
      virtual void logEvent(const char *fmt, ...) = 0;
   protected:
   private:
   };
};

#endif
