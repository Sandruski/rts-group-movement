#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "Goal.h"

#include "Brofiler\Brofiler.h"

Goal::Goal(DynamicEntity* owner, GoalType goalType) :owner(owner), goalType(goalType) {}

Goal::~Goal() {}

void Goal::Activate() {}

GoalStatus Goal::Process() { return goalStatus; }

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

GoalStatus AtomicGoal::Process() { return goalStatus; }

void AtomicGoal::Terminate() {}

// CompositeGoal ---------------------------------------------------------------------

CompositeGoal::CompositeGoal(DynamicEntity* owner, GoalType goalType) : Goal(owner, goalType) {}

void CompositeGoal::Activate() {}

GoalStatus CompositeGoal::Process() { return goalStatus; }

void CompositeGoal::Terminate() {}

void CompositeGoal::AddSubgoal(Goal* goal) 
{
	subgoals.push_front(goal);
}

GoalStatus CompositeGoal::ProcessSubgoals() 
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
		GoalStatus subgoalsStatus = subgoals.front()->Process();

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
	// TODO: Add some code here
}

void CompositeGoal::RemoveAllSubgoals() 
{
	for (list<Goal*>::const_iterator it = subgoals.begin(); it != subgoals.end(); ++it) {
		
		(*it)->Terminate();
		delete *it;
	}

	subgoals.clear();
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

GoalStatus Goal_Wander::Process() 
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
}

GoalStatus Goal_Think::Process()
{
	// If status is inactive, set it to active
	//ActivateIfInactive();

	// Process the subgoals
	ProcessSubgoals();

	// If any of the subgoals have failed then this goal replans
	ReactivateIfFailed();

	return goalStatus;
}

void Goal_Think::Terminate()
{
	// Switch the goal off
	// TODO: Add some code here
}

void Goal_Think::AddGoal_Wander()
{
	AddSubgoal(new Goal_Wander(owner));
}