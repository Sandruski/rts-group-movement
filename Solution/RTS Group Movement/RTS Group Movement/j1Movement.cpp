#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1Movement.h"
#include "j1EntityFactory.h"
#include "Entity.h"
#include "DynamicEntity.h"
#include "j1Map.h"
#include "j1Render.h"
#include "j1Scene.h"
#include "j1PathManager.h"
#include "Goal.h"

#include "Brofiler\Brofiler.h"

j1Movement::j1Movement() {}

j1Movement::~j1Movement() {}

bool j1Movement::Update(float dt)
{
	bool ret = true;

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
					App->render->DrawLine((*unit)->unit->GetPos().x + offset.x, (*unit)->unit->GetPos().y + offset.y, nextPos.x + offset.x, nextPos.y + offset.y, 255, 255, 255, 255);
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
UnitGroup* j1Movement::CreateGroupFromUnits(list<DynamicEntity*> units)
{
	list<DynamicEntity*>::const_iterator it = units.begin();
	UnitGroup* group = nullptr;

	while (it != units.end()) {

		// If a unit from the list belongs to an existing group, delete the unit from the group
		group = GetGroupByUnit(*it);

		if (group != nullptr) {
			group->RemoveUnit((*it)->GetSingleUnit());

			// If the group is empty, delete it
			if (group->GetSize() == 0)
				RemoveGroup(group);
		}

		it++;
	}

	group = new UnitGroup(units);
	unitGroups.push_back(group);

	return group;
}

// Creates a group from a single unit. Returns the pointer to the group created or nullptr
UnitGroup* j1Movement::CreateGroupFromUnit(DynamicEntity* unit)
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
UnitGroup* j1Movement::GetGroupByUnit(DynamicEntity* unit) const
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
UnitGroup* j1Movement::GetGroupByUnits(list<DynamicEntity*> units) const
{
	list<DynamicEntity*>::const_iterator it;
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

bool j1Movement::RemoveGroup(UnitGroup* unitGroup) 
{
	bool ret = false;

	list<UnitGroup*>::const_iterator it = find(unitGroups.begin(), unitGroups.end(), unitGroup);

	if (it != unitGroups.end()) {

		// Remove the group
		delete *it;

		// Remove the group from the list of groups
		unitGroups.erase(it);

		ret = true;
	}

	return ret;
}

// Returns true if another unit has any of the booleans passed as arguments to true
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
MovementState j1Movement::MoveUnit(DynamicEntity* unit, float dt)
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

	switch (singleUnit->movementState) {

	case MovementState_WaitForPath:

		// The goal of a unit cannot be the goal of another unit
		if (!IsValidTile(singleUnit, singleUnit->goal, false, false, true))

			singleUnit->isGoalNeeded = true;

		if (singleUnit->isGoalNeeded) {
		
			// Only one unit at a time can change its goal
			if (!IsAnyUnitDoingSomething(singleUnit, true)) {

				singleUnit->unit->GetPathPlanner()->RequestDijkstra(singleUnit->goal, FindActiveTrigger::ActiveTriggerType_Goal);

				singleUnit->isSearching = true; /// The unit is changing its goal
			}
		}

		// ***IS THE TILE READY?***
		if (singleUnit->isSearching) {

			if (singleUnit->unit->GetPathPlanner()->IsSearchCompleted()) {

				singleUnit->goal = singleUnit->unit->GetPathPlanner()->GetTile();
				singleUnit->unit->GetPathPlanner()->SetSearchRequested(false);

				singleUnit->isSearching = false; /// The unit has finished changing its goal
				singleUnit->isGoalNeeded = false;
			}
			break;
		}
		
		if (!singleUnit->isGoalNeeded && !singleUnit->isSearching) {

			// Request a new path for the unit
			singleUnit->unit->GetPathPlanner()->RequestAStar(singleUnit->currTile, singleUnit->goal);

			// ***IS THE PATH READY?***
			if (singleUnit->unit->GetPathPlanner()->IsSearchCompleted()) {

				singleUnit->path = singleUnit->unit->GetPathPlanner()->GetPath();
				singleUnit->unit->GetPathPlanner()->SetSearchRequested(false);

				singleUnit->movementState = MovementState_IncreaseWaypoint;
			}
			break;
		}

		break;

	case MovementState_FollowPath:

		// ---------------------------------------------------------------------
		// MOVEMENT CALCULATION
		// ---------------------------------------------------------------------

		// Calculate the difference between the nextPos and the current position of the unit and store the result inside the temporary iPoint called movePos
		movePos = { (float)nextPos.x - singleUnit->unit->GetPos().x, (float)nextPos.y - singleUnit->unit->GetPos().y };

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
		movePos.x *= singleUnit->unit->GetSpeed() * dt;
		movePos.y *= singleUnit->unit->GetSpeed() * dt;

		// ---------------------------------------------------------------------
		// COLLISION CALCULATION
		// ---------------------------------------------------------------------

		CheckForFutureCollision(singleUnit);

		if (singleUnit->coll != CollisionType_NoCollision) {

			singleUnit->unit->SetIsStill(true);

			// waitUnit doesn't exist
			if (singleUnit->waitUnit == nullptr) {
			
				// If waitUnit doesn't exist, there cannot be a collision...
				singleUnit->ResetUnitCollisionParameters();
				break;
			}

			// waitUnit is dead
			// TODO: Check that this works fine and doesn't crash
			if (singleUnit->waitUnit->unit->isRemove) {

				singleUnit->waitUnit = nullptr;
				break;
			}

			// ---------------------------------------------------------------------
			// SPECIAL CASES
			// ---------------------------------------------------------------------

			// a) The other unit is attacking and won't respond to any movement order
			if (singleUnit->waitUnit->unit->IsHitting()) {
			
				// Current unit must react to the collision
				// Current unit moves
				if (!singleUnit->isSearching) {

					// 1. Check only tiles in front of the unit (3)
					iPoint newTile = FindNewValidTile(singleUnit, true);

					if (newTile.x != -1 && newTile.y != -1) {

						// Request a new path for the unit
						singleUnit->unit->GetPathPlanner()->RequestAStar(newTile, singleUnit->goal);
						singleUnit->unit->GetPathPlanner()->SetCheckingCurrTile(true);

						singleUnit->isSearching = true; /// The unit is changing its nextTile

						break;
					}
					else {

						// 2. Check all possible tiles (8)
						newTile = FindNewValidTile(singleUnit);

						if (newTile.x != -1 && newTile.y != -1) {

							// Request a new path for the unit
							singleUnit->unit->GetPathPlanner()->RequestAStar(newTile, singleUnit->goal);
							singleUnit->unit->GetPathPlanner()->SetCheckingCurrTile(true);

							singleUnit->isSearching = true; /// The unit is changing its nextTile

							break;
						}
						break;
					}
					break;
				}

				// IS THE UNIT REALLY CHANGING ITS NEXTTILE?
				if (singleUnit->isSearching) {

					// ***IS THE PATH READY?***
					if (singleUnit->unit->GetPathPlanner()->IsSearchCompleted()) {

						singleUnit->path = singleUnit->unit->GetPathPlanner()->GetPath();
						singleUnit->unit->GetPathPlanner()->SetSearchRequested(false);

						singleUnit->isSearching = false;/// The unit has finished changing its nextTile

						// Update the unit's nextTile
						singleUnit->nextTile = singleUnit->path.front();

						if (singleUnit->unit->isSelected)
							LOG("%s: MOVED AWAY %s", singleUnit->waitUnit->unit->GetColorName().data(), singleUnit->unit->GetColorName().data());

						/// COLLISION RESOLVED
						singleUnit->ResetUnitCollisionParameters();

						break;
					}
					break;
				}
				else {

					// WARNING: THIS IS NOT BEING USED!!!
					// If the unit cannot change its nextTile, ask the waitUnit to move
					singleUnit->reversePriority = true;

					if (singleUnit->unit->isSelected)
						LOG("%s: reversed its priority", singleUnit->waitUnit->unit->GetColorName().data());

					break;
				}
				break;
			}

			// b) The waitUnit is still on its goal
			if (singleUnit->nextTile == singleUnit->waitUnit->goal && singleUnit->waitUnit->currTile == singleUnit->waitUnit->goal) {

				// Wake up the unit (it may not be in the UnitState_Walk state)
				singleUnit->waitUnit->WakeUp();

				// The unit with the higher priority politely asks the other unit to move
				if ((singleUnit->unit->GetPriority() >= singleUnit->waitUnit->unit->GetPriority() || singleUnit->reversePriority)) {

					// 1. waitUnit moves
					singleUnit->unit->GetPathPlanner()->RequestDijkstra(singleUnit->waitUnit->goal, FindActiveTrigger::ActiveTriggerType_Goal);
					singleUnit->unit->GetPathPlanner()->SetCheckingCurrTile(true);
					singleUnit->unit->GetPathPlanner()->SetCheckingNextTile(true); // TODO: trigger must be fully customizable
					singleUnit->isSearching = true;

					// ***IS THE TILE READY?***
					if (singleUnit->unit->GetPathPlanner()->IsSearchCompleted()) {

						singleUnit->waitUnit->unit->GetBrain()->AddGoal_MoveToPosition(singleUnit->unit->GetPathPlanner()->GetTile());
						singleUnit->unit->GetPathPlanner()->SetSearchRequested(false);
						singleUnit->isSearching = false;

						LOG("DEL changed path waitUnit Still Goal %i, %i", singleUnit->unit->GetPathPlanner()->GetTile().x, singleUnit->unit->GetPathPlanner()->GetTile().y);

						if (singleUnit->unit->isSelected)
							LOG("%s: MOVED AWAY %s", singleUnit->unit->GetColorName().data(), singleUnit->waitUnit->unit->GetColorName().data());

						/// COLLISION RESOLVED
						singleUnit->ResetUnitCollisionParameters();
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
							singleUnit->unit->GetPathPlanner()->RequestAStar(newTile, singleUnit->goal);
							singleUnit->unit->GetPathPlanner()->SetCheckingCurrTile(true);

							singleUnit->isSearching = true; /// The unit is changing its nextTile
							LOG("DEL waitUnit Still Goal1 %i, %i", newTile.x, newTile.y);
							break;
						}
						else {

							// 2. Check all possible tiles (8)
							newTile = FindNewValidTile(singleUnit);

							if (newTile.x != -1 && newTile.y != -1) {

								// Request a new path for the unit
								singleUnit->unit->GetPathPlanner()->RequestAStar(newTile, singleUnit->goal);
								singleUnit->unit->GetPathPlanner()->SetCheckingCurrTile(true);

								singleUnit->isSearching = true; /// The unit is changing its nextTile
								LOG("DEL waitUnit Still Goal2 %i, %i", newTile.x, newTile.y);
								break;
							}
							break;
						}
						break;
					}

					// IS THE UNIT REALLY CHANGING ITS NEXTTILE?
					if (singleUnit->isSearching) {

						// ***IS THE PATH READY?***
						if (singleUnit->unit->GetPathPlanner()->IsSearchCompleted()) {

							singleUnit->path = singleUnit->unit->GetPathPlanner()->GetPath();
							singleUnit->waitUnit->unit->GetPathPlanner()->SetSearchRequested(false);

							singleUnit->isSearching = false;/// The unit has finished changing its nextTile

							// Update the unit's nextTile
							singleUnit->nextTile = singleUnit->path.front();

							if (singleUnit->unit->isSelected)
								LOG("%s: MOVED AWAY %s", singleUnit->waitUnit->unit->GetColorName().data(), singleUnit->unit->GetColorName().data());

							/// COLLISION RESOLVED
							singleUnit->ResetUnitCollisionParameters();

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

					singleUnit->ResetUnitCollisionParameters();

					if (singleUnit->unit->isSelected)
						LOG("%s: RESOLVED ITS CELL", singleUnit->unit->GetColorName().data());

					break;
				}
				break;
			}
			else if (singleUnit->coll == CollisionType_SameCell) {

				if (singleUnit->waitUnit->nextTile != singleUnit->waitTile) {

					singleUnit->ResetUnitCollisionParameters();

					if (singleUnit->unit->isSelected)
						LOG("%s: RESOLVED SAME CELL", singleUnit->unit->GetColorName().data());

					break;
				}
				break;
			}
			else if (singleUnit->coll == CollisionType_DiagonalCrossing) {

				if (singleUnit->waitUnit->currTile == singleUnit->waitTile) {

					singleUnit->ResetUnitCollisionParameters();

					if (singleUnit->unit->isSelected)
						LOG("%s: RESOLVED CROSSING", singleUnit->unit->GetColorName().data());

					break;
				}
				break;
			}
			else if (singleUnit->coll == CollisionType_TowardsCell) {

				// Current unit moves
				if (!singleUnit->isSearching) {

					// 1. Check only tiles in front of the unit (3)
					iPoint newTile = FindNewValidTile(singleUnit, true);

					if (newTile.x != -1 && newTile.y != -1) {

						// Request a new path for the unit
						singleUnit->unit->GetPathPlanner()->RequestAStar(newTile, singleUnit->goal);
						singleUnit->unit->GetPathPlanner()->SetCheckingCurrTile(true);

						singleUnit->isSearching = true; /// The unit is changing its nextTile
						LOG("DEL towards1 %i, %i", newTile.x, newTile.y);
						break;
					}
					else {

						// 2. Check all possible tiles (8)
						newTile = FindNewValidTile(singleUnit);

						if (newTile.x != -1 && newTile.y != -1) {

							// Request a new path for the unit
							singleUnit->unit->GetPathPlanner()->RequestAStar(newTile, singleUnit->goal);
							singleUnit->unit->GetPathPlanner()->SetCheckingCurrTile(true);

							singleUnit->isSearching = true; /// The unit is changing its nextTile
							LOG("DEL towards2 %i, %i", newTile.x, newTile.y);
							break;
						}
						break;
					}
					break;
				}

				// IS THE UNIT REALLY CHANGING ITS NEXTTILE?
				if (singleUnit->isSearching) {

					// ***IS THE PATH READY?***
					if (singleUnit->unit->GetPathPlanner()->IsSearchCompleted()) {

						singleUnit->path = singleUnit->unit->GetPathPlanner()->GetPath();
						singleUnit->waitUnit->unit->GetPathPlanner()->SetSearchRequested(false);

						singleUnit->isSearching = false;/// The unit has finished changing its nextTile

						// Update the unit's nextTile
						singleUnit->nextTile = singleUnit->path.front();

						if (singleUnit->unit->isSelected)
							LOG("%s: MOVED AWAY TOWARDS %s", singleUnit->waitUnit->unit->GetColorName().data(), singleUnit->unit->GetColorName().data());

						/// COLLISION RESOLVED
						singleUnit->waitUnit->wait = false;
						singleUnit->ResetUnitCollisionParameters();

						break;
					}
					break;
				}
				else {

					// WARNING: THIS IS NOT BEING USED!!!
					// If the unit cannot change its nextTile, change the collision with the waitUnit
					singleUnit->reversePriority = true;

					if (singleUnit->unit->isSelected)
						LOG("%s: reversed its priority TOWARDS", singleUnit->waitUnit->unit->GetColorName().data());
				}
				break;
			}
			break;
		}

		if (singleUnit->wait) {

			singleUnit->unit->SetIsStill(true);
			break;
		}

		// ---------------------------------------------------------------------
		// TILE FITTING
		// ---------------------------------------------------------------------

		{
			// Predict where the unit will be after moving and store the result it inside the temporary fPoint called endPos
			fPoint endPos = { singleUnit->unit->GetPos().x + movePos.x, singleUnit->unit->GetPos().y + movePos.y };

			// Check if the unit would reach the nextTile during this move. If the answer is yes, then:
			if (singleUnit->IsTileReached(nextPos, endPos)) {

				// Update the unit's position with the nextPos
				singleUnit->unit->SetPos({ (float)nextPos.x, (float)nextPos.y });

				// Update the unit's currTile with the nextTile
				singleUnit->currTile = singleUnit->nextTile;

				// Set the unit's movement state to IncreaseWaypoint
				singleUnit->movementState = MovementState_IncreaseWaypoint;

				break;
			}
		}

		singleUnit->unit->SetIsStill(false);

		// Add the movePos to the unit's current position
		singleUnit->unit->AddToPos(movePos);

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

		if (singleUnit->group->isShapedGoal) {

			if (singleUnit->goal != singleUnit->shapedGoal) {

				singleUnit->goal = singleUnit->shapedGoal;
				singleUnit->isGoalChanged = true;
			}
		}

		// The unit is still
		singleUnit->unit->SetIsStill(true);

	case MovementState_NoState:
	default:

		// The unit is still
		singleUnit->unit->SetIsStill(true);

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
				// Check if two units would reach the tile of each other. Make sure that the collision would be frontal!
				/// The unit with the lower priority must deal with the collision

				if (singleUnit->nextTile == (*units)->currTile && (*units)->nextTile == singleUnit->currTile) {

					if (IsOppositeDirection(singleUnit, *units)) {

						if (singleUnit->unit->GetPriority() >= (*units)->unit->GetPriority()) {

							if (singleUnit->coll == CollisionType_TowardsCell && singleUnit->waitUnit == (*units))
								break;

							else if ((*units)->coll != CollisionType_TowardsCell) {
								(*units)->SetCollisionParameters(CollisionType_TowardsCell, singleUnit, (*units)->nextTile);
								singleUnit->wait = true;
								LOG("%s: TOWARDS with %s", (*units)->unit->GetColorName().data(), singleUnit->unit->GetColorName().data());
							}
						}
						else {

							if ((*units)->coll == CollisionType_TowardsCell && (*units)->waitUnit == singleUnit)
								break;

							else if (singleUnit->coll != CollisionType_TowardsCell && (*units)->waitUnit != singleUnit) {
								singleUnit->SetCollisionParameters(CollisionType_TowardsCell, *units, singleUnit->nextTile);
								(*units)->wait = true;
								LOG("%s: TOWARDS with %s", singleUnit->unit->GetColorName().data(), (*units)->unit->GetColorName().data());
							}
						}
					}
				}

				if (singleUnit->coll != CollisionType_TowardsCell) {

					// ITS CELL. A reaches B's tile
					// Check if the unit would reach another unit's tile
					/// The unit who would reach another unit's tile must deal with the collision

					if (singleUnit->nextTile == (*units)->currTile) {
						
						singleUnit->waitUnit = *units;
						singleUnit->waitTile = singleUnit->nextTile;

						if (!singleUnit->wait) {
							singleUnit->coll = CollisionType_ItsCell;
							singleUnit->wait = true;

							if (singleUnit->unit->isSelected)
								LOG("%s: ITS CELL with %s", singleUnit->unit->GetColorName().data(), (*units)->unit->GetColorName().data());
						}
					}

					// SAME CELL. A and B reach the same tile
					// Check if two units would reach the same tile
					/// The unit with the highest area inside the nextTile must deal with the collision

					else if (singleUnit->nextTile == (*units)->nextTile && !(*units)->wait) {

						singleUnit->waitUnit = *units;
						singleUnit->waitTile = singleUnit->nextTile;

						// The agent that is already inside the tile has the priority. If none of them are, choose one randomly
						SDL_Rect posA = { (int)singleUnit->unit->GetPos().x, (int)singleUnit->unit->GetPos().y, App->map->data.tile_width, App->map->data.tile_height };
						SDL_Rect posB = { (int)(*units)->unit->GetPos().x, (int)(*units)->unit->GetPos().y,  App->map->data.tile_width, App->map->data.tile_height };
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
									LOG("%s: SAME CELL with %s", singleUnit->unit->GetColorName().data(), (*units)->unit->GetColorName().data());
							}
						}
						else {
							(*units)->coll = CollisionType_SameCell;
							(*units)->wait = true;

							if ((*units)->unit->isSelected)
								LOG("%s: SAME CELL with %s", (*units)->unit->GetColorName().data(), singleUnit->unit->GetColorName().data());
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

						// The agent that is already inside the tile has the priority. If none of them are, choose one randomly
						SDL_Rect posA = { (int)singleUnit->unit->GetPos().x, (int)singleUnit->unit->GetPos().y, App->map->data.tile_width, App->map->data.tile_height };
						SDL_Rect posB = { (int)(*units)->unit->GetPos().x, (int)(*units)->unit->GetPos().y,  App->map->data.tile_width, App->map->data.tile_height };

						iPoint tilePosA = App->map->MapToWorld(singleUnit->nextTile.x, singleUnit->nextTile.y);
						iPoint tilePosB = App->map->MapToWorld((*units)->nextTile.x, (*units)->nextTile.y);
						SDL_Rect tileA = { tilePosA.x, tilePosA.y, App->map->data.tile_width, App->map->data.tile_height };
						SDL_Rect tileB = { tilePosB.x, tilePosB.y, App->map->data.tile_width, App->map->data.tile_height };
						SDL_Rect resultA, resultB;

						SDL_IntersectRect(&posA, &tileA, &resultA);
						SDL_IntersectRect(&posB, &tileB, &resultB);

						// Compare the areas of the two intersections
						int areaA = resultA.w * resultA.h;
						int areaB = resultB.w * resultB.h;

						if (singleUnit->nextTile == up) {

							// We are sure than nextTile of this unit is valid, but what about the other unit's nextTile? We haven't check it yet
							if ((*units)->nextTile == myUp) {

								// Decide which unit waits
								// The unit who has the smaller area waits
								if (areaA <= areaB) {

									if (!(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

										singleUnit->SetCollisionParameters(CollisionType_DiagonalCrossing, *units, myUp);
										LOG("%s: Up CROSSING", singleUnit->unit->GetColorName().data());
									}
								}
								else {

									if (!singleUnit->wait && IsValidTile(nullptr, singleUnit->nextTile, true)) {

										(*units)->SetCollisionParameters(CollisionType_DiagonalCrossing, singleUnit, up);
										LOG("%s: Up CROSSING", (*units)->unit->GetColorName().data());
									}
								}
							}
						}
						else if (singleUnit->nextTile == down) {

							// We are sure than nextTile of this unit is valid, but what about the other unit's nextTile? We haven't check it yet
							if ((*units)->nextTile == myDown) {
							
								// Decide which unit waits
								// The unit who has the smaller area waits
								if (areaA <= areaB) {

									if (!(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

										singleUnit->SetCollisionParameters(CollisionType_DiagonalCrossing, *units, myDown);
										LOG("%s: Down CROSSING", singleUnit->unit->GetColorName().data());
									}
								}
								else {

									if (!singleUnit->wait && IsValidTile(nullptr, singleUnit->nextTile, true)) {

										(*units)->SetCollisionParameters(CollisionType_DiagonalCrossing, singleUnit, down);
										LOG("%s: Down CROSSING", (*units)->unit->GetColorName().data());
									}
								}
							}
						}
						else if (singleUnit->nextTile == left) {

							// We are sure than nextTile of this unit is valid, but what about the other unit's nextTile? We haven't check it yet
							if ((*units)->nextTile == myLeft) {
							
								// Decide which unit waits
								// The unit who has the smaller area waits
								if (areaA <= areaB) {

									if (!(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

										singleUnit->SetCollisionParameters(CollisionType_DiagonalCrossing, *units, myLeft);
										LOG("%s: Left CROSSING", singleUnit->unit->GetColorName().data());
									}
								}
								else {

									if (!singleUnit->wait && IsValidTile(nullptr, singleUnit->nextTile, true)) {

										(*units)->SetCollisionParameters(CollisionType_DiagonalCrossing, singleUnit, left);
										LOG("%s: Left CROSSING", (*units)->unit->GetColorName().data());
									}
								}
							} 
						}
						else if (singleUnit->nextTile == right) {

							// We are sure than nextTile of this unit is valid, but what about the other unit's nextTile? We haven't check it yet
							if ((*units)->nextTile == myRight) {
							
								// Decide which unit waits
								// The unit who has the smaller area waits
								if (areaA <= areaB) {

									if (!(*units)->wait && IsValidTile(nullptr, (*units)->nextTile, true)) {

										singleUnit->SetCollisionParameters(CollisionType_DiagonalCrossing, *units, myRight);
										LOG("%s: Right CROSSING", singleUnit->unit->GetColorName().data());
									}
								}
								else {

									if (!singleUnit->wait && IsValidTile(nullptr, singleUnit->nextTile, true)) {

										(*units)->SetCollisionParameters(CollisionType_DiagonalCrossing, singleUnit, right);
										LOG("%s: Right CROSSING", (*units)->unit->GetColorName().data());
									}
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

		case UnitDirection_NoDirection:

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
		if (neighbors[i].x != -1 && neighbors[i].y != -1) {

			priorityNeighbors.point = neighbors[i];
			/// We could use the path size to set the priority, but it would be too heavy to compute
			priorityNeighbors.priority = neighbors[i].DistanceManhattan(singleUnit->goal);
			queue.push(priorityNeighbors);
		}
	}

	iPointPriority curr;
	while (queue.size() > 0) {

		curr = queue.top();
		queue.pop();

		if (singleUnit->unit->GetNavgraph()->IsWalkable(curr.point) && IsValidTile(singleUnit, curr.point, true, true))
			return curr.point;
	}

	return { -1,-1 };
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

bool j1Movement::IsNeighborTile(iPoint tile, iPoint neighbor) 
{
	iPoint neighbors[8] = { { -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 },{ -1,-1 } };

	neighbors[0].create(tile.x + 1, tile.y + 0); // Right
	neighbors[1].create(tile.x + 0, tile.y + 1); // Down
	neighbors[2].create(tile.x - 1, tile.y + 0); // Left
	neighbors[3].create(tile.x + 0, tile.y - 1); // Up
	neighbors[4].create(tile.x + 1, tile.y + 1); // DownRight
	neighbors[5].create(tile.x + 1, tile.y - 1); // UpRight
	neighbors[6].create(tile.x - 1, tile.y + 1); // DownLeft
	neighbors[7].create(tile.x - 1, tile.y - 1); // UpLeft

	for (uint i = 0; i < 8; ++i) {
	
		if (neighbors[i] == neighbor)
			return true;
	}

	return false;
}

// UnitGroup struct ---------------------------------------------------------------------------------

UnitGroup::UnitGroup(DynamicEntity* unit)
{
	AddUnit(unit->GetSingleUnit());
}

UnitGroup::UnitGroup(list<DynamicEntity*> units)
{
	list<DynamicEntity*>::const_iterator it = units.begin();

	while (it != units.end()) {

		AddUnit((*it)->GetSingleUnit());
		it++;
	}
}

UnitGroup::~UnitGroup() 
{
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

	if (GetSize() == 0)
		App->movement->RemoveGroup(this);

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
		isShapedGoal = false;

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

bool UnitGroup::DrawShapedGoal(iPoint mouseTile)
{
	bool ret = false;

	// There must be as many goals as units in the group
	if (shapedGoal.size() <= units.size()) {

		if (shapedGoal.size() > 0) {

			// mouseTile must be close to the last tile added
			if (App->movement->IsNeighborTile(shapedGoal.back(), mouseTile)) {

				// Under the mouseTile there cannot be units nor buildings
				// mouseTile must be walkable
				if (!App->entities->IsEntityOnTile(mouseTile) && App->pathfinding->IsWalkable(mouseTile)) {

					vector<iPoint>::iterator it = find(shapedGoal.begin(), shapedGoal.end(), mouseTile);

					if (it != shapedGoal.end())

						// Remove the goal tiles until reaching mouseTile
						shapedGoal.erase(it, shapedGoal.end());

					else if (it == shapedGoal.end())

						// Push the mouseTile
						if (shapedGoal.size() < units.size())
							shapedGoal.push_back(mouseTile);
				}
				else {
				
					// Draw the invalid goal
					SDL_Color col = ColorRed;

					iPoint goalTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);
					const SDL_Rect goalRect = { goalTilePos.x, goalTilePos.y, App->map->data.tile_width, App->map->data.tile_height };
					App->render->DrawQuad(goalRect, col.r, col.g, col.b, 255, false);			
				}
			}
			else if (find(shapedGoal.begin(), shapedGoal.end(), mouseTile) != shapedGoal.end()
				&& shapedGoal.back() != mouseTile) {
			
				// Remove the goal tiles until reaching mouseTile						
				shapedGoal.erase(find(shapedGoal.begin(), shapedGoal.end(), mouseTile), shapedGoal.end());			
			}
		}
		else {

			if (!App->entities->IsEntityOnTile(mouseTile) && App->pathfinding->IsWalkable(mouseTile))
				shapedGoal.push_back(mouseTile);
		}
	}

	if (shapedGoal.size() == units.size()) {

		/// The goals are ready!
		ret = true;
	}

	if (shapedGoal.size() > 1) {

		// Draw the goals
		SDL_Color col;

		if (!ret)
			/// Goals are not ready: ORANGE
			col = ColorOrange;
		else
			/// Goals are ready: GREEN
			col = ColorGreen;

		for (uint i = 0; i < shapedGoal.size(); ++i) {
			iPoint goalTilePos = App->map->MapToWorld(shapedGoal[i].x, shapedGoal[i].y);
			const SDL_Rect goalRect = { goalTilePos.x, goalTilePos.y, App->map->data.tile_width, App->map->data.tile_height };
			App->render->DrawQuad(goalRect, col.r, col.g, col.b, 255, false);
		}
	}

	return ret;
}

bool UnitGroup::SetShapedGoal()
{
	bool ret = false;

	if (shapedGoal.size() == units.size()) {
	
		this->goal = shapedGoal.front();
		isShapedGoal = true;

		// Assign the different goals to the units
		list<SingleUnit*>::const_iterator it = units.begin();
		vector<iPoint>::const_iterator goal = shapedGoal.begin();
		uint i = 0;

		while (it != units.end() && i < shapedGoal.size()) {

			(*it)->goal = goal[i];
			(*it)->shapedGoal = (*it)->goal;

			// Warn units that the goal has been changed
			(*it)->isGoalChanged = true;

			i++;
			it++;
		}

		ret = true;
	}

	shapedGoal.clear();

	return ret;
}

// SingleUnit struct ---------------------------------------------------------------------------------

SingleUnit::SingleUnit(DynamicEntity* unit, UnitGroup* group) :unit(unit), group(group)
{
	currTile = goal = App->map->WorldToMap(this->unit->GetPos().x, this->unit->GetPos().y);
}

SingleUnit::~SingleUnit() 
{
	group->RemoveUnit(this);
	group = nullptr;
	unit = nullptr;
	waitUnit = nullptr;
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

// Resets the parameters of the unit (general info)
void SingleUnit::ResetUnitParameters()
{
	isGoalNeeded = false;

	wakeUp = false;
	nextTile = { -1,-1 };
}

// Resets the collision parameters of the unit
void SingleUnit::ResetUnitCollisionParameters()
{
	coll = CollisionType_NoCollision;
	waitUnit = nullptr;
	waitTile = { -1,-1 };
	wait = false;

	reversePriority = false;
}

// When detected a collision, to set the collision parameters of the unit
void SingleUnit::SetCollisionParameters(CollisionType collisionType, SingleUnit* waitUnit, iPoint waitTile)
{
	coll = collisionType;

	wait = true;
	this->waitUnit = waitUnit;
	this->waitTile = waitTile;
}

// Prepares the unit for its next movement cycle
void SingleUnit::GetReadyForNewMove()
{
	if (IsFittingTile()) {

		ResetUnitParameters();
		unit->GetPathPlanner()->SetSearchRequested(false);
		movementState = MovementState_WaitForPath;

		isGoalChanged = false;
	}
}

// Sets the state of the unit to UnitState_Walk
void SingleUnit::WakeUp()
{
	if (!wakeUp)
		wakeUp = true;
}

bool SingleUnit::IsFittingTile() const 
{
	iPoint currTilePos = App->map->MapToWorld(currTile.x, currTile.y);

	return (int)unit->GetPos().x == currTilePos.x && (int)unit->GetPos().y == currTilePos.y;
}

void SingleUnit::SetGoal(iPoint goal) 
{
	this->goal = goal;
}