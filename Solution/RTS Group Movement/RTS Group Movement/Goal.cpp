#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "Goal.h"
#include "Entity.h"
#include "DynamicEntity.h"
#include "j1Map.h"
#include "j1PathManager.h"
#include "j1Movement.h"
#include "j1EntityFactory.h"

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

// COMPOSITE GOALS
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
}

void Goal_Think::AddGoal_Wander(uint maxDistance)
{
	AddSubgoal(new Goal_Wander(owner, maxDistance));
}

void Goal_Think::AddGoal_AttackTarget(TargetInfo* targetInfo)
{
	AddSubgoal(new Goal_AttackTarget(owner, targetInfo));
}

void Goal_Think::AddGoal_MoveToPosition(iPoint destinationTile) 
{
	AddSubgoal(new Goal_MoveToPosition(owner, destinationTile));
}

void Goal_Think::AddGoal_Patrol(iPoint originTile, iPoint destinationTile)
{
	AddSubgoal(new Goal_Patrol(owner, originTile, destinationTile));
}

// Goal_AttackTarget ---------------------------------------------------------------------

Goal_AttackTarget::Goal_AttackTarget(DynamicEntity* owner, TargetInfo* targetInfo) :CompositeGoal(owner, GoalType_AttackTarget), targetInfo(targetInfo) {}

void Goal_AttackTarget::Activate() 
{
	goalStatus = GoalStatus_Active;

	RemoveAllSubgoals();

	if (targetInfo == nullptr) {

		goalStatus = GoalStatus_Completed;
		return;
	}

	/// The target has been removed from another unit!!!
	if (targetInfo->isRemoved) {
	
		goalStatus = GoalStatus_Completed;
		return;
	}

	// It is possible for a bot's target to die while this goal is active,
	// so we must test to make sure the bot always has an active target
	if (!targetInfo->IsTargetPresent()) {

		goalStatus = GoalStatus_Completed;
		return;
	}

	// The attack is only performed if the sight distance is satisfied
	/*
	else if (!targetInfo->isSightSatisfied) {

		goalStatus = GoalStatus_Failed;
		return;	
	}
	*/

	// -----

	AddSubgoal(new Goal_HitTarget(owner, targetInfo));

	// If the target is far from the unit, head directly at the target's position
	if (!targetInfo->isAttackSatisfied) {
	
		iPoint targetTile = App->map->WorldToMap(targetInfo->target->GetPos().x, targetInfo->target->GetPos().y);
		AddSubgoal(new Goal_MoveToPosition(owner, targetTile));
	}

	/// The target is being attacked by this unit
	targetInfo->target->AddAttackingUnit(owner);

	owner->SetUnitState(UnitState_AttackTarget);
}

GoalStatus Goal_AttackTarget::Process(float dt)
{
	ActivateIfInactive();

	// The unit was chasing their target, but the attack distance has been suddenly satisfied
	if (subgoals.size() > 0) {

		if (owner->GetSingleUnit()->IsFittingTile() 
			&& (targetInfo->isRemoved || (targetInfo->isAttackSatisfied && subgoals.front()->GetType() == GoalType_MoveToPosition))) {

			subgoals.front()->Terminate();
			delete subgoals.front();
			subgoals.pop_front();
		}
	}

	// Process the subgoals
	goalStatus = ProcessSubgoals(dt);

	ReactivateIfFailed();

	return goalStatus;
}

void Goal_AttackTarget::Terminate() 
{
	RemoveAllSubgoals();

	// -----

	if (targetInfo == nullptr)
		return;

	/// The target has been removed from another unit!!!
	if (targetInfo->isRemoved) {

		// Remove the target from this owner
		owner->RemoveTargetInfo(targetInfo);

		// -----

		targetInfo = nullptr;
		owner->SetUnitState(UnitState_Idle);

		return;
	}

	// Remove this attacking unit from the owner's unitsAttacking list
	targetInfo->target->RemoveAttackingUnit(owner);

	// If the target has died, invalidate it from the rest of the units
	if (!targetInfo->IsTargetPresent())

		App->entities->InvalidateAttackEntity(targetInfo->target);

	// If the sight distance is not satisfied, remove the target from the entity targets list
	// !targetInfo->isSightSatisfied ||
	if (!targetInfo->IsTargetPresent())

		// Remove the target from this owner
		owner->RemoveTargetInfo(targetInfo);

	// -----

	targetInfo = nullptr;
	owner->SetUnitState(UnitState_Idle);
}

// Goal_Patrol ---------------------------------------------------------------------

Goal_Patrol::Goal_Patrol(DynamicEntity* owner, iPoint originTile, iPoint destinationTile) :CompositeGoal(owner, GoalType_Patrol), originTile(originTile), destinationTile(destinationTile) {}

void Goal_Patrol::Activate()
{
	goalStatus = GoalStatus_Active;

	RemoveAllSubgoals();

	if (currGoal == destinationTile)
		currGoal = originTile;
	else
		currGoal = destinationTile;

	AddSubgoal(new Goal_MoveToPosition(owner, currGoal));

	owner->SetUnitState(UnitState_Patrol);
}

GoalStatus Goal_Patrol::Process(float dt)
{
	ActivateIfInactive();

	if (goalStatus == GoalStatus_Failed)
		return goalStatus;

	goalStatus = ProcessSubgoals(dt);

	if (goalStatus == GoalStatus_Completed)
		Activate();

	ReactivateIfFailed();

	return goalStatus;
}

void Goal_Patrol::Terminate()
{
	RemoveAllSubgoals();

	owner->SetUnitState(UnitState_Idle);
}

// Goal_Wander ---------------------------------------------------------------------

Goal_Wander::Goal_Wander(DynamicEntity* owner, uint maxDistance) :CompositeGoal(owner, GoalType_Wander), maxDistance(maxDistance) {}

void Goal_Wander::Activate()
{
	maxDistance = 5;
	goalStatus = GoalStatus_Active;

	AddSubgoal(new Goal_LookAround(owner));

	Navgraph* navgraph = owner->GetNavgraph();

	iPoint destinationTile = { -1,-1 };

	int sign = rand() % 2;
	if (sign == 0)
		sign = -1;

	destinationTile.x = owner->GetSingleUnit()->currTile.x + sign * (rand() % (maxDistance + 1) + 1);

	sign = rand() % 2;
	if (sign == 0)
		sign = -1;

	destinationTile.y = owner->GetSingleUnit()->currTile.y + sign * (rand() % (maxDistance + 1) + 1);

	AddSubgoal(new Goal_MoveToPosition(owner, destinationTile));
}

GoalStatus Goal_Wander::Process(float dt)
{
	ActivateIfInactive();

	goalStatus = ProcessSubgoals(dt);

	// Wander is performed in an infinite loop
	if (goalStatus == GoalStatus_Completed)
		Activate();

	if (goalStatus == GoalStatus_Failed) {

		RemoveAllSubgoals();
		Activate();
	}

	return goalStatus;
}

void Goal_Wander::Terminate()
{
	RemoveAllSubgoals();

	maxDistance = 0;

	owner->SetUnitState(UnitState_Idle);
}

// ATOMIC GOALS
// Goal_MoveToPosition ---------------------------------------------------------------------

Goal_MoveToPosition::Goal_MoveToPosition(DynamicEntity* owner, iPoint destinationTile) :AtomicGoal(owner, GoalType_MoveToPosition), destinationTile(destinationTile) {}

void Goal_MoveToPosition::Activate()
{
	goalStatus = GoalStatus_Active;

	owner->SetHitting(false);

	if (!owner->GetNavgraph()->IsWalkable(destinationTile)) {

		goalStatus = GoalStatus_Failed;
		return;
	}

	if (owner->GetSingleUnit()->goal != destinationTile)

		owner->GetSingleUnit()->SetGoal(destinationTile);

	if (!owner->GetSingleUnit()->GetReadyForNewMove()) {

		goalStatus = GoalStatus_Failed;
		return;
	}
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

	return goalStatus;
}

void Goal_MoveToPosition::Terminate() 
{
	owner->GetSingleUnit()->ResetUnitParameters();

	owner->SetIsStill(true);
}

// Goal_HitTarget ---------------------------------------------------------------------

Goal_HitTarget::Goal_HitTarget(DynamicEntity* owner, TargetInfo* targetInfo) :AtomicGoal(owner, GoalType_HitTarget), targetInfo(targetInfo) {}

void Goal_HitTarget::Activate() 
{
	goalStatus = GoalStatus_Active;

	/// The target has been removed from another unit!!!
	if (targetInfo->isRemoved) {

		goalStatus = GoalStatus_Completed;
		return;
	}

	// It is possible for a bot's target to die while this goal is active,
	// so we must test to make sure the bot always has an active target
	if (!targetInfo->IsTargetPresent()) {

		goalStatus = GoalStatus_Completed;
		return;
	}
	else if (!targetInfo->isAttackSatisfied || !targetInfo->isSightSatisfied
		|| !owner->GetSingleUnit()->IsFittingTile()) {

		goalStatus = GoalStatus_Failed;
		return;
	}

	owner->SetHitting(true);
}

GoalStatus Goal_HitTarget::Process(float dt) 
{
	ActivateIfInactive();

	/// The target has been removed from another unit!!!
	if (targetInfo->isRemoved) {

		goalStatus = GoalStatus_Completed;
		return goalStatus;
	}

	if (!targetInfo->IsTargetPresent()) {

		goalStatus = GoalStatus_Completed;
		return goalStatus;
	}
	else if (!targetInfo->isAttackSatisfied || !targetInfo->isSightSatisfied) {

		goalStatus = GoalStatus_Failed;
		return goalStatus;
	}

	if (((DynamicEntity*)owner)->GetAnimation()->Finished()) {

		targetInfo->target->ApplyDamage(owner->GetDamage());
		((DynamicEntity*)owner)->GetAnimation()->Reset();
	}

	return goalStatus;
}

void Goal_HitTarget::Terminate()
{
	owner->SetHitting(false);
}

// Goal_LookAround ---------------------------------------------------------------------

Goal_LookAround::Goal_LookAround(DynamicEntity* owner) :AtomicGoal(owner, GoalType_LookAround) {}

void Goal_LookAround::Activate()
{
	goalStatus = GoalStatus_Active;

	uint random = rand() % 3;

	switch (owner->GetUnitDirection()) {

	case UnitDirection_Up:

		if (random == 0)
			nextOrientation = UnitDirection_UpLeft;
		else if (random == 1)
			nextOrientation = UnitDirection_UpRight;
		else
			nextOrientation = UnitDirection_Up;

		break;

	case UnitDirection_NoDirection:
	case UnitDirection_Down:

		if (random == 0)
			nextOrientation = UnitDirection_DownLeft;
		else if (random == 1)
			nextOrientation = UnitDirection_DownRight;
		else
			nextOrientation = UnitDirection_Down;

		break;

	case UnitDirection_Left:

		if (random == 0)
			nextOrientation = UnitDirection_UpLeft;
		else if (random == 1)
			nextOrientation = UnitDirection_DownLeft;
		else
			nextOrientation = UnitDirection_Left;

		break;

	case UnitDirection_Right:

		if (random == 0)
			nextOrientation = UnitDirection_UpRight;
		else if (random == 1)
			nextOrientation = UnitDirection_DownRight;
		else
			nextOrientation = UnitDirection_Right;

		break;

	case UnitDirection_UpLeft:
		
		if (random == 0)
			nextOrientation = UnitDirection_Up;
		else if (random == 1)
			nextOrientation = UnitDirection_Left;
		else
			nextOrientation = UnitDirection_UpLeft;

		break;

	case UnitDirection_UpRight:

		if (random == 0)
			nextOrientation = UnitDirection_Up;
		else if (random == 1)
			nextOrientation = UnitDirection_Right;
		else
			nextOrientation = UnitDirection_UpRight;

		break;

	case UnitDirection_DownLeft:

		if (random == 0)
			nextOrientation = UnitDirection_Down;
		else if (random == 1)
			nextOrientation = UnitDirection_Left;
		else
			nextOrientation = UnitDirection_DownLeft;

		break;

	case UnitDirection_DownRight:

		if (random == 0)
			nextOrientation = UnitDirection_Down;
		else if (random == 1)
			nextOrientation = UnitDirection_Right;
		else
			nextOrientation = UnitDirection_DownRight;

		break;
	}

	timer.Start();
	secondsToChange = (float)(rand() % 4 + 2);
	secondsUntilNextChange = (float)(rand() % 6 + 2);
}

GoalStatus Goal_LookAround::Process(float dt)
{
	ActivateIfInactive();

	if (!isChanged) {

		if (timer.ReadSec() >= secondsToChange) {

			owner->SetUnitDirection(nextOrientation);

			isChanged = true;
			timer.Start();
		}
	}
	else {
	
		if (timer.ReadSec() >= secondsUntilNextChange) {

			uint random = rand() % 3;

			if (random % 2 == 0)
				goalStatus = GoalStatus_Completed;
			else
				Activate();
		}
	}

	return goalStatus;
}

void Goal_LookAround::Terminate()
{
	secondsToChange = 0.0f;
	secondsUntilNextChange = 0.0f;
	isChanged = false;
}