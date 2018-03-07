#ifndef __j1MOVEMENT_H__
#define __j1MOVEMENT_H__

#include "j1Module.h"
#include "p2Point.h"

#include "j1Pathfinding.h"
#include "j1EntityFactory.h"

#include <list>
#include <vector>
using namespace std;

#define MAX_PATHS_CREATED 1

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

	// Creates a group from a list of units
	UnitGroup* CreateGroupFromUnits(list<Unit*> units);

	// Creates a group from a single unit
	UnitGroup* CreateGroupFromUnit(Unit* unit);

	// Returns the last group created or nullptr
	UnitGroup* GetLastGroup() const;

	// Returns an existing group by a pointer to one of its units or nullptr
	UnitGroup* GetGroupByUnit(Unit* unit) const;

	UnitGroup* GetGroupByUnits(list<Unit*> units) const;

	// Moves an entity (if it is member of a group, through group movement). Returns the state of the movement
	/// Call this method from any entity's update if you want to move the entity
	/// It is not a const method because it needs to keep track of the number of paths created in this update
	MovementState MoveUnit(Unit* unit, float dt);

	// -----

	// Returns the type of collision that there would be between the unit and another unit or CollisionType_NoCollision
	void CheckForFutureCollision(SingleUnit* singleUnit) const;

	// Returns true if the tile passed isn't and won't be occupied by a unit
	bool IsValidTile(SingleUnit* singleUnit, iPoint tile, bool currTile = false, bool nextTile = false, bool goalTile = false) const;

	// Returns a valid tile for the unit (8 possibilities) or {-1,-1}
	iPoint FindNewValidTile(SingleUnit* singleUnit, bool checkOnlyFront = false) const;

	iPoint FindNewValidGoal(SingleUnit* singleUnit) const;

	/// It is not a const method because it needs to keep track of the number of paths created in this update
	bool ChangeNextTile(SingleUnit* singleUnit);

	bool IsOppositeDirection(SingleUnit* singleUnitA, SingleUnit* singleUnitB) const;

private:
	int pathsCreated = 0;
	list<UnitGroup*> unitGroups; // contains all the existing groups
};

// ---------------------------------------------------------------------
// UnitGroup: struct representing a group
// ---------------------------------------------------------------------

struct UnitGroup
{
	UnitGroup(Unit* unit);

	UnitGroup(list<Unit*> units);

	// Adds an entity to the group. Returns the ID of the entity or -1
	bool AddUnit(SingleUnit* singleUnit);

	// Removes an entity from the group. Returns true if success or false
	bool RemoveUnit(SingleUnit* singleUnit);

	bool IsUnitInGroup(SingleUnit* singleUnit) const;

	// Returns the size of the group (the number of entities in the group)
	uint GetSize() const;

	// Sets the destination tile of the group
	bool SetGoal(iPoint goal);

	// Returns the destination tile of the group
	iPoint GetGoal() const;

	// Returns the max speed of the group
	float GetMaxSpeed() const;

	//UnitGroupType GetType() const;
	//void SetType(UnitGroupType unitGroupType);
	//fPoint GetCentroid() const;

	// -----

	list<SingleUnit*> units; // contains all the units of a given group
	iPoint goal = { -1,-1 }; // current goal of the group

	float maxSpeed = 0.0f;

	//UnitGroupType unitGroupType;
	//fPoint centroid;
};

// ---------------------------------------------------------------------
// Unit: struct representing a single unit
// ---------------------------------------------------------------------

struct SingleUnit
{
	SingleUnit(Unit* entity, UnitGroup* group);

	bool CreatePath(iPoint startPos);
	bool IsTileReached(iPoint nextPos, fPoint endPos) const;
	void StopUnit();

	// -----

	Unit* unit = nullptr;
	UnitGroup* group = nullptr;
	MovementState movementState = MovementState_WaitForPath;

	vector<iPoint> path; // path to the group goal
	iPoint currTile = { -1,-1 }; // position of the unit in map coords
	iPoint nextTile = { -1,-1 }; // next waypoint from the path (next tile the unit is heading to in map coords)

	iPoint goal = { -1,-1 }; // current goal of the unit
	iPoint newGoal = { -1,-1 }; // new goal of the unit

	float speed = 1.0f; // movement speed: it can be the speed of the entity or the speed of the group
	uint priority = 0; // priority of the unit in relation to the rest of the units of the group

	// Collision avoidance
	bool wait = false;
	bool wakeUp = false;
	iPoint waitTile = { -1,-1 };
	SingleUnit* waitUnit = nullptr;
	CollisionType collision = CollisionType_NoCollision;
};

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