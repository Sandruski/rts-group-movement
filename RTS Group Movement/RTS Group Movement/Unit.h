#ifndef __Unit_H__
#define __Unit_H__

#include "Entity.h"
#include "SDL\include\SDL.h"

#include <vector>
using namespace std;

struct SDL_Color;

enum MovementState {
	MovementState_Stop,
	MovementState_WaitForPath,
	MovementState_FollowPath,
	MovementState_GoalReached,
	MovementState_CollisionFound,
	MovementState_IncreaseWaypoint
};

struct UnitInfo {

	UnitInfo();
	UnitInfo(const UnitInfo& i);
	~UnitInfo();

	SDL_Color color = { 255,255,255,255 };

};

enum UnitState {
	UnitState_Idle,
	UnitState_Walk
};

class Unit : public Entity
{
public:

	Unit(EntityInfo entityInfo, UnitInfo unitInfo);
	void OnCollision(Collider* c1, Collider* c2);
	void Move(float dt);
	void Draw(SDL_Texture* sprites);
	void DebugDraw(SDL_Texture* sprites);

	void UnitStateMachine();
	void MovementStateMachine(float dt);

	void SetUnitState(UnitState unitState);
	UnitState GetUnitState() const;

	void CheckCollision(iPoint position, iPoint size, int offset, bool &up, bool &down, bool &left, bool &right, UnitState unitState = UnitState_Idle);
	void CalculateCollision(iPoint position, iPoint size, uint x, uint y, uint id, int offset, bool &up, bool &down, bool &left, bool &right, UnitState unitState = UnitState_Idle);

public:

	UnitInfo unitInfo;

private:

	UnitState unitState = UnitState_Idle;

	// Movement variables
	MovementState movementState = MovementState_Stop;
	vector<iPoint> path;
	iPoint nextTile = { -1,-1 }; // next tile the unit is heading to (in order to reach the goal tile)
	iPoint goalTile = { -1,-1 }; // goal tile of the unit's path
	float speed = 1.0f;

	// Footman
	SDL_Rect spriteRect = { 316,12,32,46 };
};

#endif //__Entity_H__
