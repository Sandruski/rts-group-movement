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

Unit::Unit(EntityInfo entityInfo, uint priority) : Entity(entityInfo)
{
	type = EntityType_Unit;
	unitInfo.priority = priority;

	UnitInfo u;
	u = App->entities->GetUnitInfo();

	// Save animations
	unitInfo.up = u.up;
	unitInfo.down = u.down;
	unitInfo.left = u.left;
	unitInfo.right = u.right;
	unitInfo.upLeft = u.upLeft;
	unitInfo.upRight = u.upRight;
	unitInfo.downLeft = u.downLeft;
	unitInfo.downRight = u.downRight;
	unitInfo.idle = u.idle;

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

	// ---------------------------------------------------------------------

	if (isSelected && App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN)
		unitState = UnitState_Walk;

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

void Unit::DebugDrawSelected() 
{
	SDL_Color color;

	switch (unitInfo.priority) {
	case 1:
		color.r = 255;
		color.g = 0;
		color.b = 0;
	break;
	case 2:
		color.r = 0;
		color.g = 0;
		color.b = 255;
	break;
	default:
		color.r = 255;
		color.g = 255;
		color.b = 255;
	}

	const SDL_Rect entitySize = { entityInfo.pos.x, entityInfo.pos.y, entityInfo.size.x, entityInfo.size.y };
	App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);
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

// -------------------------------------------------------------
// -------------------------------------------------------------

// UnitInfo struct
UnitInfo::UnitInfo() {}

UnitInfo::UnitInfo(const UnitInfo& i) : priority(priority) {}

UnitInfo::~UnitInfo() {}