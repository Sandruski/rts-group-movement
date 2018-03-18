#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1Movement.h"
#include "j1EntityFactory.h"
#include "Entity.h"
#include "j1Map.h"
#include "j1Render.h"
#include "j1Scene.h"
#include "j1PathManager.h"

#include "Brofiler\Brofiler.h"

#include <queue>

j1Movement::j1Movement() {}

j1Movement::~j1Movement() {}

bool j1Movement::Update(float dt)
{
	bool ret = true;

	if (App->scene->debugDrawMovement)
		DebugDraw();

	return ret;
}

void j1Movement::DebugDraw() const
{
	for (list<UnitGroup*>::const_iterator group = unitGroups.begin(); group != unitGroups.end(); ++group) {
		for (list<SingleUnit*>::const_iterator unit = (*group)->units.begin(); unit != (*group)->units.end(); ++unit) {

			if ((*unit)->movementState != MovementState_NoState && (*unit)->movementState != MovementState_GoalReached) {

				SDL_Color col = (*unit)->unit->GetColor();

				if ((*unit)->movementState != MovementState_WaitForPath && (*unit)->nextTile.x > -1 && (*unit)->nextTile.y > -1) {

					// Raycast a line between the unit and the nextTile
					iPoint offset = { App->map->data.tile_width / 2, App->map->data.tile_height / 2 };
					iPoint nextPos = App->map->MapToWorld((*unit)->nextTile.x, (*unit)->nextTile.y);
					App->render->DrawLine((*unit)->unit->entityInfo.pos.x + offset.x, (*unit)->unit->entityInfo.pos.y + offset.y, nextPos.x + offset.x, nextPos.y + offset.y, 255, 255, 255, 255);
					App->render->DrawCircle(nextPos.x + offset.x, nextPos.y + offset.y, 10, 255, 255, 255, 255);

					// Draw unit's path
					if (App->scene->debugDrawPath) {

						for (uint i = 0; i < (*unit)->path.size(); ++i)
						{
							iPoint pos = App->map->MapToWorld((*unit)->path.at(i).x, (*unit)->path.at(i).y);
							SDL_Rect rect = { pos.x, pos.y, App->map->data.tile_width, App->map->data.tile_height };
							App->render->DrawQuad(rect, col.r, col.g, col.b, 50);
						}
					}
				}

				// Draw unit's goal
				iPoint pos = App->map->MapToWorld((*unit)->goal.x, (*unit)->goal.y);
				SDL_Rect rect = { pos.x, pos.y, App->map->data.tile_width, App->map->data.tile_height };
				App->render->DrawQuad(rect, col.r, col.g, col.b, 200);
			}
		}
	}
}

bool j1Movement::CleanUp()
{
	bool ret = true;

	list<UnitGroup*>::const_iterator it = unitGroups.begin();

	while (it != unitGroups.end()) {
		delete *it;
		it++;
	}
	unitGroups.clear();

	return ret;
}

// Creates a group from a list of units. Returns the pointer to the group created or nullptr
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

// Creates a group from a single unit. Returns the pointer to the group created or nullptr
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

// Returns the last group created or nullptr
UnitGroup* j1Movement::GetLastGroup() const
{
	return unitGroups.back();
}

// Returns an existing group by a pointer to one of its units or nullptr
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

// Returns an existing group by a list to all of its units or nullptr
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

void SingleUnit::GetReadyForNewMove()
{
	iPoint currTilePos = App->map->MapToWorld(currTile.x, currTile.y);

	if ((int)unit->entityInfo.pos.x == currTilePos.x && (int)unit->entityInfo.pos.y == currTilePos.y) {

		ResetUnitVariables();
		StopUnit();
		movementState = MovementState_WaitForPath;

		isGoalChanged = false;
	}
}

void SingleUnit::ResetUnitCollisionVariables()
{
	coll = CollisionType_NoCollision;
	wait = false;

	reversePriority = false;
}

void SingleUnit::WakeUp() 
{
	if (!wakeUp)
		wakeUp = true;
}

bool j1Movement::IsAnyUnitDoingSomething(SingleUnit* singleUnit, bool isSearching) const
{
	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {
		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {

			if ((*units) != singleUnit) {

				if (isSearching) {
					if ((*units)->isSearching)
						return true;
				}
			}
		}
	}

	return false;
}

// Moves a unit applying the principles of group movement. Returns the state of the unit's movement
// - Call this method from a unit's update
MovementState j1Movement::MoveUnit(Unit* unit, float dt)
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	if (GetGroupByUnit(unit) == nullptr)
		return MovementState_NoState;

	SingleUnit* singleUnit = unit->GetSingleUnit();

	if (singleUnit == nullptr)
		return MovementState_NoState;

	// ---------------------------------------------------------------------

	iPoint nextPos = App->map->MapToWorld(singleUnit->nextTile.x, singleUnit->nextTile.y);
	fPoint movePos;
	iPoint newGoal;

	// HAS THE GOAL BEEN CHANGED?
	if (singleUnit->isGoalChanged)

		singleUnit->GetReadyForNewMove();

	switch (singleUnit->movementState) {

	case MovementState_WaitForPath:

		// The goal of a unit cannot be the goal of another unit
		if (!IsValidTile(singleUnit, singleUnit->goal, false, false, true)) {

			// Only one unit at a time can change its goal
			if (!IsAnyUnitDoingSomething(singleUnit, true)) {

				singleUnit->unit->pathPlanner->RequestDijkstra(singleUnit->group->goal, FindActiveTrigger::ActiveTriggerType_Goal);

				singleUnit->isSearching = true; /// The unit is changing its goal

				// ***IS THE TILE READY?***
				if (singleUnit->unit->pathPlanner->IsSearchCompleted()) {

					singleUnit->goal = singleUnit->unit->pathPlanner->GetTile();
					singleUnit->unit->pathPlanner->SetSearchRequested(false);

					singleUnit->isSearching = false; /// The unit has finished changing its goal

					break;
				}
				break;
			}
			break;
		}
		else {

			// Request a new path for the unit
			singleUnit->unit->pathPlanner->RequestAStar(singleUnit->currTile, singleUnit->goal);

			// ***IS THE PATH READY?***
			if (singleUnit->unit->pathPlanner->IsSearchCompleted()) {

				singleUnit->path = singleUnit->unit->pathPlanner->GetPath();
				singleUnit->unit->pathPlanner->SetSearchRequested(false);

				singleUnit->movementState = MovementState_IncreaseWaypoint;

				break;
			}
			break;
		}

		break;

	case MovementState_FollowPath:

		// ---------------------------------------------------------------------
		// MOVEMENT CALCULATION
		// ---------------------------------------------------------------------

		// Calculate the difference between the nextPos and the current position of the unit and store the result inside the temporary iPoint called movePos
		movePos = { (float)nextPos.x - singleUnit->unit->entityInfo.pos.x, (float)nextPos.y - singleUnit->unit->entityInfo.pos.y };

		// Normalize the movePos. It should be in the interval [-1,1]
		{
			float m = sqrtf(pow(movePos.x, 2.0f) + pow(movePos.y, 2.0f));

			if (m > 0.0f) {
				movePos.x /= m;
				movePos.y /= m;
			}
		}

		// Update the direction of the unit with the normalized movePos
		singleUnit->unit->SetUnitDirectionByValue(movePos);

		// Apply the speed and the dt to the movePos
		movePos.x *= singleUnit->speed * dt;
		movePos.y *= singleUnit->speed * dt;

		// ---------------------------------------------------------------------
		// COLLISION CALCULATION
		// ---------------------------------------------------------------------

		CheckForFutureCollision(singleUnit);

		if (singleUnit->coll != CollisionType_NoCollision) {

			singleUnit->StopUnit();

			if (singleUnit->waitUnit == nullptr)
				break;

			// ---------------------------------------------------------------------
			// SPECIAL CASE
			// ---------------------------------------------------------------------

			// The waitUnit is still on its goal
			if (singleUnit->nextTile == singleUnit->waitUnit->goal && singleUnit->waitUnit->currTile == singleUnit->waitUnit->goal) {

				// Wake up the unit (it may not be in the UnitState_Walk state)
				singleUnit->waitUnit->WakeUp();

				// The unit with the higher priority politely asks the other unit to move
				if (singleUnit->priority >= singleUnit->waitUnit->priority || singleUnit->reversePriority) {

					// 1. waitUnit moves
					singleUnit->waitUnit->checkEverything = true;
					singleUnit->waitUnit->unit->pathPlanner->RequestDijkstra(singleUnit->waitUnit->goal, FindActiveTrigger::ActiveTriggerType_Goal);

					// ***IS THE TILE READY?***
					if (singleUnit->waitUnit->unit->pathPlanner->IsSearchCompleted()) {

						singleUnit->waitUnit->goal = singleUnit->waitUnit->unit->pathPlanner->GetTile();
						singleUnit->waitUnit->unit->pathPlanner->SetSearchRequested(false);

						singleUnit->waitUnit->movementState = MovementState_WaitForPath;

						/// COLLISION RESOLVED
						singleUnit->ResetUnitCollisionVariables();

						if (singleUnit->unit->isSelected)
							LOG("%s: MOVED AWAY %s", singleUnit->unit->GetColorName().data(), singleUnit->waitUnit->unit->GetColorName().data());

						break;
					}
					break;
				}
				else {

					// 2. Current unit moves
					if (!singleUnit->isSearching) {

						// 1. Check only tiles in front of the unit (3)
						iPoint newTile = FindNewValidTile(singleUnit, true);

						if (newTile.x != -1 && newTile.y != -1) {

							// Request a new path for the unit
							singleUnit->unit->pathPlanner->RequestAStar(newTile, singleUnit->goal);

							singleUnit->isSearching = true; /// The unit is changing its nextTile

							break;
						}
						else {

							// 2. Check all possible tiles (8)
							newTile = FindNewValidTile(singleUnit);

							if (newTile.x != -1 && newTile.y != -1) {

								// Request a new path for the unit
								singleUnit->unit->pathPlanner->RequestAStar(newTile, singleUnit->goal);

								singleUnit->isSearching = true; /// The unit is changing its nextTile

								break;
							}
						}
					}

					// IS THE UNIT REALLY CHANGING ITS NEXTTILE?
					if (singleUnit->isSearching) {

						// ***IS THE PATH READY?***
						if (singleUnit->unit->pathPlanner->IsSearchCompleted()) {

							singleUnit->path = singleUnit->unit->pathPlanner->GetPath();

							singleUnit->isSearching = false;/// The unit has finished changing its nextTile

							// Update the unit's nextTile
							singleUnit->nextTile = singleUnit->path.front();

							/// COLLISION RESOLVED
							singleUnit->ResetUnitCollisionVariables();

							if (singleUnit->unit->isSelected)
								LOG("%s: MOVED AWAY %s", singleUnit->waitUnit->unit->GetColorName().data(), singleUnit->unit->GetColorName().data());

							break;
						}
						break;
					}
					else {

						// If the unit cannot change its nextTile, ask the waitUnit to move
						singleUnit->reversePriority = true;

						if (singleUnit->unit->isSelected)
							LOG("%s: reversed its priority", singleUnit->waitUnit->unit->GetColorName().data());

						break;
					}
					break;
				}
				break;
			}

			// ---------------------------------------------------------------------
			// NORMAL CASES
			// ---------------------------------------------------------------------

			if (singleUnit->coll == CollisionType_ItsCell) {

				if (singleUnit->waitUnit->currTile != singleUnit->waitTile) {

					singleUnit->ResetUnitCollisionVariables();

					if (singleUnit->unit->isSelected)
						LOG("%s: RESOLVED ITS CELL", singleUnit->unit->GetColorName().data());

					break;
				}
				break;
			}
			else if (singleUnit->coll == CollisionType_SameCell) {

				if (singleUnit->waitUnit->nextTile != singleUnit->waitTile) {

					singleUnit->ResetUnitCollisionVariables();

					if (singleUnit->unit->isSelected)
						LOG("%s: RESOLVED SAME CELL", singleUnit->unit->GetColorName().data());

					break;
				}
				break;
			}
			else if (singleUnit->coll == CollisionType_DiagonalCrossing) {

				if (singleUnit->waitUnit->currTile == singleUnit->waitTile) {

					singleUnit->ResetUnitCollisionVariables();

					if (singleUnit->unit->isSelected)
						LOG("%s: RESOLVED CROSSING", singleUnit->unit->GetColorName().data());

					break;
				}
				break;
			}
			else if (singleUnit->coll == CollisionType_TowardsCell) {

				/*
				// The unit with the higher priority politely asks the other unit to move
				if (singleUnit->priority >= singleUnit->waitUnit->priority || singleUnit->reversePriority) {

					// 1. waitUnit moves
					singleUnit->waitUnit->checkEverything = true;
					singleUnit->waitUnit->unit->pathPlanner->RequestDijkstra(singleUnit->waitUnit->goal, FindActiveTrigger::ActiveTriggerType_Goal);

					// ***IS THE TILE READY?***
					if (singleUnit->waitUnit->unit->pathPlanner->IsSearchCompleted()) {

						singleUnit->waitUnit->goal = singleUnit->waitUnit->unit->pathPlanner->GetTile();
						singleUnit->waitUnit->unit->pathPlanner->SetSearchRequested(false);

						singleUnit->waitUnit->movementState = MovementState_WaitForPath;

						/// COLLISION RESOLVED
						singleUnit->ResetUnitCollisionVariables();

						if (singleUnit->unit->isSelected)
							LOG("%s: MOVED AWAY %s", singleUnit->unit->GetColorName().data(), singleUnit->waitUnit->unit->GetColorName().data());

						break;
					}
					break;
				}
				else {

					// 2. Current unit moves
					if (!singleUnit->isSearching) {

						// 1. Check only tiles in front of the unit (3)
						iPoint newTile = FindNewValidTile(singleUnit, true);

						if (newTile.x != -1 && newTile.y != -1) {

							// Request a new path for the unit
							singleUnit->unit->pathPlanner->RequestAStar(newTile, singleUnit->goal);

							singleUnit->isSearching = true; /// The unit is changing its nextTile

							break;
						}
						else {

							// 2. Check all possible tiles (8)
							newTile = FindNewValidTile(singleUnit);

							if (newTile.x != -1 && newTile.y != -1) {

								// Request a new path for the unit
								singleUnit->unit->pathPlanner->RequestAStar(newTile, singleUnit->goal);

								singleUnit->isSearching = true; /// The unit is changing its nextTile

								break;
							}
						}
					}

					// IS THE UNIT REALLY CHANGING ITS NEXTTILE?
					if (singleUnit->isSearching) {

						// ***IS THE PATH READY?***
						if (singleUnit->unit->pathPlanner->IsSearchCompleted()) {

							singleUnit->path = singleUnit->unit->pathPlanner->GetPath();

							singleUnit->isSearching = false;/// The unit has finished changing its nextTile

															// Update the unit's nextTile
							singleUnit->nextTile = singleUnit->path.front();

							/// COLLISION RESOLVED
							singleUnit->ResetUnitCollisionVariables();

							if (singleUnit->unit->isSelected)
								LOG("%s: MOVED AWAY %s", singleUnit->waitUnit->unit->GetColorName().data(), singleUnit->unit->GetColorName().data());

							break;
						}
						break;
					}
					else {

						// If the unit cannot change its nextTile, ask the waitUnit to move
						singleUnit->reversePriority = true;

						if (singleUnit->unit->isSelected)
							LOG("%s: reversed its priority", singleUnit->waitUnit->unit->GetColorName().data());

						break;
					}
					break;
				}
				*/

				if (ChangeNextTile(singleUnit)) {

					singleUnit->ResetUnitCollisionVariables();

					if (singleUnit->unit->isSelected)
						LOG("%s: RESOLVED TOWARDS", singleUnit->unit->GetColorName().data());

					break;
				}
				else {

					// If this unit can't move away, the other unit must stop

					if (singleUnit->unit->isSelected)
						LOG("%s: Couldn't find newTile", singleUnit->unit->GetColorName().data());

					break;
				}

				break;
			}
			break;
		}

		// ---------------------------------------------------------------------
		// TILE FITTING
		// ---------------------------------------------------------------------

		{
			// Predict where the unit will be after moving and store the result it inside the temporary fPoint called endPos
			fPoint endPos = { singleUnit->unit->entityInfo.pos.x + movePos.x, singleUnit->unit->entityInfo.pos.y + movePos.y };

			// Check if the unit would reach the nextTile during this move. If the answer is yes, then:
			if (singleUnit->IsTileReached(nextPos, endPos)) {

				// Update the unit's position with the nextPos
				singleUnit->unit->entityInfo.pos.x = nextPos.x;
				singleUnit->unit->entityInfo.pos.y = nextPos.y;

				// Update the unit's currTile with the nextTile
				singleUnit->currTile = singleUnit->nextTile;

				// Set the unit's movement state to IncreaseWaypoint
				singleUnit->movementState = MovementState_IncreaseWaypoint;

				break;
			}
		}

		// Add the movePos to the unit's current position
		singleUnit->unit->entityInfo.pos.x += movePos.x;
		singleUnit->unit->entityInfo.pos.y += movePos.y;

		break;

	case MovementState_IncreaseWaypoint:
		
		// If the unit's path contains waypoints:
		if (singleUnit->path.size() > 0) {

			// Get the next waypoint to head to
			singleUnit->nextTile = singleUnit->path.front();
			singleUnit->path.erase(singleUnit->path.begin());

			singleUnit->movementState = MovementState_FollowPath;
		}
		else
			// If the unit's path is out of waypoints, it means that the unit has reached the goal
			singleUnit->movementState = MovementState_GoalReached;

		break;

	case MovementState_GoalReached:
	case MovementState_NoState:
	default:

		// The unit is still
		singleUnit->StopUnit();

		break;
	}

	return singleUnit->movementState;
}

// Checks for future collisions between the current unit and the rest of the units
// - Sets the collision of the unit with the type of collision found or CollisionType_NoCollision
void j1Movement::CheckForFutureCollision(SingleUnit* singleUnit) const
{
	/// We don't check the walkability of the tile since the A* algorithm already did it for us

	list<UnitGroup*>::const_iterator groups;
	list<SingleUnit*>::const_iterator units;

	for (groups = unitGroups.begin(); groups != unitGroups.end(); ++groups) {
		for (units = (*groups)->units.begin(); units != (*groups)->units.end(); ++units) {

			if ((*units) != singleUnit) {

				// TOWARDS COLLISION
				// TODO 5a: Check if two units would reach the tile of each other. Make sure that the collision would be frontal!

				if (singleUnit->nextTile == (*units)->currTile && (*units)->nextTile == singleUnit->currTile && (*units)->coll != CollisionType_TowardsCell) {

					if (singleUnit->priority >= (*units)->priority) {

						(*units)->waitUnit = singleUnit;
						(*units)->waitTile = (*units)->nextTile;

						(*units)->coll = CollisionType_TowardsCell;
						(*units)->wait = true;

						if ((*units)->unit->isSelected)
							LOG("%s: TOWARDS", (*units)->unit->GetColorName().data());
					}
					else {
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = singleUnit->nextTile;

						singleUnit->coll = CollisionType_TowardsCell;
						singleUnit->wait = true;

						if (singleUnit->unit->isSelected)
							LOG("%s: TOWARDS", singleUnit->unit->GetColorName().data());
					}
				}

				if (singleUnit->coll != CollisionType_TowardsCell && (*units)->coll != CollisionType_TowardsCell) {

					// ITS CELL. A reaches B's tile
					// TODO 5b: Check if the unit would reach another unit's tile

					if (singleUnit->nextTile == (*units)->currTile) {
						
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = singleUnit->nextTile;

						if (!singleUnit->wait) {
							singleUnit->coll = CollisionType_ItsCell;
							singleUnit->wait = true;

							if (singleUnit->unit->isSelected)
								LOG("%s: ITS CELL", singleUnit->unit->GetColorName().data());
						}
					}

					// SAME CELL. A and B reach the same tile
					// TODO 5c: Check if two units would reach the same tile

					else if (singleUnit->nextTile == (*units)->nextTile && !(*units)->wait) {
						
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
								singleUnit->coll = CollisionType_SameCell;
								singleUnit->wait = true;

								if (singleUnit->unit->isSelected)
									LOG("%s: SAME CELL", singleUnit->unit->GetColorName().data());
							}
						}
						else {
							(*units)->coll = CollisionType_SameCell;
							(*units)->wait = true;

							if ((*units)->unit->isSelected)
								LOG("%s: SAME CELL", (*units)->unit->GetColorName().data());
						}
						
					}

					// DIAGONAL CROSSING
					
					else {

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
							if ((*units)->nextTile == myUp && !(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

								// Decide which unit waits (depending on its priority value)
								singleUnit->waitUnit = *units;
								singleUnit->waitTile = myUp;

								if (!singleUnit->wait) {
									singleUnit->coll = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									if (singleUnit->unit->isSelected)
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
									singleUnit->coll = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									if (singleUnit->unit->isSelected)
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
									singleUnit->coll = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									if (singleUnit->unit->isSelected)
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
									singleUnit->coll = CollisionType_DiagonalCrossing;
									singleUnit->wait = true;

									if (singleUnit->unit->isSelected)
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

// Returns true if a tile is valid
// - Checks for all the units (except for the unit passed as an argument) if a unit's tile (currTile, nextTile or goalTile) matches the tile passed as an argument
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

// Returns a new valid tile for the unit or { -1,-1 }
// - If checkOnlyFront is true, it only checks the three tiles that are in front of the unit passed as an argument
// - If checkOnlyFront is false, it checks the eight tiles surrounding the unit passed as an argument
// - The new tile is searched using a Priority Queue containing the neighbors of the current tile of the unit passed as an argument
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
		/// We could use the path size to set the priority, but it would be too heavy to compute
		priorityNeighbors.priority = neighbors[i].DistanceManhattan(singleUnit->goal); 
		queue.push(priorityNeighbors);
	}

	iPointPriority curr;
	while (queue.size() > 0) {

		curr = queue.top();
		queue.pop();

		if (App->pathfinding->IsWalkable(curr.point) && IsValidTile(singleUnit, curr.point, true, true))
			return curr.point;
	}

	return { -1,-1 };
}

// Returns true if it succeeds in changing the next tile of the unit
bool j1Movement::ChangeNextTile(SingleUnit* singleUnit)
{
	if (!singleUnit->isSearching) {

		// 1. Check only tiles in front of the unit (3)
		iPoint newTile = FindNewValidTile(singleUnit, true);

		if (newTile.x != -1 && newTile.y != -1) {

			// Request a new path for the unit
			singleUnit->unit->pathPlanner->RequestAStar(newTile, singleUnit->goal);

			singleUnit->isSearching = true; /// The unit is changing its nextTile
		}
		else {

			// 2. Check all possible tiles (8)
			newTile = FindNewValidTile(singleUnit);

			if (newTile.x != -1 && newTile.y != -1) {

				// Request a new path for the unit
				singleUnit->unit->pathPlanner->RequestAStar(newTile, singleUnit->goal);

				singleUnit->isSearching = true; /// The unit is changing its nextTile
			}
		}
	}

	// IS THE UNIT REALLY CHANGING ITS NEXTTILE?
	if (singleUnit->isSearching) {

		// ***IS THE PATH READY?***
		if (singleUnit->unit->pathPlanner->IsSearchCompleted()) {

			singleUnit->path = singleUnit->unit->pathPlanner->GetPath();

			singleUnit->isSearching = false;/// The unit has finished changing its nextTile

			// Update the unit's nextTile
			singleUnit->nextTile = singleUnit->path.front();

			return true;
		}
	}
	else {
	
		return false;
	
	}

	return false;
}

// Returns true if two units are heading towards opposite directions
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


UnitGroup::~UnitGroup() 
{
	list<SingleUnit*>::const_iterator it = units.begin();

	while (it != units.end()) {
		delete *it;
		it++;
	}
	units.clear();
}

// Adds a singleUnit (unit) to the group. Returns false if the singleUnit was already in the group
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

// Removes a singleUnit (unit) from the group. Returns true if success
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

// Returns true if the singleUnit (unit) is in the group
bool UnitGroup::IsUnitInGroup(SingleUnit* singleUnit) const
{
	bool ret = false;

	if (find(units.begin(), units.end(), singleUnit) != units.end())
		ret = true;

	return ret;
}

// Returns the size of the group (the number of singleUnits in the group)
uint UnitGroup::GetSize() const
{
	return units.size();
}

// Sets the destination tile (goal) of the group
bool UnitGroup::SetGoal(iPoint goal)
{
	bool ret = false;

	if (App->pathfinding->IsWalkable(goal)) {

		this->goal = goal;

		// Update the goal of all units
		list<SingleUnit*>::const_iterator it = units.begin();

		while (it != units.end()) {

			(*it)->goal = goal;

			// Warn units that the goal has been changed
			(*it)->isGoalChanged = true;

			it++;
		}

		ret = true;
	}

	return ret;
}

// Returns the destination tile (goal) of the group
iPoint UnitGroup::GetGoal() const
{
	return goal;
}

// SingleUnit struct ---------------------------------------------------------------------------------

SingleUnit::SingleUnit(Unit* unit, UnitGroup* group) :unit(unit), group(group)
{
	currTile = goal = App->map->WorldToMap(this->unit->entityInfo.pos.x, this->unit->entityInfo.pos.y);
	speed = this->unit->entityInfo.speed;

	priority = unit->unitInfo.priority;
}

// Returns true if the unit would reach its next tile during this move
// - nextPos is the next tile that the unit is heading to
// - endPos is the tile that the unit would reach during this move
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

// Stops the unit
void SingleUnit::StopUnit()
{
	unit->SetUnitDirection(UnitDirection_Idle);
}

// Resets the variables of the unit
void SingleUnit::ResetUnitVariables()
{
	reversePriority = false;

	wait = false;
	wakeUp = false;
	waitTile = { -1,-1 };
	waitUnit = nullptr;
	coll = CollisionType_NoCollision;
}