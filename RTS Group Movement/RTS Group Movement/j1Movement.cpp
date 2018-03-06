#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1Movement.h"
#include "j1EntityFactory.h"
#include "Entity.h"
#include "j1Map.h"
#include "j1Render.h"

#include"Brofiler\Brofiler.h"

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
				&& (*unit)->movementState != MovementState_WaitForPath && (*unit)->nextTile.x > -1 && (*unit)->nextTile.y > -1) {

				// Raycast a line between the unit and the nextTile
				iPoint offset = { App->map->data.tile_width / 2, App->map->data.tile_height / 2 };
				iPoint nextPos = App->map->MapToWorld((*unit)->nextTile.x, (*unit)->nextTile.y);
				App->render->DrawLine((*unit)->unit->entityInfo.pos.x + offset.x, (*unit)->unit->entityInfo.pos.y + offset.y, nextPos.x + offset.x, nextPos.y + offset.y, 255, 255, 255, 255);
				App->render->DrawCircle(nextPos.x + offset.x, nextPos.y + offset.y, 10, 255, 255, 255, 255);

				// Draw unit's path
				/*
				for (uint i = 0; i < (*unit)->path.size(); ++i)
				{
				iPoint pos = App->map->MapToWorld((*unit)->path.at(i).x, (*unit)->path.at(i).y);
				SDL_Rect rect = { pos.x, pos.y, App->map->data.tile_width, App->map->data.tile_height };
				App->render->DrawQuad(rect, 0, 255, 0, 50);
				}
				*/

				// Draw unit's goal (in a different color than the path)
				iPoint pos = App->map->MapToWorld((*unit)->goal.x, (*unit)->goal.y);
				SDL_Rect rect = { pos.x, pos.y, App->map->data.tile_width, App->map->data.tile_height };
				SDL_Color col = (*unit)->unit->GetColor();
				App->render->DrawQuad(rect, col.r, col.g, col.b, 200);
			}
		}
	}
}

bool j1Movement::CleanUp()
{
	bool ret = true;

	return ret;
}

UnitGroup* j1Movement::CreateGroupFromUnits(list<Unit*> units)
{
	list<Unit*>::const_iterator it = units.begin();
	UnitGroup* group = nullptr;

	while (it != units.end()) {

		// If a unit from the list belongs to an existing group, delete the unit from the group
		group = GetGroupByUnit(*it);

		if (group != nullptr) {
			group->RemoveUnit((*it)->GetSingleUnit());

			// If the group is empty, delete it
			if (group->GetSize() == 0)
				unitGroups.remove(group);
		}

		it++;
	}

	group = new UnitGroup(units);
	unitGroups.push_back(group);

	return group;
}

UnitGroup* j1Movement::CreateGroupFromUnit(Unit* unit)
{
	// If a unit from the list belongs to an existing group, delete the unit from the group
	UnitGroup* group = nullptr;
	group = GetGroupByUnit(unit);

	if (group != nullptr) {
		group->RemoveUnit(unit->GetSingleUnit());

		// If the group is empty, delete it
		if (group->GetSize() == 0)
			unitGroups.remove(group);
	}

	group = new UnitGroup(unit);
	unitGroups.push_back(group);

	return group;
}

UnitGroup* j1Movement::GetLastGroup() const
{
	return unitGroups.back();
}

UnitGroup* j1Movement::GetGroupByUnit(Unit* unit) const
{
	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;
	UnitGroup* group = nullptr;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {
		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {
			if ((*units)->unit == unit) {
				group = *groups;
				break;
			}
		}
	}

	return group;
}

UnitGroup* j1Movement::GetGroupByUnits(list<Unit*> units) const
{
	list<Unit*>::const_iterator it;
	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator singleUnits;
	uint size = 0;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {

		for (singleUnits = (*groups)->units.begin(); singleUnits != (*groups)->units.end(); ++singleUnits) {

			for (it = units.begin(); it != units.end(); ++it) {

				if ((*singleUnits)->unit == *it)
					size++;
			}
		}

		if (size == units.size() && size == (*groups)->GetSize())
			return *groups;

		size = 0;
	}

	return nullptr;
}

MovementState j1Movement::MoveUnit(Unit* unit, float dt) const
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	MovementState ret = MovementState_NoState;

	UnitGroup* g = GetGroupByUnit(unit);

	if (g == nullptr)
		return MovementState_NoState;

	SingleUnit* u = unit->GetSingleUnit();

	if (u == nullptr)
		return MovementState_NoState;

	ret = u->movementState;

	iPoint nextPos = App->map->MapToWorld(u->nextTile.x, u->nextTile.y); // unit nextPos in map coords
	fPoint movePos, endPos;
	float m;
	CollisionType coll;
	iPoint changedGoal;

	/// For each step:
	switch (u->movementState) {

	case MovementState_WaitForPath:

		// Check if the goal is valid. Valid means that it isn't the goal of another unit and that it isn't { -1,-1 }
		if (!IsValidTile(u, u->goal, false, false, true))

			// If the goal is not valid, find a new goal
			u->goal = u->newGoal = FindNewValidGoal(u);

		// Check if the new goal is valid. Valid means that it isn't { -1,-1 }
		if (IsValidTile(nullptr, u->goal))

			// If the goal is valid, find a path
			if (u->CreatePath(u->currTile))

				// Set state to IncreaseWaypoint, in order to start following the path
				u->movementState = MovementState_IncreaseWaypoint;

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

		CheckForFutureCollision(u);

		// Treat the collision
		if (u->collision != CollisionType_NoCollision) {

			u->StopUnit();

			// If the waitTile is the waitUnit's goal, the tile will never be available (unless the waitUnit changes its goal)
			// In this case, find a new, valid nextTile

			if (u->waitUnit != nullptr) {
				if (u->nextTile == u->waitUnit->goal && u->waitUnit->currTile == u->waitUnit->goal) {

					// Unit politely asks the other unit to move

					// If the unit wants to go on the other unit's goal tile
					/*
					if (u->goal == u->waitUnit->goal)

						// The other unit must find a new goal to go
						u->waitUnit->newGoal = FindNewValidGoal(u->waitUnit);
					else
					*/

					LOG("CHANGED GOALS!");
					
					changedGoal = u->waitUnit->goal;

					u->waitUnit->goal = u->waitUnit->newGoal = u->goal;
					u->waitUnit->movementState = MovementState_WaitForPath;

					u->goal = u->newGoal = changedGoal;
					u->movementState = MovementState_WaitForPath;

					if (!u->waitUnit->wakeUp)
						u->waitUnit->wakeUp = true;
					
					break;
				}
			}

			if (u->collision == CollisionType_ItsCell) {

				if (u->waitUnit != nullptr)
					if (u->waitUnit->currTile != u->waitTile) {

						u->collision = CollisionType_NoCollision;
						u->wait = false;

						LOG("%s: RESOLVED ITS CELL", u->unit->GetColorName().data());
					}
			}
			else if (u->collision == CollisionType_SameCell) {

				if (u->waitUnit != nullptr)
					if (u->waitUnit->nextTile != u->waitTile) {

						u->collision = CollisionType_NoCollision;
						u->wait = false;

						LOG("%s: RESOLVED SAME CELL", u->unit->GetColorName().data());
					}
			}
			else if (u->collision == CollisionType_DiagonalCrossing) {

				if (u->waitUnit != nullptr)
					if (u->waitUnit->currTile == u->waitTile) {

						u->collision = CollisionType_NoCollision;
						u->wait = false;

						LOG("%s: RESOLVED CROSSING", u->unit->GetColorName().data());
					}
			}
			else if (u->collision == CollisionType_TowardsCell) {

				// Units get stuck. Find a new, valid nextTile for one of them

				if (ChangeNextTile(u))
					LOG("%s: RESOLVED TOWARDS", u->unit->GetColorName().data());
				else
					// WHAT TO DO IF NO TILE HAS BEEN FOUND? STOP THE UNIT!
					LOG("%s: Couldn't find newTile", u->unit->GetColorName().data());
			}

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

			// Notice that we update the currTile here!
			u->currTile = u->nextTile;

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

bool j1Movement::ChangeNextTile(SingleUnit* singleUnit) const
{
	bool ret = false;

	// 1. Check only tiles in front of the unit (3)
	iPoint newTile = FindNewValidTile(singleUnit, true);

	if (newTile.x != -1 && newTile.y != -1) {

		// Update the unit's nextTile
		singleUnit->nextTile = newTile;

		// Recalculate the path
		singleUnit->CreatePath(singleUnit->nextTile);

		singleUnit->collision = CollisionType_NoCollision;
		singleUnit->wait = false;

		ret = true;
	}
	else {

		// 2. Check all possible tiles (8)
		newTile = FindNewValidTile(singleUnit);

		if (newTile.x != -1 && newTile.y != -1) {

			// Update the unit's nextTile
			singleUnit->nextTile = newTile;

			// Recalculate the path
			singleUnit->CreatePath(singleUnit->nextTile);

			singleUnit->collision = CollisionType_NoCollision;
			singleUnit->wait = false;

			ret = true;
		}
	}

	return ret;
}

void j1Movement::CheckForFutureCollision(SingleUnit* singleUnit) const
{
	/// We don't check the walkability of the tile since the A* algorithm already did it for us

	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {
		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {

			if ((*units) != singleUnit) {

				// TOWARDS COLLISION
				if (singleUnit->nextTile == (*units)->currTile && (*units)->nextTile == singleUnit->currTile && (*units)->collision != CollisionType_TowardsCell) {

					if (IsOppositeDirection(singleUnit, (*units))) {

						// Decide which unit waits (depending on its priority value)
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = singleUnit->nextTile;

						/// DRAWING A: Here we don't check if the unit is already waiting. The unit can be waiting because of a previous check with another unit
						/// But the important collision, above any else, is this. So if in a previous check with another unit, the unit has been detected
						/// another kind of collision, it should have wait in true. We don't care and we subscribe the collision:)
						/// We delete the check if (!singleUnit->wait)

						singleUnit->collision = CollisionType_TowardsCell;
						singleUnit->wait = true;

						LOG("%s: TOWARDS", singleUnit->unit->GetColorName().data());
					}
				}

				if (singleUnit->collision != CollisionType_TowardsCell && (*units)->collision != CollisionType_TowardsCell) {

					// ITS CELL
					if (singleUnit->nextTile == (*units)->currTile) {

						// A reaches B's tile
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = singleUnit->nextTile;

						if (!singleUnit->wait) {
							singleUnit->collision = CollisionType_ItsCell;
							singleUnit->wait = true;

							LOG("%s: ITS CELL", singleUnit->unit->GetColorName().data());
						}
					}

					// SAME CELL
					else if (singleUnit->nextTile == (*units)->nextTile && !(*units)->wait) {

						// A and B reach the same tile
						UnitDirection dirA = singleUnit->unit->GetUnitDirection();
						UnitDirection dirB = (*units)->unit->GetUnitDirection();

						singleUnit->waitUnit = *units;
						singleUnit->waitTile = singleUnit->nextTile;

						// The agent that is already inside the tile has the priority. If none of them are, choose one randomly
						SDL_Rect posA = { (int)singleUnit->unit->entityInfo.pos.x, (int)singleUnit->unit->entityInfo.pos.y, App->map->data.tile_width, App->map->data.tile_height };
						SDL_Rect posB = { (int)(*units)->unit->entityInfo.pos.x, (int)(*units)->unit->entityInfo.pos.y,  App->map->data.tile_width, App->map->data.tile_height };
						iPoint tilePos = App->map->MapToWorld(singleUnit->nextTile.x, singleUnit->nextTile.y);
						SDL_Rect tile = { tilePos.x, tilePos.y, App->map->data.tile_width, App->map->data.tile_height };
						SDL_Rect resultA, resultB;

						SDL_IntersectRect(&posA, &tile, &resultA);
						SDL_IntersectRect(&posB, &tile, &resultB);

						// Compare the areas of the two intersections
						int areaA = resultA.w * resultA.h;
						int areaB = resultB.w * resultB.h;

						// Decide which unit waits
						// The unit who has the smaller area waits
						if (areaA <= areaB) {
							if (!singleUnit->wait) {
								singleUnit->collision = CollisionType_SameCell;
								singleUnit->wait = true;

								LOG("%s: SAME CELL", singleUnit->unit->GetColorName().data());
							}
						}
						else {
							(*units)->collision = CollisionType_SameCell;
							(*units)->wait = true;

							LOG("%s: SAME CELL", (*units)->unit->GetColorName().data());
						}
					}

					// DIAGONAL CROSSING
					else {

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

							// We are sure than nextTile of this unit is valid, but what about the other unit's nextTile? We haven't check it yet
							/// DRAWING B: IsValidTile(nullptr, (*units)->nextTile, true) we need it because the other unit may not be waiting yet (update order),
							/// but will wait when it is its turn this update. Its nextTile may be occupied, whereas the current unit nextTile is not.
							/// If this is the case, when checking for collisions for the other unit, it would give us an ItsCell or SameCell collision, probably...

							if ((*units)->nextTile == myUp && !(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

								// Decide which unit waits (depending on its priority value)
								singleUnit->waitUnit = *units;
								singleUnit->waitTile = myUp;

								if (!singleUnit->wait) {
									singleUnit->collision = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									LOG("%s: CROSSING", singleUnit->unit->GetColorName().data());
								}
							}
						}
						else if (singleUnit->nextTile == down) {
							if ((*units)->nextTile == myDown && !(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

								// Decide which unit waits (depending on its priority value)
								singleUnit->waitUnit = *units;
								singleUnit->waitTile = myDown;

								if (!singleUnit->wait) {
									singleUnit->collision = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									LOG("%s: CROSSING", singleUnit->unit->GetColorName().data());
								}
							}
						}
						else if (singleUnit->nextTile == left) {
							if ((*units)->nextTile == myLeft && !(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

								// Decide which unit waits (depending on its priority value)
								singleUnit->waitUnit = *units;
								singleUnit->waitTile = myLeft;

								if (!singleUnit->wait) {
									singleUnit->collision = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									LOG("%s: CROSSING", singleUnit->unit->GetColorName().data());
								}
							}
						}
						else if (singleUnit->nextTile == right) {
							if ((*units)->nextTile == myRight && !(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

								// Decide which unit waits (depending on its priority value)
								singleUnit->waitUnit = *units;
								singleUnit->waitTile = myRight;

								if (!singleUnit->wait) {
									singleUnit->collision = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									LOG("%s: CROSSING", singleUnit->unit->GetColorName().data());
								}
							}
						}
					}
				}
			}
		}
	}
}

bool j1Movement::IsValidTile(SingleUnit* singleUnit, iPoint tile, bool currTile, bool nextTile, bool goalTile) const
{
	if (tile.x == -1 && tile.y == -1)
		return false;

	if (!currTile && !nextTile && !goalTile)
		return true;

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
	iPoint neighbors[8] = { { -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 } };

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

bool j1Movement::IsOppositeDirection(SingleUnit* singleUnitA, SingleUnit* singleUnitB) const
{
	UnitDirection dirA = singleUnitA->unit->GetUnitDirection();
	UnitDirection dirB = singleUnitB->unit->GetUnitDirection();

	switch (dirA) {

	case UnitDirection_Up:

		if (dirB == UnitDirection_Down)
			return true;

		break;

	case UnitDirection_Down:

		if (dirB == UnitDirection_Up)
			return true;

		break;

	case UnitDirection_Left:

		if (dirB == UnitDirection_Right)
			return true;

		break;

	case UnitDirection_Right:

		if (dirB == UnitDirection_Left)
			return true;

		break;

	case UnitDirection_UpLeft:

		if (dirB == UnitDirection_DownRight)
			return true;

		break;

	case UnitDirection_UpRight:

		if (dirB == UnitDirection_DownLeft)
			return true;

		break;

	case UnitDirection_DownLeft:

		if (dirB == UnitDirection_UpRight)
			return true;

		break;

	case UnitDirection_DownRight:

		if (dirB == UnitDirection_UpLeft)
			return true;

		break;
	}
}

// UnitGroup struct ---------------------------------------------------------------------------------

UnitGroup::UnitGroup(Unit* unit)
{
	AddUnit(unit->GetSingleUnit());
}

UnitGroup::UnitGroup(list<Unit*> units)
{
	list<Unit*>::const_iterator it = units.begin();

	while (it != units.end()) {

		AddUnit((*it)->GetSingleUnit());
		it++;
	}
}

bool UnitGroup::AddUnit(SingleUnit* singleUnit)
{
	bool ret = false;

	ret = IsUnitInGroup(singleUnit);

	// If the unit is not in the group, add it
	if (!ret) {

		singleUnit->group = this;

		units.push_back(singleUnit);

		ret = true;
	}

	return ret;
}

bool UnitGroup::RemoveUnit(SingleUnit* singleUnit)
{
	bool ret = false;

	list<SingleUnit*>::const_iterator it = find(units.begin(), units.end(), singleUnit);

	if (it != units.end()) {

		// Set the unit group to nullptr
		singleUnit->group = nullptr;

		// Remove the unit from the group
		units.remove(*it);
		ret = true;
	}

	return ret;
}

bool UnitGroup::IsUnitInGroup(SingleUnit* singleUnit) const
{
	bool ret = false;

	if (find(units.begin(), units.end(), singleUnit) != units.end())
		ret = true;

	return ret;
}

uint UnitGroup::GetSize() const
{
	return units.size();
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
	currTile = goal = newGoal = App->map->WorldToMap(this->unit->entityInfo.pos.x, this->unit->entityInfo.pos.y);
	speed = this->unit->entityInfo.speed;
	speed *= 50.0f; // MYTODO: delete this 50.0f magic number!!!

	priority = unit->unitInfo.priority;
}

bool SingleUnit::CreatePath(iPoint startPos)
{
	bool ret = true;

	// Find a path
	if (App->pathfinding->CreatePath(startPos, goal, DISTANCE_MANHATTAN) == -1)
		ret = false;

	// Save the path found
	if (ret)
		path = *App->pathfinding->GetLastPath();

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