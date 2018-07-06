/// @file Aesop.h
/// Main file for Aesop open planning library.

#ifndef _AE_TYPES_H_
#define _AE_TYPES_H_

#include "AesopConfig.h"

#include <map>
#include <vector>
#include <cstdarg>
#include <iostream>

namespace Aesop {
   /// @addtogroup Aesop
   /// @{

   /// A unique identifier for a predicate.
   typedef unsigned int PName;
   /// The value predicate parameters are allowed to take on.
   typedef unsigned int Object;
   const Object NullObject(0);
   /// A list of parameter values.
   typedef std::vector<int> paramlist;
   /// A list of objects.
   typedef std::vector<Object> objects;
   /// A list of object combinations.
   typedef std::vector<objects> paramset;

   struct Parameter {
      int index;
      Parameter(int i) : index(i) {}
   };

   /// A combination of a predicate and its parameters.
   struct Fact {
      /// Predicate identifier that this fact refers to.
      PName name;
      /// Arguments of this fact, a list of objects.
      objects args;
      /// Parameter indices for this Fact, to be filled in later.
      paramlist indices;

      /// Default constructor.
      Fact(Aesop::PName n = 0) : name(n) {}

      /// Compare Facts based on their predicate ID.
      bool operator<(const Fact &other) const
      {
         if(name < other.name)
            return true;
         else if(other.name < name)
            return false;
         return args < other.args;
      }

      /// Equality is on predicate and parameters.
      bool operator==(const Fact &other) const
      { return name == other.name && args == other.args; }

      /// Use Fact(pred) % obj1 % obj2 % ...; to create a Fact with fixed
      /// parameters.
      Fact &operator%(const Object &obj)
      {
         // Add an element to our parameters.
         args.push_back(obj);
         // Add an entry to or arguments that does not allow this parameter to be filled.
         indices.push_back(-1);
         return *this;
      }
      /// Use Fact(pred) % Parameter(0) % Parameter(1) % ...; to create a Fact
      /// with variable parameters.
      Fact &operator%(const Parameter &p)
      {
         // Add parameter index to our list.
         indices.push_back(p.index);
         // Dummy object in this slot.
         args.push_back(NullObject);
         return *this;
      }

      friend std::ostream &operator<<(std::ostream &stream, const Fact &f)
      {
         stream << f.name;
         if(f.args.size())
         {
            stream << "(";
            for(unsigned int i = 0; i < f.args.size(); i++)
            {
               stream << f.args[i];
               if(i < f.args.size() - 1)
                  stream << ", ";
            }
            /*for(unsigned int i = 0; i < f.indices.size(); i++)
            {
               stream << f.indices[i];
               if(i < f.indices.size() - 1)
                  stream << ", ";
            }*/
            stream << ")";
         }
         return stream;
      }
   };

   /// Value that a Fact can be mapped to in a WorldState.
   typedef unsigned char PVal;

   /// We represent the world as a series of Fact -> PVal associations.
   typedef std::map<Fact, PVal> worldrep;

   /// Types of requirements an Action can place on a Fact.
   enum ConditionType {
      NoCondition,
      IsSet,        ///< Value does not matter as long as the Fact is set to something.
      IsUnset,      ///< Fact may not be set at all.
      Equals,       ///< Fact must be equal to the given value.
      NotEqual,     ///< Fact must not be equal to the given value.
      Less,         ///< Fact must be less than the given value.
      Greater,      ///< Fact must be greater than the given value.
      LessEqual,    ///< Fact can be less than or equal to the given value.
      GreaterEqual, ///< Fact must be greater than or equal to the given value.
   };

   enum SpecialConditionType {
      ArgsNotEqual, ///< Arguments passed to the Action must not be equal.
   };

   /// Types of effects Actions can have.
   enum EffectType {
      NoEffect,
      Set,       ///< The Action sets the Fact to a given value.
      Unset,     ///< The Action unsets knowledge of the Fact.
      Increment, ///< The Action increments the value the Fact is set to.
      Decrement, ///< The Action decrements the value the Fact is set to.
   };

   /// Stores conditions and effects for a single Fact. Each condition and
   /// effect may be one of several types, and may either operate with a
   /// constant value (cval/eval) or a value given by a parameter to the Action
   /// (which parameter is specified by cidx/eidx).
   struct Operation {
      ConditionType ctype; ///< Type of condition.
      PVal cval;           ///< Value to validate condition with.
      int cidx;            ///< Index of parameter to compare with.

      EffectType etype; ///< Type of effect.
      PVal eval;        ///< Value for effect to use.
      int eidx;         ///< Index of parameter for effect to use.
      
      Operation()
      {
         ctype = NoCondition;
         etype = NoEffect;
         cval = eval = 0;
         cidx = eidx = -1;
      }
   };

   /// Map Facts to the Operations upon them.
   typedef std::map<Fact, Operation> operations;

   /// @}
};

#endif
