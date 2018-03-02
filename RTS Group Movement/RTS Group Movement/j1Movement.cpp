#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1Movement.h"
#include "j1EntityFactory.h"
#include "Entity.h"
#include "j1Map.h"
#include "j1Render.h"

#include "Brofiler\Brofiler.h"

#include <queue>

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

			if ((*unit)->movementState != MovementState_NoState && (*unit)->movementState != MovementState_GoalReached 
				&& (*unit)->nextTile.x > -1 && (*unit)->nextTile.y > -1) {

				// Raycast a line between the unit and the nextTile
				iPoint offset = { App->map->data.tile_width / 2, App->map->data.tile_height / 2 };
				iPoint nextPos = App->map->MapToWorld((*unit)->nextTile.x, (*unit)->nextTile.y);
				App->render->DrawLine((*unit)->unit->entityInfo.pos.x + offset.x, (*unit)->unit->entityInfo.pos.y + offset.y, nextPos.x + offset.x, nextPos.y + offset.y, 255, 255, 255, 255);
				App->render->DrawCircle(nextPos.x + offset.x, nextPos.y + offset.y, 10, 255, 255, 255, 255);

				/*
				// Draw unit's path
				for (uint i = 0; i < (*unit)->path.size(); ++i)
				{
					iPoint pos = App->map->MapToWorld((*unit)->path.at(i).x, (*unit)->path.at(i).y);
					SDL_Rect rect = { pos.x, pos.y, App->map->data.tile_width, App->map->data.tile_height };
					App->render->DrawQuad(rect, 0, 255, 0, 50);
				}

				// Draw unit's goal (in a different color than the path)
				iPoint pos = App->map->MapToWorld((*unit)->goal.x, (*unit)->goal.y);
				SDL_Rect rect = { pos.x, pos.y, App->map->data.tile_width, App->map->data.tile_height };
				App->render->DrawQuad(rect, 0, 0, 0, 200);
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

UnitGroup* j1Movement::CreateGroupFromList(list<Entity*> entities)
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

UnitGroup* j1Movement::CreateGroupFromEntity(Entity* entity) 
{
	// If the entity belongs to an existing group, delete the entity from the group
	UnitGroup* group = nullptr;
	group = GetGroupByEntity(entity);
	if (group != nullptr) {

		group->units.remove(group->GetUnitByEntity(entity));

		// If the group is empty, delete it
		if (group->GetSize() == 0)
			unitGroups.remove(group);
	}

	group = new UnitGroup(entity);
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
			if ((Entity*)(*units)->unit == entity) {
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

				if ((Entity*)(*units)->unit == *it)
					size++;
			}
		}

		if (size == entities.size() && size == (*groups)->GetSize())
			return *groups;

		size = 0;
	}

	return nullptr;
}

MovementState j1Movement::MoveEntity(Entity* entity, float dt) const
{
	MovementState ret = MovementState_NoState;

	UnitGroup* g = GetGroupByEntity(entity);

	if (g == nullptr)
		return MovementState_NoState;

	SingleUnit* u = g->GetUnitByEntity(entity);

	if (u == nullptr)
		return MovementState_NoState;

	ret = u->movementState;

	u->currTile = App->map->WorldToMap(u->unit->entityInfo.pos.x, u->unit->entityInfo.pos.y); // unit current pos in map coords
	iPoint nextPos = App->map->MapToWorld(u->nextTile.x, u->nextTile.y); // unit nextPos in map coords
	
	fPoint movePos, endPos;
	iPoint newTile;

	float m;
	CollisionType coll;

	/// For each step:
	switch (u->movementState) {

	case MovementState_WaitForPath:

		if (u->goal.x != -1 && u->goal.y != -1) {

			// Check if the goal is valid
			if (!IsValidTile(u, u->goal, false, false, true))

				// If the goal is not valid, find a new goal
				u->goal = u->newGoal = FindNewValidGoal(u);

			// Check if the goal is valid (the new valid goal could not be valid)
			if (u->goal.x != -1 && u->goal.y != -1)

				// If the goal is valid, find a path
				if (u->CreatePath(u->currTile))

					// Set state to IncreaseWaypoint, in order to start following the path
					u->movementState = MovementState_IncreaseWaypoint;
		}

		break;

	case MovementState_FollowPath:

		// ---------------------------------------------------------------------
		// MOVEMENT CALCULATION
		// ---------------------------------------------------------------------

		// Calculate the difference between nextTile and currTile. The result will be in the interval [-1,1]
		movePos = { (float)nextPos.x - u->unit->entityInfo.pos.x, (float)nextPos.y - u->unit->entityInfo.pos.y };

		// Normalize
		m = sqrtf(pow(movePos.x, 2.0f) + pow(movePos.y, 2.0f));

		if (m > 0.0f) {
			movePos.x /= m;
			movePos.y /= m;
		}

		u->unit->SetUnitDirectionByValue(movePos);

		// Apply the speed and the dt to the previous result
		movePos.x *= u->speed * dt;
		movePos.y *= u->speed * dt;

		// ---------------------------------------------------------------------
		// COLLISION CALCULATION
		// ---------------------------------------------------------------------

		// Check if there would be a collision if the unit reached nextTile during this move
		coll = CheckForFutureCollision(u);

		// Treat the collision
		if (coll != CollisionType_NoCollision) {

			if (coll == CollisionType_ItsCell || coll == CollisionType_SameCell) {

				if (coll == CollisionType_ItsCell)
					// Find a new, valid nextTile to move
					newTile = FindNewValidTile(u, true);
				else if (coll == CollisionType_SameCell)
					// Find a new, valid nextTile to move
					newTile = FindNewValidTile(u);

				if (newTile.x != -1 && newTile.y != -1) {

					// If the nextTile was going to be the goal tile:
					if (u->nextTile == u->goal) {

						// Stop the unit when it reaches its new nextTile
						u->path.clear();
						u->nextTile = newTile;
						u->goal = u->newGoal = newTile;

						break;
					}

					// Update the unit's nextTile
					u->nextTile = newTile;

					// Recalculate the path
					u->CreatePath(u->nextTile);
				}
				else if (newTile.x == -1 && newTile.y == -1) {

					u->unit->SetUnitDirection(UnitDirection_Idle);

					/// Timer!!!
					u->timer += 1 * dt;

					if (u->timer >= 3) {
						// Stop the unit when it reaches its new nextTile
						u->path.clear();
						u->nextTile = u->currTile;
						u->goal = u->newGoal = u->currTile;

						u->timer = 0;
					}

					// If the goal has been changed:
					/// We only want to update the goal when the unit has reached nextTile. That's why we do it here
					if (u->goal != u->newGoal) {
						u->goal = u->newGoal;
						u->movementState = MovementState_WaitForPath;
						break;
					}

				}

				break;
			}
			else if (coll == CollisionType_DiagonalCrossing) {

				// One of the units must stop and let the other one pass
				u->wait = true;
			}
		}

		u->timer = 0;

		if (u->wait) {

			// The other unit must notificate to the waiting unit when it has finished its movement
			/// We check that by simply checking if the other unit's nextTile is different from the collision tile
			if (u->waitUnit->nextTile != u->waitTile || u->waitUnit->nextTile == u->waitUnit->currTile) {
				u->wait = false;
				u->waitUnit = nullptr;
			}
			else
				break;
		}

		// ---------------------------------------------------------------------
		// TILE FITTING
		// ---------------------------------------------------------------------

		// Predict where the unit will be after moving
		endPos = { u->unit->entityInfo.pos.x + movePos.x,u->unit->entityInfo.pos.y + movePos.y };

		// Check if the unit would reach the nextTile during this move
		/// We check with an offset, in order to avoid the unit to miss the nextTile
		if (u->IsTileReached(nextPos, endPos)) {
		
			// If the unit's going to reach the nextTile during this move:
			u->unit->entityInfo.pos.x = nextPos.x;
			u->unit->entityInfo.pos.y = nextPos.y;
			u->movementState = MovementState_IncreaseWaypoint;
			break;
		}

		// If the unit's not going to reach the nextTile yet, keep moving
		u->unit->entityInfo.pos.x += movePos.x;
		u->unit->entityInfo.pos.y += movePos.y;

		break;

	case MovementState_GoalReached:

		// The unit is still
		u->StopUnit();

		// If the goal has been changed:
		if (u->goal != u->newGoal) {
			u->goal = u->newGoal;
			u->movementState = MovementState_WaitForPath;
			break;
		}

		break;

	case MovementState_IncreaseWaypoint:

		// If the goal has been changed:
		/// We only want to update the goal when the unit has reached nextTile. That's why we do it here
		if (u->goal != u->newGoal) {
			u->goal = u->newGoal;
			u->movementState = MovementState_WaitForPath;
			break;
		}

		// If the unit's path contains waypoints:
		if (u->path.size() > 0) {

			// Get the next waypoint to head to
			u->nextTile = u->path.front();
			u->path.erase(u->path.begin());

			u->movementState = MovementState_FollowPath;
		}
		else
			// If the unit's path is out of waypoints, it means that the unit has reached the goal
			u->movementState = MovementState_GoalReached;

		break;
	}

	return ret;
}

CollisionType j1Movement::CheckForFutureCollision(SingleUnit* singleUnit) const
{
	/// We don't check the walkability of the tile since the A* algorithm already did it for us

	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {
		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {

			if ((*units) != singleUnit) {

				if (singleUnit->nextTile == (*units)->currTile) {
					// A reaches B's tile
					return CollisionType_ItsCell;
				}
				else if (singleUnit->nextTile == (*units)->nextTile) {
					// A and B reach the same tile
					return CollisionType_SameCell;
				}

				// Diagonal crossing: even though the units don't reach the same tile, we want to avoid this situation
				iPoint up = { (*units)->currTile.x, (*units)->currTile.y - 1 };
				iPoint down = { (*units)->currTile.x, (*units)->currTile.y + 1 };
				iPoint left = { (*units)->currTile.x - 1, (*units)->currTile.y };
				iPoint right = { (*units)->currTile.x + 1, (*units)->currTile.y };

				iPoint myUp = { singleUnit->currTile.x, singleUnit->currTile.y - 1 };
				iPoint myDown = { singleUnit->currTile.x, singleUnit->currTile.y + 1 };
				iPoint myLeft = { singleUnit->currTile.x - 1, singleUnit->currTile.y };
				iPoint myRight = { singleUnit->currTile.x + 1, singleUnit->currTile.y };

				if (singleUnit->nextTile == up) {
					if ((*units)->nextTile == myUp && !(*units)->wait) {

						// Decide which unit waits (depending on its priority value)
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = myUp;

						return CollisionType_DiagonalCrossing;
					}
				}
				else if (singleUnit->nextTile == down) {
					if ((*units)->nextTile == myDown && !(*units)->wait) {

						// Decide which unit waits (depending on its priority value)
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = myDown;

						return CollisionType_DiagonalCrossing;
					}
				}
				else if (singleUnit->nextTile == left) {
					if ((*units)->nextTile == myLeft && !(*units)->wait) {

						// Decide which unit waits (depending on its priority value)
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = myLeft;

						return CollisionType_DiagonalCrossing;
					}
				}
				else if (singleUnit->nextTile == right) {
					if ((*units)->nextTile == myRight && !(*units)->wait) {

						// Decide which unit waits (depending on its priority value)
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = myRight;

						return CollisionType_DiagonalCrossing;
					}
				}
			}
		}
	}

	return CollisionType_NoCollision;
}

bool j1Movement::IsValidTile(SingleUnit* singleUnit, iPoint tile, bool currTile, bool nextTile, bool goalTile) const
{
	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {
		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {

			if ((*units) != singleUnit) {

				if (currTile) {
					if ((*units)->currTile == tile)
						return false;
				}
				if (nextTile) {
					if ((*units)->nextTile == tile)
						return false;
				}
				if (goalTile) {
					if ((*units)->goal == tile)
						return false;
				}
			}
		}
	}

	return true;
}

iPoint j1Movement::FindNewValidTile(SingleUnit* singleUnit, bool checkOnlyFront) const
{
	if (singleUnit == nullptr)
		return { -1,-1 };

	// 1. Units can only move in 8 directions from their current tile (the search only expands to 8 possible tiles)
	iPoint neighbors[8] = { {-1,-1},{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 } };

	if (checkOnlyFront) {

		// Check only the 3 tiles in front of the unit (depending on its direction)
		/// This way, the unit can only move forward in its direction

		switch (singleUnit->unit->GetUnitDirection()) {
		
		case UnitDirection_Idle:

			neighbors[0].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 0); // Right
			neighbors[1].create(singleUnit->currTile.x + 0, singleUnit->currTile.y + 1); // Down
			neighbors[2].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 0); // Left
			neighbors[3].create(singleUnit->currTile.x + 0, singleUnit->currTile.y - 1); // Up
			neighbors[4].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 1); // DownRight
			neighbors[5].create(singleUnit->currTile.x + 1, singleUnit->currTile.y - 1); // UpRight
			neighbors[6].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 1); // DownLeft
			neighbors[7].create(singleUnit->currTile.x - 1, singleUnit->currTile.y - 1); // UpLeft

			break;

		case UnitDirection_Up:

			neighbors[0].create(singleUnit->currTile.x + 0, singleUnit->currTile.y - 1); // Up
			neighbors[1].create(singleUnit->currTile.x + 1, singleUnit->currTile.y - 1); // UpRight
			neighbors[2].create(singleUnit->currTile.x - 1, singleUnit->currTile.y - 1); // UpLeft

			break;

		case UnitDirection_Down:

			neighbors[0].create(singleUnit->currTile.x + 0, singleUnit->currTile.y + 1); // Down
			neighbors[1].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 1); // DownRight
			neighbors[2].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 1); // DownLeft

			break;

		case UnitDirection_Left:

			neighbors[0].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 0); // Left
			neighbors[1].create(singleUnit->currTile.x - 1, singleUnit->currTile.y - 1); // UpLeft
			neighbors[2].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 1); // DownLeft

			break;

		case UnitDirection_Right:

			neighbors[0].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 0); // Right
			neighbors[1].create(singleUnit->currTile.x + 1, singleUnit->currTile.y - 1); // UpRight
			neighbors[2].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 1); // DownRight

			break;

		case UnitDirection_UpLeft:

			neighbors[0].create(singleUnit->currTile.x - 1, singleUnit->currTile.y - 1); // UpLeft
			neighbors[1].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 0); // Left
			neighbors[2].create(singleUnit->currTile.x + 0, singleUnit->currTile.y - 1); // Up

			break;

		case UnitDirection_UpRight:

			neighbors[0].create(singleUnit->currTile.x + 1, singleUnit->currTile.y - 1); // UpRight
			neighbors[1].create(singleUnit->currTile.x + 0, singleUnit->currTile.y - 1); // Up
			neighbors[2].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 0); // Right

			break;

		case UnitDirection_DownLeft:

			neighbors[0].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 1); // DownLeft
			neighbors[1].create(singleUnit->currTile.x + 0, singleUnit->currTile.y + 1); // Down
			neighbors[2].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 0); // Left

			break;

		case UnitDirection_DownRight:

			neighbors[0].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 1); // DownRight
			neighbors[1].create(singleUnit->currTile.x + 0, singleUnit->currTile.y + 1); // Down
			neighbors[2].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 0); // Right

			break;
		}
	}
	else {

		// Consider all tiles (the unit will be able to move backwards, then)
		neighbors[0].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 0);
		neighbors[1].create(singleUnit->currTile.x + 0, singleUnit->currTile.y + 1);
		neighbors[2].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 0);
		neighbors[3].create(singleUnit->currTile.x + 0, singleUnit->currTile.y - 1);
		neighbors[4].create(singleUnit->currTile.x + 1, singleUnit->currTile.y + 1);
		neighbors[5].create(singleUnit->currTile.x + 1, singleUnit->currTile.y - 1);
		neighbors[6].create(singleUnit->currTile.x - 1, singleUnit->currTile.y + 1);
		neighbors[7].create(singleUnit->currTile.x - 1, singleUnit->currTile.y - 1);
	}

	// 2. PRIORITY: the neighbor closer to the unit's goal
	priority_queue<iPointPriority, vector<iPointPriority>, Comparator> queue;
	iPointPriority priorityNeighbors;

	for (uint i = 0; i < 8; ++i)
	{
		priorityNeighbors.point = neighbors[i];
		priorityNeighbors.priority = neighbors[i].DistanceManhattan(singleUnit->goal); /// TODO: we could use the path size to track the priority, but it's too heavy...
		queue.push(priorityNeighbors);
	}
	
	iPointPriority curr;
	while (queue.size() > 0) {

		curr = queue.top();
		queue.pop();

		if (App->pathfinding->IsWalkable(curr.point) && IsValidTile(singleUnit, curr.point, true, true)) {

			/// TODO: be careful! currTile may be closer to the goal than the neighbor found!
			//if (curr.point.DistanceManhattan(unit->goal) < unit->currTile.DistanceManhattan(unit->goal))
			return curr.point;
			//else
				//return unit->currTile;
		}
	}

	return { -1,-1 };
}

iPoint j1Movement::FindNewValidGoal(SingleUnit* singleUnit) const
{
	if (singleUnit == nullptr)
		return { -1,-1 };

	// 1. We use BFS to calculate a new goal for the unit (we want to expand the search to all the possible tiles)
	// 2. PRIORITY: the neighbor closer to the group goal
	priority_queue<iPointPriority, vector<iPointPriority>, Comparator> priorityQueue;
	list<iPoint> visited;

	iPointPriority curr;
	curr.point = singleUnit->group->goal;
	curr.priority = curr.point.DistanceManhattan(singleUnit->group->goal);
	priorityQueue.push(curr);

	while (priorityQueue.size() > 0) {

		curr = priorityQueue.top();
		priorityQueue.pop();

		if (App->pathfinding->IsWalkable(curr.point) && IsValidTile(singleUnit, curr.point, false, false, true))
			return curr.point;

		iPoint neighbors[8];
		neighbors[0].create(curr.point.x + 1, curr.point.y + 0);
		neighbors[1].create(curr.point.x + 0, curr.point.y + 1);
		neighbors[2].create(curr.point.x - 1, curr.point.y + 0);
		neighbors[3].create(curr.point.x + 0, curr.point.y - 1);
		neighbors[4].create(curr.point.x + 1, curr.point.y + 1);
		neighbors[5].create(curr.point.x + 1, curr.point.y - 1);
		neighbors[6].create(curr.point.x - 1, curr.point.y + 1);
		neighbors[7].create(curr.point.x - 1, curr.point.y - 1);

		for (uint i = 0; i < 8; ++i)
		{
			if (find(visited.begin(), visited.end(), neighbors[i]) == visited.end()) {
				
				iPointPriority priorityNeighbors;
				priorityNeighbors.point = neighbors[i];
				priorityNeighbors.priority = neighbors[i].DistanceManhattan(singleUnit->group->goal);
				priorityQueue.push(priorityNeighbors);

				visited.push_back(neighbors[i]);
			}
		}
	}

	return { -1,-1 };
}

// UnitGroup struct ---------------------------------------------------------------------------------

UnitGroup::UnitGroup(Entity* entity) 
{
	AddUnit(entity);
}

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

	unit = new SingleUnit((Unit*)entity, this);
	units.push_back(unit);

	return unit;
}

bool UnitGroup::RemoveUnit(Entity* entity)
{
	bool ret = false;

	list<SingleUnit*>::const_iterator it = units.begin();

	while (it != units.end()) {

		if ((Entity*)(*it)->unit == entity) {
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
		if ((Entity*)(*it)->unit == entity)
			return *it;

		it++;
	}

	return nullptr;
}

bool UnitGroup::SetGoal(iPoint goal)
{
	bool ret = false;

	if (App->pathfinding->IsWalkable(goal)) {

		this->goal = goal;

		// Update all units' newGoal
		list<SingleUnit*>::const_iterator it = units.begin();

		while (it != units.end()) {
			(*it)->newGoal = goal;

			// If the unit didn't have a goal defined, update also its goal
			if ((*it)->goal.x == -1 && (*it)->goal.y == -1)
				(*it)->goal = goal;

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

float UnitGroup::GetMaxSpeed() const
{
	return maxSpeed;
}

// Unit struct ---------------------------------------------------------------------------------

SingleUnit::SingleUnit(Unit* unit, UnitGroup* group) :unit(unit), group(group)
{
	currTile = App->map->WorldToMap(this->unit->entityInfo.pos.x, this->unit->entityInfo.pos.y);
	speed = this->unit->entityInfo.speed;
	speed *= 50.0f; // MYTODO: delete this 50.0f magic number!!!

	goal = group->goal;
}

bool SingleUnit::CreatePath(iPoint startPos)
{
	bool ret = true;

	// If there is a valid goal:
	if (goal.x != -1 && goal.y != -1) {

		// Find a path
		if (App->pathfinding->CreatePath(startPos, goal, DISTANCE_MANHATTAN) == -1)
			ret = false;

		// Save the path found
		if (ret)
			path = *App->pathfinding->GetLastPath();
	}

	return ret;
}

bool SingleUnit::IsTileReached(iPoint nextPos, fPoint endPos) const 
{
	bool ret = false;

	fPoint dir = unit->GetUnitDirectionByValue();

	if (dir.x >= 0) { // Right or stop

		if (dir.y >= 0) { // Down or stop
			if (endPos.x >= nextPos.x && endPos.y >= nextPos.y)
				ret = true;
		}
		else { // Up
			if (endPos.x >= nextPos.x && endPos.y <= nextPos.y)
				ret = true;
		}
	}
	else { // Left

		if (dir.y >= 0) { // Down or stop
			if (endPos.x <= nextPos.x && endPos.y >= nextPos.y)
				ret = true;
		}
		else { // Up
			if (endPos.x <= nextPos.x && endPos.y <= nextPos.y)
				ret = true;
		}
	}

	return ret;
}

void SingleUnit::StopUnit()
{
	unit->SetUnitDirection(UnitDirection_Idle);
}