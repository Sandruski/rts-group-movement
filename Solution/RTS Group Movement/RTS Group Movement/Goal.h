#ifndef __GOAL_H__
#define __GOAL_H__

#include "j1Module.h"
#include "p2Point.h"
#include "Defs.h"

#include <queue>
#include <list>
#include <algorithm>
using namespace std;

class DynamicEntity;

enum GoalType {

	GoalType_NoType,
	GoalType_Think,
	GoalType_Wander,
	GoalType_MaxTypes,

	// Composite Goals
	/// Goal_Think

	/// Goal_GetItem
	/// Goal_Patrol

	/// Goal_MoveToPosition
	/// Goal_FollowPath

	/// Goal_HuntTarget
	/// Goal_AttackTarget

	// Atomic Goals
	/// Goal_Wander
};

enum GoalStatus {

	GoalStatus_Inactive, // the goal is waiting to be activated
	GoalStatus_Active, // the goal has been activated and will be processed each update step
	GoalStatus_Completed, // the goal has completed and will be removed on the next update
	GoalStatus_Failed // the goal has failed and will either replan or be removed on the next update
};

class Goal
{
public:

	Goal(DynamicEntity* owner, GoalType goalType);
	~Goal();

	// Contains initialization logic (planning phase of the goal)
	/// It can be called any number of times to replan
	virtual void Activate();

	// It is executed each update step
	virtual GoalStatus Process();

	// Undertakes any necessary tidying up before a goal is exited
	/// It is called just before a goal is destroyed
	virtual void Terminate();

	virtual void AddSubgoal(Goal* goal);

	// -----

	void ActivateIfInactive();

	bool IsActive() const;
	bool IsInactive() const;
	bool IsCompleted() const;
	bool HasFailed() const;

	GoalType GetType() const;

protected:

	DynamicEntity* owner = nullptr;
	GoalStatus goalStatus = GoalStatus_Inactive;
	GoalType goalType = GoalType_NoType;
};

// Atomic Goal: it cannot aggregate child goals
// Composite Goal: it can aggregate child goals

class AtomicGoal :public Goal 
{
public:

	AtomicGoal(DynamicEntity* owner, GoalType goalType);

	virtual void Activate();
	virtual GoalStatus Process();
	virtual void Terminate();
};

class CompositeGoal :public Goal
{
public:

	CompositeGoal(DynamicEntity* owner, GoalType goalType);

	virtual void Activate();
	virtual GoalStatus Process();
	virtual void Terminate();

	// -----

	void AddSubgoal(Goal* goal);

	// It is called each update step to process the subgoals
	// It ensures that all completed and failed goals are removed from the list before
	// processing the next subgoal in line and returning its status
	// If the subgoal list is empty, 'completed' is returned
	GoalStatus ProcessSubgoals();

	void ReactivateIfFailed();

	// Clears the subgoals list
	// It ensures that all subgoals are destroyed cleanly by calling each one's 'Terminate' method
	// before deletion
	void RemoveAllSubgoals();

private:

	list<Goal*> subgoals;
};

// ---------------------------------------------------------------------

class Goal_Wander :public AtomicGoal
{
public:

	Goal_Wander(DynamicEntity* owner);

	void Activate();
	GoalStatus Process();
	void Terminate();

private:

	int number = 0;
	int i = 0;
};

class Goal_Think :public CompositeGoal
{
public:

	Goal_Think(DynamicEntity* owner);

	void Activate();
	GoalStatus Process();
	void Terminate();

	// Arbitrate between available strategies, choosing the most appropriate
	// to be pursued. Calculate the desirability of the strategies
	//void Arbitrate();
	void AddGoal_Wander();
};

#endif //__GOAL_H__