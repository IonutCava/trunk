/// @file Aesop.h
/// Main file for Aesop open planning library.

#ifndef _AE_ACTION_H_
#define _AE_ACTION_H_

#include "AesopTypes.h"
#include "AesopContext.h"

#include <list>
#include <string>
#include <set>

namespace Aesop {
   /// An atomic change that can be made to the world state.
   class Action {
   public:
      /// 

      /// Add a condition to this Action.
      void condition(const Fact &fact, ConditionType type, PVal val = 0);

      /// Add a parameter condition to this Action.
      void condition(const Fact &fact, unsigned int param, ConditionType type);

      void condition(SpecialConditionType type);

      /// Add an effect to this Action.
      void effect(const Fact &fact, EffectType type, PVal val = 0);

      /// Add a parameter effect to this Action.
      void effect(const Fact &fact, unsigned int param, EffectType type);

      /// Add parameters to the Action.
      void parameters(unsigned int num);

      /// How many parameters do we have?
      unsigned int getNumParams() const { return mNumParams; }

      bool checkSpecialConditions(const objects &params) const;

      /// Get this Action's friendly name.
      /// @return This Action's name.
      const std::string& getName() const { return mName; }

      /// Get the cost of using this Action.
      /// @return This Action's cost.
      float getCost() const { return mCost; }

      std::string str(const objects &params) const;

      operations::const_iterator begin() const { return mOperations.begin(); }
      operations::const_iterator end()   const { return mOperations.end(); }

      /// Default constructor.
      /// @param[in] name   Friendly name for this Action.
      /// @param[in] params The number of variable parameters this Action has.
      /// @param[in] cost   Cost of performing this Action.
      Action(std::string name = "", float cost = 1.0f);

      /// Default destructor.
      ~Action();

   protected:
      /// Number of parameters we operate on.
      unsigned int mNumParams;

   private:
      /// Friendly name of this Action.
      std::string mName;
      /// Cost of using this Action in a plan.
      float mCost;

      /// Encode our conditions and effects as Operations on Facts.
      operations mOperations;

      std::set<SpecialConditionType> mSpecialConditions;
   };

   /// Represents an instance of an Action with a list of defined parameter
   /// values.
   struct ActionEntry {
      /// The Action this entry is an 'instance' of.
      const Action* ac;
      /// Array of parameter values 
      objects params;

      /// Default constructor.
      /// @param[in] a Action this ActionEntry is an instance of.
      ActionEntry()
      {
         ac = NULL;
      }

      bool operator==(const ActionEntry &other) const
      { return ac == other.ac && params == other.params; }
   };

   /// A Plan is a sequence of Actions that take us from one WorldState to
   /// another.
   typedef std::list<ActionEntry> Plan;

   /// An ActionSet is a bunch of Actions that we are allowed to use as well as
   /// multipliers on their cost representing user preferences.
   class ActionSet {
   public:
      /// Redefinition of std::map type as ActionSet.
      typedef std::map<const Action*, float> actionmap;

      /// @name STL
      /// @{
      typedef actionmap::const_iterator const_iterator;
      actionmap::const_iterator begin() const { return mActions.begin(); }
      actionmap::const_iterator end() const { return mActions.end(); }
      /// @}

      /// Add an Action to this set with a given preference multiplier.
      void add(const Action* ac, float pref = 1.0f) { if(pref < 0.0f) pref = 0.0f; mActions[ac] = pref; }
      /// Remove an Action from this set.
      void remove(const Action *ac) { mActions.erase(ac); }
   protected:
   private:
      /// Store a map of Action pointers to preferences.
      actionmap mActions;
   };
};

#endif
