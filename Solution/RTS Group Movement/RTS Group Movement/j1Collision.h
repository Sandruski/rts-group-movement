#ifndef __j1COLLISION_H__
#define __j1COLLISION_H__

#include "j1Module.h"
#include "p2Point.h"

#include "SDL\include\SDL_rect.h"

#include <list>
#include <vector>
#include <algorithm>
using namespace std;

enum ColliderType {

	ColliderType_NoType = -1,
	ColliderType_PlayerUnit,
	ColliderType_EnemyUnit,
	ColliderType_NeutralUnit,
	ColliderType_PlayerSightRadius,
	ColliderType_EnemySightRadius,
	ColliderType_PlayerAttackRadius,
	ColliderType_EnemyAttackRadius,
	ColliderType_MaxColliders
};

enum CollisionState {

	CollisionState_NoCollision,
	CollisionState_OnEnter,
	CollisionState_OnExit
};

struct Entity;
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

	bool ProcessCollision(ColliderGroup* I, ColliderGroup* J);
	void HandleTriggers();

	// ColliderGroups
	ColliderGroup* CreateAndAddColliderGroup(vector<Collider*> colliders, ColliderType colliderType, j1Module* callback, Entity* entity = nullptr);
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
};

// ---------------------------------------------------------------------
// ColliderGroup: struct representing a group of colliders
// ---------------------------------------------------------------------

struct ColliderGroup
{
	ColliderGroup(vector<Collider*> colliders, ColliderType colliderType, j1Module* callback, Entity* entity = nullptr);

	~ColliderGroup();

	bool IsColliderInGroup(Collider* collider);

	void CreateOffsetCollider();
	Collider* GetCollider(bool left = false, bool right = false, bool top = false, bool bottom = false);

	void RemoveCollider(Collider* collider);
	bool RemoveAllColliders();

	// -----

	ColliderType colliderType = ColliderType_NoType;

	bool isTrigger = false; // the collider behaves as a trigger
	bool isRemove = false; // the collider will be removed
	bool isValid = true; // the collider is not valid anymore

	Collider* offsetCollider = nullptr;
	vector<Collider*> colliders;
	list<ColliderGroup*> collidingGroups;
	list<ColliderGroup*> lastCollidingGroups;

	j1Module* callback = nullptr;
	Entity* entity = nullptr;
};

// ---------------------------------------------------------------------
// Collider: struct representing a single collider
// ---------------------------------------------------------------------

struct Collider
{
	Collider(SDL_Rect colliderRect);

	~Collider();

	void SetPos(int x, int y);
	iPoint GetPos() const;

	void SetColliderGroup(ColliderGroup* colliderGroup);

	bool CheckCollision(const SDL_Rect& r) const;

	// -----

	SDL_Rect colliderRect = { 0,0,0,0 };

	ColliderGroup* colliderGroup = nullptr; // parent
};

#endif //__j1COLLISION_H__