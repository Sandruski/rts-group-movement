#ifndef __Unit_H__
#define __Unit_H__

#include "Entity.h"
#include "Animation.h"
#include "SDL\include\SDL.h"

#include <vector>
using namespace std;

struct SDL_Color;

struct UnitInfo {

	UnitInfo();
	UnitInfo(const UnitInfo& i);
	~UnitInfo();

	Animation up, down, left, right;
	Animation upLeft, upRight, downLeft, downRight;
	Animation idle;

	int priority = 0;
};

enum UnitState {
	UnitState_Idle,
	UnitState_Walk,
	UnitState_Attack
};

class Unit : public Entity
{
public:

	Unit(EntityInfo entityInfo, uint priority);
	void OnCollision(Collider* c1, Collider* c2);
	void Move(float dt);
	void Draw(SDL_Texture* sprites);
	void DebugDrawSelected();

	void UpdateAnimationsSpeed(float dt);
	void ChangeAnimation();

	void UnitStateMachine(float dt);

	void SetUnitState(UnitState unitState);
	UnitState GetUnitState() const;

public:

	UnitInfo unitInfo;

private:

	UnitState unitState = UnitState_Idle;

	// Animations speed
	float upSpeed, downSpeed, leftSpeed, rightSpeed;
	float upLeftSpeed, upRightSpeed, downLeftSpeed, downRightSpeed;
	float idleSpeed;
};

#endif //__Entity_H__
