#include "Headers/WarSceneActions.h"

#include "Core/Headers/Console.h"

namespace AI {

    WarSceneGoal::WarSceneGoal(const std::string& name) : GOAPGoal(name),
                                                          _queuedScene(nullptr),
                                                          _relevancyUpdateQueued(false)
    {
    }

    WarSceneGoal::WarSceneGoal(GOAPGoal const & other) : GOAPGoal(other),
                                                         _queuedScene(nullptr),
                                                         _relevancyUpdateQueued(false)
    {
    }

    WarSceneGoal::~WarSceneGoal()
    {
    }

    void WarSceneGoal::update(const U64 deltaTime) {
        if (_relevancyUpdateQueued) {
            // more details here (_queuedScene stuff)
            _relevancy = 1.0f;
            _relevancyUpdateQueued = false;
        }
    }
    
    void WarSceneGoal::evaluateRelevancy(AISceneImpl* const AIScene) {
        _relevancyUpdateQueued = true;
        _queuedScene = AIScene;
    }

    WarSceneAction::WarSceneAction(ActionType type, const std::string& name, F32 cost) : GOAPAction(name, cost)
    {
        _type = type;
    }

    WarSceneAction::~WarSceneAction()
    {
    }

    bool WarSceneAction::preAction() const {
        PRINT_FN("Starting normal action");
        return true;
    }
    bool WarSceneAction::postAction() const {
        PRINT_FN("Normal action over");
        return true;
    }

    WaitAction::WaitAction(std::string name, F32 cost) : WarSceneAction(ACTION_WAIT, name, cost)
    {
    }

    bool WaitAction::preAction() const {
        PRINT_FN("Starting to wait");
        return true;
    }

    bool WaitAction::postAction() const {
        PRINT_FN("Waiting over");
        return true;
    }

    ScoutAction::ScoutAction(std::string name, F32 cost) : WarSceneAction(ACTION_SCOUT, name, cost)
    {
    }

    bool ScoutAction::preAction() const {
        PRINT_FN("Starting to scout");
        return true;
    }

    bool ScoutAction::postAction() const {
        PRINT_FN("Scouting over");
        return true;
    }
 
    ApproachAction::ApproachAction(std::string name, F32 cost) : WarSceneAction(ACTION_APPROACH, name, cost)
    {
    }

    bool ApproachAction::preAction() const {
        PRINT_FN("Starting to approach");
        return true;
    }

    bool ApproachAction::postAction() const {
        PRINT_FN("Approaching over");
        return true;
    }

    TargetAction::TargetAction(std::string name, F32 cost) : WarSceneAction(ACTION_TARGET, name, cost)
    {
    }

    bool TargetAction::preAction() const {
        PRINT_FN("Starting to target");
        return true;
    }

    bool TargetAction::postAction() const {
        PRINT_FN("Targeting over");
        return true;
    }
 
    AttackAction::AttackAction(std::string name, F32 cost) : WarSceneAction(ACTION_ATTACK, name, cost)
    {
    }

    bool AttackAction::preAction() const {
        PRINT_FN("Starting to attack");
        return true;
    }

    bool AttackAction::postAction() const {
        PRINT_FN("Attacking over");
        return true;
    }
    
    RetreatAction::RetreatAction(std::string name, F32 cost) : WarSceneAction(ACTION_RETREAT, name, cost)
    {
    }

    bool RetreatAction::preAction() const {
        PRINT_FN("Starting to retreat");
        return true;
    }

    bool RetreatAction::postAction() const {
        PRINT_FN("Retreating over");
        return true;
    }

    KillAction::KillAction(std::string name, F32 cost) : WarSceneAction(ACTION_KILL, name, cost)
    {
    }

    bool KillAction::preAction() const {
        PRINT_FN("Starting to kill");
        return true;
    }

    bool KillAction::postAction() const {
        PRINT_FN("Killing over. Hallelujah!");
        return true;
    }
};