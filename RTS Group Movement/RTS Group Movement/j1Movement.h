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
	CollisionType_ItsCell,
	CollisionType_DiagonalCrossing
};

enum CollisionBehaviour {
	CollisionBehaviour_FindNewCell,
	CollisionBehaviour_Wait
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

	// Creates a group from a list of entities
	UnitGroup* CreateGroupFromList(list<Entity*> entities);

	// Creates a group from a single entity
	UnitGroup* CreateGroupFromEntity(Entity* entity);

	// Returns the last group created or nullptr
	UnitGroup* GetLastGroup() const;

	// Returns an existing group by its ID or nullptr
	UnitGroup* GetGroupByIndex(uint id) const;

	// Returns an existing group by a pointer to one of its units or nullptr
	UnitGroup* GetGroupByEntity(Entity* entity) const;

	// Returns an existing group by the list of all of its entities or nullptr
	UnitGroup* GetGroupByEntities(list<Entity*> entities) const;

	// Moves an entity (if it is member of a group, through group movement). Returns the state of the movement
	/// Call this method from any entity's update if you want to move the entity
	MovementState MoveEntity(Entity* entity, float dt) const;

	// Returns the type of collision that there would be between the unit and another unit or CollisionType_NoCollision
	CollisionType CheckForFutureCollision(SingleUnit* unit) const;

	// Returns true if the tile passed isn't and won't be occupied by a unit
	bool IsValidTile(SingleUnit* unit, iPoint tile) const;

	// Returns a valid tile for the unit (8 possibilities) or {-1,-1}
	iPoint FindNewValidTile(SingleUnit* unit) const;

	bool IsTileOccupied(SingleUnit* unit, iPoint tile) const;

private:

	list<UnitGroup*> unitGroups; // contains all the existing groups
};

// ---------------------------------------------------------------------
// UnitGroup: struct representing a group
// ---------------------------------------------------------------------

struct UnitGroup
{
	UnitGroup(Entity* entity);

	UnitGroup(list<Entity*> entities);

	// Adds an entity to the group. Returns the ID of the entity or -1
	SingleUnit* AddUnit(Entity* entity);

	// Removes an entity from the group. Returns true if success or false
	bool RemoveUnit(Entity* entity);

	// Returns an existing unit in the group by its ID or nullptr
	SingleUnit* GetUnitByIndex(uint id);

	// Returns an existing unit in the group by a pointer to its entity or nullptr
	SingleUnit* GetUnitByEntity(Entity* entity) const;

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
	SingleUnit(Entity* entity, UnitGroup* group);

	bool CreatePath(iPoint startPos);
	bool IsTileReached(iPoint nextPos, fPoint endPos) const;

	// -----

	Entity* entity = nullptr;
	UnitGroup* group = nullptr;
	MovementState movementState = MovementState_WaitForPath;

	vector<iPoint> path; // path to the group goal
	iPoint currTile = { -1,-1 }; // position of the unit in map coords
	iPoint nextTile = { -1,-1 }; // next waypoint from the path (next tile the unit is heading to in map coords)

	iPoint goal = { -1,-1 }; // current goal of the unit
	iPoint newGoal = { -1,-1 }; // new goal of the unit

	float speed = 1.0f; // movement speed: it can be the speed of the entity or the speed of the group
	int priority = 0; // priority of the unit in relation to the rest of the units of the group

	bool wait = false;
	SingleUnit* waitForUnit = nullptr;
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
	int priority = 0;
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