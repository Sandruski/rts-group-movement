#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "Goal.h"
#include "Entity.h"
#include "DynamicEntity.h"
#include "j1Map.h"
#include "j1PathManager.h"
#include "j1Movement.h"

#include "Brofiler\Brofiler.h"

Goal::Goal(DynamicEntity* owner, GoalType goalType) :owner(owner), goalType(goalType) {}

Goal::~Goal() {}

void Goal::Activate() {}

GoalStatus Goal::Process(float dt) { return goalStatus; }

void Goal::Terminate() {}

void Goal::AddSubgoal(Goal* goal) {}

void Goal::ActivateIfInactive()
{
	if (IsInactive())
		Activate();
}

bool Goal::IsActive() const 
{
	return goalStatus == GoalStatus_Active;
}

bool Goal::IsInactive() const 
{
	return goalStatus == GoalStatus_Inactive;
}

bool Goal::IsCompleted() const 
{
	return goalStatus == GoalStatus_Completed;
}

bool Goal::HasFailed() const 
{
	return goalStatus == GoalStatus_Failed;
}

GoalType Goal::GetType() const 
{
	return goalType;
}

// AtomicGoal ---------------------------------------------------------------------

AtomicGoal::AtomicGoal(DynamicEntity* owner, GoalType goalType) :Goal(owner, goalType) {}

void AtomicGoal::Activate() {}

GoalStatus AtomicGoal::Process(float dt) { return goalStatus; }

void AtomicGoal::Terminate() {}

// CompositeGoal ---------------------------------------------------------------------

CompositeGoal::CompositeGoal(DynamicEntity* owner, GoalType goalType) : Goal(owner, goalType) {}

void CompositeGoal::Activate() {}

GoalStatus CompositeGoal::Process(float dt) { return goalStatus; }

void CompositeGoal::Terminate() {}

void CompositeGoal::AddSubgoal(Goal* goal) 
{
	subgoals.push_front(goal);
}

GoalStatus CompositeGoal::ProcessSubgoals(float dt) 
{
	// Remove all completed and failed goals from the front of the subgoal list
	while (subgoals.size() > 0
		&& (subgoals.front()->IsCompleted() || subgoals.front()->HasFailed())) {
	
		subgoals.front()->Terminate();
		delete subgoals.front();
		subgoals.pop_front();
	}

	// If any subgoals remain, process the one at the front of the list
	if (subgoals.size() > 0) {

		// Grab the status of the frontmost subgoal
		GoalStatus subgoalsStatus = subgoals.front()->Process(dt);

		// SPECIAL CASE: the frontmost subgoal reports 'completed' and the subgoal list
		// contains additional goals. To ensure the parent keeps processing its subgoal list,
		// the 'active' status is returned
		if (subgoalsStatus == GoalStatus_Completed && subgoals.size() > 1)
			return GoalStatus_Active;

		return subgoalsStatus;
	}
	else

		// No more subgoals to process. Return 'completed'
		return GoalStatus_Completed;
}

void CompositeGoal::ReactivateIfFailed()
{
	if (HasFailed())
		Activate();
}

void CompositeGoal::RemoveAllSubgoals() 
{
	for (list<Goal*>::const_iterator it = subgoals.begin(); it != subgoals.end(); ++it) {
		
		(*it)->Terminate();
		delete *it;
	}

	subgoals.clear();
}

// Goal_Think ---------------------------------------------------------------------

Goal_Think::Goal_Think(DynamicEntity* owner) :CompositeGoal(owner, GoalType_Think) {}

void Goal_Think::Activate()
{
	goalStatus = GoalStatus_Active;

	// If this goal is reactivated then there may be some existing subgoals that
	// must be removed
	RemoveAllSubgoals();

	// Initialize the goal
	// TODO: Add some code here

	owner->SetUnitState(UnitState_Idle);
}

GoalStatus Goal_Think::Process(float dt)
{
	// Process the subgoals
	ProcessSubgoals(dt);

	// If any of the subgoals have failed then this goal replans
	ReactivateIfFailed();

	return goalStatus;
}

void Goal_Think::Terminate()
{
	// Switch the goal off
	// TODO: Add some code here

	owner->SetUnitState(UnitState_Idle);
	owner->SetUnitDirection(UnitDirection_NoDirection);
}

void Goal_Think::AddGoal_Wander()
{
	AddSubgoal(new Goal_Wander(owner));
}

void Goal_Think::AddGoal_AttackTarget(Entity* target)
{
	AddSubgoal(new Goal_AttackTarget(owner, target));
}

void Goal_Think::AddGoal_MoveToPosition(iPoint destinationTile) 
{
	AddSubgoal(new Goal_MoveToPosition(owner, destinationTile));
}

// Goal_AttackTarget ---------------------------------------------------------------------

Goal_AttackTarget::Goal_AttackTarget(DynamicEntity* owner, Entity* target) :CompositeGoal(owner, GoalType_AttackTarget), target(target) {}

void Goal_AttackTarget::Activate() 
{
	goalStatus = GoalStatus_Active;

	RemoveAllSubgoals();

	// It is possible for a bot's target to die while this goal is active,
	// so we must test to make sure the bot always has an active target
	if (!owner->IsTargetPresent()) {

		goalStatus = GoalStatus_Completed;
		return;
	}

	AddSubgoal(new Goal_HitTarget(owner, target));

	// If the target is far from the unit, head directly at the target's position
	if (!owner->IsAttackSatisfied()) {
	
		iPoint targetTile = App->map->WorldToMap(target->GetPos().x, target->GetPos().y);
		AddSubgoal(new Goal_MoveToPosition(owner, targetTile));
	}

	owner->SetUnitState(UnitState_AttackTarget);
}

GoalStatus Goal_AttackTarget::Process(float dt)
{
	ActivateIfInactive();

	if (owner->IsAttackSatisfied() && owner->GetSingleUnit()->IsFittingTile() 
		&& subgoals.front()->GetType() == GoalType_MoveToPosition) {

		subgoals.front()->Terminate();
		delete subgoals.front();
		subgoals.pop_front();
	}

	// Process the subgoals
	goalStatus = ProcessSubgoals(dt);

	ReactivateIfFailed();

	return goalStatus;
}

void Goal_AttackTarget::Terminate() 
{
	RemoveAllSubgoals();

	if (target == owner->GetTarget())
		owner->SetTarget(nullptr);

	target = nullptr;

	owner->SetUnitState(UnitState_Idle);
	owner->SetUnitDirection(UnitDirection_NoDirection);
}

// Goal_MoveToPosition ---------------------------------------------------------------------

Goal_MoveToPosition::Goal_MoveToPosition(DynamicEntity* owner, iPoint destinationTile) :CompositeGoal(owner, GoalType_MoveToPosition), destinationTile(destinationTile) {}

void Goal_MoveToPosition::Activate() 
{
	goalStatus = GoalStatus_Active;

	RemoveAllSubgoals();

	if (owner->GetSingleUnit()->goal != destinationTile)

		owner->GetSingleUnit()->SetGoal(destinationTile);

	owner->GetSingleUnit()->GetReadyForNewMove();

	owner->SetUnitState(UnitState_MoveToPosition);

	LOG("MoveToPosition is activated");
}

GoalStatus Goal_MoveToPosition::Process(float dt) 
{
	ActivateIfInactive();

	if (goalStatus == GoalStatus_Failed)
		return goalStatus;

	App->movement->MoveUnit(owner, dt);

	if (owner->GetSingleUnit()->movementState == MovementState_GoalReached) {

		if (owner->GetSingleUnit()->group->isShapedGoal) {

			if (owner->GetSingleUnit()->goal == owner->GetSingleUnit()->shapedGoal)
				goalStatus = GoalStatus_Completed;
		}
		else
			goalStatus = GoalStatus_Completed;
	}

	LOG("MoveToPosition is in progress");

	return goalStatus;
}

void Goal_MoveToPosition::Terminate() 
{
	LOG("MoveToPosition is terminated");

	owner->GetSingleUnit()->ResetUnitParameters();

	owner->SetUnitState(UnitState_Idle);
	owner->SetUnitDirection(UnitDirection_NoDirection);
}

// Goal_HitTarget ---------------------------------------------------------------------

Goal_HitTarget::Goal_HitTarget(DynamicEntity* owner, Entity* target) :AtomicGoal(owner, GoalType_HitTarget), target(target) {}

void Goal_HitTarget::Activate() 
{
	goalStatus = GoalStatus_Active;

	// It is possible for a bot's target to die while this goal is active,
	// so we must test to make sure the bot always has an active target
	if (!owner->IsTargetPresent()) {

		goalStatus = GoalStatus_Completed;
		return;
	}

	owner->SetUnitState(UnitState_HitTarget);

	LOG("HitTarget is terminated");
}

GoalStatus Goal_HitTarget::Process(float dt) 
{
	ActivateIfInactive();

	if (!owner->IsTargetPresent()) {

		goalStatus = GoalStatus_Completed;
		return goalStatus;
	}

	if (((DynamicEntity*)owner)->GetAnimation()->Finished()) {

		owner->GetTarget()->ApplyDamage(owner->GetDamage());
		((DynamicEntity*)owner)->GetAnimation()->Reset();
	}

	LOG("HitTarget is in progress");

	return goalStatus;
}

void Goal_HitTarget::Terminate()
{
	owner->SetUnitState(UnitState_Idle);
	owner->SetUnitDirection(UnitDirection_NoDirection);

	LOG("HitTarget is terminated");
}

// Goal_Wander ---------------------------------------------------------------------

Goal_Wander::Goal_Wander(DynamicEntity* owner) :AtomicGoal(owner, GoalType_Wander) {}

void Goal_Wander::Activate()
{
	goalStatus = GoalStatus_Active;

	// Initialize the goal
	number = 40;
	i = 0;
}

GoalStatus Goal_Wander::Process(float dt)
{
	// If status is inactive, activate it
	ActivateIfInactive();

	if (number == 40)
		LOG("I'm Happy!");
	else
		LOG("I'm sad...");

	i++;
	if (i > 50)
		goalStatus = GoalStatus_Completed;

	return goalStatus;
}

void Goal_Wander::Terminate()
{
	// Switch the goal off
	number = 0;

	if (number == 0)
		LOG("I'm Happy again!");
	else
		LOG("I'm sad...");
}