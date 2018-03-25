#ifndef __Footman_H__
#define __Footman_H__

#include "DynamicEntity.h"

struct Collider;

struct FootmanInfo
{
	UnitInfo unitInfo;

	Animation idle;
	Animation up, down, left, right;
	Animation upLeft, upRight, downLeft, downRight;
	Animation attackUp, attackDown, attackLeft, attackRight;
	Animation attackUpLeft, attackUpRight, attackDownLeft, attackDownRight;
	Animation deathUp, deathDown;
};

class Footman :public DynamicEntity
{
public:

	Footman(fPoint pos, iPoint size, int currLife, uint maxLife, const UnitInfo& unitInfo, const FootmanInfo& footmanInfo);
	~Footman() {};
	void Move(float dt);
	void Draw(SDL_Texture* sprites);
	void DebugDrawSelected();
	void OnCollision(Collider* c1, Collider* c2);

	// State machine
	void UnitStateMachine(float dt);

	// Animations
	void LoadAnimationsSpeed();
	void UpdateAnimationsSpeed(float dt);
	void ChangeAnimation();

private:

	FootmanInfo footmanInfo;

	// Animations speed
	float idleSpeed = 0.0f;
	float upSpeed = 0.0f, downSpeed = 0.0f, leftSpeed = 0.0f, rightSpeed = 0.0f;
	float upLeftSpeed = 0.0f, upRightSpeed = 0.0f, downLeftSpeed = 0.0f, downRightSpeed = 0.0f;
	float attackUpSpeed = 0.0f, attackDownSpeed = 0.0f, attackLeftSpeed = 0.0f, attackRightSpeed = 0.0f;
	float attackUpLeftSpeed = 0.0f, attackUpRightSpeed = 0.0f, attackDownLeftSpeed = 0.0f, attackDownRightSpeed = 0.0f;
	float deathUpSpeed = 0.0f, deathDownSpeed = 0.0f;
};

#endif //__Footman_H__