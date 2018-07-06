/// @file AesopAction.cpp
/// Implementation of Action class as defined in AesopAction.h

#include "AesopAction.h"
#include <sstream>

namespace Aesop {
   /// @class Action
   ///
   /// Based on the STRIPS concept of an action, an Action represents an atomic
   /// change we can make to the world, and is the building block of all plans
   /// made with Aesop.
   /// An Action is essentially a change to the world state. To perform an
   /// Action, the world must be in a certain state. After the Action is
   /// performed, certain changes will be made to that world state.

   Action::Action(std::string name, float cost)
   {
      mName = name;
      if(cost < 0.0f)
         cost = 0.0f;
      mCost = cost;
      mNumParams = 0;
   }

   Action::~Action()
   {
   }

   void Action::condition(const Fact &fact, ConditionType type, PVal val)
   {
      Operation &op = mOperations[fact];
      op.ctype = type;
      op.cval = val;
      op.cidx = -1;
   }

   void Action::condition(const Fact &fact, unsigned int param, ConditionType type)
   {
      Operation &op = mOperations[fact];
      op.ctype = type;
      op.cval = 0;
      op.cidx = param;
   }

   void Action::condition(SpecialConditionType type)
   {
      mSpecialConditions.insert(type);
   }

   bool Action::checkSpecialConditions(const objects &params) const
   {
      std::set<SpecialConditionType>::const_iterator it;
      for(it = mSpecialConditions.begin(); it != mSpecialConditions.end(); it++)
      {
         switch(*it)
         {
         case ArgsNotEqual:
            if(params.size() > 1)
            {
               for(unsigned int i = 0; i < params.size() - 1; i++)
               {
                  if(params[i] == params[i+1])
                     return false;
               }
            }
            break;
         }
      }
      return true;
   }

   void Action::effect(const Fact &fact, EffectType type, PVal val)
   {
      Operation &op = mOperations[fact];
      op.etype = type;
      op.eval = val;
      op.eidx = -1;
   }

   void Action::effect(const Fact &fact, unsigned int param, EffectType type)
   {
      Operation &op = mOperations[fact];
      op.etype = type;
      op.eval = 0;
      op.eidx = param;
   }

   void Action::parameters(unsigned int num)
   {
      mNumParams = num;
   }

   std::string Action::str(const objects &params) const
   {
      std::string rep = "(";
      rep += getName();
      objects::const_iterator it;
      for(it = params.begin(); it != params.end(); it++)
      {
         std::stringstream s;
         s << (char)*it;
         rep += " " + s.str();
      }
      rep += ")";
      return rep;
   }
};
