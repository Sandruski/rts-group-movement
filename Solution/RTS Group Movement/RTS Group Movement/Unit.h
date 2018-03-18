#ifndef __Unit_H__
#define __Unit_H__

#include "Entity.h"
#include "Animation.h"
#include "SDL\include\SDL.h"

#include <vector>
#include <string>
using namespace std;

struct SDL_Color;
struct SingleUnit;

class PathPlanner;
class Navgraph;

#define MAX_UNIT_PRIORITY 8

struct UnitInfo {

	UnitInfo();
	UnitInfo(const UnitInfo& i);
	~UnitInfo();

	Animation up, down, left, right;
	Animation upLeft, upRight, downLeft, downRight;
	Animation idle;

	uint priority = 0;
};

enum UnitState {
	UnitState_Idle,
	UnitState_Walk,
	UnitState_Attack
};

enum UnitDirection {
	UnitDirection_Idle,
	UnitDirection_Up,
	UnitDirection_Down,
	UnitDirection_Left,
	UnitDirection_Right,
	UnitDirection_UpLeft,
	UnitDirection_UpRight,
	UnitDirection_DownLeft,
	UnitDirection_DownRight
};

class Unit : public Entity
{
public:

	Unit(EntityInfo entityInfo, uint priority);
	void Move(float dt);
	void Draw(SDL_Texture* sprites);
	void DebugDrawSelected();
	void OnCollision(Collider* c1, Collider* c2);

	void UpdateAnimationsSpeed(float dt);
	void ChangeAnimation();

	void UnitStateMachine(float dt);

	void SetUnitState(UnitState unitState);
	UnitState GetUnitState() const;

	void SetUnitDirection(UnitDirection unitDirection);
	UnitDirection GetUnitDirection() const;
	void SetUnitDirectionByValue(fPoint unitDirection);
	fPoint GetUnitDirectionByValue() const;

	SingleUnit* GetSingleUnit() const;

	void SetColor(SDL_Color color, string colorName);
	SDL_Color GetColor() const;
	string GetColorName() const;

public:

	UnitInfo unitInfo;

	Navgraph* navgraph = nullptr;
	PathPlanner* pathPlanner = nullptr;

	SingleUnit* singleUnit = nullptr;

private:

	UnitState unitState = UnitState_Idle;
	fPoint direction = { 0.0f,0.0f };

	SDL_Color color = ColorWhite;
	string colorName = "White";

	// Animations speed
	float upSpeed, downSpeed, leftSpeed, rightSpeed;
	float upLeftSpeed, upRightSpeed, downLeftSpeed, downRightSpeed;
	float idleSpeed;
};

#endif //__Unit_H__