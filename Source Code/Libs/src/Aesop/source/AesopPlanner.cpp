/// @file AesopPlanner.cpp
/// Implementation of Planner class as defined in AesopPlanner.h

#include "AesopPlanner.h"

#include <functional>
#include <algorithm>
#include <vector>
#include <cmath>

namespace Aesop {
   /// @class Planner
   ///
   /// A Planner object actually performs plan queries on the world state.
   /// It represents an entire planning state, with its own start and end
   /// states and plan-specific data.
   /// This will include, among other things, a set of vetoed Actions (for
   /// example, Actions that we tried but failed in practis, and we now
   /// want to exclude from our planning process temporarily).

   Planner::Planner(const WorldState *start, const WorldState *goal, const WorldState *con, const ActionSet *set)
   {
      setStart(start);
      setGoal(goal);
      setActions(set);
      setConstants(con);
      mSuccess = false;
      mId = 0;
   }

   Planner::Planner()
   {
      Planner(NULL, NULL, NULL, NULL);
   }

   Planner::~Planner()
   {
   }

   void Planner::setStart(const WorldState *start)
   {
      mStart = start;
   }

   void Planner::setGoal(const WorldState *goal)
   {
      mGoal = goal;
   }

   void Planner::setConstants(const WorldState *con)
   {
      mConstants = con;
   }

   void Planner::setActions(const ActionSet *set)
   {
      mActions = set;
   }

   const Plan& Planner::getPlan() const
   {
      return mPlan;
   }

   /// This method is actually just a wrapper for a series of calls to the
   /// sliced planning methods.
   bool Planner::plan(Context *ctx)
   {
      // Try to start planning.
      if(!initSlicedPlan(ctx))
         return false;

      while(updateSlicedPlan(ctx)) ;

      finaliseSlicedPlan(ctx);

      return success();
   }

   bool Planner::initSlicedPlan(Context *ctx)
   {
      // Validate pointers.
      if(!mStart || !mGoal || !mActions)
      {
         if(ctx) ctx->logEvent("Planning failed due to unset start, goal or action set!");
         return false;
      }

      if(ctx) ctx->logEvent("Starting new plan.");

      // Reset intermediate data.
      mSuccess = false;
      mOpenList.clear();
      mClosedList.clear();
      mId = 0;

      // Push initial state onto the open list.
      mOpenList.push_back(IntermediateState());
      mOpenList.back().state = *mGoal;
      mOpenList.back().ID = mId++;

      return true;
   }

   void Planner::finaliseSlicedPlan(Context *ctx)
   {
      if(ctx) ctx->logEvent("Finalising plan!");
      // Work backwards up the closed list to get the final plan.
      mPlan.clear();
      if(success())
      {
         unsigned int i = mClosedList.size() - 1;
         while(i)
         {
            // Extract the Action performed at this step.
            mPlan.push_back(ActionEntry());
            mPlan.back().ac = mClosedList[i].ac;
            mPlan.back().params = mClosedList[i].params;
            // Iterate.
            i = mClosedList[i].prev;
         }
      }
      // Purge intermediate results.
      mOpenList.clear();
      mClosedList.clear();
   }

   bool Planner::updateSlicedPlan(Context *ctx)
   {
      // Main loop of A* search.
      if(!mOpenList.empty())
      {
         // Remove best IntermediateState from open list.
         pop_heap(mOpenList.begin(), mOpenList.end(), std::greater<IntermediateState>());
         IntermediateState s = mOpenList.back();
         mOpenList.pop_back();

         if(ctx) ctx->logEvent("Moving state %d from open to closed.", s.ID);

         // Add to closed list.
         mClosedList.push_back(s);

         // Check for completeness.
         if(s.state == *mStart)
         {
            mSuccess = true;
            return false;
         }

         // Find all actions we can use that may result in the current state.
         ActionSet::const_iterator it;
         for(it = mActions->begin(); it != mActions->end(); it++)
         {
            const Action *ac = it->first;
            if(!ac)
               continue;
            paramset params;
            // Get number of params and create a set of paramlists.
            unsigned int nparams = ac->getNumParams();
            if(nparams && mObjects.size())
            {
               // Permute defined objects to feed as parameters.
               unsigned int permutations = (unsigned int)pow((float)mObjects.size(), (float)nparams);
               // Number of argument permutations we can make with our objects.
               params.resize(permutations);
               // Keeps track of the current 
               std::vector<unsigned int> objs(nparams, 0);
               for(unsigned int i = 0; i < permutations; i++)
               {
                  // Number of arguments in this permutation.
                  params[i].resize(nparams);
                  // Copy objects into permutation.
                  unsigned int j;
                  for(j = 0; j < nparams; j++)
                     params[i][j] = mObjects[objs[j]];
                  // Increment and overflow.
                  unsigned int obj = ++objs[--j];
                  while(obj == mObjects.size() && j > 0)
                  {
                     objs[j] = 0;
                     j--;
                     objs[j]++;
                  }
               }
               // Loop on the parameter set and try all permutations.
               paramset::iterator pit;
               for(pit = params.begin(); pit != params.end(); pit++)
                  attemptIntermediate(ctx, s, *ac, it->second, *pit);
            }
            else
            {
               objects temp;
               attemptIntermediate(ctx, s, *ac, it->second, temp);
            }
         }
      }
      else
         return false;

      return true;
   }

   void Planner::attemptIntermediate(Context *ctx, IntermediateState &s, const Action &ac, float pref, objects &plist)
   {
      if(!s.state.postMatch(ac, plist))
         return;

      IntermediateState n;
      // Copy the current state, then apply the Action to it in reverse to get
      // the previous state.
      n.state = s.state;
      n.state.applyReverse(ac, plist);

      closedlist::const_iterator cli;
      // Check to see if the world state is in the closed list.
      bool found = false;
      for(cli = mClosedList.begin(); cli != mClosedList.end(); cli++)
      {
         if(n.state == cli->state)
         {
            found = true;
            break;
         }
      }
      if(found)
         return;

      // H (heuristic) cost is the estimated number of Actions to get from new
      // state to start.
      n.H = (float)WorldState::comp(n.state, *mStart);
      // G cost is the total weight of all Actions we've taken to get to this
      // state. By default, the cost of an Action is 1.
      n.G = s.G + ac.getCost() * pref;
      // Save this to avoid recalculating every time.
      n.F = n.G + n.H;
      // Remember Action we used to to this state.
      n.ac = &ac;
      n.params = plist;
      // Predecessor is the last state to be added to the closed list.
      n.prev = mClosedList.size() - 1;

      openlist::iterator oli;
      // Check to see if the world state is already in the open list.
      for(oli = mOpenList.begin(); oli != mOpenList.end(); oli++)
      {
         if(n.state == oli->state)
         {
            if(n < *oli)
            {
               // We've found a more efficient way of getting here.
               *oli = n;
               // Reorder the heap.
               make_heap(mOpenList.begin(), mOpenList.end(),
                  std::greater<IntermediateState>());

               if(ctx) ctx->logEvent("Updating state %d to F=%f",
                  oli->ID, oli->G + oli->H);
            }
            break;
         }
      }
      // No match found in open list.
      if(oli == mOpenList.end())
      {
         // Give the state an ID.
         n.ID = mId++;
         // Add the new intermediate state to the open list.
         mOpenList.push_back(n);
         // Heapify open list.
         push_heap(mOpenList.begin(), mOpenList.end(), std::greater<IntermediateState>());

         if(ctx) ctx->logEvent("Pushing new state %d %s via action %s onto open list with score F=%.3f.",
            n.ID, n.state.str().c_str(), ac.str(n.params).c_str(), n.G + n.H);
      }
   }
};
