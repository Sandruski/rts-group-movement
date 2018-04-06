#ifndef __j1PARTICLES_H__
#define __j1PARTICLES_H__

#include "j1Module.h"
#include "Defs.h"
#include "p2Point.h"
#include "Animation.h"

#include "j1Collision.h"

#define MAX_ACTIVE_PARTICLES 500 // if max particles are exceeded, they should not be created nor printed (not crush!)

struct SDL_Texture;

struct Collider;
struct ColliderGroup;
enum ColliderType;

enum ParticleType {

	ParticleType_NoType,

	ParticleType_Paws,

	ParticleType_MaxTypes
};

struct SheepPawsInfo {

	Animation up, down, left, right;
	Animation upLeft, upRight, downLeft, downRight;
};

struct BoarPawsInfo {

	Animation up, down, left, right;
	Animation upLeft, upRight, downLeft, downRight;
};

struct Particle
{
	ParticleType particleType = ParticleType_NoType;

	Collider* collider = nullptr;
	Animation animation;

	uint fx = 0;
	fPoint pos = { 0.0f,0.0f };
	fPoint speed = { 0, 0 };
	Uint32 born = 0;
	Uint32 life = 0;

	iPoint collisionSize = { 0,0 };
	fPoint destination = { 0,0 };

	bool isRemove = false;

	Particle();
	Particle(const Particle& p);
	~Particle();
	bool Update(float dt);
};

class j1Particles : public j1Module
{
public:

	j1Particles();
	~j1Particles();

	// Called before render is available
	bool Awake(pugi::xml_node&);

	bool Start();
	bool Update(float dt);
	void Draw();
	bool CleanUp();

	Particle* AddParticle(const Particle& particle, iPoint pos, ColliderType colliderType = ColliderType_NoType, Uint32 delay = 0, fPoint speed = { 0.0f,0.0f });
	void OnCollision(Collider* c1, Collider* c2, CollisionState collisionState);

	bool IsParticleOnTile(iPoint tile) const;

	void LoadAnimationsSpeed();
	void UpdateAnimations(float dt);

	BoarPawsInfo& GetBoarPawsInfo();
	SheepPawsInfo& GetSheepPawsInfo();

private:

	Particle* active[MAX_ACTIVE_PARTICLES];
	uint lastParticle = 0;

	string pawsTexName;
	SDL_Texture* pawsTex = nullptr;

	BoarPawsInfo boarPawsInfo;
	SheepPawsInfo sheepPawsInfo;

	// Animations speed
	/// Boar Paws
	float boarPawsUpSpeed = 0.0f, boarPawsDownSpeed = 0.0f, boarPawsLeftSpeed = 0.0f, boarPawsRightSpeed = 0.0f;
	float boarPawsUpLeftSpeed = 0.0f, boarPawsUpRightSpeed = 0.0f, boarPawsDownLeftSpeed = 0.0f, boarPawsDownRightSpeed = 0.0f;

	/// Sheep Paws
	float sheepPawsUpSpeed = 0.0f, sheepPawsDownSpeed = 0.0f, sheepPawsLeftSpeed = 0.0f, sheepPawsRightSpeed = 0.0f;
	float sheepPawsUpLeftSpeed = 0.0f, sheepPawsUpRightSpeed = 0.0f, sheepPawsDownLeftSpeed = 0.0f, sheepPawsDownRightSpeed = 0.0f;

public:

	Particle paws;
};

#endif //__j1PARTICLES_H__
