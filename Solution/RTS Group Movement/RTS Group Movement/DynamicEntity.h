#ifndef __DynamicEntity_H__
#define __DynamicEntity_H__

#include "p2Point.h"
#include "Animation.h"

#include "Entity.h"

#include <string>
#include <vector>
using namespace std;

struct SDL_Texture;
struct SDL_Color;

struct Collider;
struct ColliderGroup;
struct SingleUnit;

class PathPlanner; 
class Navgraph;

enum DynamicEntityType
{
	DynamicEntityType_NoType,

	// Player types
	DynamicEntityType_Footman,

	// Enemy types
	DynamicEntityType_Grunt,

	DynamicEntityType_MaxTypes
};

enum UnitState 
{
	UnitState_Idle,
	UnitState_Walk,
	UnitState_Attack
};

enum UnitDirection 
{
	UnitDirection_NoDirection,
	UnitDirection_Up,
	UnitDirection_Down,
	UnitDirection_Left,
	UnitDirection_Right,
	UnitDirection_UpLeft,
	UnitDirection_UpRight,
	UnitDirection_DownLeft,
	UnitDirection_DownRight
};

// Struct UnitInfo: contains all necessary information of the movement and attack of the unit
struct UnitInfo 
{
	uint priority = 1;

	uint sightRadius = 0;
	uint attackRadius = 0;

	float maxSpeed = 0.0f;
	float currSpeed = 0.0f;
};

class DynamicEntity :public Entity
{
public:

	DynamicEntity(fPoint pos, iPoint size, int currLife, uint maxLife, const UnitInfo& unitInfo);
	virtual ~DynamicEntity();
	virtual void Move(float dt);
	virtual void Draw(SDL_Texture* sprites);
	virtual void DebugDrawSelected();
	virtual void OnCollision(Collider* c1, Collider* c2);

	// State machine
	void UnitStateMachine(float dt);

	void SetUnitState(UnitState unitState);
	UnitState GetUnitState() const;

	// Movement
	SingleUnit* GetSingleUnit() const;
	PathPlanner* GetPathPlanner() const;
	float GetSpeed() const;
	uint GetPriority() const;

	// Animations
	virtual void LoadAnimationsSpeed();
	virtual void UpdateAnimationsSpeed(float dt);
	virtual void ChangeAnimation();

	// Direction
	void SetUnitDirection(UnitDirection unitDirection);
	UnitDirection GetUnitDirection() const;

	void SetUnitDirectionByValue(fPoint unitDirection);
	fPoint GetUnitDirectionByValue() const;

	// Selection color
	void SetColor(SDL_Color color, string colorName);
	SDL_Color GetColor() const;
	string GetColorName() const;

	// Collision
	ColliderGroup* GetSightRadiusCollider() const;
	bool CreateUnitSightCollider(EntitySide entitySide);
	void UpdateUnitSightColliderPos();

public:

	DynamicEntityType dynamicEntityType = DynamicEntityType_NoType;

protected:

	Animation* animation = nullptr;

	// Movement
	UnitInfo unitInfo;
	UnitState unitState;
	fPoint direction = { 0.0f,0.0f };

	SingleUnit* singleUnit = nullptr;
	PathPlanner* pathPlanner = nullptr;
	Navgraph* navgraph = nullptr;

	// Selection color
	SDL_Color color = ColorWhite;
	string colorName = "White";

	// Collision
	ColliderGroup* sightRadiusCollider = nullptr;
};

#endif //__DynamicEntity_H__