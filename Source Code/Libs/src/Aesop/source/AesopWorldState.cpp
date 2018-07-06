/// @file AesopWorldState.cpp
/// Implementation of WorldState class as defined in AesopWorldState.h

#include "AesopWorldState.h"

#include <algorithm>
#include <sstream>

namespace Aesop {
   /// @class WorldState
   ///
   /// This class represents a set of knowledge (facts, or predicates) about
   /// the state of the world that we are planning within. A WorldState can be
   /// used by individual characters as a representation of their knowledge,
   /// but is also used internally in planning.

   WorldState::WorldState()
   {
      mHash = 0;
   }

   WorldState::~WorldState()
   {
   }

   bool WorldState::involves(PName pred) const
   {
      return false;
   }

   void WorldState::set(const Fact &fact, PVal val)
   {
      _set(fact, val);
      updateHash();
   }

   void WorldState::_set(const Fact &fact, PVal val)
   {
      mState[fact] = val;
   }

   void WorldState::unset(const Fact &fact)
   {
      _unset(fact);
      updateHash();
   }

   void WorldState::_unset(const Fact &fact)
   {
      mState.erase(fact);
   }

   bool WorldState::get(const Fact &fact, PVal &val, PVal def) const
   {
      worldrep::const_iterator it = mState.find(fact);
      if(it == mState.end())
      {
         val = def;
         return false;
      }
      val = getPVal(it);
      return true;
   }

   /// Is the given PVal consistent with an Operation of the given condition
   /// and specified value?
   static bool consistent(PVal val, ConditionType cond, PVal cval)
   {
      switch(cond)
      {
      case IsUnset:
         // We have a mapping and we're not supposed to, so we fail.
         return false;
      case Equals:
         // If the value is not what it's supposed to be, fail.
         if(val != cval)
            return false;
         break;
      case NotEqual:
         // If the value is what it's not supposed to be, fail.
         if(val != cval)
            return false;
         break;
      case Less:
         if(val >= cval)
            return false;
         break;
      case Greater:
         if(val <= cval)
            return false;
         break;
      case LessEqual:
         if(val > cval)
            return false;
         break;
      case GreaterEqual:
         if(val < cval)
            return false;
         break;
      }
      return true;
   }

   /// Is the given PVal consistent with following an Operation with the effect
   /// type and value given?
   static bool consistent(PVal val, EffectType eff, PVal eval)
   {
      switch(eff)
      {
      case Set:
         // Fact must be set to the same value.
         return val == eval;
      case Unset:
         // Fact is clearly set, so no.
         return false;
      case Increment:
         // Value must have been incremented, so it should be eval+1
         if(val != eval + 1)
            return false;
         break;
      case Decrement:
         // Value was decremented.
         if(val != eval - 1)
            return false;
         break;
      }
      return true;
   }

   static void fillOp(Operation &op, const objects &params)
   {
      // Check condition index.
      if(op.cidx > -1)
         op.cval = params[op.cidx];
      // Check effect index.
      if(op.eidx > -1)
         op.eval = params[op.eidx];
   }

   static void fillFact(Fact &f, const objects &params)
   {
      // Check whether the Fact needs to be completed.
      unsigned int i = 0;
      for(paramlist::const_iterator p = f.indices.begin(); p != f.indices.end(); p++, i++)
      {
         if(*p != -1)
            f.args[i] = params[*p];
      }
   }

   /// For a 'pre-match' to be valid, we compare the Action's required
   /// predicates to the values in the current world state. All values must
   /// match for the Action to be valid.
   bool WorldState::preMatch(const Action &ac, const objects &params) const
   {
      if(!ac.checkSpecialConditions(params))
         return false;
      operations::const_iterator o;
      Operation op;
      Fact f;
      for(o = ac.begin(); o != ac.end(); o++)
      {
         // If there's no condition, just carry merrily on.
         if(o->second.ctype == NoCondition)
            continue;
         // Copy Operation and Fact for modification.
         op = o->second;
         f = o->first;
         // Check whether we need to set target value based on parameter.
         if(params.size())
         {
            fillOp(op, params);
            fillFact(f, params);
         }
         PVal val;
         if(get(f, val))
         {
            // We have a mapping for this Fact. Check for consistency.
            if(!consistent(val, op.ctype, op.cval))
               return false;
         }
         else
         {
            // No mapping for this Fact. Only escape is if we don't want it to.
            if(op.ctype != IsUnset)
               return false;
         }
      }

      // No inconsistencies, so we pass.
      return true;
   }

   /// This method compares a desired world state with an action's results. The
   /// comparison returns true if each predicate in our current state is either
   /// set by the Action, or required by it and not changed.
   /// In this method, params is an output argument. The method fills in the
   /// values of each parameter required for the Action to result in the given
   /// world state.
   /// @todo This method seems to be giving false positives.
   bool WorldState::postMatch(const Action &ac, const objects &params) const
   {
      if(!ac.checkSpecialConditions(params))
         return false;
      operations::const_iterator o;
      Operation op;
      Fact f;
      int consistencies = 0;
      for(o = ac.begin(); o != ac.end(); o++)
      {
         // Construct a new operation based on the parameters passed.
         op = o->second;
         f = o->first;
         if(params.size())
         {
            fillOp(op, params);
            fillFact(f, params);
         }
         // If there's no effect, look at the conditions.
         if(op.etype == NoEffect)
         {
            // Check that there's actually a condition.
            if(op.ctype != NoCondition)
            {
               PVal val;
               if(get(f, val))
               {
                  // We have a mapping for this Fact. Check for consistency.
                  if(!consistent(val, op.ctype, op.cval))
                     return false;
                  else
                     consistencies++;
               }
               else
               {
                  // No mapping for this Fact. If that's not desired, bail.
                  //if(op.ctype != IsUnset)
                     //return false;
               }
            }
         }
         else
         {
            PVal val;
            if(get(f, val))
            {
               // Check for consistency.
               if(!consistent(val, op.etype, op.eval))
                  return false;
               else
                  consistencies++;
            }
            else
            {
               // No mapping. If that's not what's desired, bail.
               //if(op.etype != Unset)
                  //return false;
            }
         }
      }

      return consistencies > 0;
   }

   /// Apply an Action to the current world state. The Action's effects are
   /// applied to the current set of predicates.
   void WorldState::applyForward(const Action &ac, const objects &params)
   {

      updateHash();
   }

   /// This method applies an Action to a WorldState in reverse. In effect,
   /// it determines the state of the world required that when this Action is
   /// applied to it, the result is the current state.
   /// This involves making sure that the new state's predicates match the
   /// Action's prerequisites, and clearing any predicates that the Action
   /// sets.
   void WorldState::applyReverse(const Action &ac, const objects &params)
   {
      operations::const_iterator o;
      Operation op;
      Fact f;
      for(o = ac.begin(); o != ac.end(); o++)
      {
         op = o->second;
         f = o->first;
         if(params.size())
         {
            fillOp(op, params);
            fillFact(f, params);
         }
         // If there's no condition, check the effects.
         if(op.ctype == NoCondition)
         {
            switch(op.etype)
            {
            case Set:
               _set(f, op.eval);
               break;
            case Unset:
               _unset(f);
               break;
            case Increment:
               _set(f, op.eval - 1);
               break;
            case Decrement:
               _set(f, op.eval + 1);
               break;
            }
         }
         else
         {
            switch(op.ctype)
            {
            case IsSet:
               _set(f, 0);
               break;
            case Equals:
               _set(f, op.cval);
               break;
            case IsUnset:
               _unset(f);
               break;
            }
         }
      }

      updateHash();
   }

   std::string WorldState::str() const
   {
      worldrep::const_iterator it;
      std::string rep = "{\n";
      for(it = mState.begin(); it != mState.end(); it++)
      {
         std::stringstream s;
         s << "    " << it->first << " -> " << it->second;
         rep += s.str() + "\n";
      }
      rep += "}";
      return rep;
   }

   /// This hash method sums the string hashes of all the predicate names in
   /// this state, XORing certain bits based on the values of each predicate.
   /// @todo Is there a better hash func to use? Should do some unit testing.
   void WorldState::updateHash()
   {
      mHash = 0;
      worldrep::const_iterator it;
      for(it = mState.begin(); it != mState.end(); it++)
      {
         //unsigned int l = getPName(it).length();
         //while(l)
         //   mHash = 31 * mHash + getPName(it)[--l];
         //mHash ^= getPVal(it) << getPName(it).length() % (sizeof(unsigned int) - sizeof(PVal));
         mHash = 31 * mHash + (getPVal(it) << getPName(it));
      }
   }

   /// The difference score between two WorldStates is equal to the number of
   /// predicates which they both have defined, but to different values.
   /// Predicates that are not defined in one state, or are flagged as unset
   /// in either, are not considered.
   unsigned int WorldState::comp(const WorldState &ws1, const WorldState &ws2)
   {
      int score = 0;
      return ws1.mState == ws2.mState ? 0 : 1;

      // Iterators run from lowest to highest key values.
      worldrep::const_iterator p1 = ws1.mState.begin();
      worldrep::const_iterator p2 = ws2.mState.begin();

      while(p1 != ws1.mState.end() || p2 != ws2.mState.end())
      {
         // One state may have run out of keys.
         if(p1 == ws1.mState.end())
         {
            score++;
            p2++;
            continue;
         }
         if(p2 == ws2.mState.end())
         {
            score++;
            p1++;
            continue;
         }

         // Compare names of predicates (keys).
         int cmp = getPName(p1) != getPName(p2);
         if(cmp == 0)
         {
            // Names are equal. Check for different values.
            if(getPVal(p1) != getPVal(p2))
               score++;
            p1++;
            p2++;
         }
         else if(cmp > 0)
         {
            // Key 1 is greater.
            score++;
            p2++;
         }
         else // if(cmp < 0)
         {
            // Key 2 is greater.
            score++;
            p1++;
         }
      }

      return score;
   }
};
