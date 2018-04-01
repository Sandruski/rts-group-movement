#include "Defs.h"
#include "p2Log.h"

#include "CritterSheep.h"

#include "j1App.h"
#include "j1Render.h"
#include "j1Collision.h"
#include "j1Particles.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1EntityFactory.h"
#include "j1Movement.h"
#include "j1PathManager.h"
#include "Goal.h"

#include "j1Scene.h" // isFrameByFrame
#include "j1Input.h" // isFrameByFrame

#include "Brofiler\Brofiler.h"

CritterSheep::CritterSheep(fPoint pos, iPoint size, int currLife, uint maxLife, const UnitInfo& unitInfo, const CritterSheepInfo& critterSheepInfo) :DynamicEntity(pos, size, currLife, maxLife, unitInfo), critterSheepInfo(critterSheepInfo)
{
	// XML loading
	/// Animations
	CritterSheepInfo info = (CritterSheepInfo&)App->entities->GetDynamicEntityInfo(DynamicEntityType_CritterSheep);

	this->critterSheepInfo.up = info.up;
	this->critterSheepInfo.down = info.down;
	this->critterSheepInfo.left = info.left;
	this->critterSheepInfo.right = info.right;
	this->critterSheepInfo.upLeft = info.upLeft;
	this->critterSheepInfo.upRight = info.upRight;
	this->critterSheepInfo.downLeft = info.downLeft;
	this->critterSheepInfo.downRight = info.downRight;
	this->critterSheepInfo.deathUpLeft = info.deathUpLeft;
	this->critterSheepInfo.deathUpRight = info.deathUpRight;
	this->critterSheepInfo.deathDownLeft = info.deathDownLeft;
	this->critterSheepInfo.deathDownRight = info.deathDownRight;

	LoadAnimationsSpeed();

	// Initialize the goals
	brain->RemoveAllSubgoals();

	brain->AddGoal_Wander(5);
}

void CritterSheep::Move(float dt)
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
		entityCollider->isTrigger = true;

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

	UpdatePaws();

	// ---------------------------------------------------------------------

	// PROCESS THE CURRENTLY ACTIVE GOAL
	brain->Process(dt);

	UnitStateMachine(dt);

	// Update animations
	ChangeAnimation();

	if (!isDead)

		// Update colliders
		UpdateEntityColliderPos();
}

void CritterSheep::Draw(SDL_Texture* sprites)
{
	if (animation != nullptr)
		App->render->Blit(sprites, pos.x, pos.y, &(animation->GetCurrentFrame()));

	if (isSelected)
		DebugDrawSelected();
}

void CritterSheep::DebugDrawSelected()
{
	const SDL_Rect entitySize = { pos.x, pos.y, size.x, size.y };
	App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);

	for (uint i = 0; i < unitInfo.priority; ++i) {
		const SDL_Rect entitySize = { pos.x + 2 * i, pos.y + 2 * i, size.x - 4 * i, size.y - 4 * i };
		App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);
	}
}

void CritterSheep::OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState)
{

}

// State machine
void CritterSheep::UnitStateMachine(float dt)
{
	switch (unitState) {

	case UnitState_Wander:

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
bool CritterSheep::ChangeAnimation()
{
	bool ret = false;

	// The unit is dead
	if (isDead) {

		animation = &critterSheepInfo.deathDownLeft;
		ret = true;

		return ret;
	}

	// The unit is moving
	else {

		switch (GetUnitDirection()) {

		case UnitDirection_Up:

			animation = &critterSheepInfo.up;
			ret = true;
			break;

		case UnitDirection_NoDirection:
		case UnitDirection_Down:

			animation = &critterSheepInfo.down;
			ret = true;
			break;

		case UnitDirection_Left:

			animation = &critterSheepInfo.left;
			ret = true;
			break;

		case UnitDirection_Right:

			animation = &critterSheepInfo.right;
			ret = true;
			break;

		case UnitDirection_UpLeft:

			animation = &critterSheepInfo.upLeft;
			ret = true;
			break;

		case UnitDirection_UpRight:

			animation = &critterSheepInfo.upRight;
			ret = true;
			break;

		case UnitDirection_DownLeft:

			animation = &critterSheepInfo.downLeft;
			ret = true;
			break;

		case UnitDirection_DownRight:

			animation = &critterSheepInfo.downRight;
			ret = true;
			break;
		}
		return ret;
	}
	return ret;
}

// Paws
void CritterSheep::UpdatePaws() 
{
	// Add footprints along the critter's path
	if (singleUnit->currTile.x != -1 && singleUnit->currTile.y != -1
		&& singleUnit->nextTile.x != -1 && singleUnit->nextTile.y != -1) {

		if (lastPaw != nullptr) {

			// Reset the animation of the last paw
			if (GetUnitDirection() != UnitDirection_NoDirection && !lastPaw->isRemove) {

				lastPaw->isRemove = true;

				switch (GetUnitDirection()) {

				case UnitDirection_Up:

					lastPaw->animation = App->particles->GetSheepPawsInfo().up;
					break;

				case UnitDirection_NoDirection:
				case UnitDirection_Down:

					lastPaw->animation = App->particles->GetSheepPawsInfo().down;
					break;

				case UnitDirection_Left:

					lastPaw->animation = App->particles->GetSheepPawsInfo().left;
					break;

				case UnitDirection_Right:

					lastPaw->animation = App->particles->GetSheepPawsInfo().right;
					break;

				case UnitDirection_UpLeft:

					lastPaw->animation = App->particles->GetSheepPawsInfo().upLeft;
					break;

				case UnitDirection_UpRight:

					lastPaw->animation = App->particles->GetSheepPawsInfo().upRight;
					break;

				case UnitDirection_DownLeft:

					lastPaw->animation = App->particles->GetSheepPawsInfo().downLeft;
					break;

				case UnitDirection_DownRight:

					lastPaw->animation = App->particles->GetSheepPawsInfo().downRight;
					break;
				}
			}
		}

		// Create a new paw
		if (lastPawTile != singleUnit->currTile) {

			iPoint currTilePos = App->map->MapToWorld(singleUnit->currTile.x, singleUnit->currTile.y);
			lastPaw = App->particles->AddParticle(App->particles->paws, currTilePos);

			lastPawTile = singleUnit->currTile;
		}
	}
}