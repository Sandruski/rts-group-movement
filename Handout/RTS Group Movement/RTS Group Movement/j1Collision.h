#ifndef __j1COLLISION_H__
#define __j1COLLISION_H__

#define MAX_COLLIDERS 1000

#include "j1Module.h"

#include "SDL\include\SDL_rect.h"

enum COLLIDER_TYPE
{
	COLLIDER_NONE = -1,
	COLLIDER_PLAYER,
	COLLIDER_ARROW,
	COLLIDER_CATPEASANT, COLLIDER_CATPEASANT_SHOT,
	COLLIDER_IMP, COLLIDER_IMP_BOMB, COLLIDER_IMP_BOMB_EXPLOSION,
	COLLIDER_MONKEY, COLLIDER_MONKEY_HIT,
	COLLIDER_CAT, COLLIDER_CAT_SPARKLE,
	COLLIDER_MAX
};

struct Collider
{
	SDL_Rect rect;
	bool to_delete = false;
	COLLIDER_TYPE type;
	j1Module* callback /*= nullptr*/;

	Collider(SDL_Rect rectangle, COLLIDER_TYPE type, j1Module* callback /*= nullptr*/) :
		rect(rectangle),
		type(type),
		callback(callback)
	{}

	void SetPos(int x, int y)
	{
		rect.x = x;
		rect.y = y;
	}

	bool CheckCollision(const SDL_Rect& r) const;
};

class j1Collision : public j1Module
{
public:

	j1Collision();
	~j1Collision();

	bool PreUpdate();
	bool Update(float dt);
	bool CleanUp();

	Collider* AddCollider(SDL_Rect rect, COLLIDER_TYPE type, j1Module* /*callback = nullptr*/);
	bool EraseCollider(Collider* collider);
	void DebugDraw();
	bool GetDebug() { return debug; }

	bool matrix[COLLIDER_MAX][COLLIDER_MAX];

private:

	Collider* colliders[MAX_COLLIDERS];
	bool debug = false;
};

#endif //__j1COLLISION_H__


