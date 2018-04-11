#ifndef __Entity_H__
#define __Entity_H__

#include "p2Point.h"
#include "Animation.h"

#include <vector>
#include <list>
using namespace std;

struct SDL_Texture;

struct Collider;
struct ColliderGroup;

enum CollisionState;

enum EntityType
{
	EntityType_NoType,
	EntityType_StaticEntity,
	EntityType_DynamicEntity,
	EntityType_MaxTypes
};

enum EntitySide
{
	EntitySide_NoSide,
	EntitySide_Player,
	EntitySide_Enemy,
	EntitySide_Neutral,
	EntitySide_MaxSides
};

class Entity;

struct EntityInfo; // empty container

struct TargetInfo
{
	bool isSightSatisfied = false; // if true, sight distance is satisfied
	bool isAttackSatisfied = false; // if true, attack distance is satisfied

	bool isRemoved = false; // if true, it means that the entity has been killed

	Entity* target = nullptr;

	// -----

	bool IsTargetPresent() const;
};

class Entity
{
public:

	Entity(fPoint pos, iPoint size, int currLife, uint maxLife);
	virtual ~Entity();
	virtual void Draw(SDL_Texture* sprites);
	virtual void DebugDrawSelected();
	virtual void OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState);

	// Position and size
	void SetPos(fPoint pos);
	void AddToPos(fPoint pos);
	fPoint GetPos() const;
	iPoint GetSize() const;

	// Life and damage
	int GetMaxLife() const;
	void SetCurrLife(int currLife);
	int GetCurrLife() const;
	void ApplyDamage(int damage);

	// Collision
	ColliderGroup* GetEntityCollider() const;
	bool CreateEntityCollider(EntitySide entitySide);
	void UpdateEntityColliderPos();

	// Attack
	/// Entity is being attacked by units
	bool AddAttackingUnit(Entity* entity);
	bool RemoveAttackingUnit(Entity* entity);
	uint GetAttackingUnitsSize(Entity* attackingUnit) const;

public:

	EntityType entityType = EntityType_NoType;
	EntitySide entitySide = EntitySide_NoSide;

	bool isRemove = false;
	bool isSelected = false;

protected:

	fPoint pos = { 0.0f,0.0f };
	iPoint size = { 0,0 };

	int currLife = 0;
	uint maxLife = 0;

	// Collision
	ColliderGroup* entityCollider = nullptr;

	// Attack
	list<Entity*> unitsAttacking;
};

#endif //__Entity_H__