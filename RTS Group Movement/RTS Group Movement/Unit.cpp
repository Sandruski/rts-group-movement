#include "p2Log.h"

#include "j1App.h"
#include "Unit.h"

#include "j1Collision.h"
#include "j1Render.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1EntityFactory.h"
#include "j1Input.h"
#include "j1Movement.h"

Unit::Unit(EntityInfo entityInfo, UnitInfo unitInfo) : Entity(entityInfo)
{
	this->unitInfo = unitInfo;
	type = EntityType_Unit;

	UnitInfo u;
	u = App->entities->GetUnitInfo();

	// Save animations
	this->unitInfo.up = u.up;
	this->unitInfo.down = u.down;
	this->unitInfo.left = u.left;
	this->unitInfo.right = u.right;
	this->unitInfo.upLeft = u.upLeft;
	this->unitInfo.upRight = u.upRight;
	this->unitInfo.downLeft = u.downLeft;
	this->unitInfo.downRight = u.downRight;
	this->unitInfo.idle = u.idle;

	// Save animations speed
	upSpeed = u.up.speed;
	downSpeed = u.down.speed;
	leftSpeed = u.left.speed;
	rightSpeed = u.right.speed;
	upLeftSpeed = u.upLeft.speed;
	upRightSpeed = u.upRight.speed;
	downLeftSpeed = u.downLeft.speed;
	downRightSpeed = u.downRight.speed;
	idleSpeed = u.idle.speed;

	App->movement->CreateGroupFromEntity(this);
}

void Unit::Move(float dt)
{
	// Save mouse position (world and map coords)
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	if (isSelected && App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN) {
		unitState = UnitState_Walk;
		iPoint currTile = App->map->WorldToMap(entityInfo.pos.x, entityInfo.pos.y);
		LOG("%d,%d", currTile.x, currTile.y);
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

	UnitStateMachine(dt);

	// Update animations
	UpdateAnimationsSpeed(dt);
	ChangeAnimation();
}

void Unit::UpdateAnimationsSpeed(float dt) 
{
	unitInfo.up.speed = upSpeed * dt;
	unitInfo.down.speed = downSpeed * dt;
	unitInfo.left.speed = leftSpeed * dt;
	unitInfo.right.speed = rightSpeed * dt;
	unitInfo.upLeft.speed = upLeftSpeed * dt;
	unitInfo.upRight.speed = upRightSpeed * dt;
	unitInfo.downLeft.speed = downLeftSpeed * dt;
	unitInfo.downRight.speed = downRightSpeed * dt;
	unitInfo.idle.speed = idleSpeed * dt;
}

void Unit::ChangeAnimation() 
{
	if (entityInfo.direction.x > 0.0f) {

		if (entityInfo.direction.y > 0.0f) { // Down-right
			animation = &unitInfo.downRight;
		}
		else if (entityInfo.direction.y < 0.0f) { // Up-right
			animation = &unitInfo.upRight;
		}
		else { // Right
			animation = &unitInfo.right;
		}
	}
	else if (entityInfo.direction.x < 0.0f) {

		if (entityInfo.direction.y > 0.0f) { // Down-left
			animation = &unitInfo.downLeft;
		}
		else if (entityInfo.direction.y < 0.0f) { // Up-left
			animation = &unitInfo.upLeft;
		}
		else { // Left
			animation = &unitInfo.left;
		}
	}
	else {

		if (entityInfo.direction.y > 0.0f) { // Down
			animation = &unitInfo.down;
		}
		else if (entityInfo.direction.y < 0.0f) { // Up
			animation = &unitInfo.up;
		}
		else { // Stop
			animation = &unitInfo.idle;
		}
	}
}

void Unit::Draw(SDL_Texture* sprites)
{
	fPoint offset = { 60.0f / 4.0f, 75.0f / 2.0f };

	if (animation != nullptr)
		App->render->Blit(sprites, entityInfo.pos.x - offset.x, entityInfo.pos.y - offset.y, &(animation->GetCurrentFrame()));

	if (isSelected)
		DebugDrawSelected();
}

void Unit::OnCollision(Collider* c1, Collider* c2)
{
}

void Unit::UnitStateMachine(float dt) {

	switch (unitState) {

	case UnitState_Idle:

		break;

	case UnitState_Walk:

		App->movement->MoveEntity(this, dt);

		break;
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