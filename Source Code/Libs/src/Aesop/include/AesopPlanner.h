/// @file AesopPlanner.h
/// Defines Planner class.

#ifndef _AE_PLANNER_H_
#define _AE_PLANNER_H_

#include "AesopTypes.h"
#include "AesopWorldState.h"
#include "AesopContext.h"

namespace Aesop {
   /// A context in which we can make plans.
   class Planner {
   public:
      /// Set our starting WorldState.
      /// @param[in] start Pointer to a WorldState.
      void setStart(const WorldState *start);

      /// Set our goal state.
      /// @param[in] goal Pointer to a WorldState.
      void setGoal(const WorldState *goal);

      /// Set a WorldState representing constants for our problem.
      /// @param[in] con Pointer to a WorldState.
      void setConstants(const WorldState *con);

      /// Create a plan.
      /// @param[in] ctx Context object to record the Planner's activity.
      /// @return True if the plan was successfully calculated, false if no
      ///         plan exists or something went wrong in the planning process.
      bool plan(Context *ctx = NULL);

      /// Start a sliced plan.
      /// @param[in] ctx Context object to record the Planner's activity.
      /// @return True if the plan was successfully initialised, false if
      ///         something went wrong in initialisation.
      bool initSlicedPlan(Context *ctx = NULL);

      /// Update a sliced plan.
      /// @param[in] ctx Context object to record the Planner's activity.
      /// @return False if planning should stop, true if it should continue.
      bool updateSlicedPlan(Context *ctx = NULL);

      /// Output the result of a computed plan to 
      /// @param[in] ctx Context object to record the Planner's activity.
      void finaliseSlicedPlan(Context *ctx = NULL);

      /// US English spelling of finaliseSlicedPlan. 'Cause I'm a nice guy.
      /// @see Planner::finaliseSlicedPlan
      inline void finalizeSlicedPlan(Context *ctx = NULL)
      { finaliseSlicedPlan(ctx); }

      /// Did we plan successfully?
      /// @return True iff a valid plan was found.
      bool success() const { return mSuccess; }

      /// Get the currently constructed plan.
      /// @return A Plan.
      const Plan& getPlan() const;

      /// Set the ActionSet we can use.
      /// @param[in] set The ActionSet to pull from.
      void setActions(const ActionSet *set);

      /// Add an object.
      void addObject(Object obj) { mObjects.push_back(obj); }

      /// Value constructor.
      /// @param[in] start Starting world state.
      /// @param[in] goal  Target world state.
      /// @param[in] goal  Constants.
      /// @param[in] set   ActionSet that defines the Actions we may perform.
      Planner(const WorldState *start, const WorldState *goal, const WorldState *con, const ActionSet *set);

      /// Default constructor.
      Planner();
      /// Default destructor.
      ~Planner();

   protected:

   private:
      /// A WorldState instance used during planning.
      struct IntermediateState {
         /// ID number of this IntermediateState within the current plan.
         /// Not really used, except to identify states for debugging purposes.
         unsigned int ID;
         /// State of the world at this step.
         WorldState state;
         /// Current cost to get to this state from starting state.
         float G;
         /// Guess at cost to get from this state to goal.
         float H;
         /// The sum of G and H.
         float F;
         /// IntermediateState leading to this one.
         unsigned int prev;
         /// Action leading to this one.
         const Action *ac;
         /// Parameters to pass to our Action.
         objects params;

         /// Default constructor.
         IntermediateState()
         {
            G = H = F = 0.0f;
            prev = 0;
            ac = NULL;
            ID = 0;
         }

         /// Compare based on F score.
         bool operator>(const IntermediateState s) const
         { return F > s.F; }

         /// Compare based on F score.
         bool operator<(const IntermediateState s) const
         { return F < s.F; }

         /// Equality is based on the state represented, not auxiliary
         ///        data.
         bool operator==(const IntermediateState &s) const
         { return state == s.state; }
      };
      typedef std::vector<IntermediateState> openlist;
      typedef std::vector<IntermediateState> closedlist;

      /// Starting state.
      /// Not allowed to modify this.
      const WorldState *mStart;
      /// Goal state.
      /// Not allowed to modify this.
      const WorldState *mGoal;
      /// Constants.
      /// Not allowed to modify this.
      const WorldState *mConstants;
      /// Objects we're working with.
      objects mObjects;
      /// A* algorithm open list.
      openlist mOpenList;
      /// A* algorithm closed list.
      closedlist mClosedList;
      /// Did we find a valid plan?
      bool mSuccess;
      /// IntermediateState ID number for debug purposes.
      /// @todo Find a better way to identify states!
      unsigned int mId;
      /// Current plan to get from mStart to mGoal.
      Plan mPlan;
      /// Set of Actions we are allowed to perform.
      const ActionSet *mActions;

      /// Internal function used by pathfinding.
      void attemptIntermediate(Context *ctx, IntermediateState &s, const Action &ac, float pref, objects &plist);
   };
};

#endif
