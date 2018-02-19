#include "p2Log.h"

#include "j1App.h"
#include "Unit.h"

#include "j1Collision.h"
#include "j1Render.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1EntityFactory.h"
#include "j1Input.h"
#include "j1Scene.h"

Unit::Unit(EntityInfo entityInfo, UnitInfo unitInfo) : Entity(entityInfo)
{
	this->unitInfo = unitInfo;
	type = EntityType_Unit;

	speed = 100.0f;
}

void Unit::Move(float dt)
{
	// Save mouse position (world and map coords)
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	if (isSelected) {
		
		// Mouse left click: select a new goal
		if (App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN) {
			goalTile = mouseTile;
			movementState = MovementState_WaitForPath;
		}
	}

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

	//UnitStateMachine();
	MovementStateMachine(dt);
}

void Unit::Draw(SDL_Texture* sprites)
{
	iPoint offset = { spriteRect.w - App->map->data.tile_width, spriteRect.h - App->map->data.tile_height };
	App->render->Blit(sprites, (int)entityInfo.pos.x - offset.x, (int)entityInfo.pos.y - offset.y, &spriteRect);

	if (isSelected)
		DrawSelected();

	DebugDraw(App->scene->debugTex);
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

void Unit::MovementStateMachine(float dt)
{
	iPoint currTile = App->map->WorldToMap(entityInfo.pos.x, entityInfo.pos.y);
	fPoint movePos;
	iPoint endTile;
	iPoint nextPos = App->map->MapToWorld(nextTile.x, nextTile.y);

	/*
	else if (waypoints.size() == 0)
		// Out of waypoints? Wait for path.
		u->movementState = MovementState_WaitForPath;
	*/

	switch (movementState) {

	case MovementState_Stop:

		// Do nothing

		break;

	case MovementState_WaitForPath:

		// Find a path
		if (App->pathfinding->CreatePath(currTile, goalTile, DISTANCE_MANHATTAN) == -1)
			break;

		// Save the path found
		path = *App->pathfinding->GetLastPath();

		// Set state to IncreaseWaypoint, in order to start following the path
		movementState = MovementState_IncreaseWaypoint;

		break;

	case MovementState_FollowPath:

		// MOVEMENT CALCULATION

		// Calculate the difference between nextTile and currTile. The result will be in the interval [-1,1]
		movePos = { (float)(nextTile.x - currTile.x), (float)(nextTile.y - currTile.y) };

		// Apply the speed and the dt to the previous result
		movePos.x *= speed * dt;
		movePos.y *= speed * dt;

		// COLLISION CALCULATION

		// Predict where the unit will be after moving
		endTile = { (int)(entityInfo.pos.x + movePos.x),(int)(entityInfo.pos.y + movePos.y) };
		endTile = App->map->WorldToMap(endTile.x, endTile.y);

		// Check for future collisions before moving
		if (App->pathfinding->IsWalkable(nextTile) && !App->entities->IsAnotherEntityOnTile(this, nextTile)) { // endTile may change (terrain modification) or may be occupied by a unit

			if (endTile.x == nextTile.x && endTile.y == nextTile.y)	
				// If we're going to jump over the waypoint during this move
				movementState = MovementState_IncreaseWaypoint;

			// Do the actual move
			entityInfo.pos.x += movePos.x;
			entityInfo.pos.y += movePos.y;
		}
		else {
			//nextTile = App->entities->FindClosestWalkableTile(this, nextTile);
			
			if (nextTile.x == -1 && nextTile.y == -1)
				LOG("-1!!!");
			
			//movementState = MovementState_CollisionFound;
		}

		break;

	case MovementState_GoalReached:

		// Make the appropiate notifications

		break;

	case MovementState_CollisionFound:

		// If we want to prevent units from standing in line, we must implement local avoidance
		
		movementState = MovementState_FollowPath;

		break;

	case MovementState_IncreaseWaypoint:

		// Get the next waypoint to head to
		if (path.size() > 0) {
			nextTile = path.front();
			path.erase(path.begin());

			movementState = MovementState_FollowPath;
		}

		break;
	}
}

void Unit::DebugDraw(SDL_Texture* sprites)
{
	if (nextTile.x > -1 && nextTile.y > -1) {

		// Raycast a line between the unit and the nextTile
		iPoint nextPos = App->map->MapToWorld(nextTile.x, nextTile.y);
		App->render->DrawLine(entityInfo.pos.x, entityInfo.pos.y, nextPos.x, nextPos.y, 255, 255, 255, 255);
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