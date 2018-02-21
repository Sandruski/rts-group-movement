#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1Movement.h"
#include "j1EntityFactory.h"
#include "Entity.h"
#include "j1Map.h"
#include "j1Render.h"

#include "Brofiler\Brofiler.h"

j1Movement::j1Movement() {}

j1Movement::~j1Movement() {}

bool j1Movement::Update(float dt) 
{
	bool ret = true;

	DebugDraw();

	return ret;
}

void j1Movement::DebugDraw() const 
{
	for (list<UnitGroup*>::const_iterator group = unitGroups.begin(); group != unitGroups.end(); ++group) {
		for (list<SingleUnit*>::const_iterator unit = (*group)->units.begin(); unit != (*group)->units.end(); ++unit) {

			if ((*unit)->nextTile.x > -1 && (*unit)->nextTile.y > -1) {

				// Raycast a line between the unit and the nextTile
				iPoint nextPos = App->map->MapToWorld((*unit)->nextTile.x, (*unit)->nextTile.y);
				App->render->DrawLine((*unit)->entity->entityInfo.pos.x, (*unit)->entity->entityInfo.pos.y, nextPos.x, nextPos.y, 255, 255, 255, 255);
				App->render->DrawCircle(nextPos.x, nextPos.y, 10, 255, 255, 255, 255);

				// Draw path
				/*
				for (uint i = 0; i < path.size(); ++i)
				{
				iPoint pos = App->map->MapToWorld(path.at(i).x, path.at(i).y);
				App->render->Blit(sprites, pos.x, pos.y);
				}
				*/
			}
		}
	}
}

bool j1Movement::CleanUp()
{
	bool ret = true;

	return ret;
}

UnitGroup* j1Movement::CreateGroup(list<Entity*> entities)
{
	// If an entity from the list belongs to an existing group, first delete the group
	list<Entity*>::const_iterator it = entities.begin();
	UnitGroup* group = nullptr;

	while (it != entities.end()) {

		group = GetGroupByEntity(*it);
		if (group != nullptr)
			unitGroups.remove(group);

		it++;
	}
	group = nullptr;

	group = new UnitGroup(entities);
	unitGroups.push_back(group);

	return group;
}

UnitGroup* j1Movement::GetLastGroup() const 
{
	return unitGroups.back();
}

UnitGroup* j1Movement::GetGroupByIndex(uint id) const
{
	list<UnitGroup*>::const_iterator it = unitGroups.begin();
	UnitGroup* group = nullptr;

	advance(it, id);
	if (it != unitGroups.end())
		group = *it;

	return group;
}

UnitGroup* j1Movement::GetGroupByEntity(Entity* entity) const
{
	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;
	UnitGroup* group = nullptr;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {
		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {
			if ((*units)->entity == entity) {
				group = *groups;
				break;
			}
		}
	}

	return group;
}

bool j1Movement::MoveEntity(Entity* entity, float dt) const 
{
	bool ret = false;

	UnitGroup* g = GetGroupByEntity(entity);
	SingleUnit* u = g->GetUnitByEntity(entity);

	iPoint currTile = App->map->WorldToMap(u->entity->entityInfo.pos.x, u->entity->entityInfo.pos.y); // unit current pos in map coords
	iPoint nextPos = App->map->MapToWorld(u->nextTile.x, u->nextTile.y); // unit nextPos in map coords

	fPoint movePos;
	fPoint endPos;

	float m;
	fPoint dir;

	switch (u->movementState) {

	case MovementState_WaitForPath:

		// Find a path
		if (App->pathfinding->CreatePath(currTile, u->group->GetGoal(), DISTANCE_MANHATTAN) == -1)
			break;

		// Save the path found
		u->path = *App->pathfinding->GetLastPath();

		// Set state to IncreaseWaypoint, in order to start following the path
		u->movementState = MovementState_IncreaseWaypoint;

		break;

	case MovementState_FollowPath:

		// MOVEMENT CALCULATION

		// Calculate the difference between nextTile and currTile. The result will be in the interval [-1,1]
		movePos = { (float)nextPos.x - u->entity->entityInfo.pos.x, (float)nextPos.y - u->entity->entityInfo.pos.y };

		// Normalize
		m = sqrtf(pow(movePos.x, 2.0f) + pow(movePos.y, 2.0f));

		if (m > 0.0f) {
			movePos.x /= m;
			movePos.y /= m;
		}

		dir.x = movePos.x;
		dir.y = movePos.y;

		// Apply the speed and the dt to the previous result
		movePos.x *= u->speed * dt;
		movePos.y *= u->speed * dt;

		// COLLISION CALCULATION

		// Predict where the unit will be after moving
		endPos = { u->entity->entityInfo.pos.x + movePos.x,u->entity->entityInfo.pos.y + movePos.y };

		// endTile to check collisions

		// Check for future collisions before moving
		//if (App->pathfinding->IsWalkable(nextTile) && !App->entities->IsAnotherEntityOnTile(this, nextTile)) { // endTile may change (terrain modification) or may be occupied by a unit

		// necessary offset

		// If we're going to jump over the waypoint during this move


		if (dir.x >= 0) { // Right or stop

			if (dir.y >= 0) { // Up or stop
				if (endPos.x >= nextPos.x && endPos.y >= nextPos.y) {
					u->entity->entityInfo.pos.x = nextPos.x;
					u->entity->entityInfo.pos.y = nextPos.y;
					u->movementState = MovementState_IncreaseWaypoint;
					break;
				}
			}
			else { // Down
				if (endPos.x >= nextPos.x && endPos.y <= nextPos.y) {
					u->entity->entityInfo.pos.x = nextPos.x;
					u->entity->entityInfo.pos.y = nextPos.y;
					u->movementState = MovementState_IncreaseWaypoint;
					break;
				}
			}
		}
		else { // Left

			   // Up or stop
			if (dir.y >= 0) {
				if (endPos.x <= nextPos.x && endPos.y >= nextPos.y) {
					u->entity->entityInfo.pos.x = nextPos.x;
					u->entity->entityInfo.pos.y = nextPos.y;
					u->movementState = MovementState_IncreaseWaypoint;
					break;
				}
			}
			else { // Down
				if (endPos.x <= nextPos.x && endPos.y <= nextPos.y) {
					u->entity->entityInfo.pos.x = nextPos.x;
					u->entity->entityInfo.pos.y = nextPos.y;
					u->movementState = MovementState_IncreaseWaypoint;
					break;
				}
			}
		}

		// Do the actual move
		u->entity->entityInfo.pos.x += movePos.x;
		u->entity->entityInfo.pos.y += movePos.y;
		//}
		//else {
		//nextTile = App->entities->FindClosestWalkableTile(this, nextTile);

		//if (nextTile.x == -1 && nextTile.y == -1)
		//LOG("-1!!!");

		//movementState = MovementState_CollisionFound;
		//}

		break;

	case MovementState_GoalReached:

		// Make the appropiate notifications
		ret = true;

		break;

	case MovementState_CollisionFound:

		// If we want to prevent units from standing in line, we must implement local avoidance

		u->movementState = MovementState_FollowPath;

		break;

	case MovementState_IncreaseWaypoint:

		// Get the next waypoint to head to
		if (u->path.size() > 0) {
			u->nextTile = u->path.front();
			u->path.erase(u->path.begin());

			u->movementState = MovementState_FollowPath;
		}

		break;
	}

	return ret;
}

// UnitGroup struct ---------------------------------------------------------------------------------

UnitGroup::UnitGroup(list<Entity*> entities)
{
	list<Entity*>::const_iterator it = entities.begin();

	while (it != entities.end()) {

		AddUnit(*it);
		it++;
	}
}

SingleUnit* UnitGroup::AddUnit(Entity* entity)
{
	// If an the entity belongs to an existing unit, first delete the unit
	SingleUnit* unit = GetUnitByEntity(entity);

	if (unit != nullptr)
		units.remove(unit);
	unit = nullptr;

	unit = new SingleUnit(entity, this);
	units.push_back(unit);

	return unit;
}

bool UnitGroup::RemoveUnit(Entity* entity)
{
	bool ret = false;

	list<SingleUnit*>::const_iterator it = units.begin();

	while (it != units.end()) {

		if ((*it)->entity == entity) {
			units.remove(*it);
			ret = true;
		}
		it++;
	}

	return ret;
}

uint UnitGroup::GetSize() const
{
	return units.size();
}

SingleUnit* UnitGroup::GetUnitByIndex(uint id)
{
	list<SingleUnit*>::const_iterator it = units.begin();

	advance(it, id);
	if (it != units.end())
		return *it;

	return nullptr;
}

SingleUnit* UnitGroup::GetUnitByEntity(Entity* entity) const
{
	list<SingleUnit*>::const_iterator it = units.begin();

	while (it != units.end()) {
		if ((*it)->entity == entity)
			return *it;

		it++;
	}

	return nullptr;
}

float UnitGroup::GetMaxSpeed() const
{
	return maxSpeed;
}

bool UnitGroup::SetGoal(iPoint goal)
{
	bool ret = false;

	if (App->pathfinding->IsWalkable(goal)) {
		this->goal.x = goal.x;
		this->goal.y = goal.y;

		ret = true;
	}

	return ret;
}

iPoint UnitGroup::GetGoal() const
{
	return goal;
}

// Unit struct ---------------------------------------------------------------------------------

SingleUnit::SingleUnit(Entity* entity, UnitGroup* group) :entity(entity), group(group)
{
	speed = entity->entityInfo.speed;
	speed *= 50.0f; // MYTODO: delete this 50.0f magic number!!!
}