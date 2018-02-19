#include "p2Log.h"

#include "j1App.h"
#include "Unit.h"

#include "j1Collision.h"
#include "j1Render.h"
#include "j1Map.h"
#include "j1Pathfinding.h"

Unit::Unit(EntityInfo entityInfo, UnitInfo unitInfo) : Entity(entityInfo)
{
	this->unitInfo = unitInfo;
	type = EntityType_Unit;
}

void Unit::Move(float dt)
{
	/*
	i_pos.x = (int)position.x;
	i_pos.y = (int)position.y;

	// Check for collisions
	up = true;
	down = true;
	left = true;
	right = true;
	CheckCollision(collider_pos, { player.coll_size.x + player.coll_offset.w, player.coll_size.y + player.coll_offset.h }, player.check_collision_offset, up, down, left, right, player.GetState());

	// Update state
	if (!App->scene->pause)
	PlayerStateMachine();

	// Update collider
	collider_pos = { (int)position.x + player.coll_offset.x, (int)position.y + player.coll_offset.y };
	collider->SetPos(collider_pos.x, collider_pos.y);
	*/

	// -------------------------------------------------------

	UnitStateMachine();
	MovementStateMachine();
}

void Unit::Draw(SDL_Texture* sprites)
{
	const SDL_Rect entitySize = { entityInfo.pos.x, entityInfo.pos.y, entityInfo.size.x, entityInfo.size.y };
	App->render->DrawQuad(entitySize, unitInfo.color.r, unitInfo.color.g, unitInfo.color.b, unitInfo.color.a);

	if (isSelected)
		DrawSelected();
}

void Unit::OnCollision(Collider* c1, Collider* c2)
{
}

void Unit::UnitStateMachine() {

	switch (unitState) {

	case UnitState_Idle:

		break;

	case UnitState_Walk:

		break;
	}
}

void Unit::MovementStateMachine() 
{
	/*
	if (currTile == g->goal)
		// currentPos is the same as goal? Goal reached!
		movementState = MovementState_GoalReached;
	
	else if (u->waypoints.size() == 0)
		// Out of waypoints? Wait for path.
		u->movementState = MovementState_WaitForPath;

	// Raycast a line between the units and the nextPos they're heading to
	iPoint nextPos = App->map->MapToWorld(u->nextTile.x, u->nextTile.y);
	App->render->DrawLine(u->entity->entityInfo.pos.x, u->entity->entityInfo.pos.y, nextPos.x, nextPos.y, 255, 255, 255, 255);
	App->render->DrawCircle(nextPos.x, nextPos.y, 10, 255, 255, 255, 255);

	switch (u->movementState) {

	case MovementState_WaitForPath:

		// Find a path
		if (App->pathfinding->CreatePath(u->currTile, g->GetGoal(), MANHATTAN) == -1)
			break;

		// Save the path found
		u->waypoints = *App->pathfinding->GetLastPath();

		u->movementState = MovementState_IncreaseWaypoint;

		break;

	case MovementState_FollowPath:

		// MOVEMENT CALCULATION (update predicted positions)
		// Calculate the difference between the destination tile and the current tile (1, -1 or 0)
		moveTile = { u->nextTile.x - u->currTile.x, u->nextTile.y - u->currTile.y };

		// Multiply the calculated speed to reach the destination by the unit speed and the dt
		moveTile.x *= u->speed * dt;
		moveTile.y *= u->speed * dt;

		// COLLISION CALCULATION
		// Check for collisions before moving towards the destination tile
		endTile = { (int)u->entity->entityInfo.pos.x + moveTile.x ,(int)u->entity->entityInfo.pos.y + moveTile.y };
		endTile = App->map->WorldToMap(endTile.x, endTile.y);

		if (App->pathfinding->IsWalkable(endTile)) { // MYTODO: Add a complete collision detection system to check collisions within units, etc.

			if (endTile.x == u->nextTile.x && endTile.y == u->nextTile.y) {
				// If we're going to jump over the waypoint during this move
				u->movementState = MovementState_IncreaseWaypoint;
			}

			// Do the actual move
			u->entity->entityInfo.pos.x += moveTile.x;
			u->entity->entityInfo.pos.y += moveTile.y;
		}
		else
			u->movementState = MovementState_CollisionFound;

		break;

	case MovementState_GoalReached:

		// Make the appropiate notifications
		LOG("Goal reached!");

		break;

	case MovementState_CollisionFound:

		// Handle the collision
		u->movementState = MovementState_WaitForPath;

		break;

	case MovementState_IncreaseWaypoint:

		// Get the next waypoint to head to
		u->nextTile = u->waypoints.front();
		u->waypoints.erase(u->waypoints.begin());

		u->movementState = MovementState_FollowPath;

		break;
	}
	*/
}

void Unit::SetUnitState(UnitState unitState)
{
	this->unitState = unitState;
}

UnitState Unit::GetUnitState() const
{
	return unitState;
}

void Unit::CheckCollision(iPoint position, iPoint size, int offset, bool &up, bool &down, bool &left, bool &right, UnitState unitState) {

	for (int i = position.x - App->map->culing_offset; i < position.x + App->map->culing_offset; i++) {
		for (int j = position.y - App->map->culing_offset; j < position.y + App->map->culing_offset; j++) {

			iPoint ij = App->map->WorldToMap(i, j);

			uint id = App->map->collisionLayer->Get(ij.x, ij.y);

			if (id != 0) {
				iPoint world = App->map->MapToWorld(ij.x, ij.y);
				CalculateCollision(position, size, world.x, world.y, id, offset, up, down, left, right, unitState);
			}
		}
	}
}

void Unit::CalculateCollision(iPoint position, iPoint size, uint x, uint y, uint id, int offset, bool &up, bool &down, bool &left, bool &right, UnitState unitState) {

	if (App->toCap && App->capFrames <= 30) {
		offset = 6;
	}

	if (App->toCap && App->capFrames <= 10) {
		offset = 15;
	}

	SDL_Rect B = { x, y, 16, 16 }; //object rectangle

	iPoint c_up = { 0, -offset };
	iPoint c_down = { 0, offset };
	iPoint c_left = { -offset, 0 };
	iPoint c_right = { offset, 0 };

	// TODO: polish collisions

	//UP
	SDL_Rect A_up = { position.x + c_up.x, position.y + c_up.y, size.x, size.y }; //player rectangle
	if (SDL_HasIntersection(&A_up, &B)) {
		/*
		if (id == 1181 || (id == 1182 && App->scene->gate == false))
		up = false;
		else if (id == 1183 && state != null_) {
		player.SetState(punished_);
		lava_dead = true;
		}
		*/
	}

	//DOWN
	SDL_Rect A_down = { position.x + c_down.x, position.y + c_down.y, size.x, size.y }; //player rectangle
	if (SDL_HasIntersection(&A_down, &B))
		if (id == 1181)
			down = false;

	//LEFT
	SDL_Rect A_left = { position.x + c_left.x, position.y + c_left.y, size.x, size.y }; //player rectangle
	if (SDL_HasIntersection(&A_left, &B))
		if (id == 1181)
			left = false;

	//RIGHT
	SDL_Rect A_right = { position.x + c_right.x, position.y + c_right.y, size.x, size.y }; //player rectangle
	if (SDL_HasIntersection(&A_right, &B))
		if (id == 1181)
			right = false;
}

// -------------------------------------------------------------
// -------------------------------------------------------------

// UnitInfo struct
UnitInfo::UnitInfo() {}

UnitInfo::UnitInfo(const UnitInfo& i) : color(i.color) {}

UnitInfo::~UnitInfo() {}