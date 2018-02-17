#ifndef __Entity_H__
#define __Entity_H__

#include "p2Point.h"

struct SDL_Texture;
struct Collider;

enum EntityType
{
	EntityType_NoType,
	EntityType_Unit,
	EntityType_MaxTypes
};

struct EntityInfo 
{
	fPoint pos = { 0,0 };
	iPoint size = { 0,0 };
};

class Entity
{
public:

	EntityInfo entityInfo;
	EntityType type = EntityType_NoType;
	bool remove = false;
	bool isSelected = false;

protected:

	Collider* collider = nullptr;
	bool up = false, down = false, left = false, right = false;

public:

	Entity(EntityInfo entityInfo);
	virtual ~Entity();

	const Collider* GetCollider() const;

	virtual void Move(float dt) {};
	virtual void LoadAnimationsSpeed() {};
	virtual void UpdateAnimations(float dt) {};
	virtual void Draw(SDL_Texture* sprites);
	virtual void DrawSelected();
	virtual void OnCollision(Collider* collider, Collider* c2);
};

#endif //__Entity_H__