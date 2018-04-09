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

	// Collisions
	CreateEntityCollider(EntitySide_Enemy);
	sightRadiusCollider = CreateRhombusCollider(ColliderType_EnemySightRadius, unitInfo.sightRadius);
	attackRadiusCollider = CreateRhombusCollider(ColliderType_EnemyAttackRadius, unitInfo.attackRadius);
	entityCollider->isTrigger = true;
	sightRadiusCollider->isTrigger = true;
	attackRadiusCollider->isTrigger = true;
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



		isSpawned = true;
	}

	// ---------------------------------------------------------------------

	// Is the unit dead?
	/// The unit must fit the tile (it is more attractive for the player)
	if (currLife <= 0 && unitState != UnitState_Die && singleUnit->IsFittingTile()) {

		isDead = true;

		// Invalidate colliders
		sightRadiusCollider->isValid = false;
		attackRadiusCollider->isValid = false;
		entityCollider->isValid = false;

		// If the player dies, remove all their goals
		unitCommand = UnitCommand_Stop;
	}

	/// GOAL: MoveToPosition
	// The goal of the unit has been changed manually
	if (singleUnit->isGoalChanged)

		brain->AddGoal_MoveToPosition(singleUnit->goal);

	/// GOAL: AttackTarget
	// If currTarget is null, check if there are available targets
	if (currTarget == nullptr) {

		/// Prioritize a type of target (static or dynamic)

		// If the targets list is not empty, pick the next currTarget
		if (targets.size() > 0)
			currTarget = targets.front();

		// If currTarget is not null, attack them
		if (currTarget != nullptr) {

			list<DynamicEntity*> unit;
			unit.push_back(this);
			App->movement->CreateGroupFromUnits(unit);

			brain->AddGoal_AttackTarget(currTarget);
		}
	}

	// ---------------------------------------------------------------------

	// Process the currently active goal
	brain->Process(dt);

	UnitStateMachine(dt);

	// Update animations
	if (!isStill)
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

			DynamicEntity* dynEnt = (DynamicEntity*)c2->entity;
			LOG("Enemy Sight Radius %s", dynEnt->GetColorName().data());

			// The Alliance is within the SIGHT radius

			list<TargetInfo*>::const_iterator it = targets.begin();
			bool isTargetFound = false;

			// If the target is already in the targets list, set its isSightSatisfied to true
			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					(*it)->isSightSatisfied = true;
					isTargetFound = true;
					break;
				}
				it++;
			}

			// Else, add the new target to the targets list (and set its isSightSatisfied to true)
			if (!isTargetFound) {

				TargetInfo* targetInfo = new TargetInfo();
				targetInfo->target = c2->entity;
				targetInfo->isSightSatisfied = true;

				targets.push_back(targetInfo);
			}
		}
		else if (c1->colliderType == ColliderType_EnemyAttackRadius && c2->colliderType == ColliderType_PlayerUnit) { // || c2->colliderType == ColliderType_PlayerBuilding

			DynamicEntity* dynEnt = (DynamicEntity*)c2->entity;
			LOG("Enemy Attack Radius %s", dynEnt->GetColorName().data());

			// The Alliance is within the ATTACK radius

			list<TargetInfo*>::const_iterator it = targets.begin();

			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					(*it)->isAttackSatisfied = true;
					break;
				}
				it++;
			}
		}

		break;

	case CollisionState_OnExit:

		// Reset attack parameters
		if (c1->colliderType == ColliderType_EnemySightRadius && c2->colliderType == ColliderType_PlayerUnit) { // || c2->colliderType == ColliderType_PlayerBuilding

			DynamicEntity* dynEnt = (DynamicEntity*)c2->entity;
			LOG("NO MORE Enemy Sight Radius %s", dynEnt->GetColorName().data());

			// The Alliance is NO longer within the SIGHT radius

			// Remove the target from the targets list
			list<TargetInfo*>::const_iterator it = targets.begin();

			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					// If currTarget matches the target that needs to be removed, set its isSightSatisfied to false and it will be removed later
					if (currTarget->target == c2->entity)

						currTarget->isSightSatisfied = false;

					// If currTarget is different from the target that needs to be removed, remove the target from the list
					else {

						delete *it;
						targets.erase(it);
					}

					break;
				}
				it++;
			}
		}
		else if (c1->colliderType == ColliderType_EnemyAttackRadius && c2->colliderType == ColliderType_PlayerUnit) { // || c2->colliderType == ColliderType_PlayerBuilding

			DynamicEntity* dynEnt = (DynamicEntity*)c2->entity;
			LOG("NO MORE Enemy Attack Radius %s", dynEnt->GetColorName().data());

			// The Alliance is NO longer within the ATTACK radius

			list<TargetInfo*>::const_iterator it = targets.begin();

			while (it != targets.end()) {

				if ((*it)->target == c2->entity) {

					(*it)->isAttackSatisfied = false;
					break;
				}
				it++;
			}
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
		if (currTarget != nullptr) {

			fPoint orientation = { currTarget->target->GetPos().x - pos.x, currTarget->target->GetPos().y - pos.y };

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

	// The unit is either moving or still
	else {

		switch (GetUnitDirection()) {

		case UnitDirection_Up:

			if (isStill) {

				gruntInfo.up.loop = false;
				gruntInfo.up.Reset();
				gruntInfo.up.speed = 0.0f;
			}
			else
				gruntInfo.up.loop = true;

			animation = &gruntInfo.up;

			ret = true;
			break;

		case UnitDirection_NoDirection:
		case UnitDirection_Down:

			if (isStill) {

				gruntInfo.down.loop = false;
				gruntInfo.down.Reset();
				gruntInfo.down.speed = 0.0f;
			}
			else
				gruntInfo.down.loop = true;

			animation = &gruntInfo.down;

			ret = true;
			break;

		case UnitDirection_Left:

			if (isStill) {

				gruntInfo.left.loop = false;
				gruntInfo.left.Reset();
				gruntInfo.left.speed = 0.0f;
			}
			else
				gruntInfo.left.loop = true;

			animation = &gruntInfo.left;

			ret = true;
			break;

		case UnitDirection_Right:

			if (isStill) {

				gruntInfo.right.loop = false;
				gruntInfo.right.Reset();
				gruntInfo.right.speed = 0.0f;
			}
			else
				gruntInfo.right.loop = true;

			animation = &gruntInfo.right;

			ret = true;
			break;

		case UnitDirection_UpLeft:

			if (isStill) {

				gruntInfo.upLeft.loop = false;
				gruntInfo.upLeft.Reset();
				gruntInfo.upLeft.speed = 0.0f;
			}
			else
				gruntInfo.upLeft.loop = true;

			animation = &gruntInfo.upLeft;

			ret = true;
			break;

		case UnitDirection_UpRight:

			if (isStill) {

				gruntInfo.upRight.loop = false;
				gruntInfo.upRight.Reset();
				gruntInfo.upRight.speed = 0.0f;
			}
			else
				gruntInfo.upRight.loop = true;

			animation = &gruntInfo.upRight;

			ret = true;
			break;

		case UnitDirection_DownLeft:

			if (isStill) {

				gruntInfo.downLeft.loop = false;
				gruntInfo.downLeft.Reset();
				gruntInfo.downLeft.speed = 0.0f;
			}
			else
				gruntInfo.downLeft.loop = true;

			animation = &gruntInfo.downLeft;

			ret = true;
			break;

		case UnitDirection_DownRight:

			if (isStill) {

				gruntInfo.downRight.loop = false;
				gruntInfo.downRight.Reset();
				gruntInfo.downRight.speed = 0.0f;
			}
			else
				gruntInfo.downRight.loop = true;

			animation = &gruntInfo.downRight;

			ret = true;
			break;
		}
		return ret;
	}
	return ret;
}