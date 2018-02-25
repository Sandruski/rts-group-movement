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

	SDL_Color color = { 255,255,255,255 };

	Animation up, down, left, right;
	Animation upLeft, upRight, downLeft, downRight;
	Animation idle;
};

enum UnitState {
	UnitState_Idle,
	UnitState_Walk,
	UnitState_Attack
};

class Unit : public Entity
{
public:

	Unit(EntityInfo entityInfo, UnitInfo unitInfo);
	void OnCollision(Collider* c1, Collider* c2);
	void Move(float dt);
	void Draw(SDL_Texture* sprites);

	void UpdateAnimationsSpeed(float dt);
	void ChangeAnimation();

	void UnitStateMachine(float dt);

	void SetUnitState(UnitState unitState);
	UnitState GetUnitState() const;

	void CheckCollision(iPoint position, iPoint size, int offset, bool &up, bool &down, bool &left, bool &right, UnitState unitState = UnitState_Idle);
	void CalculateCollision(iPoint position, iPoint size, uint x, uint y, uint id, int offset, bool &up, bool &down, bool &left, bool &right, UnitState unitState = UnitState_Idle);

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
