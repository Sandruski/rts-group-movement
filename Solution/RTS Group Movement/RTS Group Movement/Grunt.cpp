#include "Defs.h"
#include "p2Log.h"

#include "Grunt.h"

#include "j1App.h"
#include "j1Render.h"
#include "j1Collision.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1EntityFactory.h"
#include "j1Movement.h"
#include "j1PathManager.h"
#include "Goal.h"

#include "j1Scene.h" // isFrameByFrame
#include "j1Input.h" // isFrameByFrame

#include "Brofiler\Brofiler.h"

Grunt::Grunt(fPoint pos, iPoint size, int currLife, uint maxLife, const UnitInfo& unitInfo, const GruntInfo& gruntInfo) :DynamicEntity(pos, size, currLife, maxLife, unitInfo), gruntInfo(gruntInfo)
{
	// XML loading
	/// Animations
	GruntInfo info = (GruntInfo&)App->entities->GetDynamicEntityInfo(DynamicEntityType_Grunt);
	this->gruntInfo.idle = info.idle;
	this->gruntInfo.up = info.up;
	this->gruntInfo.down = info.down;
	this->gruntInfo.left = info.left;
	this->gruntInfo.right = info.right;
	this->gruntInfo.upLeft = info.upLeft;
	this->gruntInfo.upRight = info.upRight;
	this->gruntInfo.downLeft = info.downLeft;
	this->gruntInfo.downRight = info.downRight;

	this->gruntInfo.attackUp = info.attackUp;
	this->gruntInfo.attackDown = info.attackDown;
	this->gruntInfo.attackLeft = info.attackLeft;
	this->gruntInfo.attackRight = info.attackRight;
	this->gruntInfo.attackUpLeft = info.attackUpLeft;
	this->gruntInfo.attackUpRight = info.attackUpRight;
	this->gruntInfo.attackDownLeft = info.attackDownLeft;
	this->gruntInfo.attackDownRight = info.attackDownRight;

	this->gruntInfo.deathUp = info.deathUp;
	this->gruntInfo.deathDown = info.deathDown;

	LoadAnimationsSpeed();

	// Initialize the goals
	brain->RemoveAllSubgoals();
}

void Grunt::Move(float dt)
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
		CreateEntityCollider(EntitySide_Enemy);
		sightRadiusCollider = CreateRhombusCollider(ColliderType_EnemySightRadius, unitInfo.sightRadius);
		attackRadiusCollider = CreateRhombusCollider(ColliderType_EnemyAttackRadius, unitInfo.attackRadius);
		entityCollider->isTrigger = true;
		sightRadiusCollider->isTrigger = true;
		attackRadiusCollider->isTrigger = true;

		isSpawned = true;
	}

	// ---------------------------------------------------------------------

	// Is the unit dead?
	/// The unit must fit the tile (it is more attractive for the player)
	if (currLife <= 0 && unitState != UnitState_Die && singleUnit->IsFittingTile()) {

		isDead = true;

		// If the player dies, remove all their goals
		brain->RemoveAllSubgoals();
	}

	// The goal of the unit has been changed manually
	if (singleUnit->isGoalChanged)

		brain->AddGoal_MoveToPosition(singleUnit->goal);

	// ---------------------------------------------------------------------

	// Process the currently active goal
	brain->Process(dt);

	UnitStateMachine(dt);

	// Update animations
	UpdateAnimationsSpeed(dt);
	ChangeAnimation();

	if (!isDead && lastColliderUpdateTile != singleUnit->currTile) {

		// Update colliders
		UpdateEntityColliderPos();
		UpdateRhombusColliderPos(sightRadiusCollider, unitInfo.sightRadius);
		UpdateRhombusColliderPos(attackRadiusCollider, unitInfo.attackRadius);

		lastColliderUpdateTile = singleUnit->currTile;
	}
}

void Grunt::Draw(SDL_Texture* sprites) 
{
	if (animation != nullptr) {

		fPoint offset = { animation->GetCurrentFrame().w / 4.0f, animation->GetCurrentFrame().h / 2.0f };
		App->render->Blit(sprites, pos.x - offset.x, pos.y - offset.y, &(animation->GetCurrentFrame()));
	}

	if (isSelected)
		DebugDrawSelected();
}

void Grunt::DebugDrawSelected() 
{
	const SDL_Rect entitySize = { pos.x, pos.y, size.x, size.y };
	App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);

	for (uint i = 0; i < unitInfo.priority; ++i) {
		const SDL_Rect entitySize = { pos.x + 2 * i, pos.y + 2 * i, size.x - 4 * i, size.y - 4 * i };
		App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);
	}
}

void Grunt::OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState)
{
	switch (collisionState) {

	case CollisionState_OnEnter:

		// An player is within the sight of this player unit
		if (c1->colliderType == ColliderType_EnemySightRadius && c2->colliderType == ColliderType_PlayerUnit) { // || c2->colliderType == ColliderType_PlayerBuilding

			LOG("Enemy Sight Radius");

			// The Alliance is within the SIGHT radius
			isSightSatisfied = true;
			target = c2->entity;

			if (target != nullptr) {

				list<DynamicEntity*> unit;
				unit.push_back(this);
				UnitGroup* group = App->movement->CreateGroupFromUnits(unit);

				brain->AddGoal_AttackTarget(target);
			}
		}
		else if (c1->colliderType == ColliderType_EnemyAttackRadius && c2->colliderType == ColliderType_PlayerUnit) { // || c2->colliderType == ColliderType_PlayerBuilding

			LOG("Enemy Attack Radius");

			// The Alliance is within the ATTACK radius
			isAttackSatisfied = true;
		}

		break;

	case CollisionState_OnExit:

		// Reset attack parameters
		if (c1->colliderType == ColliderType_EnemySightRadius && c2->colliderType == ColliderType_PlayerUnit) { // || c2->colliderType == ColliderType_PlayerBuilding

			// The Alliance is NO longer within the SIGHT radius
			isSightSatisfied = false;
			target = nullptr;
		}
		else if (c1->colliderType == ColliderType_EnemyAttackRadius && c2->colliderType == ColliderType_PlayerUnit) { // || c2->colliderType == ColliderType_PlayerBuilding

			// The Alliance is NO longer within the ATTACK radius
			isAttackSatisfied = false;
		}

		break;
	}
}

// State machine
void Grunt::UnitStateMachine(float dt)
{
	switch (unitState) {

	case UnitState_AttackTarget:

		break;

	case UnitState_Patrol:

		break;

	case UnitState_Die:

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
void Grunt::LoadAnimationsSpeed()
{
	idleSpeed = gruntInfo.idle.speed;
	upSpeed = gruntInfo.up.speed;
	downSpeed = gruntInfo.down.speed;
	leftSpeed = gruntInfo.left.speed;
	rightSpeed = gruntInfo.right.speed;
	upLeftSpeed = gruntInfo.upLeft.speed;
	upRightSpeed = gruntInfo.upRight.speed;
	downLeftSpeed = gruntInfo.downLeft.speed;
	downRightSpeed = gruntInfo.downRight.speed;

	attackUpSpeed = gruntInfo.attackUp.speed;
	attackDownSpeed = gruntInfo.attackDown.speed;
	attackLeftSpeed = gruntInfo.attackLeft.speed;
	attackRightSpeed = gruntInfo.attackRight.speed;
	attackUpLeftSpeed = gruntInfo.attackUpLeft.speed;
	attackUpRightSpeed = gruntInfo.attackUpRight.speed;
	attackDownLeftSpeed = gruntInfo.attackDownLeft.speed;
	attackDownRightSpeed = gruntInfo.attackDownRight.speed;

	deathUpSpeed = gruntInfo.deathUp.speed;
	deathDownSpeed = gruntInfo.deathDown.speed;
}

void Grunt::UpdateAnimationsSpeed(float dt)
{
	gruntInfo.idle.speed = idleSpeed * dt;
	gruntInfo.up.speed = upSpeed * dt;
	gruntInfo.down.speed = downSpeed * dt;
	gruntInfo.left.speed = leftSpeed * dt;
	gruntInfo.right.speed = rightSpeed * dt;
	gruntInfo.upLeft.speed = upLeftSpeed * dt;
	gruntInfo.upRight.speed = upRightSpeed * dt;
	gruntInfo.downLeft.speed = downLeftSpeed * dt;
	gruntInfo.downRight.speed = downRightSpeed * dt;

	gruntInfo.attackUp.speed = attackUpSpeed * dt;
	gruntInfo.attackDown.speed = attackDownSpeed * dt;
	gruntInfo.attackLeft.speed = attackLeftSpeed * dt;
	gruntInfo.attackRight.speed = attackRightSpeed * dt;
	gruntInfo.attackUpLeft.speed = attackUpLeftSpeed * dt;
	gruntInfo.attackUpRight.speed = attackUpRightSpeed * dt;
	gruntInfo.attackDownLeft.speed = attackDownLeftSpeed * dt;
	gruntInfo.attackDownRight.speed = attackDownRightSpeed * dt;

	gruntInfo.deathUp.speed = deathUpSpeed * dt;
	gruntInfo.deathDown.speed = deathDownSpeed * dt;
}

bool Grunt::ChangeAnimation()
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

			animation = &gruntInfo.deathUp;
			ret = true;
		}
		else if (dir == UnitDirection_Down || dir == UnitDirection_DownLeft || dir == UnitDirection_DownRight) {

			if (animation->Finished() && unitState != UnitState_Die) {
				unitState = UnitState_Die;
				deadTimer.Start();
			}

			animation = &gruntInfo.deathDown;
			ret = true;
		}

		return ret;
	}

	// The unit is hitting their target
	else if (isHitting) {

		// Set the direction of the unit as the orientation towards the attacking target
		if (target != nullptr) {

			fPoint orientation = { target->GetPos().x - pos.x, (float)target->GetPos().y - pos.y };

			float m = sqrtf(pow(orientation.x, 2.0f) + pow(orientation.y, 2.0f));

			if (m > 0.0f) {
				orientation.x /= m;
				orientation.y /= m;
			}

			SetUnitDirectionByValue(orientation);
		}

		switch (GetUnitDirection()) {

		case UnitDirection_Up:

			animation = &gruntInfo.attackUp;
			ret = true;
			break;

		case UnitDirection_Down:

			animation = &gruntInfo.attackDown;
			ret = true;
			break;

		case UnitDirection_Left:

			animation = &gruntInfo.attackLeft;
			ret = true;
			break;

		case UnitDirection_Right:

			animation = &gruntInfo.attackRight;
			ret = true;
			break;

		case UnitDirection_UpLeft:

			animation = &gruntInfo.attackUpLeft;
			ret = true;
			break;

		case UnitDirection_UpRight:

			animation = &gruntInfo.attackUpRight;
			ret = true;
			break;

		case UnitDirection_DownLeft:

			animation = &gruntInfo.attackDownLeft;
			ret = true;
			break;

		case UnitDirection_DownRight:

			animation = &gruntInfo.attackDownRight;
			ret = true;
			break;
		}

		return ret;
	}

	// The unit is moving
	else {

		switch (GetUnitDirection()) {

		case UnitDirection_NoDirection:

			animation = &gruntInfo.idle;
			ret = true;
			break;

		case UnitDirection_Up:

			animation = &gruntInfo.up;
			ret = true;
			break;

		case UnitDirection_Down:

			animation = &gruntInfo.down;
			ret = true;
			break;

		case UnitDirection_Left:

			animation = &gruntInfo.left;
			ret = true;
			break;

		case UnitDirection_Right:

			animation = &gruntInfo.right;
			ret = true;
			break;

		case UnitDirection_UpLeft:

			animation = &gruntInfo.upLeft;
			ret = true;
			break;

		case UnitDirection_UpRight:

			animation = &gruntInfo.upRight;
			ret = true;
			break;

		case UnitDirection_DownLeft:

			animation = &gruntInfo.downLeft;
			ret = true;
			break;

		case UnitDirection_DownRight:

			animation = &gruntInfo.downRight;
			ret = true;
			break;
		}

		return ret;
	}

	return ret;
}