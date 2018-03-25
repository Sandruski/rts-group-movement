#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"

#include "j1Collision.h"
#include "j1Input.h"
#include "j1Render.h"
#include "Entity.h"

j1Collision::j1Collision()
{
	name.assign("collision");

	// COLLIDERS
	/// PlayerUnit
	matrix[ColliderType_PlayerUnit][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_PlayerUnit][ColliderType_EnemySightRadius] = true;

	/// EnemyUnit
	matrix[ColliderType_EnemyUnit][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_EnemyUnit][ColliderType_PlayerSightRadius] = true;

	/// PlayerAttackRadius
	matrix[ColliderType_PlayerSightRadius][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_PlayerSightRadius][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_PlayerSightRadius][ColliderType_PlayerUnit] = false;
	matrix[ColliderType_PlayerSightRadius][ColliderType_EnemyUnit] = true;

	/// EnemyAttackRadius
	matrix[ColliderType_EnemySightRadius][ColliderType_EnemySightRadius] = false;
	matrix[ColliderType_EnemySightRadius][ColliderType_PlayerSightRadius] = false;
	matrix[ColliderType_EnemySightRadius][ColliderType_EnemyUnit] = false;
	matrix[ColliderType_EnemySightRadius][ColliderType_PlayerUnit] = true;

	// DEBUG COLORS
	debugColors[ColliderType_PlayerUnit] = ColorDarkBlue;
	debugColors[ColliderType_EnemyUnit] = ColorDarkRed;
	debugColors[ColliderType_PlayerSightRadius] = ColorLightBlue;
	debugColors[ColliderType_EnemySightRadius] = ColorLightRed;
}

// Destructor
j1Collision::~j1Collision()
{}

bool j1Collision::PreUpdate()
{
	bool ret = true;

	// Remove all colliders scheduled for deletion
	list<ColliderGroup*>::iterator it = colliderGroups.begin();

	while (it != colliderGroups.end()) {

		if ((*it)->isRemove)
			colliderGroups.erase(remove(colliderGroups.begin(), colliderGroups.end(), *it), colliderGroups.end());

		it++;
	}

	return ret;
}

// Called before render is available
bool j1Collision::Update(float dt)
{
	bool ret = true;

	Collider* c1 = nullptr;
	Collider* c2 = nullptr;

	list<ColliderGroup*>::const_iterator I = colliderGroups.begin();

	while (I != colliderGroups.end()) {

		for (uint i = 0; i < (*I)->colliders.size(); ++i) {

			c1 = (*I)->colliders[i];

			// Avoid checking collisions already checked
			list<ColliderGroup*>::const_iterator J = I;
			J++;

			while (J != colliderGroups.end()) {

				for (uint j = 0; j < (*J)->colliders.size(); ++j) {

					c2 = (*J)->colliders[j];

					// Check for the collision
					if (c1->CheckCollision(c2->colliderRect))
					{
						if (matrix[c1->colliderGroup->colliderType][c2->colliderGroup->colliderType] && c1->colliderGroup->callback != nullptr)
							c1->colliderGroup->callback->OnCollision(c1->colliderGroup, c2->colliderGroup);

						if (matrix[c2->colliderGroup->colliderType][c1->colliderGroup->colliderType] && c2->colliderGroup->callback != nullptr)
							c2->colliderGroup->callback->OnCollision(c2->colliderGroup, c1->colliderGroup);
					}
				}
				J++;
			}
		}
		I++;
	}

	DebugDraw();

	return ret;
}

// Called before quitting
bool j1Collision::CleanUp()
{
	bool ret = true;
	LOG("Freeing all colliders");

	// Remove all colliderGroups
	list<ColliderGroup*>::iterator it = colliderGroups.begin();

	while (it != colliderGroups.end()) {
		colliderGroups.erase(remove(colliderGroups.begin(), colliderGroups.end(), *it), colliderGroups.end());

		it++;
	}
	colliderGroups.clear();

	return ret;
}

void j1Collision::DebugDraw()
{
	Uint8 alpha = 80;
	SDL_Color color;

	list<ColliderGroup*>::const_iterator it = colliderGroups.begin();

	while (it != colliderGroups.end()) {

		color = debugColors[(*it)->colliderType];

		for (uint i = 0; i < (*it)->colliders.size(); ++i)

			App->render->DrawQuad((*it)->colliders[i]->colliderRect, color.r, color.g, color.b, alpha);

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
	if (colliderGroups.erase(remove(colliderGroups.begin(), colliderGroups.end(), colliderGroup), colliderGroups.end()) != colliderGroups.end())
		return true;

	return false;
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
	if (colliderGroup->colliders.erase(remove(colliderGroup->colliders.begin(), colliderGroup->colliders.end(), collider), colliderGroup->colliders.end()) != colliderGroup->colliders.end())
		return true;

	return false;
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

	// Remove all colliders
	for (uint i = 0; i < colliders.size(); ++i)

		colliders.erase(remove(colliders.begin(), colliders.end(), colliders[i]), colliders.end());
}

bool ColliderGroup::IsColliderInGroup(Collider* collider) 
{
	for (uint i = 0; i < colliders.size(); ++i)

		if (colliders[i] == collider)
			return true;

	return false;
}