#include "Defs.h"
#include "p2Log.h"

#include "CritterBoar.h"

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

CritterBoar::CritterBoar(fPoint pos, iPoint size, int currLife, uint maxLife, const UnitInfo& unitInfo, const CritterBoarInfo& critterBoarInfo) :DynamicEntity(pos, size, currLife, maxLife, unitInfo), critterBoarInfo(critterBoarInfo)
{
	// XML loading
	/// Animations
	CritterBoarInfo info = (CritterBoarInfo&)App->entities->GetDynamicEntityInfo(DynamicEntityType_CritterBoar);
	
	this->critterBoarInfo.up = info.up;
	this->critterBoarInfo.down = info.down;
	this->critterBoarInfo.left = info.left;
	this->critterBoarInfo.right = info.right;
	this->critterBoarInfo.upLeft = info.upLeft;
	this->critterBoarInfo.upRight = info.upRight;
	this->critterBoarInfo.downLeft = info.downLeft;
	this->critterBoarInfo.downRight = info.downRight;
	this->critterBoarInfo.deathUpLeft = info.deathUpLeft;
	this->critterBoarInfo.deathUpRight = info.deathUpRight;
	this->critterBoarInfo.deathDownLeft = info.deathDownLeft;
	this->critterBoarInfo.deathDownRight = info.deathDownRight;

	LoadAnimationsSpeed();

	// Initialize the goals
	brain->RemoveAllSubgoals();

	brain->AddGoal_Wander(5);
}

void CritterBoar::Move(float dt)
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

void CritterBoar::Draw(SDL_Texture* sprites)
{
	if (animation != nullptr)
		App->render->Blit(sprites, pos.x, pos.y, &(animation->GetCurrentFrame()));

	if (isSelected)
		DebugDrawSelected();
}

void CritterBoar::DebugDrawSelected()
{
	const SDL_Rect entitySize = { pos.x, pos.y, size.x, size.y };
	App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);

	for (uint i = 0; i < unitInfo.priority; ++i) {
		const SDL_Rect entitySize = { pos.x + 2 * i, pos.y + 2 * i, size.x - 4 * i, size.y - 4 * i };
		App->render->DrawQuad(entitySize, color.r, color.g, color.b, 255, false);
	}
}

void CritterBoar::OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState)
{

}

// State machine
void CritterBoar::UnitStateMachine(float dt)
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
bool CritterBoar::ChangeAnimation()
{
	bool ret = false;

	// The unit is dead
	if (isDead) {

		animation = &critterBoarInfo.deathDownLeft;
		ret = true;

		return ret;
	}

	// The unit is moving
	else {

		switch (GetUnitDirection()) {

		case UnitDirection_Up:

			animation = &critterBoarInfo.up;
			ret = true;
			break;

		case UnitDirection_NoDirection:
		case UnitDirection_Down:

			animation = &critterBoarInfo.down;
			ret = true;
			break;

		case UnitDirection_Left:

			animation = &critterBoarInfo.left;
			ret = true;
			break;

		case UnitDirection_Right:

			animation = &critterBoarInfo.right;
			ret = true;
			break;

		case UnitDirection_UpLeft:

			animation = &critterBoarInfo.upLeft;
			ret = true;
			break;

		case UnitDirection_UpRight:

			animation = &critterBoarInfo.upRight;
			ret = true;
			break;

		case UnitDirection_DownLeft:

			animation = &critterBoarInfo.downLeft;
			ret = true;
			break;

		case UnitDirection_DownRight:

			animation = &critterBoarInfo.downRight;
			ret = true;
			break;
		}
		return ret;
	}
	return ret;
}

// Paws
void CritterBoar::UpdatePaws() 
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

					lastPaw->animation = App->particles->GetBoarPawsInfo().up;
					break;

				case UnitDirection_NoDirection:
				case UnitDirection_Down:

					lastPaw->animation = App->particles->GetBoarPawsInfo().down;
					break;

				case UnitDirection_Left:

					lastPaw->animation = App->particles->GetBoarPawsInfo().left;
					break;

				case UnitDirection_Right:

					lastPaw->animation = App->particles->GetBoarPawsInfo().right;
					break;

				case UnitDirection_UpLeft:

					lastPaw->animation = App->particles->GetBoarPawsInfo().upLeft;
					break;

				case UnitDirection_UpRight:

					lastPaw->animation = App->particles->GetBoarPawsInfo().upRight;
					break;

				case UnitDirection_DownLeft:

					lastPaw->animation = App->particles->GetBoarPawsInfo().downLeft;
					break;

				case UnitDirection_DownRight:

					lastPaw->animation = App->particles->GetBoarPawsInfo().downRight;
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