#ifndef __GOAL_H__
#define __GOAL_H__

#include "j1Module.h"
#include "p2Point.h"
#include "Defs.h"

#include <queue>
#include <list>
#include <algorithm>
using namespace std;

class Entity;
class DynamicEntity;

enum GoalType {

	GoalType_NoType,
	
	// Composite Goals
	GoalType_Think,
	GoalType_AttackTarget,
	GoalType_Wander,
	GoalType_Patrol,

	// Atomic Goals
	GoalType_MoveToPosition,
	GoalType_HitTarget,

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
	virtual GoalStatus Process(float dt);

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
	virtual GoalStatus Process(float dt);
	virtual void Terminate();
};

class CompositeGoal :public Goal
{
public:

	CompositeGoal(DynamicEntity* owner, GoalType goalType);

	virtual void Activate();
	virtual GoalStatus Process(float dt);
	virtual void Terminate();

	// -----

	void AddSubgoal(Goal* goal);

	// It is called each update step to process the subgoals
	// It ensures that all completed and failed goals are removed from the list before
	// processing the next subgoal in line and returning its status
	// If the subgoal list is empty, 'completed' is returned
	GoalStatus ProcessSubgoals(float dt);

	void ReactivateIfFailed();

	// Clears the subgoals list
	// It ensures that all subgoals are destroyed cleanly by calling each one's 'Terminate' method
	// before deletion
	void RemoveAllSubgoals();

protected:

	list<Goal*> subgoals;
};

// ---------------------------------------------------------------------

class Goal_Wander :public AtomicGoal
{
public:

	Goal_Wander(DynamicEntity* owner);

	void Activate();
	GoalStatus Process(float dt);
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
	GoalStatus Process(float dt);
	void Terminate();

	// Arbitrate between available strategies, choosing the most appropriate
	// to be pursued. Calculate the desirability of the strategies
	//void Arbitrate();
	void AddGoal_Wander();
	void AddGoal_AttackTarget(Entity* target);
	void AddGoal_MoveToPosition(iPoint destinationTile);
	void AddGoal_Patrol(iPoint originTile, iPoint destinationTile);
};

class Goal_AttackTarget :public CompositeGoal
{
public:

	Goal_AttackTarget(DynamicEntity* owner, Entity* target);

	void Activate();
	GoalStatus Process(float dt);
	void Terminate();

private:

	Entity* target = nullptr;
};

class Goal_MoveToPosition :public AtomicGoal
{
public:

	Goal_MoveToPosition(DynamicEntity* owner, iPoint destinationTile);

	void Activate();
	GoalStatus Process(float dt);
	void Terminate();

private:

	iPoint destinationTile = { -1,-1 }; // the position the bot wants to reach
};

class Goal_Patrol :public CompositeGoal
{
public:

	Goal_Patrol(DynamicEntity* owner, iPoint originTile, iPoint destinationTile);

	void Activate();
	GoalStatus Process(float dt);
	void Terminate();

private:

	iPoint originTile = { -1,-1 }; // the position from which the bot starts
	iPoint destinationTile = { -1,-1 }; // the position the bot wants to reach

	iPoint currGoal = { -1,-1 };
};

class Goal_HitTarget :public AtomicGoal
{
public:

	Goal_HitTarget(DynamicEntity* owner, Entity* target);

	void Activate();
	GoalStatus Process(float dt);
	void Terminate();

private:

	Entity* target = nullptr;
};

#endif //__GOAL_H__