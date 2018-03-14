#ifndef __j1MOVEMENT_H__
#define __j1MOVEMENT_H__

#include "j1Module.h"
#include "p2Point.h"

#include "j1Pathfinding.h"
#include "j1EntityFactory.h"

#include <list>
#include <vector>
using namespace std;

class Entity;

enum MovementState {
	MovementState_NoState,
	MovementState_WaitForPath,
	MovementState_FollowPath,
	MovementState_GoalReached,
	MovementState_IncreaseWaypoint
};

enum CollisionType {
	CollisionType_NoCollision,
	CollisionType_SameCell,
	CollisionType_TowardsCell,
	CollisionType_ItsCell,
	CollisionType_DiagonalCrossing
};

// forward declaration
struct SingleUnit;
struct UnitGroup;

class j1Movement : public j1Module
{
public:

	j1Movement();

	// Destructor
	~j1Movement();

	bool Update(float dt);

	void DebugDraw() const;

	// Called before quitting
	bool CleanUp();

	// Creates a group from a list of units. Returns the pointer to the group created or nullptr
	UnitGroup* CreateGroupFromUnits(list<Unit*> units);

	// Creates a group from a single unit. Returns the pointer to the group created or nullptr
	UnitGroup* CreateGroupFromUnit(Unit* unit);

	// Returns the last group created or nullptr
	UnitGroup* GetLastGroup() const;

	// Returns an existing group by a pointer to one of its units or nullptr
	UnitGroup* GetGroupByUnit(Unit* unit) const;

	// Returns an existing group by a list to all of its units or nullptr
	UnitGroup* GetGroupByUnits(list<Unit*> units) const;

	// Moves a unit applying the principles of group movement. Returns the state of the unit's movement
	/// Call this method from a unit's update
	MovementState MoveUnit(Unit* unit, float dt); /// Not const because it needs to keep track of the number of paths created at the current update

	// -----

	// Checks for future collisions between the current unit and the rest of the units
	/// Sets the collision of the unit with the type of collision found or CollisionType_NoCollision
	void CheckForFutureCollision(SingleUnit* singleUnit) const;

	// Returns true if a tile is valid
	/// Checks for all the units (except for the unit passed as an argument) if a unit's tile (currTile, nextTile or goalTile) matches the tile passed as an argument
	bool IsValidTile(SingleUnit* singleUnit, iPoint tile, bool currTile = false, bool nextTile = false, bool goalTile = false) const;

	// Returns a new valid tile for the unit or { -1,-1 }
	/// If checkOnlyFront is true, it only checks the three tiles that are in front of the unit passed as an argument
	/// If checkOnlyFront is false, it checks the eight tiles surrounding the unit passed as an argument
	/// The new tile is searched using a Priority Queue containing the neighbors of the current tile of the unit passed as an argument
	iPoint FindNewValidTile(SingleUnit* singleUnit, bool checkOnlyFront = false) const;

	// Returns a new valid goal for the unit or { -1,-1 }
	/// The new goal is searched using BFS from the goal tile passed as an argument
	iPoint FindNewValidGoal(SingleUnit* singleUnit, iPoint goal, bool checkEverything = false);

	// Returns true if it succeeds in changing the next tile of the unit
	bool ChangeNextTile(SingleUnit* singleUnit); /// Not const because it needs to keep track of the number of paths created at the current update

	// Returns true if two units are heading towards opposite directions
	bool IsOppositeDirection(SingleUnit* singleUnitA, SingleUnit* singleUnitB) const;

private:

	list<UnitGroup*> unitGroups; // contains all the existing groups
};

// ---------------------------------------------------------------------
// UnitGroup: struct representing a group
// ---------------------------------------------------------------------

struct UnitGroup
{
	UnitGroup(Unit* unit);

	UnitGroup(list<Unit*> units);

	~UnitGroup();

	// Adds a singleUnit (unit) to the group. Returns false if the singleUnit was already in the group
	bool AddUnit(SingleUnit* singleUnit);

	// Removes a singleUnit (unit) from the group. Returns true if success
	bool RemoveUnit(SingleUnit* singleUnit);

	// Returns true if the singleUnit (unit) is in the group
	bool IsUnitInGroup(SingleUnit* singleUnit) const;

	// Returns the size of the group (the number of singleUnits in the group)
	uint GetSize() const;

	// Sets the destination tile (goal) of the group
	bool SetGoal(iPoint goal);

	// Returns the destination tile (goal) of the group
	iPoint GetGoal() const;

	// -----

	list<SingleUnit*> units; // contains all the units of a given group
	iPoint goal = { -1,-1 }; // current goal of the group
};

// ---------------------------------------------------------------------
// SingleUnit: struct representing a single unit
// ---------------------------------------------------------------------

struct SingleUnit
{
	SingleUnit(Unit* entity, UnitGroup* group);

	// Returns true if the unit would reach its next tile during this move
	/// nextPos is the next tile that the unit is heading to
	/// endPos is the tile that the unit would reach during this move
	bool IsTileReached(iPoint nextPos, fPoint endPos) const;

	// Stops the unit
	void StopUnit();

	// Resets the variables of the unit
	void ResetVariables();

	// -----

	Unit* unit = nullptr;
	UnitGroup* group = nullptr;
	MovementState movementState = MovementState_NoState;

	vector<iPoint> path; // path to the unit's goal
	iPoint currTile = { -1,-1 }; // position of the unit in map coords
	iPoint nextTile = { -1,-1 }; // next waypoint of the path (next tile the unit is heading to in map coords)

	iPoint goal = { -1,-1 }; // current goal of the unit
	iPoint newGoal = { -1,-1 }; // new goal of the unit
	/// newGoal exists to save the new goal set for the unit and not change abruptly the current goal
	/// The current goal will be changed when the unit has reached its next tile

	float speed = 1.0f; // movement speed
	uint priority = 0; // priority of the unit in relation to the rest of the units of the group
	bool reversePriority = false; // if true, the priority of the unit is not taken into account

	// Collision avoidance
	bool wait = false;
	bool wakeUp = false; // sets a unit's unitState to UnitState_Walk
	/// If a unit is not in the UnitState_Walk and another unit needs this unit to move away, set wakeUp to true
	iPoint waitTile = { -1,-1 }; // conflict tile (tile where the collision has been found)
	SingleUnit* waitUnit = nullptr; // conflict unit (unit whom the collision has been found with)
	CollisionType coll = CollisionType_NoCollision; // type of collision

	bool pathRequested = false;
};

// Helper classes to compare iPoints by its priority
class iPointPriority
{
public:
	iPointPriority() {}
	iPointPriority(iPoint point, int priority) :point(point), priority(priority) {}
	iPointPriority(const iPointPriority& i)
	{
		point = i.point;
		priority = i.priority;
	}

	iPoint point = { 0,0 };
	uint priority = 0;
};

class Comparator
{
public:
	int operator() (const iPointPriority a, const iPointPriority b)
	{
		return a.priority > b.priority;
	}
};

#endif //__j1MOVEMENT_H__