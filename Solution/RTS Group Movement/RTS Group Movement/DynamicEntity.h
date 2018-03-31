#ifndef __DynamicEntity_H__
#define __DynamicEntity_H__

#include "p2Point.h"
#include "Animation.h"
#include "j1Timer.h"

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
class Goal_Think;

enum ColliderType;
enum CollisionState;

#define TIME_REMOVE_CORPSE 3.0

enum DynamicEntityType
{
	DynamicEntityType_NoType,

	// Player types
	DynamicEntityType_Footman,

	// Enemy types
	DynamicEntityType_Grunt,

	// Neutral types
	DynamicEntityType_CritterSheep,
	DynamicEntityType_CritterBoar,

	DynamicEntityType_MaxTypes
};

enum UnitState 
{
	UnitState_NoState,

	UnitState_Idle,
	UnitState_Die,

	// Composite goals only
	UnitState_AttackTarget,
	UnitState_Patrol,
	UnitState_Wander,

	UnitState_MaxStates
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

enum UnitCommand {

	UnitCommand_NoCommand,

	UnitCommand_Stop,
	UnitCommand_AttackTarget,
	UnitCommand_Patrol,
	UnitCommand_Wander,

	UnitCommand_MaxCommands
};

// Struct UnitInfo: contains all necessary information of the movement and attack of the unit
struct UnitInfo 
{
	uint priority = 1;

	uint sightRadius = 0;
	uint attackRadius = 0;
	uint damage = 0;

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
	virtual void OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState);

	Animation* GetAnimation() const;
	Goal_Think* GetBrain() const;

	// State machine
	void UnitStateMachine(float dt);

	void SetUnitState(UnitState unitState);
	UnitState GetUnitState() const;

	// Movement
	SingleUnit* GetSingleUnit() const;
	PathPlanner* GetPathPlanner() const;
	float GetSpeed() const;
	uint GetPriority() const;
	uint GetDamage() const;

	// Animations
	virtual void LoadAnimationsSpeed();
	virtual void UpdateAnimationsSpeed(float dt);
	virtual bool ChangeAnimation();

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
	ColliderGroup* GetAttackRadiusCollider() const;

	ColliderGroup* CreateRhombusCollider(ColliderType colliderType, uint radius);
	void UpdateRhombusColliderPos(ColliderGroup* collider, uint radius);

	// Attack
	Entity* GetTarget() const;
	void SetTarget(Entity* target);
	bool IsTargetPresent() const;

	bool IsSightSatisfied() const;
	bool IsAttackSatisfied() const;

	void SetHitting(bool isHitting);
	bool IsHitting() const;

	// Player commands
	bool SetCommand(UnitCommand unitCommand);

public:

	DynamicEntityType dynamicEntityType = DynamicEntityType_NoType;

	// Dead
	bool isDead = false; // if true, the unit is performing their dead animation

protected:

	UnitInfo unitInfo;
	UnitState unitState = UnitState_Idle;

	Animation* animation = nullptr;
	fPoint direction = { 0.0f,0.0f };

	bool isFlying = false; /// Dragon and Gryphon Rider fly

	// Root of a bot's goal hierarchy
	Goal_Think* brain = nullptr;

	// Player commands
	UnitCommand unitCommand = UnitCommand_NoCommand;

	// Movement
	SingleUnit* singleUnit = nullptr;
	PathPlanner* pathPlanner = nullptr;
	Navgraph* navgraph = nullptr;

	// Attack
	bool isSightSatisfied = false; // if true, sight distance is satisfied
	bool isAttackSatisfied = false; // if true, attack distance is satisfied
	Entity* target = nullptr;

	bool isHitting = false; // if true, the unit is hitting their target (to change animation)

	// Collision
	ColliderGroup* sightRadiusCollider = nullptr;
	ColliderGroup* attackRadiusCollider = nullptr;
	bool isSpawned = false; // the colliders must start working once the unit has been spawned
	iPoint lastColliderUpdateTile = { -1,-1 };

	// Death
	j1Timer deadTimer;

	// Selection color
	SDL_Color color = ColorWhite;
	string colorName = "White";
};

#endif //__DynamicEntity_H__