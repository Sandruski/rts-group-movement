#ifndef __j1COLLISION_H__
#define __j1COLLISION_H__

#include "j1Module.h"

#include "SDL\include\SDL_rect.h"

#include <list>
#include <vector>
#include <algorithm>
using namespace std;

enum ColliderType {

	ColliderType_NoType = -1,
	ColliderType_PlayerUnit,
	ColliderType_EnemyUnit,
	ColliderType_PlayerSightRadius,
	ColliderType_EnemySightRadius,
	ColliderType_MaxColliders
};

struct Collider;
struct ColliderGroup;

class j1Collision : public j1Module
{
public:

	j1Collision();
	~j1Collision();

	bool PreUpdate();
	bool Update(float dt);
	bool CleanUp();
	void DebugDraw();

	// ColliderGroups
	ColliderGroup* CreateAndAddColliderGroup(vector<Collider*> colliders, ColliderType colliderType, j1Module* callback);
	bool EraseColliderGroup(ColliderGroup* colliderGroup);

	// Colliders
	Collider* CreateCollider(SDL_Rect colliderRect);

	bool AddColliderToAColliderGroup(ColliderGroup* colliderGroup, Collider* collider);
	bool EraseColliderFromAColliderGroup(ColliderGroup* colliderGroup, Collider* collider);

public:

	bool matrix[ColliderType_MaxColliders][ColliderType_MaxColliders];
	SDL_Color debugColors[ColliderType_MaxColliders];

private:

	list<ColliderGroup*> colliderGroups;
	bool debug = true;
};

// ---------------------------------------------------------------------
// ColliderGroup: struct representing a group of colliders
// ---------------------------------------------------------------------

struct ColliderGroup
{
	ColliderGroup(vector<Collider*> colliders, ColliderType colliderType, j1Module* callback);

	~ColliderGroup();

	bool IsColliderInGroup(Collider* collider);

	// -----

	ColliderType colliderType = ColliderType_NoType;
	j1Module* callback = nullptr;
	bool isRemove = false;

	vector<Collider*> colliders;
};

// ---------------------------------------------------------------------
// Collider: struct representing a single collider
// ---------------------------------------------------------------------

struct Collider
{
	Collider(SDL_Rect colliderRect);

	~Collider();

	void SetPos(int x, int y);
	void SetColliderGroup(ColliderGroup* colliderGroup);

	bool CheckCollision(const SDL_Rect& r) const;

	// -----

	SDL_Rect colliderRect = { 0,0,0,0 };

	ColliderGroup* colliderGroup = nullptr;
};

#endif //__j1COLLISION_H__


