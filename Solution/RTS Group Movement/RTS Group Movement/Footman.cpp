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
}

void Footman::Move(float dt)
{
	// Save mouse position (world and map coords)
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	// Create colliders
	if (!isSpawned) {

		// Collisions
		CreateEntityCollider(EntitySide_Player);
		sightRadiusCollider = CreateRhombusCollider(ColliderType_PlayerSightRadius, unitInfo.sightRadius);
		attackRadiusCollider = CreateRhombusCollider(ColliderType_PlayerAttackRadius, unitInfo.attackRadius);
		entityCollider->isTrigger = true;
		sightRadiusCollider->isTrigger = true;
		attackRadiusCollider->isTrigger = true;

		isSpawned = true;
	}

	// ---------------------------------------------------------------------

	// Is the unit dead?
	if (currLife <= 0 && unitState != UnitState_Die) {

		/// The unit must fit the tile (it is more attractive for the player)
		if (singleUnit->IsFittingTile()) {

			isDead = true;
			unitState = UnitState_NoState;
		}
	}

	if (!isDead) {

		// UnitState_Walk
		if (singleUnit != nullptr)
			if ((isSelected && App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN) || singleUnit->wakeUp)
				unitState = UnitState_Walk;

		// UnitState_Attack
		if (isSightSatisfied) {
		
			if (isWantingAttack)
				unitState = UnitState_Attack;
		}
	}

	UnitStateMachine(dt);

	// Update animations
	UpdateAnimationsSpeed(dt);
	ChangeAnimation();

	if (!isDead) {

		// Update colliders
		UpdateEntityColliderPos();
		UpdateRhombusColliderPos(sightRadiusCollider, unitInfo.sightRadius);
		UpdateRhombusColliderPos(attackRadiusCollider, unitInfo.attackRadius);
	}
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

void Footman::OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState)
{
	switch (collisionState) {

	case CollisionState_OnEnter:

		// An enemy is within the sight of this player unit
		if (c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_EnemyUnit) {

			LOG("Player Sight Radius");
			// The Horde is within the SIGHT radius
			isSightSatisfied = true;
			attackingTarget = c2->entity;

			// Go attack the Horde
			/// The unit only attacks automatically if it isn't going anywhere
			if (isWantingAttack) {

				list<DynamicEntity*> unit;
				unit.push_back(this);
				UnitGroup* group = App->movement->CreateGroupFromUnits(unit);

				/// Chase the attackingTarget
				DynamicEntity* dynamicEntity = (DynamicEntity*)attackingTarget;
				group->SetGoal(dynamicEntity->GetSingleUnit()->currTile);
			}
		}
		else if (c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_EnemyUnit) {

			LOG("Player Attack Radius");
			// The Horde is within the ATTACK radius
			isAttackSatisfied = true;
		}

		break;

	case CollisionState_OnExit:

		// Reset attack parameters
		if (c1->colliderType == ColliderType_PlayerSightRadius && c2->colliderType == ColliderType_EnemyUnit) {

			// The Horde is NO longer within the SIGHT radius
			isSightSatisfied = false;
			attackingTarget = nullptr;
		}
		else if (c1->colliderType == ColliderType_PlayerAttackRadius && c2->colliderType == ColliderType_EnemyUnit) {

			// The Horde is NO longer within the ATTACK radius
			isAttackSatisfied = false;
			isAttacking = false;
		}

		break;
	}
}

// State machine
void Footman::UnitStateMachine(float dt)
{
	switch (unitState) {

	case UnitState_Walk:

		isWantingAttack = false;

		if (App->scene->isFrameByFrame) { /// debug
			if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN)
				App->movement->MoveUnit(this, dt);
		}
		else
			App->movement->MoveUnit(this, dt);

		break;

	case UnitState_Attack:

		// The unit is ordered to attack (this happens when the sight distance is satisfied)

		// The attackingTarget has died. Stop chasing and/or attacking
		if (((DynamicEntity*)attackingTarget)->isDead) {

			LOG("Player killed!");

			// Reset the attack parameters (they will also be reseted later when the attackingTarget is removed)
			ResetUnitAttackParameters();

			/// The goal tile of this unit must be their currTile
			singleUnit->group->SetGoal(singleUnit->currTile);

			break;
		}

		// 1. The attack distance is satisfied
		if (isAttackSatisfied
			&& singleUnit->coll == CollisionType_NoCollision
			&& singleUnit->movementState != MovementState_FollowPath && singleUnit->movementState != MovementState_NoState) {

			singleUnit->movementState = MovementState_GoalReached;

			// Attack the other unit until killed
			if (animation->Finished()) {
				attackingTarget->ApplyDamage(unitInfo.damage);
				LOG("Enemy: IT HURTS!");
				animation->Reset();
			}
			isAttacking = true;
		}

		// 2. The attack distance is not satisfied
		else {

			// The unit has reached the goal but the attack distance is not satisfied. The attacking target may have moved
			if (singleUnit->movementState == MovementState_GoalReached) {

				/// Keep chasing the attackingTarget
				singleUnit->group->SetGoal(((DynamicEntity*)attackingTarget)->GetSingleUnit()->currTile);
			}

			App->movement->MoveUnit(this, dt);
			isAttacking = false;
		}

	break;

	case UnitState_Patrol:

		isWantingAttack = true;

		break;

	case UnitState_Die:

		isWantingAttack = false;

		// Remove the corpse when a certain time is reached
		if (deadTimer.ReadSec() >= TIME_REMOVE_CORPSE)
			isRemove = true;

		break;

	case UnitState_Idle:
	case UnitState_NoState:
	default:

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
	footmanInfo.idle.speed = idleSpeed * dt;
	footmanInfo.up.speed = upSpeed * dt;
	footmanInfo.down.speed = downSpeed * dt;
	footmanInfo.left.speed = leftSpeed * dt;
	footmanInfo.right.speed = rightSpeed * dt;
	footmanInfo.upLeft.speed = upLeftSpeed * dt;
	footmanInfo.upRight.speed = upRightSpeed * dt;
	footmanInfo.downLeft.speed = downLeftSpeed * dt;
	footmanInfo.downRight.speed = downRightSpeed * dt;

	footmanInfo.attackUp.speed = attackUpSpeed * dt;
	footmanInfo.attackDown.speed = attackDownSpeed * dt;
	footmanInfo.attackLeft.speed = attackLeftSpeed * dt;
	footmanInfo.attackRight.speed = attackRightSpeed * dt;
	footmanInfo.attackUpLeft.speed = attackUpLeftSpeed * dt;
	footmanInfo.attackUpRight.speed = attackUpRightSpeed * dt;
	footmanInfo.attackDownLeft.speed = attackDownLeftSpeed * dt;
	footmanInfo.attackDownRight.speed = attackDownRightSpeed * dt;

	footmanInfo.deathUp.speed = deathUpSpeed * dt;
	footmanInfo.deathDown.speed = deathDownSpeed * dt;
}

bool Footman::ChangeAnimation()
{
	bool ret = false;

	// The unit is dead
	if (isDead) {
	
		UnitDirection dir = GetUnitDirection();

		if (dir == UnitDirection_Up || dir == UnitDirection_Up || dir == UnitDirection_UpLeft || dir == UnitDirection_UpRight
			|| dir == UnitDirection_Left || dir == UnitDirection_Right || dir == UnitDirection_NoDirection) {

			if (animation->Finished() && unitState != UnitState_Die) {
				unitState = UnitState_Die;
				deadTimer.Start();
			}

			animation = &footmanInfo.deathUp;
			ret = true;
		}
		else if (dir == UnitDirection_Down || dir == UnitDirection_DownLeft || dir == UnitDirection_DownRight) {

			if (animation->Finished() && unitState != UnitState_Die) {
				unitState = UnitState_Die;
				deadTimer.Start();
			}

			animation = &footmanInfo.deathDown;
			ret = true;
		}

		return ret;
	}

	if (!isAttacking) {

		// The unit is in UnitState_Walk
		switch (GetUnitDirection()) {

		case UnitDirection_NoDirection:

			animation = &footmanInfo.idle;
			ret = true;
			break;

		case UnitDirection_Up:

			animation = &footmanInfo.up;
			ret = true;
			break;

		case UnitDirection_Down:

			animation = &footmanInfo.down;
			ret = true;
			break;

		case UnitDirection_Left:

			animation = &footmanInfo.left;
			ret = true;
			break;

		case UnitDirection_Right:

			animation = &footmanInfo.right;
			ret = true;
			break;

		case UnitDirection_UpLeft:

			animation = &footmanInfo.upLeft;
			ret = true;
			break;

		case UnitDirection_UpRight:

			animation = &footmanInfo.upRight;
			ret = true;
			break;

		case UnitDirection_DownLeft:

			animation = &footmanInfo.downLeft;
			ret = true;
			break;

		case UnitDirection_DownRight:

			animation = &footmanInfo.downRight;
			ret = true;
			break;
		}

		return ret;
	}
	else {

		// The unit is in UnitState_Attack

		// Set the direction of the unit as the orientation towards the attacking target
		fPoint orientation = { attackingTarget->GetPos().x - pos.x, (float)attackingTarget->GetPos().y - pos.y };

		float m = sqrtf(pow(orientation.x, 2.0f) + pow(orientation.y, 2.0f));

		if (m > 0.0f) {
			orientation.x /= m;
			orientation.y /= m;
		}

		SetUnitDirectionByValue(orientation);

		switch (GetUnitDirection()) {

		case UnitDirection_Up:

			animation = &footmanInfo.attackUp;
			ret = true;
			break;

		case UnitDirection_Down:

			animation = &footmanInfo.attackDown;
			ret = true;
			break;

		case UnitDirection_Left:

			animation = &footmanInfo.attackLeft;
			ret = true;
			break;

		case UnitDirection_Right:

			animation = &footmanInfo.attackRight;
			ret = true;
			break;

		case UnitDirection_UpLeft:

			animation = &footmanInfo.attackUpLeft;
			ret = true;
			break;

		case UnitDirection_UpRight:

			animation = &footmanInfo.attackUpRight;
			ret = true;
			break;

		case UnitDirection_DownLeft:

			animation = &footmanInfo.attackDownLeft;
			ret = true;
			break;

		case UnitDirection_DownRight:

			animation = &footmanInfo.attackDownRight;
			ret = true;
			break;
		}

		return ret;
	}

	return ret;
}