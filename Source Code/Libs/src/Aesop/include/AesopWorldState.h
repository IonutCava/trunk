/// @file Aesop.h
/// Main file for Aesop open planning library.

#ifndef _AE_WORLDSTATE_H_
#define _AE_WORLDSTATE_H_

#include "AesopTypes.h"
#include "AesopAction.h"

#include <string>

namespace Aesop {
   /// Knowledge about a state of the world, current or possible.
   class WorldState {
   public:
      /// Do any of the Facts in this WorldState involve this predicate?
      bool involves(PName pred) const;

      /// Set the value of a Fact.
      void set(const Fact &fact, PVal val = 0);

      /// Unset all knowledge of a Fact.
      void unset(const Fact &fact);

      /// Get the value a Fact is set to.
      bool get(const Fact &fact, PVal &val, PVal def = 0) const;

      /// Do the given Action's pre-conditions match this world state?
      /// @param[in] ac     Action instance to test against this world state.
      /// @param[in] params Parameters to the Action instance if it takes any.
      /// @return True iff the Action is valid under the current world state.
      bool preMatch(const Action &ac, const objects &params) const;

      /// Does the given Action, executed from an arbitrary world state,
      ///        result in this world state?
      /// @param[in]  ac     Action to compare.
      /// @param[out] params Parameters the Action must use for it to result in
      ///                    this world state.
      /// @return True iff the Action results in the current world state.
      bool postMatch(const Action &ac, const objects &params) const;

      /// Apply the given Action to this WorldState in the forwards
      ///        direction.
      /// @param[in] ac     Action to apply to the current state of the world.
      /// @param[in] params Parameters to the Action instance if it takes any.
      void applyForward(const Action &ac, const objects &params);

      /// Remove the effects of the given Action from the world.
      /// @param[in] ac     Action to remove from the current state.
      /// @param[in] params Parameters to the Action instance if it takes any.
      void applyReverse(const Action &ac, const objects &params);

      std::string str() const;

      /// Compare two world states.
      /// @param[in] ws1 First WorldState to compare.
      /// @param[in] ws2 Another WorldState to compare.
      /// @return Number of predicates that differ in value between states.
      static unsigned int comp(const WorldState &ws1, const WorldState &ws2);

      /// Default constructor.
      WorldState();
      /// Default destructor.
      ~WorldState();

      /// Boolean equality test.
      /// This equality test will compare WorldStates based on their hash codes,
      /// providing a faster negative result. If their hash codes are equal, then
      /// WorldState::comp is used to verify.
      bool operator==(const WorldState &s) const
      { return mHash != s.mHash? false: !comp(*this, s); }

      /// Boolean inequality test.
      bool operator!=(const WorldState &s) const
      { return !this->operator==(s); }

   protected:
   private:
      /// Get the predicate name from a world state entry.
      static inline PName getPName(worldrep::const_iterator it)
      { return it->first.name; }
      /// Get the value from a world state entry.
      static inline PVal getPVal(worldrep::const_iterator it)
      { return it->second; }

      /// Internal representation of world state.
      worldrep mState;

      /// Calculated hash value of this state.
      unsigned int mHash;
      /// Update our hash value.
      void updateHash();

      /// Internal method to set the value of a predicate.
      /// @param[in] pred Name of predicate to set.
      /// @param[in] val Value to set the predicate to.
      void _set(const Fact &fact, PVal val);

      /// Internal method to mark that a predicate is unset.
      /// @param[in] pred Name of the predicate to clear.
      void _unset(const Fact &fact);
   };
};

#endif
