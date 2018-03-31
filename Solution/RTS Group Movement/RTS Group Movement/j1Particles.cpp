#include "p2Log.h"

#include "j1App.h"

#include "j1Render.h"
#include "j1Textures.h"
#include "j1Particles.h"
#include "j1Collision.h"
#include "j1EntityFactory.h"
#include "j1Scene.h"
#include "j1Map.h"

#include <math.h>

#include "SDL/include/SDL_timer.h"

j1Particles::j1Particles()
{
	name.assign("particles");

	for (uint i = 0; i < MAX_ACTIVE_PARTICLES; ++i)
		active[i] = nullptr;
}

j1Particles::~j1Particles()
{}

// Called before render is available
bool j1Particles::Awake(pugi::xml_node& config) {

	bool ret = true;

	// Load spritesheets
	pugi::xml_node spritesheets = config.child("spritesheets");
	pawsTexName = spritesheets.child("paws").attribute("name").as_string();

	// Load animations
	// Sheep Paws
	pugi::xml_node sheepPawsAnimation = config.child("animations").child("sheepPaws");
	pugi::xml_node currentAnimation;

	// up
	currentAnimation = sheepPawsAnimation.child("up");
	sheepPawsInfo.up.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.up.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.up.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down
	currentAnimation = sheepPawsAnimation.child("down");
	sheepPawsInfo.down.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.down.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.down.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// left
	currentAnimation = sheepPawsAnimation.child("left");
	sheepPawsInfo.left.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.left.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.left.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// right
	currentAnimation = sheepPawsAnimation.child("right");
	sheepPawsInfo.right.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.right.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.right.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-left
	currentAnimation = sheepPawsAnimation.child("upLeft");
	sheepPawsInfo.upLeft.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.upLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.upLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-right
	currentAnimation = sheepPawsAnimation.child("upRight");
	sheepPawsInfo.upRight.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.upRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.upRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-left
	currentAnimation = sheepPawsAnimation.child("downLeft");
	sheepPawsInfo.downLeft.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.downLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.downLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-right
	currentAnimation = sheepPawsAnimation.child("downRight");
	sheepPawsInfo.downRight.speed = currentAnimation.attribute("speed").as_float();
	sheepPawsInfo.downRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		sheepPawsInfo.downRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	// Boar Paws
	pugi::xml_node boarPawsAnimations = config.child("animations").child("boarPaws");

	// up
	currentAnimation = boarPawsAnimations.child("up");
	boarPawsInfo.up.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.up.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.up.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down
	currentAnimation = boarPawsAnimations.child("down");
	boarPawsInfo.down.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.down.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.down.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// left
	currentAnimation = boarPawsAnimations.child("left");
	boarPawsInfo.left.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.left.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.left.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// right
	currentAnimation = boarPawsAnimations.child("right");
	boarPawsInfo.right.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.right.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.right.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-left
	currentAnimation = boarPawsAnimations.child("upLeft");
	boarPawsInfo.upLeft.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.upLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.upLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-right
	currentAnimation = boarPawsAnimations.child("upRight");
	boarPawsInfo.upRight.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.upRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.upRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-left
	currentAnimation = boarPawsAnimations.child("downLeft");
	boarPawsInfo.downLeft.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.downLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.downLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-right
	currentAnimation = boarPawsAnimations.child("downRight");
	boarPawsInfo.downRight.speed = currentAnimation.attribute("speed").as_float();
	boarPawsInfo.downRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		boarPawsInfo.downRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	return ret;
}

// Load assets
bool j1Particles::Start()
{
	bool ret = true;

	LOG("Loading particles");

	paws.particleType = ParticleType_Paws;

	LoadAnimationsSpeed();

	pawsTex = App->tex->Load(pawsTexName.data());

	return ret;
}

// Unload assets
bool j1Particles::CleanUp()
{
	LOG("Unloading particles");

	for (uint i = 0; i < MAX_ACTIVE_PARTICLES; ++i)
	{
		if (active[i] != nullptr)
		{
			delete active[i];
			active[i] = nullptr;
		}
	}

	App->tex->UnLoad(pawsTex);

	return true;
}

// Update: draw background
bool j1Particles::Update(float dt)
{
	bool ret = true;

	UpdateAnimations(dt);

	for (uint i = 0; i < MAX_ACTIVE_PARTICLES; ++i)
	{
		Particle* p = active[i];

		if (p == nullptr)
			continue;

		if (!p->Update(dt))
		{
			delete p;
			active[i] = nullptr;
		}
	}

	return ret;
}

void j1Particles::Draw()
{
	for (uint i = 0; i < MAX_ACTIVE_PARTICLES; ++i)
	{
		Particle* p = active[i];

		if (p == nullptr)
			continue;

		if (SDL_GetTicks() >= p->born)
		{
			if (p->particleType == ParticleType_Paws)
				App->render->Blit(pawsTex, p->pos.x, p->pos.y, &(p->animation.GetCurrentFrame()));
		}
	}
}

Particle* j1Particles::AddParticle(const Particle& particle, iPoint pos, ColliderType colliderType, Uint32 delay, fPoint speed)
{
	for (uint i = 0; i < MAX_ACTIVE_PARTICLES; ++i)
	{
		if (active[i] == nullptr)
		{
			Particle* p = new Particle(particle);

			p->born = SDL_GetTicks() + delay;
			p->pos = { (float)pos.x, (float)pos.y };
			p->speed = speed;

			active[i] = p;

			return p;
		}
	}
}

void j1Particles::OnCollision(Collider* c1, Collider* c2, CollisionState collisionState)
{
	for (uint i = 0; i < MAX_ACTIVE_PARTICLES; ++i)
	{
		// Always destroy particles that collide
		if (active[i] != nullptr && active[i]->collider == c1)
		{
			delete active[i];
			active[i] = nullptr;
			break;
		}
	}
}

bool j1Particles::IsParticleOnTile(iPoint tile) const
{
	for (uint i = 0; i < MAX_ACTIVE_PARTICLES; ++i)
	{
		Particle* p = active[i];

		if (p == nullptr)
			continue;

		iPoint particleTile = App->map->WorldToMap(p->pos.x, p->pos.y);

		if (particleTile == tile)
			return true;
	}

	return false;
}

void j1Particles::LoadAnimationsSpeed()
{
	/// Boar Paws
	boarPawsUpSpeed = boarPawsInfo.up.speed;
	boarPawsDownSpeed = boarPawsInfo.down.speed;
	boarPawsLeftSpeed = boarPawsInfo.left.speed;
	boarPawsRightSpeed = boarPawsInfo.right.speed;
	boarPawsUpLeftSpeed = boarPawsInfo.upLeft.speed;
	boarPawsUpRightSpeed = boarPawsInfo.upRight.speed;
	boarPawsDownLeftSpeed = boarPawsInfo.downLeft.speed;
	boarPawsDownRightSpeed = boarPawsInfo.downRight.speed;

	/// Sheep Paws
	sheepPawsUpSpeed = sheepPawsInfo.up.speed; 
	sheepPawsDownSpeed = sheepPawsInfo.down.speed;
	sheepPawsLeftSpeed = sheepPawsInfo.left.speed;
	sheepPawsRightSpeed = sheepPawsInfo.right.speed;
	sheepPawsUpLeftSpeed = sheepPawsInfo.upLeft.speed;
	sheepPawsUpRightSpeed = sheepPawsInfo.upRight.speed;
	sheepPawsDownLeftSpeed = sheepPawsInfo.downLeft.speed;
	sheepPawsDownRightSpeed = sheepPawsInfo.downRight.speed;
}

void j1Particles::UpdateAnimations(float dt)
{
	/// Boar Paws
	boarPawsInfo.up.speed = boarPawsUpSpeed * dt;
	boarPawsInfo.down.speed = boarPawsDownSpeed * dt;
	boarPawsInfo.left.speed = boarPawsLeftSpeed * dt;
	boarPawsInfo.right.speed = boarPawsRightSpeed * dt;
	boarPawsInfo.upLeft.speed = boarPawsUpLeftSpeed * dt;
	boarPawsInfo.upRight.speed = boarPawsUpRightSpeed * dt;
	boarPawsInfo.downLeft.speed = boarPawsDownLeftSpeed * dt;
	boarPawsInfo.downRight.speed = boarPawsDownRightSpeed * dt;

	/// Sheep Paws
	sheepPawsInfo.up.speed = sheepPawsUpSpeed * dt;
	sheepPawsInfo.down.speed = sheepPawsDownSpeed * dt;
	sheepPawsInfo.left.speed = sheepPawsLeftSpeed * dt;
	sheepPawsInfo.right.speed = sheepPawsRightSpeed * dt;
	sheepPawsInfo.upLeft.speed = sheepPawsUpLeftSpeed * dt;
	sheepPawsInfo.upRight.speed = sheepPawsUpRightSpeed * dt;
	sheepPawsInfo.downLeft.speed = sheepPawsDownLeftSpeed * dt;
	sheepPawsInfo.downRight.speed = sheepPawsDownRightSpeed * dt;
}

BoarPawsInfo& j1Particles::GetBoarPawsInfo() 
{
	return boarPawsInfo;
}

SheepPawsInfo& j1Particles::GetSheepPawsInfo() 
{
	return sheepPawsInfo;
}

// -------------------------------------------------------------
// -------------------------------------------------------------

Particle::Particle()
{
	pos.SetToZero();
	speed.SetToZero();
}

Particle::Particle(const Particle& p) :
	animation(p.animation), pos(p.pos), speed(p.speed), particleType(p.particleType),
	fx(p.fx), born(p.born), life(p.life), collisionSize(p.collisionSize)
{}

Particle::~Particle()
{
}

bool Particle::Update(float dt)
{
	bool ret = true;

	if (particleType == ParticleType_Paws) {

		if (animation.Finished() && isRemove)
			ret = false;
	}

	/*
	if (life > 0)
	{
		if ((SDL_GetTicks() - born) > life)
			ret = false;
	}
	else
		if (anim.Finished() || life == 0)
			ret = false;
	*/

	return ret;
}

