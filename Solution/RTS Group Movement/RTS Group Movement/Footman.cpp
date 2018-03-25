#include "Defs.h"
#include "p2Log.h"

#include "Footman.h"

#include "j1App.h"
#include "j1Render.h"
#include "j1Collision.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1EntityFactory.h"
#include "j1Movement.h"
#include "j1PathManager.h"

#include "j1Scene.h" // isFrameByFrame
#include "j1Input.h" // isFrameByFrame

#include "Brofiler\Brofiler.h"

Footman::Footman(fPoint pos, iPoint size, int currLife, uint maxLife, const UnitInfo& unitInfo, const FootmanInfo& footmanInfo) :DynamicEntity(pos, size, currLife, maxLife, unitInfo), footmanInfo(footmanInfo)
{
	// XML loading
	/// Animations
	FootmanInfo info = (FootmanInfo&)App->entities->GetDynamicEntityInfo(DynamicEntityType_Footman);
	this->footmanInfo.idle = info.idle;
	this->footmanInfo.up = info.up;
	this->footmanInfo.down = info.down;
	this->footmanInfo.left = info.left;
	this->footmanInfo.right = info.right;
	this->footmanInfo.upLeft = info.upLeft;
	this->footmanInfo.upRight = info.upRight;
	this->footmanInfo.downLeft = info.downLeft;
	this->footmanInfo.downRight = info.downRight;

	this->footmanInfo.attackUp = info.attackUp;
	this->footmanInfo.attackDown = info.attackDown;
	this->footmanInfo.attackLeft = info.attackLeft;
	this->footmanInfo.attackRight = info.attackRight;
	this->footmanInfo.attackUpLeft = info.attackUpLeft;
	this->footmanInfo.attackUpRight = info.attackUpRight;
	this->footmanInfo.attackDownLeft = info.attackDownLeft;
	this->footmanInfo.attackDownRight = info.attackDownRight;

	this->footmanInfo.deathUp = info.deathUp;
	this->footmanInfo.deathDown = info.deathDown;

	LoadAnimationsSpeed();

	// Collisions
	CreateUnitSightCollider(EntitySide_Player);
	CreateEntityCollider(EntitySide_Player);
}

void Footman::Move(float dt)
{
	// Save mouse position (world and map coords)
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	// ---------------------------------------------------------------------

	if (singleUnit != nullptr)
		if ((isSelected && App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN) || singleUnit->wakeUp)
			unitState = UnitState_Walk;

	UnitStateMachine(dt);

	// Update animations
	UpdateAnimationsSpeed(dt);
	ChangeAnimation();

	// Update colliders
	UpdateEntityColliderPos();
	UpdateUnitSightColliderPos();
}

void Footman::Draw(SDL_Texture* sprites)
{
	fPoint offset = { animation->GetCurrentFrame().w / 4.0f, animation->GetCurrentFrame().h / 2.0f };

	if (animation != nullptr)
		App->render->Blit(sprites, pos.x - offset.x, pos.y - offset.y, &(animation->GetCurrentFrame()));

	if (isSelected)
		DebugDrawSelected();
}

void Footman::DebugDrawSelected()
{
	const SDL_Rect entitySize = { pos.x, pos.y, size.x, size.y };
	App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);

	for (uint i = 0; i < unitInfo.priority; ++i) {
		const SDL_Rect entitySize = { pos.x + 2 * i, pos.y + 2 * i, size.x - 4 * i, size.y - 4 * i };
		App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);
	}
}

void Footman::OnCollision(Collider* c1, Collider* c2)
{

}

// State machine
void Footman::UnitStateMachine(float dt)
{
	switch (unitState) {

	case UnitState_Idle:

		break;

	case UnitState_Walk:

		if (App->scene->isFrameByFrame) {
			if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
				App->movement->MoveUnit(this, dt);
		}
		else
			App->movement->MoveUnit(this, dt);

		break;
	}
}

// -------------------------------------------------------------

// Animations
void Footman::LoadAnimationsSpeed()
{
	idleSpeed = footmanInfo.idle.speed;
	upSpeed = footmanInfo.up.speed;
	downSpeed = footmanInfo.down.speed;
	leftSpeed = footmanInfo.left.speed;
	rightSpeed = footmanInfo.right.speed;
	upLeftSpeed = footmanInfo.upLeft.speed;
	upRightSpeed = footmanInfo.upRight.speed;
	downLeftSpeed = footmanInfo.downLeft.speed;
	downRightSpeed = footmanInfo.downRight.speed;

	attackUpSpeed = footmanInfo.attackUp.speed;
	attackDownSpeed = footmanInfo.attackDown.speed;
	attackLeftSpeed = footmanInfo.attackLeft.speed;
	attackRightSpeed = footmanInfo.attackRight.speed;
	attackUpLeftSpeed = footmanInfo.attackUpLeft.speed;
	attackUpRightSpeed = footmanInfo.attackUpRight.speed;
	attackDownLeftSpeed = footmanInfo.attackDownLeft.speed;
	attackDownRightSpeed = footmanInfo.attackDownRight.speed;

	deathUpSpeed = footmanInfo.deathUp.speed;
	deathDownSpeed = footmanInfo.deathDown.speed;
}

void Footman::UpdateAnimationsSpeed(float dt)
{
	footmanInfo.up.speed = upSpeed * dt;
	footmanInfo.down.speed = downSpeed * dt;
	footmanInfo.left.speed = leftSpeed * dt;
	footmanInfo.right.speed = rightSpeed * dt;
	footmanInfo.upLeft.speed = upLeftSpeed * dt;
	footmanInfo.upRight.speed = upRightSpeed * dt;
	footmanInfo.downLeft.speed = downLeftSpeed * dt;
	footmanInfo.downRight.speed = downRightSpeed * dt;
	footmanInfo.idle.speed = idleSpeed * dt;
}

void Footman::ChangeAnimation()
{
	switch (GetUnitDirection()) {

	case UnitDirection_NoDirection:

		animation = &footmanInfo.idle;
		break;

	case UnitDirection_Up:

		animation = &footmanInfo.up;
		break;

	case UnitDirection_Down:

		animation = &footmanInfo.down;
		break;

	case UnitDirection_Left:

		animation = &footmanInfo.left;
		break;

	case UnitDirection_Right:

		animation = &footmanInfo.right;
		break;

	case UnitDirection_UpLeft:

		animation = &footmanInfo.upLeft;
		break;

	case UnitDirection_UpRight:

		animation = &footmanInfo.upRight;
		break;

	case UnitDirection_DownLeft:

		animation = &footmanInfo.downLeft;
		break;

	case UnitDirection_DownRight:

		animation = &footmanInfo.downRight;
		break;
	}
}