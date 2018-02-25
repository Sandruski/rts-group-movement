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

/*
enum UnitGroupType {
UnitGroupType_NoType

};
*/

enum MovementState {
	MovementState_WaitForPath,
	MovementState_FollowPath,
	MovementState_GoalReached,
	MovementState_CollisionFound,
	MovementState_IncreaseWaypoint
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
	UnitGroup* CreateGroup(list<Entity*> entities);

	// Returns the last group created or nullptr
	UnitGroup* GetLastGroup() const;

	// Returns an existing group by its ID or nullptr
	UnitGroup* GetGroupByIndex(uint id) const;

	// Returns an existing group by a pointer to one of its units or nullptr
	UnitGroup* GetGroupByEntity(Entity* entity) const;

	// Returns an existing group by the list of all of its entities or nullptr
	UnitGroup* GetGroupByEntities(list<Entity*> entities) const;

	// Moves an entity (if it is member of a group, through group movement). Returns true if the goal is reached
	bool MoveEntity(Entity* entity, float dt) const;

private:

	// Stores all the existing groups
	list<UnitGroup*> unitGroups;
};

// ---------------------------------------------------------------------
// UnitGroup: struct representing a group
// ---------------------------------------------------------------------

struct UnitGroup
{
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

	list<SingleUnit*> units;

	iPoint goal = { 0,0 };

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

	// -----

	Entity* entity = nullptr;
	UnitGroup* group = nullptr;
	MovementState movementState = MovementState_WaitForPath;

	/// path to the macro objective
	vector<iPoint> path; 
	/// local tracking target for the steering behaviour
	iPoint nextTile = { -1,-1 }; // next tile the unit is heading to (in order to reach the goal tile)

	float speed = 1.0f; // movement speed: it can be the speed of the entity or the speed of the group
};

#endif //__j1MOVEMENT_H__