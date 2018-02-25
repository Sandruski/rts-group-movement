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
				iPoint offset = { App->map->data.tile_width / 2, App->map->data.tile_height / 2 };
				iPoint nextPos = App->map->MapToWorld((*unit)->nextTile.x, (*unit)->nextTile.y);
				App->render->DrawLine((*unit)->entity->entityInfo.pos.x + offset.x, (*unit)->entity->entityInfo.pos.y + offset.y, nextPos.x + offset.x, nextPos.y + offset.y, 255, 255, 255, 255);
				App->render->DrawCircle(nextPos.x + offset.x, nextPos.y + offset.y, 10, 255, 255, 255, 255);

				// Draw path
				
				for (uint i = 0; i < (*unit)->path.size(); ++i)
				{
					iPoint pos = App->map->MapToWorld((*unit)->path.at(i).x, (*unit)->path.at(i).y);
					SDL_Rect rect = { pos.x, pos.y, App->map->data.tile_width, App->map->data.tile_height };
					App->render->DrawQuad(rect, 0, 255, 0, 50);
				}
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
	// If an entity from the list belongs to an existing group, delete the entity from the group
	list<Entity*>::const_iterator it = entities.begin();
	UnitGroup* group = nullptr;

	while (it != entities.end()) {

		group = GetGroupByEntity(*it);
		if (group != nullptr) {
			group->units.remove(group->GetUnitByEntity(*it));

			// If the group is empty, delete it
			if (group->GetSize() == 0)
				unitGroups.remove(group);
		}

		it++;
	}

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

UnitGroup* j1Movement::GetGroupByEntities(list<Entity*> entities) const 
{
	list<Entity*>::const_iterator it;
	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;
	uint size = 0;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {

		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {

			for (it = entities.begin(); it != entities.end(); ++it) {

				if ((*units)->entity == *it)
					size++;
			}
		}

		if (size == entities.size() && size == (*groups)->GetSize())
			return *groups;

		size = 0;
	}

	return nullptr;
}

bool j1Movement::MoveEntity(Entity* entity, float dt) const
{
	bool ret = false;

	UnitGroup* g = GetGroupByEntity(entity);

	if (g == nullptr)
		return ret;

	SingleUnit* u = g->GetUnitByEntity(entity);

	if (u == nullptr)
		return ret;

	iPoint currTile = App->map->WorldToMap(u->entity->entityInfo.pos.x, u->entity->entityInfo.pos.y); // unit current pos in map coords
	iPoint nextPos = App->map->MapToWorld(u->nextTile.x, u->nextTile.y); // unit nextPos in map coords
	
	fPoint movePos;
	fPoint endPos;
	iPoint endTile;
	iPoint updatedTile;

	float m;
	bool increaseWaypoint = false;

	/// For each step:
	switch (u->movementState) {

	case MovementState_WaitForPath:

		// Find a path
		if (App->pathfinding->CreatePath(currTile, u->group->GetGoal(), DISTANCE_MANHATTAN) == -1)
			break;

		// Save the path found
		u->path = *App->pathfinding->GetLastPath();

		// Erase the first waypoint of the path, so the entities don't behave weird
		//u->path.erase(u->path.begin());

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

		u->entity->entityInfo.direction.x = movePos.x;
		u->entity->entityInfo.direction.y = movePos.y;

		// Apply the speed and the dt to the previous result
		movePos.x *= u->speed * dt;
		movePos.y *= u->speed * dt;

		// COLLISION CALCULATION

		// Predict where the unit will be after moving
		endPos = { u->entity->entityInfo.pos.x + movePos.x,u->entity->entityInfo.pos.y + movePos.y };
		endTile = App->map->WorldToMap(endPos.x, endPos.y);

		// Check for future collisions before moving
		// If the tile the unit is running towards an invalid tile:
		// We stablish 2 levels of depth for the collision checking:

		/// CONS to check endTile: the entity doesn't move in a lot of occasions, because in order to reach nextTile, the entity may stand on an invalid endTile.
		/// PROS to check endTile: we avoid situations like jumping over another entity, etc.
		/// nextTile: if the entity is going in the -x, -y diagonal, the endTile will be the same as the nextTile once the entity has arrived at the nextTile.
		/// So, what if the nextTile was invalid?

		if (!App->pathfinding->IsWalkable(endTile) || App->entities->IsAnotherEntityOnTile(u->entity, endTile)
			|| !App->pathfinding->IsWalkable(u->nextTile) || App->entities->IsAnotherEntityOnTile(u->entity, u->nextTile)) { // endTile may change (terrain modification) or may be occupied by an entity

			LOG("Invalid tile");
			/// Okay... You don't want to be here forever. If the tile you want to go is occupied for a certain ammount of time, stop!
			/// This check should be done in pairs. Entities start getting crazy otherwise...

			updatedTile = App->entities->FindClosestWalkableTile(u->entity, currTile);

			if (updatedTile.x != -1 && updatedTile.y != -1) {
				
				// Recalculate the path. PROBLEM: the new path may be the same as the old one. This can happen infinite times! The entity will be trapped in a back-and-forth cycle
				// Find a path
				/// The new path shouldn't consider the tile that was not walkable because of an entity laying on it...
				if (App->pathfinding->CreatePath(u->nextTile, u->group->GetGoal(), DISTANCE_MANHATTAN) == -1)
					break;

				u->nextTile = updatedTile;

				// Save the path found
				u->path = *App->pathfinding->GetLastPath();

				// Erase the first waypoint of the path - we have nextTile already updated
				u->path.erase(u->path.begin());
			}

			break;
		}

		// If we're going to jump over the waypoint during this move:
		if (u->entity->entityInfo.direction.x >= 0) { // Right or stop

			if (u->entity->entityInfo.direction.y >= 0) { // Down or stop
				if (endPos.x >= nextPos.x && endPos.y >= nextPos.y)
					increaseWaypoint = true;
			}
			else { // Up
				if (endPos.x >= nextPos.x && endPos.y <= nextPos.y)
					increaseWaypoint = true;
			}
		}
		else { // Left

			if (u->entity->entityInfo.direction.y >= 0) { // Down or stop
				if (endPos.x <= nextPos.x && endPos.y >= nextPos.y)
					increaseWaypoint = true;
			}
			else { // Up
				if (endPos.x <= nextPos.x && endPos.y <= nextPos.y)
					increaseWaypoint = true;
			}
		}

		if (increaseWaypoint) {
			u->entity->entityInfo.pos.x = nextPos.x;
			u->entity->entityInfo.pos.y = nextPos.y;
			u->movementState = MovementState_IncreaseWaypoint;
			break;
		}

		// Do the actual move
		u->entity->entityInfo.pos.x += movePos.x;
		u->entity->entityInfo.pos.y += movePos.y;

		break;

	case MovementState_GoalReached:

		// Make the appropiate notifications
		u->entity->entityInfo.direction.x = 0.0f;
		u->entity->entityInfo.direction.y = 0.0f;

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
		else
			u->movementState = MovementState_GoalReached;

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

		// Set all units to wait for a new path (the goal has changed!)
		list<SingleUnit*>::const_iterator it = units.begin();

		while (it != units.end()) {
			(*it)->movementState = MovementState_WaitForPath;

			it++;
		}

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