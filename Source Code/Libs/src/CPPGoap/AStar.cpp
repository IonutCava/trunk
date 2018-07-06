#include "AStar.h"

#include <algorithm>
//#include <iostream>
#include <assert.h>
#include <sstream>

goap::AStar::AStar() {
}

int goap::AStar::calculateHeuristic(const WorldState& now, const WorldState& goal) const {
    return now.distanceTo(goal);
}

void goap::AStar::addToOpenList(Node&& n) {
    // insert maintaining sort order
    auto it = std::lower_bound(open_.begin(),
                               open_.end(),
                               n);
    open_.insert(it, std::move(n));
}

goap::Node& goap::AStar::popAndClose() {
    //if (open_.size() == 0) {
        //throw std::invalid_argument("You cannot call popAndClose on an empty open-list!");
    //}
    assert(!open_.empty() && "You cannot call popAndClose on an empty open-list!");

    closed_.push_back(std::move(open_.front()));
    open_.erase(open_.begin());

    return closed_.back();
}

bool goap::AStar::memberOfClosed(const WorldState& ws) const {
    if ( vectorAlg::find_if( closed_.begin(), closed_.end(), [&]( const Node& n )->bool { return n.ws_ == ws; } ) == closed_.end() ) {
        return false;
    } else {
        return true;
    }
}

vectorImpl<goap::Node>::iterator goap::AStar::memberOfOpen(const WorldState& ws) {
    return vectorAlg::find_if( open_.begin(), open_.end(), [&]( const Node& n )->bool { return n.ws_ == ws; } );
}

bool goap::AStar::plan(const WorldState& start, const WorldState& goal, const vectorImpl<Action*>& actions, vectorImpl<const Action*>& plan) {
    // Feasible we'd re-use a planner, so clear out the prior results
    open_.clear();
    closed_.clear();
    known_nodes_.clear();

    Node starting_node(start, 0, calculateHeuristic(start, goal), 0, nullptr);

    // TODO figure out a more memory-friendly way of doing this...
    known_nodes_[starting_node.id_] = starting_node;
    open_.push_back(std::move(starting_node));

    //int iters = 0;
    while (open_.size() > 0) {
        //++iters;
        //std::cout << "\n-----------------------\n";
        //std::cout << "Iteration " << iters << std::endl;

        // Look for Node with the lowest-F-score on the open list. Switch it to closed,
        // and hang onto it -- this is our latest node.
        Node& current(popAndClose());

        //std::cout << "Open list\n";
        //printOpenList();
        //std::cout << "Closed list\n";
        //printClosedList();
        //std::cout << "\nCurrent is " << current << std::endl;

        // Is our current state the goal state? If so, we've found a path, yay.
        if (current.ws_.meetsGoal(goal)) {
            plan.resize(0);
            do {
                plan.push_back(current.action_);
                current = known_nodes_.at(current.parent_id_);
            } while (current.parent_id_ != 0);
            return true;
        }

        // Check each node REACHABLE from current
        for (const Action* action : actions) {
            if (action->eligibleFor(current.ws_)) {
                WorldState possibility = action->actOn(current.ws_);
                //std::cout << "Hmm, " << action.name() << " could work..." << "resulting in " << possibility << std::endl;

                // Skip if already closed
                if (memberOfClosed(possibility)) {
                    //std::cout << "...but that one's closed out.\n";
                    continue;
                }

                auto needle = memberOfOpen(possibility);
                if (needle==open_.end()) { // not a member of open list
                    // Make a new node, with current as its parent, recording G & H
                    Node found(possibility, current.g_ + action->cost(), calculateHeuristic(possibility, goal), current.id_, action);
                    known_nodes_[found.id_] = found;

                    // Add it to the open list (maintaining sort-order therein)
                    addToOpenList(std::move(found));
                } else { // already a member of the open list
                    // check if the current G is better than the recorded G
                    if ((current.g_ + action->cost()) < needle->g_) {
                        //std::cout << "My path is better\n";
                        needle->parent_id_ = current.id_;                     // make current its parent
                        needle->g_ = current.g_ + action->cost();              // recalc G & H
                        needle->h_ = calculateHeuristic(possibility, goal);
                        std::sort(open_.begin(), open_.end());                // resort open list to account for the new F
                    }
                }
            }
        }
    }

    // If there's nothing left to evaluate, then we have no possible path left
    //throw std::runtime_error("A* planner could not find a path from start to goal");
    return false;
}
