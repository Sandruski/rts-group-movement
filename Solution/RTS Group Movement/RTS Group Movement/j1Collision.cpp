#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"

#include "j1Collision.h"
#include "j1Input.h"
#include "j1Render.h"
#include "Entity.h"
#include "j1Map.h"

#include "Brofiler\Brofiler.h"

j1Collision::j1Collision()
{
	name.assign("collision");

	// COLLIDERS
	/// PlayerUnit
	matrix[ColliderType_PlayerUnit][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_NeutralUnit] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_PlayerAttackRadius] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_EnemyAttackRadius] = false;

	/// EnemyUnit
	matrix[ColliderType_EnemyUnit][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_NeutralUnit] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_EnemyAttackRadius] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_PlayerAttackRadius] = false;

	/// NeutralUnit
	matrix[ColliderType_NeutralUnit][ColliderType_NeutralUnit] = false;
	matrix[ColliderType_NeutralUnit][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_NeutralUnit][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_NeutralUnit][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_NeutralUnit][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_NeutralUnit][ColliderType_PlayerAttackRadius] = false;
	matrix[ColliderType_NeutralUnit][ColliderType_EnemyAttackRadius] = false;

	/// PlayerSightRadius
	matrix[ColliderType_PlayerSightRadius][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_PlayerSightRadius][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_PlayerSightRadius][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_PlayerSightRadius][ColliderType_EnemyUnit] = true;
	matrix[ColliderType_PlayerSightRadius][ColliderType_NeutralUnit] = true;
	matrix[ColliderType_PlayerSightRadius][ColliderType_PlayerAttackRadius] = false;
	matrix[ColliderType_PlayerSightRadius][ColliderType_EnemyAttackRadius] = false;

	/// EnemySightRadius
	matrix[ColliderType_EnemySightRadius][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_EnemySightRadius][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_EnemySightRadius][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_EnemySightRadius][ColliderType_PlayerUnit] = true;
	matrix[ColliderType_EnemySightRadius][ColliderType_NeutralUnit] = true;
	matrix[ColliderType_EnemySightRadius][ColliderType_EnemyAttackRadius] = false;
	matrix[ColliderType_EnemySightRadius][ColliderType_PlayerAttackRadius] = false;

	/// PlayerAttackRadius
	matrix[ColliderType_PlayerAttackRadius][ColliderType_PlayerAttackRadius] = false;
	matrix[ColliderType_PlayerAttackRadius][ColliderType_EnemyAttackRadius] = false;
	matrix[ColliderType_PlayerAttackRadius][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_PlayerAttackRadius][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_PlayerAttackRadius][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_PlayerAttackRadius][ColliderType_EnemyUnit] = true;
	matrix[ColliderType_PlayerAttackRadius][ColliderType_NeutralUnit] = true;

	/// EnemyAttackRadius
	matrix[ColliderType_EnemyAttackRadius][ColliderType_EnemyAttackRadius] = false;
	matrix[ColliderType_EnemyAttackRadius][ColliderType_PlayerAttackRadius] = false;
	matrix[ColliderType_EnemyAttackRadius][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_EnemyAttackRadius][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_EnemyAttackRadius][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_EnemyAttackRadius][ColliderType_PlayerUnit] = true;
	matrix[ColliderType_EnemyAttackRadius][ColliderType_NeutralUnit] = true;

	// DEBUG COLORS
	debugColors[ColliderType_PlayerUnit] = ColorDarkBlue;
	debugColors[ColliderType_EnemyUnit] = ColorDarkRed;
	debugColors[ColliderType_NeutralUnit] = ColorWhite;
	debugColors[ColliderType_PlayerSightRadius] = ColorLightBlue;
	debugColors[ColliderType_EnemySightRadius] = ColorLightRed;
	debugColors[ColliderType_PlayerAttackRadius] = ColorLightBlue;
	debugColors[ColliderType_EnemyAttackRadius] = ColorLightRed;
}

// Destructor
j1Collision::~j1Collision()
{}

bool j1Collision::PreUpdate()
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	bool ret = true;

	// Remove all colliders scheduled for deletion
	list<ColliderGroup*>::iterator it = colliderGroups.begin();

	while (it != colliderGroups.end()) {

		if ((*it)->isRemove) {

			EraseColliderGroup(*it);

			it = colliderGroups.begin();
			continue;
		}

		it++;
	}

	return ret;
}

// Called before render is available
bool j1Collision::Update(float dt)
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	bool ret = true;

	Collider* c1 = nullptr;
	Collider* c2 = nullptr;

	list<ColliderGroup*>::const_iterator I = colliderGroups.begin();

	while (I != colliderGroups.end()) {

		if (!(*I)->isValid) {
		
			I++;
			continue;
		}

		list<ColliderGroup*>::const_iterator J = colliderGroups.begin();

		// Avoid checking collisions already checked
		while (J != colliderGroups.end()) {

			if (!matrix[(*I)->colliderType][(*J)->colliderType]
				|| !(*J)->isValid) {

				J++;
				continue;
			}

			for (uint i = 0; i < (*I)->colliders.size(); ++i) {

				c1 = (*I)->colliders[i];

				bool isCollision = false;

				for (uint j = 0; j < (*J)->colliders.size(); ++j) {

					c2 = (*J)->colliders[j];

					// Check for the collision
					if (c1->CheckCollision(c2->colliderRect)) {

						if (c1->colliderGroup->callback != nullptr) {

							if (c1->colliderGroup->isTrigger) {

								if (find(c1->colliderGroup->collidingGroups.begin(), c1->colliderGroup->collidingGroups.end(), c2->colliderGroup) == c1->colliderGroup->collidingGroups.end()) {

									c1->colliderGroup->collidingGroups.push_back(c2->colliderGroup);
									c1->colliderGroup->lastCollidingGroups.push_back(c2->colliderGroup);

									// Collision!
									c1->colliderGroup->callback->OnCollision(c1->colliderGroup, c2->colliderGroup, CollisionState_OnEnter);
									isCollision = true;
								}

								else {

									c1->colliderGroup->lastCollidingGroups.push_back(c2->colliderGroup);
									isCollision = true;
								}
							}
							else
								c1->colliderGroup->callback->OnCollision(c1->colliderGroup, c2->colliderGroup, CollisionState_OnEnter);
						}

						/*
						if (matrix[c2->colliderGroup->colliderType][c1->colliderGroup->colliderType] && c2->colliderGroup->callback != nullptr) {

							if (c2->colliderGroup->isTrigger) {

								if (find(c2->colliderGroup->collidingGroups.begin(), c2->colliderGroup->collidingGroups.end(), c1->colliderGroup) == c2->colliderGroup->collidingGroups.end()) {

									c2->colliderGroup->collidingGroups.push_back(c1->colliderGroup);
									c2->colliderGroup->lastCollidingGroups.push_back(c1->colliderGroup);

									// Collision!
									c2->colliderGroup->callback->OnCollision(c2->colliderGroup, c1->colliderGroup, CollisionState_OnEnter);
									isCollision = true;
								}
								else {
									c2->colliderGroup->lastCollidingGroups.push_back(c1->colliderGroup);
									isCollision = true;
								}
							}
							else
								c2->colliderGroup->callback->OnCollision(c2->colliderGroup, c1->colliderGroup, CollisionState_OnEnter);
						}
						*/
					}

					if (isCollision)
						break;
				}

				if (isCollision)
					break;
			}
			J++;
		}
		I++;
	}
	
	HandleTriggers();

	return ret;
}

void j1Collision::HandleTriggers()
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	list<ColliderGroup*>::const_iterator groups = colliderGroups.begin();

	while (groups != colliderGroups.end()) {

		list<ColliderGroup*>::const_iterator collisions = (*groups)->collidingGroups.begin();

		while (collisions != (*groups)->collidingGroups.end()) {

			if (find((*groups)->lastCollidingGroups.begin(), (*groups)->lastCollidingGroups.end(), *collisions) == (*groups)->lastCollidingGroups.end()) {

				// Not collision anymore...
				(*groups)->callback->OnCollision(*groups, *collisions, CollisionState_OnExit);

				(*groups)->collidingGroups.remove(*collisions++);
				continue;
			}
			else
				(*groups)->lastCollidingGroups.remove(*collisions);

			collisions++;
		}
		groups++;
	}
}

// Called before quitting
bool j1Collision::CleanUp()
{
	bool ret = true;
	LOG("Freeing all colliders");

	// Remove all colliderGroups
	list<ColliderGroup*>::iterator it = colliderGroups.begin();

	while (it != colliderGroups.end()) {

		delete *it;
		it++;
	}
	colliderGroups.clear();

	return ret;
}

void j1Collision::DebugDraw()
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	Uint8 alpha = 80;
	SDL_Color color;

	list<ColliderGroup*>::const_iterator it = colliderGroups.begin();

	while (it != colliderGroups.end()) {

		color = debugColors[(*it)->colliderType];

		for (uint i = 0; i < (*it)->colliders.size(); ++i)
			App->render->DrawQuad((*it)->colliders[i]->colliderRect, color.r, color.g, color.b, alpha);

		App->render->DrawQuad((*it)->offsetCollider->colliderRect, 255, 255, 255, alpha);

		it++;
	}
}

// ColliderGroups
ColliderGroup* j1Collision::CreateAndAddColliderGroup(vector<Collider*> colliders, ColliderType colliderType, j1Module* callback, Entity* entity)
{
	if (callback == nullptr)
		return nullptr;

	ColliderGroup* collGroup = new ColliderGroup(colliders, colliderType, callback, entity);

	for (uint i = 0; i < colliders.size(); ++i)
		colliders[i]->SetColliderGroup(collGroup);

	colliderGroups.push_back(collGroup);

	return collGroup;
}

bool j1Collision::EraseColliderGroup(ColliderGroup* colliderGroup) 
{
	bool ret = false;

	if (colliderGroup != nullptr) {

		delete colliderGroup;

		if (colliderGroups.erase(remove(colliderGroups.begin(), colliderGroups.end(), colliderGroup), colliderGroups.end()) != colliderGroups.end())
			ret = true;

		colliderGroup = nullptr;
	}

	return ret;
}

// Colliders
Collider* j1Collision::CreateCollider(SDL_Rect colliderRect) 
{
	Collider* coll = new Collider(colliderRect);
	return coll;
}

bool j1Collision::AddColliderToAColliderGroup(ColliderGroup* colliderGroup, Collider* collider) 
{
	if (colliderGroup != nullptr && collider != nullptr)

		if (!colliderGroup->IsColliderInGroup(collider)) {

			collider->SetColliderGroup(colliderGroup);
			colliderGroup->colliders.push_back(collider);
			return true;
		}

	return false;
}

bool j1Collision::EraseColliderFromAColliderGroup(ColliderGroup* colliderGroup, Collider* collider) 
{
	bool ret = false;

	if (collider != nullptr) {

		delete collider;

		if (colliderGroup->colliders.erase(remove(colliderGroup->colliders.begin(), colliderGroup->colliders.end(), collider), colliderGroup->colliders.end()) != colliderGroup->colliders.end())
			ret = true;

		collider = nullptr;
	}

	return ret;
}

// Collider struct ---------------------------------------------------------------------------------

Collider::Collider(SDL_Rect colliderRect) :colliderRect(colliderRect) {}

Collider::~Collider() 
{
	colliderGroup = nullptr;
}

void Collider::SetPos(int x, int y)
{
	colliderRect.x = x;
	colliderRect.y = y;
}

iPoint Collider::GetPos() const 
{
	return { colliderRect.x, colliderRect.y };
}

void Collider::SetColliderGroup(ColliderGroup* colliderGroup) 
{
	this->colliderGroup = colliderGroup;
}

bool Collider::CheckCollision(const SDL_Rect& r) const
{
	return (colliderRect.x < r.x + r.w && colliderRect.x + colliderRect.w > r.x && colliderRect.y < r.y + r.h && colliderRect.h + colliderRect.y > r.y);
}

// ColliderGroup struct ---------------------------------------------------------------------------------

ColliderGroup::ColliderGroup(vector<Collider*> colliders, ColliderType colliderType, j1Module* callback, Entity* entity) :colliders(colliders), colliderType(colliderType), callback(callback), entity(entity) {}

ColliderGroup::~ColliderGroup() 
{
	callback = nullptr;
	entity = nullptr;

	// Remove all colliders
	for (uint i = 0; i < colliders.size(); ++i)

		colliders.erase(remove(colliders.begin(), colliders.end(), colliders[i]), colliders.end());

	collidingGroups.clear();
	lastCollidingGroups.clear();

	if (offsetCollider != nullptr)
		delete offsetCollider;
	offsetCollider = nullptr;
}

bool ColliderGroup::IsColliderInGroup(Collider* collider) 
{
	for (uint i = 0; i < colliders.size(); ++i)

		if (colliders[i] == collider)
			return true;

	return false;
}

void ColliderGroup::CreateOffsetCollider() 
{
	if (offsetCollider != nullptr)
		delete offsetCollider;
	offsetCollider = nullptr;

	Collider* left = GetCollider(true);
	Collider* right = GetCollider(false, true);
	Collider* top = GetCollider(false, false, true);
	Collider* bottom = GetCollider(false, false, false, true);

	SDL_Rect colliderRect;
	colliderRect.x = left->GetPos().x;
	colliderRect.y = top->GetPos().y;
	colliderRect.w = (right->GetPos().x + App->map->data.tile_width) - left->GetPos().x;
	colliderRect.h = (bottom->GetPos().y + App->map->data.tile_height) - top->GetPos().y;

	offsetCollider = new Collider(colliderRect);
}

Collider* ColliderGroup::GetCollider(bool left, bool right, bool top, bool bottom) 
{
	if (colliders.empty())
		return nullptr;

	Collider* result = colliders.front();

	for (uint i = 1; i < colliders.size(); ++i) {

		if (left) {

			if (result->GetPos().x > colliders[i]->GetPos().x)

				result = colliders[i];
		}
		else if (right) {
			
			if (result->GetPos().x < colliders[i]->GetPos().x)

				result = colliders[i];
		}
		else if (top) {

			if (result->GetPos().y > colliders[i]->GetPos().y)

				result = colliders[i];
		}
		else if (bottom) {

			if (result->GetPos().y < colliders[i]->GetPos().y)

				result = colliders[i];
		}
	}

	return result;
}

void ColliderGroup::RemoveCollider(Collider* collider) 
{
	colliders.erase(remove(colliders.begin(), colliders.end(), collider), colliders.end());
	collider = nullptr;
}

bool ColliderGroup::RemoveAllColliders() 
{
	bool ret = false;

	for (uint i = 0; i < colliders.size(); ++i) {
	
		delete colliders[i];
		colliders[i] = nullptr;

		ret = true;
	}
	colliders.clear();

	return ret;
}